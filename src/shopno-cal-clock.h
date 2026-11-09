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

#ifndef __SHOPNO_CAL_CLOCK_H__
#define __SHOPNO_CAL_CLOCK_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define SHOPNO_CAL_CLOCK_SECOND 1
#define SHOPNO_CAL_CLOCK_MINUTE 60

/* The time source of the plugin. It emits "tick" at the start of every
 * second or minute of the wall clock, and also right away when the
 * computer wakes up, the clock is set, or the time zone changes. */
#define SHOPNO_TYPE_CAL_CLOCK (shopno_cal_clock_get_type ())
G_DECLARE_FINAL_TYPE (ShopnoCalClock, shopno_cal_clock, SHOPNO, CAL_CLOCK, GObject)

ShopnoCalClock *shopno_cal_clock_new          (void);

void            shopno_cal_clock_set_interval (ShopnoCalClock *clock,
                                               guint           seconds);

/* The current local time, in the time zone the system uses right now */
GDateTime      *shopno_cal_clock_get_now      (ShopnoCalClock *clock);

G_END_DECLS

#endif /* !__SHOPNO_CAL_CLOCK_H__ */
