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

#include "shopno-cal-sync.h"

#define TIMEDATE_NAME      "org.freedesktop.timedate1"
#define TIMEDATE_PATH      "/org/freedesktop/timedate1"
#define TIMEDATE_INTERFACE "org.freedesktop.timedate1"

enum
{
  CHANGED,
  N_SIGNALS
};

struct _ShopnoCalSync
{
  GObject             __parent__;

  GDBusConnection    *connection;
  guint               subscription;
  GCancellable       *cancellable;
  ShopnoCalSyncState  state;
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE (ShopnoCalSync, shopno_cal_sync, G_TYPE_OBJECT)



static void
shopno_cal_sync_set_state (ShopnoCalSync      *self,
                           ShopnoCalSyncState  state)
{
  if (self->state == state)
    return;

  self->state = state;
  g_signal_emit (self, signals[CHANGED], 0);
}



static void
shopno_cal_sync_got_properties (GObject      *source,
                                GAsyncResult *result,
                                gpointer      data)
{
  ShopnoCalSync *self;
  GVariant      *reply;
  GVariant      *properties;
  GError        *error = NULL;
  gboolean       ntp;
  gboolean       synchronized;

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), result, &error);

  /* Gone while the call was out */
  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_error_free (error);
      return;
    }

  self = SHOPNO_CAL_SYNC (data);

  /* No timedated, as on systems without systemd: show nothing */
  if (reply == NULL)
    {
      g_debug ("Failed to read the clock sync state: %s", error->message);
      g_error_free (error);
      shopno_cal_sync_set_state (self, SHOPNO_CAL_SYNC_UNKNOWN);
      return;
    }

  g_variant_get (reply, "(@a{sv})", &properties);

  if (!g_variant_lookup (properties, "NTP", "b", &ntp)
      || !g_variant_lookup (properties, "NTPSynchronized", "b", &synchronized))
    shopno_cal_sync_set_state (self, SHOPNO_CAL_SYNC_UNKNOWN);
  else if (!ntp)
    shopno_cal_sync_set_state (self, SHOPNO_CAL_SYNC_OFF);
  else
    shopno_cal_sync_set_state (self, synchronized ? SHOPNO_CAL_SYNC_SYNCED : SHOPNO_CAL_SYNC_NOT_SYNCED);

  g_variant_unref (properties);
  g_variant_unref (reply);
}



void
shopno_cal_sync_refresh (ShopnoCalSync *sync)
{
  g_return_if_fail (SHOPNO_IS_CAL_SYNC (sync));

  if (sync->connection == NULL)
    return;

  /* GetAll reads every property as it is now, also those that do not
   * announce their changes. It starts timedated when it is not running. */
  g_dbus_connection_call (sync->connection, TIMEDATE_NAME, TIMEDATE_PATH,
                          "org.freedesktop.DBus.Properties", "GetAll",
                          g_variant_new ("(s)", TIMEDATE_INTERFACE),
                          G_VARIANT_TYPE ("(a{sv})"), G_DBUS_CALL_FLAGS_NONE,
                          -1, sync->cancellable,
                          shopno_cal_sync_got_properties, sync);
}



static void
shopno_cal_sync_properties_changed (GDBusConnection *connection,
                                    const gchar     *sender,
                                    const gchar     *path,
                                    const gchar     *interface,
                                    const gchar     *signal,
                                    GVariant        *parameters,
                                    gpointer         data)
{
  shopno_cal_sync_refresh (SHOPNO_CAL_SYNC (data));
}



static void
shopno_cal_sync_set_connection (ShopnoCalSync   *self,
                                GDBusConnection *connection)
{
  self->connection = g_object_ref (connection);
  self->subscription =
    g_dbus_connection_signal_subscribe (connection, TIMEDATE_NAME,
                                        "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                        TIMEDATE_PATH, TIMEDATE_INTERFACE,
                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                        shopno_cal_sync_properties_changed, self, NULL);

  shopno_cal_sync_refresh (self);
}



static void
shopno_cal_sync_got_bus (GObject      *source,
                         GAsyncResult *result,
                         gpointer      data)
{
  GDBusConnection *connection;
  GError          *error = NULL;

  connection = g_bus_get_finish (result, &error);
  if (connection == NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_debug ("No system bus for the clock sync state: %s", error->message);
      g_error_free (error);
      return;
    }

  shopno_cal_sync_set_connection (SHOPNO_CAL_SYNC (data), connection);
  g_object_unref (connection);
}



static void
shopno_cal_sync_dispose (GObject *object)
{
  ShopnoCalSync *self = SHOPNO_CAL_SYNC (object);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);

  if (self->connection != NULL)
    {
      g_dbus_connection_signal_unsubscribe (self->connection, self->subscription);
      g_clear_object (&self->connection);
    }

  G_OBJECT_CLASS (shopno_cal_sync_parent_class)->dispose (object);
}



static void
shopno_cal_sync_class_init (ShopnoCalSyncClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = shopno_cal_sync_dispose;

  signals[CHANGED] =
    g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}



static void
shopno_cal_sync_init (ShopnoCalSync *self)
{
  self->cancellable = g_cancellable_new ();
  self->state = SHOPNO_CAL_SYNC_UNKNOWN;
}



ShopnoCalSync *
shopno_cal_sync_new (void)
{
  ShopnoCalSync *sync = g_object_new (SHOPNO_TYPE_CAL_SYNC, NULL);

  g_bus_get (G_BUS_TYPE_SYSTEM, sync->cancellable, shopno_cal_sync_got_bus, sync);

  return sync;
}



ShopnoCalSync *
shopno_cal_sync_new_for_connection (GDBusConnection *connection)
{
  ShopnoCalSync *sync;

  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);

  sync = g_object_new (SHOPNO_TYPE_CAL_SYNC, NULL);
  shopno_cal_sync_set_connection (sync, connection);

  return sync;
}



ShopnoCalSyncState
shopno_cal_sync_get_state (ShopnoCalSync *sync)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SYNC (sync), SHOPNO_CAL_SYNC_UNKNOWN);

  return sync->state;
}



const gchar *
shopno_cal_sync_describe (ShopnoCalSyncState state)
{
  switch (state)
    {
    case SHOPNO_CAL_SYNC_SYNCED:
      return _("Time synced automatically");

    case SHOPNO_CAL_SYNC_NOT_SYNCED:
      return _("Time not synced yet");

    case SHOPNO_CAL_SYNC_OFF:
      return _("Automatic time sync is off");

    case SHOPNO_CAL_SYNC_UNKNOWN:
    default:
      return NULL;
    }
}
