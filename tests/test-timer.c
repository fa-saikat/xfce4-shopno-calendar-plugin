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

/* Runs a few short timers in real time, and the notification against a
 * fake notification server on a private bus */

#include "config.h"

#include <locale.h>

#include "shopno-cal-timer.h"



/* Runs the main loop for a while, or until *done */
static void
run (guint     ms,
     gboolean *done)
{
  gint64 end = g_get_monotonic_time () + ms * 1000;

  while (g_get_monotonic_time () < end && (done == NULL || !*done))
    {
      g_main_context_iteration (NULL, FALSE);
      g_usleep (5000);
    }
}



static void
count (ShopnoCalTimer *timer,
       guint          *n)
{
  (*n)++;
}



static void
set_done (ShopnoCalTimer *timer,
          gboolean       *done)
{
  *done = TRUE;
}



static void
test_idle (void)
{
  ShopnoCalTimer *timer = shopno_cal_timer_new ();

  /* Five minutes until set */
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_IDLE);
  g_assert_cmpuint (shopno_cal_timer_get_duration (timer), ==, 300);
  g_assert_cmpuint (shopno_cal_timer_get_remaining (timer), ==, 300);
  g_assert_cmpfloat (shopno_cal_timer_get_fraction (timer), ==, 1.0);

  shopno_cal_timer_set_duration (timer, 25 * 60);
  g_assert_cmpuint (shopno_cal_timer_get_remaining (timer), ==, 1500);

  /* Pause and reset do nothing to an idle timer */
  shopno_cal_timer_pause (timer);
  shopno_cal_timer_reset (timer);
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_IDLE);

  g_object_unref (timer);
}



static void
test_run_pause_resume (void)
{
  ShopnoCalTimer *timer = shopno_cal_timer_new ();
  gboolean        done = FALSE;
  guint           finished = 0;
  guint           changed = 0;

  shopno_cal_timer_set_duration (timer, 2);
  g_signal_connect (timer, "finished", G_CALLBACK (count), &finished);
  g_signal_connect (timer, "finished", G_CALLBACK (set_done), &done);
  g_signal_connect (timer, "changed", G_CALLBACK (count), &changed);

  /* The full duration shows when started */
  shopno_cal_timer_start (timer);
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_RUNNING);
  g_assert_cmpuint (shopno_cal_timer_get_remaining (timer), ==, 2);
  g_assert_cmpuint (changed, ==, 1);

  run (1200, NULL);
  g_assert_cmpuint (shopno_cal_timer_get_remaining (timer), ==, 1);
  g_assert_cmpfloat (shopno_cal_timer_get_fraction (timer), <, 0.5);

  /* Paused, it does not count down or finish */
  shopno_cal_timer_pause (timer);
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_PAUSED);
  run (1500, NULL);
  g_assert_cmpuint (shopno_cal_timer_get_remaining (timer), ==, 1);
  g_assert_cmpuint (finished, ==, 0);

  /* Resumed, it finishes in the time it had left, and is set to its
   * duration again */
  shopno_cal_timer_start (timer);
  run (3000, &done);
  g_assert_true (done);
  g_assert_cmpuint (finished, ==, 1);
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_IDLE);
  g_assert_cmpuint (shopno_cal_timer_get_remaining (timer), ==, 2);

  /* start, pause, start, finish */
  g_assert_cmpuint (changed, ==, 4);

  g_object_unref (timer);
}



static void
test_reset (void)
{
  ShopnoCalTimer *timer = shopno_cal_timer_new ();
  guint           finished = 0;

  shopno_cal_timer_set_duration (timer, 1);
  g_signal_connect (timer, "finished", G_CALLBACK (count), &finished);

  shopno_cal_timer_start (timer);
  run (300, NULL);
  shopno_cal_timer_reset (timer);
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_IDLE);
  g_assert_cmpuint (shopno_cal_timer_get_remaining (timer), ==, 1);

  /* A reset timer does not finish */
  run (1500, NULL);
  g_assert_cmpuint (finished, ==, 0);

  /* Nor does one set to a new duration while running */
  shopno_cal_timer_start (timer);
  shopno_cal_timer_set_duration (timer, 60);
  run (1500, NULL);
  g_assert_cmpuint (finished, ==, 0);
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_IDLE);

  g_object_unref (timer);
}



static void
test_describe (void)
{
  gchar *text;

  text = shopno_cal_timer_describe (60);
  g_assert_cmpstr (text, ==, "1 minute is up.");
  g_free (text);

  text = shopno_cal_timer_describe (25 * 60);
  g_assert_cmpstr (text, ==, "25 minutes are up.");
  g_free (text);

  text = shopno_cal_timer_describe (2);
  g_assert_cmpstr (text, ==, "2 seconds are up.");
  g_free (text);
}



/*
 * Notification
 */

static const gchar introspection[] =
  "<node>"
  "  <interface name='org.freedesktop.Notifications'>"
  "    <method name='Notify'>"
  "      <arg type='s' direction='in'/>"
  "      <arg type='u' direction='in'/>"
  "      <arg type='s' direction='in'/>"
  "      <arg type='s' direction='in'/>"
  "      <arg type='s' direction='in'/>"
  "      <arg type='as' direction='in'/>"
  "      <arg type='a{sv}' direction='in'/>"
  "      <arg type='i' direction='in'/>"
  "      <arg type='u' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";



static void
method_call (GDBusConnection       *connection,
             const gchar           *sender,
             const gchar           *path,
             const gchar           *interface,
             const gchar           *method,
             GVariant              *parameters,
             GDBusMethodInvocation *invocation,
             gpointer               data)
{
  GVariant **received = data;

  *received = g_variant_ref (parameters);
  g_dbus_method_invocation_return_value (invocation, g_variant_new ("(u)", 1));
}



static const GDBusInterfaceVTable vtable = { method_call, NULL, NULL, { 0 } };



static void
test_notify (void)
{
  GTestDBus       *bus;
  GDBusConnection *connection;
  GDBusNodeInfo   *info;
  GVariant        *received = NULL;
  GVariant        *hints;
  const gchar     *app_name;
  const gchar     *icon;
  const gchar     *summary;
  const gchar     *body;
  guint            id;
  guchar           urgency;

  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);
  connection = g_dbus_connection_new_for_address_sync (g_test_dbus_get_bus_address (bus),
                                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT
                                                       | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                       NULL, NULL, NULL);

  /* The server and the plugin on one connection: a call to its own
   * unique name is delivered like any other */
  info = g_dbus_node_info_new_for_xml (introspection, NULL);
  id = g_dbus_connection_register_object (connection, "/org/freedesktop/Notifications",
                                          info->interfaces[0], &vtable, &received, NULL, NULL);
  g_dbus_node_info_unref (info);
  g_variant_unref (g_dbus_connection_call_sync (connection, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                "org.freedesktop.DBus", "RequestName",
                                                g_variant_new ("(su)", "org.freedesktop.Notifications", 0),
                                                NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL));

  shopno_cal_timer_notify (connection, 25 * 60);
  {
    gint64 end = g_get_monotonic_time () + 5 * G_USEC_PER_SEC;

    while (received == NULL && g_get_monotonic_time () < end)
      g_main_context_iteration (NULL, TRUE);
  }
  g_assert_nonnull (received);

  g_variant_get (received, "(&su&s&s&s^a&s@a{sv}i)",
                 &app_name, NULL, &icon, &summary, &body, NULL, &hints, NULL);
  g_assert_cmpstr (app_name, ==, "Shopno Calendar");
  g_assert_cmpstr (icon, ==, "shopno-calendar");
  g_assert_cmpstr (summary, ==, "Timer finished");
  g_assert_cmpstr (body, ==, "25 minutes are up.");
  g_assert_true (g_variant_lookup (hints, "urgency", "y", &urgency));
  g_assert_cmpuint (urgency, ==, 1);

  g_variant_unref (hints);
  g_variant_unref (received);
  g_dbus_connection_unregister_object (connection, id);
  g_object_unref (connection);
  g_test_dbus_down (bus);
  g_object_unref (bus);
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

  g_test_add_func ("/timer/idle", test_idle);
  g_test_add_func ("/timer/run-pause-resume", test_run_pause_resume);
  g_test_add_func ("/timer/reset", test_reset);
  g_test_add_func ("/timer/describe", test_describe);

  /* As in test-sync: GTestDBus needs dbus-daemon */
  dbus_daemon = g_find_program_in_path ("dbus-daemon");
  g_test_add_func ("/timer/notify", dbus_daemon != NULL ? test_notify : test_skip_no_bus);
  g_free (dbus_daemon);

  return g_test_run ();
}
