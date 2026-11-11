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

#ifndef __SHOPNO_CAL_TIMER_H__
#define __SHOPNO_CAL_TIMER_H__

#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum
{
  SHOPNO_CAL_TIMER_IDLE,    /* set to its whole duration */
  SHOPNO_CAL_TIMER_RUNNING,
  SHOPNO_CAL_TIMER_PAUSED,
} ShopnoCalTimerState;

/* A countdown, as the Focus timer in Windows. It counts in the monotonic
 * clock, so a change of the system time does not move it; the system
 * being suspended pauses it.
 *
 * "changed" is emitted when the state or the duration changes, not every
 * second: whoever shows the remaining time reads it as often as it
 * redraws. "finished" is emitted when the time is up, after which the
 * timer is idle again, set to the same duration. */
#define SHOPNO_TYPE_CAL_TIMER (shopno_cal_timer_get_type ())
G_DECLARE_FINAL_TYPE (ShopnoCalTimer, shopno_cal_timer, SHOPNO, CAL_TIMER, GObject)

ShopnoCalTimer      *shopno_cal_timer_new           (void);

/* Stops the timer and sets it to a new duration, in seconds */
void                 shopno_cal_timer_set_duration  (ShopnoCalTimer *timer,
                                                     guint           seconds);
guint                shopno_cal_timer_get_duration  (ShopnoCalTimer *timer);

ShopnoCalTimerState  shopno_cal_timer_get_state     (ShopnoCalTimer *timer);

/* The time left, in whole seconds rounded up, so a timer shows its full
 * duration when started and 0 only when it is up */
guint                shopno_cal_timer_get_remaining (ShopnoCalTimer *timer);

/* The share of the duration left, from 1.0 down to 0.0 */
gdouble              shopno_cal_timer_get_fraction  (ShopnoCalTimer *timer);

/* Starts an idle timer, or resumes a paused one */
void                 shopno_cal_timer_start         (ShopnoCalTimer *timer);
void                 shopno_cal_timer_pause         (ShopnoCalTimer *timer);

/* Stops the timer and sets it back to its whole duration */
void                 shopno_cal_timer_reset         (ShopnoCalTimer *timer);

/* Shows the desktop notification that the timer is up, through
 * org.freedesktop.Notifications on a bus, or on the session bus when
 * connection is NULL. Its Restart action starts the timer again; starting
 * the timer any other way closes it. */
void                 shopno_cal_timer_notify        (ShopnoCalTimer  *timer,
                                                     GDBusConnection *connection);

/* The notification's text for a duration, in seconds */
gchar               *shopno_cal_timer_describe      (guint            seconds);

/* A readout of a time in seconds: 05:00, or 1:05:00 from an hour */
gchar               *shopno_cal_timer_format        (guint            seconds);

G_END_DECLS

#endif /* !__SHOPNO_CAL_TIMER_H__ */
