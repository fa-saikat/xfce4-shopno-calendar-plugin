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



static void
test_format (void)
{
  gchar *text;

  text = shopno_cal_timer_format (0);
  g_assert_cmpstr (text, ==, "00:00");
  g_free (text);

  text = shopno_cal_timer_format (25 * 60);
  g_assert_cmpstr (text, ==, "25:00");
  g_free (text);

  text = shopno_cal_timer_format (59 * 60 + 59);
  g_assert_cmpstr (text, ==, "59:59");
  g_free (text);

  text = shopno_cal_timer_format (3600 + 5 * 60 + 7);
  g_assert_cmpstr (text, ==, "1:05:07");
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
  "    <method name='CloseNotification'>"
  "      <arg type='u' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";

/* What the fake server was asked */
typedef struct
{
  GVariant *notify;
  guint     closed;
} Server;



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
  Server *server = data;

  if (g_strcmp0 (method, "Notify") == 0)
    {
      g_clear_pointer (&server->notify, g_variant_unref);
      server->notify = g_variant_ref (parameters);
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(u)", 7));
    }
  else
    {
      g_variant_get (parameters, "(u)", &server->closed);
      g_dbus_method_invocation_return_value (invocation, NULL);
    }
}



static const GDBusInterfaceVTable vtable = { method_call, NULL, NULL, { 0 } };



/* Runs the main loop until a condition, for up to five seconds */
#define WAIT_FOR(condition)                                             \
  G_STMT_START {                                                        \
    gint64 end_ = g_get_monotonic_time () + 5 * G_USEC_PER_SEC;         \
    while (!(condition) && g_get_monotonic_time () < end_)              \
      g_main_context_iteration (NULL, FALSE);                           \
  } G_STMT_END



static void
emit_action (GDBusConnection *connection,
             guint            id,
             const gchar     *action)
{
  g_dbus_connection_emit_signal (connection, NULL, "/org/freedesktop/Notifications",
                                 "org.freedesktop.Notifications", "ActionInvoked",
                                 g_variant_new ("(us)", id, action), NULL);
  g_dbus_connection_flush_sync (connection, NULL, NULL);
}



static void
test_notify (void)
{
  GTestDBus       *bus;
  GDBusConnection *connection;
  GDBusNodeInfo   *info;
  ShopnoCalTimer  *timer;
  Server           server = { NULL, 0 };
  GVariant        *hints;
  const gchar     *app_name;
  const gchar     *icon;
  const gchar     *summary;
  const gchar     *body;
  const gchar    **actions;
  guint            replaces;
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
                                          info->interfaces[0], &vtable, &server, NULL, NULL);
  g_dbus_node_info_unref (info);
  g_variant_unref (g_dbus_connection_call_sync (connection, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                "org.freedesktop.DBus", "RequestName",
                                                g_variant_new ("(su)", "org.freedesktop.Notifications", 0),
                                                NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL));

  timer = shopno_cal_timer_new ();
  shopno_cal_timer_set_duration (timer, 25 * 60);
  shopno_cal_timer_notify (timer, connection);
  WAIT_FOR (server.notify != NULL);
  g_assert_nonnull (server.notify);

  g_variant_get (server.notify, "(&su&s&s&s^a&s@a{sv}i)",
                 &app_name, &replaces, &icon, &summary, &body, &actions, &hints, NULL);
  g_assert_cmpstr (app_name, ==, "Shopno Calendar");
  g_assert_cmpuint (replaces, ==, 0);
  g_assert_cmpstr (icon, ==, "shopno-calendar");
  g_assert_cmpstr (summary, ==, "Timer finished");
  g_assert_cmpstr (body, ==, "25 minutes are up.");
  g_assert_cmpuint (g_strv_length ((gchar **) actions), ==, 2);
  g_assert_cmpstr (actions[0], ==, "restart");
  g_assert_cmpstr (actions[1], ==, "Restart");
  g_assert_true (g_variant_lookup (hints, "urgency", "y", &urgency));
  g_assert_cmpuint (urgency, ==, 1);
  g_free (actions);
  g_variant_unref (hints);

  /* The next one replaces it, once the server has answered with its id */
  g_clear_pointer (&server.notify, g_variant_unref);
  run (200, NULL);
  shopno_cal_timer_notify (timer, connection);
  WAIT_FOR (server.notify != NULL);
  g_variant_get_child (server.notify, 1, "u", &replaces);
  g_assert_cmpuint (replaces, ==, 7);
  run (200, NULL);

  /* Another notification's action, or another action, does nothing */
  emit_action (connection, 8, "restart");
  emit_action (connection, 7, "default");
  run (200, NULL);
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_IDLE);

  /* Restart starts the timer again, and the server closes the
   * notification itself */
  emit_action (connection, 7, "restart");
  WAIT_FOR (shopno_cal_timer_get_state (timer) == SHOPNO_CAL_TIMER_RUNNING);
  g_assert_cmpint (shopno_cal_timer_get_state (timer), ==, SHOPNO_CAL_TIMER_RUNNING);
  run (200, NULL);
  g_assert_cmpuint (server.closed, ==, 0);

  /* Started from the flyout instead, the timer closes it */
  shopno_cal_timer_reset (timer);
  shopno_cal_timer_notify (timer, connection);
  run (200, NULL);
  shopno_cal_timer_start (timer);
  WAIT_FOR (server.closed != 0);
  g_assert_cmpuint (server.closed, ==, 7);

  g_object_unref (timer);
  g_clear_pointer (&server.notify, g_variant_unref);
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
  g_test_add_func ("/timer/format", test_format);

  /* As in test-sync: GTestDBus needs dbus-daemon */
  dbus_daemon = g_find_program_in_path ("dbus-daemon");
  g_test_add_func ("/timer/notify", dbus_daemon != NULL ? test_notify : test_skip_no_bus);
  g_free (dbus_daemon);

  return g_test_run ();
}
