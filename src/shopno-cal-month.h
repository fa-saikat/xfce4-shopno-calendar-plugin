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

#ifndef __SHOPNO_CAL_MONTH_H__
#define __SHOPNO_CAL_MONTH_H__

#include <glib.h>

#include "shopno-cal-settings.h"

G_BEGIN_DECLS

/* A month is shown as six weeks, so the grid keeps its height from one
 * month to the next */
#define SHOPNO_CAL_MONTH_WEEKS 6

/* Weekdays are numbered as GDateWeekday does, 1 for Monday to 7 for
 * Sunday */

/* The first day of the week in the user's locale */
GDateWeekday shopno_cal_month_locale_week_start (void);

/* The first day of the week for the week-start setting */
GDateWeekday shopno_cal_month_week_start        (ShopnoCalWeekStart week_start);

gboolean     shopno_cal_month_is_weekend        (ShopnoCalWeekend   weekend,
                                                 GDateWeekday       weekday);

/* The ISO 8601 week number of a row of the grid that begins on start.
 * A row that does not begin on Monday holds days of two ISO weeks; it
 * takes the number of the week its Monday belongs to, which covers most
 * of its days. */
guint        shopno_cal_month_row_week          (const GDate       *start);

/* The date of the top-left cell of the month's grid: the month's first
 * day, or the last week start before it */
void         shopno_cal_month_grid_start        (GDateYear     year,
                                                 GDateMonth    month,
                                                 GDateWeekday  week_start,
                                                 GDate        *start);

/* The weekday name in the user's locale, full or abbreviated */
gchar       *shopno_cal_month_weekday_name      (GDateWeekday  weekday,
                                                 gboolean      abbreviated);

G_END_DECLS

#endif /* !__SHOPNO_CAL_MONTH_H__ */
