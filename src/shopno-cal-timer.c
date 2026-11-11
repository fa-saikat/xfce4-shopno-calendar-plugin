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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "shopno-cal-timer.h"

#define DEFAULT_DURATION (5 * 60)

#define NOTIFICATIONS_NAME      "org.freedesktop.Notifications"
#define NOTIFICATIONS_PATH      "/org/freedesktop/Notifications"
#define NOTIFICATIONS_INTERFACE "org.freedesktop.Notifications"
#define RESTART_ACTION          "restart"

enum
{
  CHANGED,
  FINISHED,
  N_SIGNALS
};

struct _ShopnoCalTimer
{
  GObject             __parent__;

  ShopnoCalTimerState state;

  /* In microseconds. While running the time left is end - now, otherwise
   * it is remaining. */
  gint64              duration;
  gint64              remaining;
  gint64              end;

  guint               timeout_id;

  /* The notification shown, if any, and the bus it is on */
  GDBusConnection    *bus;
  guint               action_id;
  guint               closed_id;
  guint               notification;
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE (ShopnoCalTimer, shopno_cal_timer, G_TYPE_OBJECT)



static gint64
shopno_cal_timer_left (ShopnoCalTimer *self)
{
  if (self->state == SHOPNO_CAL_TIMER_RUNNING)
    return MAX (self->end - g_get_monotonic_time (), 0);

  return self->remaining;
}



static void
shopno_cal_timer_stop (ShopnoCalTimer *self)
{
  g_clear_handle_id (&self->timeout_id, g_source_remove);
  self->state = SHOPNO_CAL_TIMER_IDLE;
  self->remaining = self->duration;
}



static gboolean shopno_cal_timer_timeout (gpointer data);

static void
shopno_cal_timer_schedule (ShopnoCalTimer *self)
{
  gint64 left = shopno_cal_timer_left (self);

  /* Round up, so the timeout never fires before the end */
  self->timeout_id = g_timeout_add ((left + 999) / 1000, shopno_cal_timer_timeout, self);
}



static gboolean
shopno_cal_timer_timeout (gpointer data)
{
  ShopnoCalTimer *self = SHOPNO_CAL_TIMER (data);

  self->timeout_id = 0;

  /* Timeouts may fire a little early */
  if (shopno_cal_timer_left (self) > 0)
    {
      shopno_cal_timer_schedule (self);
      return G_SOURCE_REMOVE;
    }

  shopno_cal_timer_stop (self);
  g_signal_emit (self, signals[CHANGED], 0);
  g_signal_emit (self, signals[FINISHED], 0);

  return G_SOURCE_REMOVE;
}



static void
shopno_cal_timer_close_notification (ShopnoCalTimer *self)
{
  if (self->notification == 0)
    return;

  g_dbus_connection_call (self->bus, NOTIFICATIONS_NAME, NOTIFICATIONS_PATH, NOTIFICATIONS_INTERFACE,
                          "CloseNotification", g_variant_new ("(u)", self->notification),
                          NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
  self->notification = 0;
}



static void
shopno_cal_timer_unwatch (ShopnoCalTimer *self)
{
  if (self->bus == NULL)
    return;

  g_dbus_connection_signal_unsubscribe (self->bus, self->action_id);
  g_dbus_connection_signal_unsubscribe (self->bus, self->closed_id);
  g_clear_object (&self->bus);
  self->notification = 0;
}



static void
shopno_cal_timer_finalize (GObject *object)
{
  ShopnoCalTimer *self = SHOPNO_CAL_TIMER (object);

  g_clear_handle_id (&self->timeout_id, g_source_remove);
  shopno_cal_timer_unwatch (self);

  G_OBJECT_CLASS (shopno_cal_timer_parent_class)->finalize (object);
}



static void
shopno_cal_timer_class_init (ShopnoCalTimerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shopno_cal_timer_finalize;

  signals[CHANGED] =
    g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
  signals[FINISHED] =
    g_signal_new ("finished", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}



static void
shopno_cal_timer_init (ShopnoCalTimer *self)
{
  self->state = SHOPNO_CAL_TIMER_IDLE;
  self->duration = DEFAULT_DURATION * G_USEC_PER_SEC;
  self->remaining = self->duration;
}



ShopnoCalTimer *
shopno_cal_timer_new (void)
{
  return g_object_new (SHOPNO_TYPE_CAL_TIMER, NULL);
}



void
shopno_cal_timer_set_duration (ShopnoCalTimer *timer,
                               guint           seconds)
{
  g_return_if_fail (SHOPNO_IS_CAL_TIMER (timer));
  g_return_if_fail (seconds > 0);

  timer->duration = (gint64) seconds * G_USEC_PER_SEC;
  shopno_cal_timer_stop (timer);
  g_signal_emit (timer, signals[CHANGED], 0);
}



guint
shopno_cal_timer_get_duration (ShopnoCalTimer *timer)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_TIMER (timer), 0);

  return timer->duration / G_USEC_PER_SEC;
}



ShopnoCalTimerState
shopno_cal_timer_get_state (ShopnoCalTimer *timer)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_TIMER (timer), SHOPNO_CAL_TIMER_IDLE);

  return timer->state;
}



guint
shopno_cal_timer_get_remaining (ShopnoCalTimer *timer)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_TIMER (timer), 0);

  return (shopno_cal_timer_left (timer) + G_USEC_PER_SEC - 1) / G_USEC_PER_SEC;
}



gdouble
shopno_cal_timer_get_fraction (ShopnoCalTimer *timer)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_TIMER (timer), 0.0);

  return (gdouble) shopno_cal_timer_left (timer) / timer->duration;
}



void
shopno_cal_timer_start (ShopnoCalTimer *timer)
{
  g_return_if_fail (SHOPNO_IS_CAL_TIMER (timer));

  if (timer->state == SHOPNO_CAL_TIMER_RUNNING)
    return;

  shopno_cal_timer_close_notification (timer);

  timer->end = g_get_monotonic_time () + timer->remaining;
  timer->state = SHOPNO_CAL_TIMER_RUNNING;
  shopno_cal_timer_schedule (timer);
  g_signal_emit (timer, signals[CHANGED], 0);
}



void
shopno_cal_timer_pause (ShopnoCalTimer *timer)
{
  g_return_if_fail (SHOPNO_IS_CAL_TIMER (timer));

  if (timer->state != SHOPNO_CAL_TIMER_RUNNING)
    return;

  timer->remaining = shopno_cal_timer_left (timer);
  timer->state = SHOPNO_CAL_TIMER_PAUSED;
  g_clear_handle_id (&timer->timeout_id, g_source_remove);
  g_signal_emit (timer, signals[CHANGED], 0);
}



void
shopno_cal_timer_reset (ShopnoCalTimer *timer)
{
  g_return_if_fail (SHOPNO_IS_CAL_TIMER (timer));

  if (timer->state == SHOPNO_CAL_TIMER_IDLE)
    return;

  shopno_cal_timer_stop (timer);
  g_signal_emit (timer, signals[CHANGED], 0);
}



gchar *
shopno_cal_timer_describe (guint seconds)
{
  if (seconds % 60 == 0)
    return g_strdup_printf (ngettext ("%u minute is up.", "%u minutes are up.", seconds / 60),
                            seconds / 60);

  return g_strdup_printf (ngettext ("%u second is up.", "%u seconds are up.", seconds), seconds);
}



gchar *
shopno_cal_timer_format (guint seconds)
{
  if (seconds >= 3600)
    return g_strdup_printf ("%u:%02u:%02u", seconds / 3600, seconds / 60 % 60, seconds % 60);

  return g_strdup_printf ("%02u:%02u", seconds / 60, seconds % 60);
}



static void
shopno_cal_timer_action_invoked (GDBusConnection *connection,
                                 const gchar     *sender,
                                 const gchar     *path,
                                 const gchar     *interface,
                                 const gchar     *signal,
                                 GVariant        *parameters,
                                 gpointer         data)
{
  ShopnoCalTimer *self = SHOPNO_CAL_TIMER (data);
  const gchar    *action;
  guint           id;

  g_variant_get (parameters, "(u&s)", &id, &action);
  if (id == 0 || id != self->notification || g_strcmp0 (action, RESTART_ACTION) != 0)
    return;

  /* The server closes the notification itself */
  self->notification = 0;
  if (self->state == SHOPNO_CAL_TIMER_IDLE)
    shopno_cal_timer_start (self);
}



static void
shopno_cal_timer_notification_closed (GDBusConnection *connection,
                                      const gchar     *sender,
                                      const gchar     *path,
                                      const gchar     *interface,
                                      const gchar     *signal,
                                      GVariant        *parameters,
                                      gpointer         data)
{
  ShopnoCalTimer *self = SHOPNO_CAL_TIMER (data);
  guint           id;

  g_variant_get (parameters, "(uu)", &id, NULL);
  if (id == self->notification)
    self->notification = 0;
}



static void
shopno_cal_timer_watch (ShopnoCalTimer  *self,
                        GDBusConnection *connection)
{
  if (self->bus == connection)
    return;

  shopno_cal_timer_unwatch (self);
  self->bus = g_object_ref (connection);
  self->action_id =
    g_dbus_connection_signal_subscribe (connection, NOTIFICATIONS_NAME, NOTIFICATIONS_INTERFACE,
                                        "ActionInvoked", NOTIFICATIONS_PATH, NULL,
                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                        shopno_cal_timer_action_invoked, self, NULL);
  self->closed_id =
    g_dbus_connection_signal_subscribe (connection, NOTIFICATIONS_NAME, NOTIFICATIONS_INTERFACE,
                                        "NotificationClosed", NOTIFICATIONS_PATH, NULL,
                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                        shopno_cal_timer_notification_closed, self, NULL);
}



/* Holds a reference to the timer, so it outlives the call */
static void
shopno_cal_timer_notified (GObject      *source,
                           GAsyncResult *result,
                           gpointer      data)
{
  ShopnoCalTimer *self = SHOPNO_CAL_TIMER (data);
  GVariant       *reply;
  GError         *error = NULL;

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), result, &error);
  if (reply == NULL)
    {
      g_warning ("Failed to show the timer notification: %s", error->message);
      g_error_free (error);
    }
  else
    {
      /* Started again before the server answered: the notification is
       * already out of date */
      g_variant_get (reply, "(u)", &self->notification);
      if (self->state != SHOPNO_CAL_TIMER_IDLE)
        shopno_cal_timer_close_notification (self);
      g_variant_unref (reply);
    }

  g_object_unref (self);
}



static void
shopno_cal_timer_send (ShopnoCalTimer  *self,
                       GDBusConnection *connection)
{
  GVariantBuilder  hints;
  const gchar     *actions[] = { RESTART_ACTION, _("Restart"), NULL };
  gchar           *body = shopno_cal_timer_describe (shopno_cal_timer_get_duration (self));

  shopno_cal_timer_watch (self, connection);

  g_variant_builder_init (&hints, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&hints, "{sv}", "urgency", g_variant_new_byte (1));
  /* From the freedesktop sound theme, for servers that play sounds */
  g_variant_builder_add (&hints, "{sv}", "sound-name", g_variant_new_string ("alarm-clock-elapsed"));

  /* Replaces the last one, if it is still shown */
  g_dbus_connection_call (connection, NOTIFICATIONS_NAME, NOTIFICATIONS_PATH, NOTIFICATIONS_INTERFACE,
                          "Notify",
                          g_variant_new ("(susss^asa{sv}i)",
                                         _("Shopno Calendar"), self->notification, "shopno-calendar",
                                         _("Timer finished"), body, actions, &hints, -1),
                          G_VARIANT_TYPE ("(u)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                          shopno_cal_timer_notified, g_object_ref (self));
  g_free (body);
}



static void
shopno_cal_timer_got_session (GObject      *source,
                              GAsyncResult *result,
                              gpointer      data)
{
  ShopnoCalTimer  *self = SHOPNO_CAL_TIMER (data);
  GDBusConnection *connection;
  GError          *error = NULL;

  connection = g_bus_get_finish (result, &error);
  if (connection == NULL)
    {
      g_warning ("Failed to show the timer notification: %s", error->message);
      g_error_free (error);
    }
  else
    {
      shopno_cal_timer_send (self, connection);
      g_object_unref (connection);
    }

  g_object_unref (self);
}



void
shopno_cal_timer_notify (ShopnoCalTimer  *timer,
                         GDBusConnection *connection)
{
  g_return_if_fail (SHOPNO_IS_CAL_TIMER (timer));
  g_return_if_fail (connection == NULL || G_IS_DBUS_CONNECTION (connection));

  if (connection != NULL)
    shopno_cal_timer_send (timer, connection);
  else
    g_bus_get (G_BUS_TYPE_SESSION, NULL, shopno_cal_timer_got_session, g_object_ref (timer));
}
