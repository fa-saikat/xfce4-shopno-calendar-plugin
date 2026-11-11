/*
 * Copyright (C) 2026 FA Saikat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/* A fake systemd-timedated on a private bus. Like the real one, it does
 * not announce changes of NTPSynchronized. */

#include "config.h"

#include <locale.h>

#include "shopno-cal-sync.h"

static const gchar introspection[] =
  "<node>"
  "  <interface name='org.freedesktop.timedate1'>"
  "    <property name='NTP' type='b' access='read'/>"
  "    <property name='NTPSynchronized' type='b' access='read'/>"
  "  </interface>"
  "</node>";

typedef struct
{
  GTestDBus       *bus;
  GDBusConnection *service;
  GDBusConnection *client;
  guint            object_id;
  guint            name_id;
  gboolean         ntp;
  gboolean         synchronized;
} Fixture;



static GVariant *
get_property (GDBusConnection *connection,
              const gchar     *sender,
              const gchar     *path,
              const gchar     *interface,
              const gchar     *name,
              GError         **error,
              gpointer         data)
{
  Fixture *fixture = data;

  if (g_strcmp0 (name, "NTP") == 0)
    return g_variant_new_boolean (fixture->ntp);

  return g_variant_new_boolean (fixture->synchronized);
}



static const GDBusInterfaceVTable vtable = { NULL, get_property, NULL, { 0 } };



static GDBusConnection *
connect (Fixture *fixture)
{
  GDBusConnection *connection;

  connection = g_dbus_connection_new_for_address_sync (g_test_dbus_get_bus_address (fixture->bus),
                                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT
                                                       | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                       NULL, NULL, NULL);
  g_assert_nonnull (connection);

  return connection;
}



static void
fixture_set_up (Fixture       *fixture,
                gconstpointer  data)
{
  GDBusNodeInfo *info;
  GVariant      *reply;

  fixture->bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (fixture->bus);

  fixture->service = connect (fixture);
  fixture->client = connect (fixture);
  fixture->ntp = TRUE;
  fixture->synchronized = TRUE;

  info = g_dbus_node_info_new_for_xml (introspection, NULL);
  fixture->object_id = g_dbus_connection_register_object (fixture->service, "/org/freedesktop/timedate1",
                                                          info->interfaces[0], &vtable,
                                                          fixture, NULL, NULL);
  g_dbus_node_info_unref (info);

  reply = g_dbus_connection_call_sync (fixture->service, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus", "RequestName",
                                       g_variant_new ("(su)", "org.freedesktop.timedate1", 0),
                                       NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
  g_assert_nonnull (reply);
  g_variant_unref (reply);
}



static void
fixture_tear_down (Fixture       *fixture,
                   gconstpointer  data)
{
  g_dbus_connection_unregister_object (fixture->service, fixture->object_id);
  g_object_unref (fixture->client);
  g_object_unref (fixture->service);
  g_test_dbus_down (fixture->bus);
  g_object_unref (fixture->bus);
}



static void
changed (ShopnoCalSync *sync,
         gboolean      *done)
{
  *done = TRUE;
}



/* Runs the main loop until the state changes */
static ShopnoCalSyncState
wait_for_change (ShopnoCalSync *sync)
{
  gboolean done = FALSE;
  gulong   id = g_signal_connect (sync, "changed", G_CALLBACK (changed), &done);
  guint    i;

  for (i = 0; i < 500 && !done; i++)
    {
      g_main_context_iteration (NULL, FALSE);
      g_usleep (10000);
    }

  g_signal_handler_disconnect (sync, id);
  g_assert_true (done);

  return shopno_cal_sync_get_state (sync);
}



static void
emit_properties_changed (Fixture *fixture)
{
  GVariantBuilder changed;

  g_variant_builder_init (&changed, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&changed, "{sv}", "NTP", g_variant_new_boolean (fixture->ntp));

  g_dbus_connection_emit_signal (fixture->service, NULL, "/org/freedesktop/timedate1",
                                 "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                 g_variant_new ("(sa{sv}as)", "org.freedesktop.timedate1", &changed, NULL),
                                 NULL);
}



static void
test_states (Fixture       *fixture,
             gconstpointer  data)
{
  ShopnoCalSync *sync = shopno_cal_sync_new_for_connection (fixture->client);

  g_assert_cmpint (shopno_cal_sync_get_state (sync), ==, SHOPNO_CAL_SYNC_UNKNOWN);
  g_assert_cmpint (wait_for_change (sync), ==, SHOPNO_CAL_SYNC_SYNCED);

  /* NTPSynchronized changes silently; a refresh reads it */
  fixture->synchronized = FALSE;
  shopno_cal_sync_refresh (sync);
  g_assert_cmpint (wait_for_change (sync), ==, SHOPNO_CAL_SYNC_NOT_SYNCED);

  /* NTP is announced, and read without a refresh */
  fixture->ntp = FALSE;
  emit_properties_changed (fixture);
  g_assert_cmpint (wait_for_change (sync), ==, SHOPNO_CAL_SYNC_OFF);

  fixture->ntp = TRUE;
  fixture->synchronized = TRUE;
  emit_properties_changed (fixture);
  g_assert_cmpint (wait_for_change (sync), ==, SHOPNO_CAL_SYNC_SYNCED);

  g_object_unref (sync);
}



static void
test_no_timedated (Fixture       *fixture,
                   gconstpointer  data)
{
  ShopnoCalSync *sync;
  GVariant      *reply;
  guint          i;

  reply = g_dbus_connection_call_sync (fixture->service, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus", "ReleaseName",
                                       g_variant_new ("(s)", "org.freedesktop.timedate1"),
                                       NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
  g_variant_unref (reply);

  /* The call fails, and the state stays unknown, which hides it */
  sync = shopno_cal_sync_new_for_connection (fixture->client);
  for (i = 0; i < 50; i++)
    {
      g_main_context_iteration (NULL, FALSE);
      g_usleep (10000);
    }
  g_assert_cmpint (shopno_cal_sync_get_state (sync), ==, SHOPNO_CAL_SYNC_UNKNOWN);
  g_assert_null (shopno_cal_sync_describe (SHOPNO_CAL_SYNC_UNKNOWN));

  g_object_unref (sync);
}



static void
test_describe (void)
{
  g_assert_cmpstr (shopno_cal_sync_describe (SHOPNO_CAL_SYNC_SYNCED), ==, "Time synced automatically");
  g_assert_cmpstr (shopno_cal_sync_describe (SHOPNO_CAL_SYNC_NOT_SYNCED), ==, "Time not synced yet");
  g_assert_cmpstr (shopno_cal_sync_describe (SHOPNO_CAL_SYNC_OFF), ==, "Automatic time sync is off");
}



static void
test_skip_no_bus (void)
{
  g_test_skip ("dbus-daemon is not installed");
}



int
main (int    argc,
      char **argv)
{
  gchar *dbus_daemon;

  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);

  /* GTestDBus runs its own dbus-daemon, which is missing where the
   * system bus is dbus-broker, as on Fedora, or in a minimal build root */
  dbus_daemon = g_find_program_in_path ("dbus-daemon");
  if (dbus_daemon != NULL)
    {
      g_test_add ("/sync/states", Fixture, NULL, fixture_set_up, test_states, fixture_tear_down);
      g_test_add ("/sync/no-timedated", Fixture, NULL, fixture_set_up, test_no_timedated, fixture_tear_down);
    }
  else
    {
      g_test_add_func ("/sync/states", test_skip_no_bus);
      g_test_add_func ("/sync/no-timedated", test_skip_no_bus);
    }
  g_free (dbus_daemon);

  g_test_add_func ("/sync/describe", test_describe);

  return g_test_run ();
}
