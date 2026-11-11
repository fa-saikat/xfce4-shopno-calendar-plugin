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

#ifndef __SHOPNO_CAL_SYNC_H__
#define __SHOPNO_CAL_SYNC_H__

#include <gio/gio.h>

G_BEGIN_DECLS

/* Whether the system clock is kept right, as systemd-timedated reports */
typedef enum
{
  SHOPNO_CAL_SYNC_UNKNOWN,     /* no answer yet, or no timedated */
  SHOPNO_CAL_SYNC_SYNCED,      /* NTP is on and the clock is synchronized */
  SHOPNO_CAL_SYNC_NOT_SYNCED,  /* NTP is on, but not synchronized yet */
  SHOPNO_CAL_SYNC_OFF,         /* automatic time sync is off */
} ShopnoCalSyncState;

/* Reads the sync state from org.freedesktop.timedate1. Two of its
 * properties, NTPSynchronized and CanNTP, do not announce their changes,
 * so the state is read again on every PropertiesChanged and on
 * shopno_cal_sync_refresh(). Emits "changed" when the state changes. */
#define SHOPNO_TYPE_CAL_SYNC (shopno_cal_sync_get_type ())
G_DECLARE_FINAL_TYPE (ShopnoCalSync, shopno_cal_sync, SHOPNO, CAL_SYNC, GObject)

/* On the system bus */
ShopnoCalSync      *shopno_cal_sync_new                (void);

/* On a given bus, for the tests */
ShopnoCalSync      *shopno_cal_sync_new_for_connection (GDBusConnection *connection);

void                shopno_cal_sync_refresh            (ShopnoCalSync   *sync);

ShopnoCalSyncState  shopno_cal_sync_get_state          (ShopnoCalSync   *sync);

/* The footer's text for a state, translated, or NULL for UNKNOWN */
const gchar        *shopno_cal_sync_describe           (ShopnoCalSyncState state);

G_END_DECLS

#endif /* !__SHOPNO_CAL_SYNC_H__ */
