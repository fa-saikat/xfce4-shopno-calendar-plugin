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

#ifndef __SHOPNO_CAL_DETAILS_H__
#define __SHOPNO_CAL_DETAILS_H__

#include <glib.h>

#include "shopno-cal-settings.h"

G_BEGIN_DECLS

/* A day in a g_date_time_format() format, or "" when the format gives
 * nothing, such as a broken translation */
gchar *shopno_cal_details_format      (const GDate      *date,
                                       const gchar      *format);

/* The details under the calendar have a title over a line of facts.
 *
 * For one day the title is "Today" or the full date, and the facts are
 * such as "In 23 days · Week 49 · Day 337 of 365", without the "Today"
 * part on today. The week is numbered as the grid numbers its rows when
 * the week starts on week_start. */
gchar *shopno_cal_details_day_title   (const GDate      *day,
                                       const GDate      *today);
gchar *shopno_cal_details_day         (const GDate      *day,
                                       const GDate      *today,
                                       GDateWeekday      week_start);

/* For two days the title is such as "10 Nov → 2 Dec 2026", and the facts
 * such as "22 days apart (3 weeks, 1 day) · 17 working days including
 * both". The order of the two days does not matter. */
gchar *shopno_cal_details_range_title (const GDate      *first,
                                       const GDate      *last);
gchar *shopno_cal_details_range       (const GDate      *first,
                                       const GDate      *last,
                                       ShopnoCalWeekend  weekend);

/* The Pango markup of the panel clock's tooltip: the full date in bold
 * over today's facts, dimmed, such as "Week 46 · Day 315 of 365" */
gchar *shopno_cal_details_tooltip     (const GDate      *today,
                                       GDateWeekday      week_start);

G_END_DECLS

#endif /* !__SHOPNO_CAL_DETAILS_H__ */
