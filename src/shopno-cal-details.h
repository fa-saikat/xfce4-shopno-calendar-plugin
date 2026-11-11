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

/* The line under the calendar for one day, such as
 * "In 23 days · Week 49 · Day 337 of 365". The week is numbered as the
 * grid numbers its rows when the week starts on week_start. */
gchar *shopno_cal_details_day   (const GDate      *day,
                                 const GDate      *today,
                                 GDateWeekday      week_start);

/* The line under the calendar for two days, such as
 * "22 days apart (3 weeks, 1 day) · 17 working days including both".
 * The order of the two days does not matter. */
gchar *shopno_cal_details_range (const GDate      *first,
                                 const GDate      *last,
                                 ShopnoCalWeekend  weekend);

G_END_DECLS

#endif /* !__SHOPNO_CAL_DETAILS_H__ */
