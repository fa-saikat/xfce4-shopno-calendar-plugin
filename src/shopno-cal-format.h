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

#ifndef __SHOPNO_CAL_FORMAT_H__
#define __SHOPNO_CAL_FORMAT_H__

#include <glib.h>

#include "shopno-cal-settings.h"

G_BEGIN_DECLS

/* The g_date_time_format() format of the panel clock. Compact is for a
 * narrow vertical panel (Xfce's Deskbar mode), where the date and time
 * are stacked and the year is left out. */
gchar *shopno_cal_format_panel    (ShopnoCalLayout layout,
                                   const gchar    *custom_format,
                                   gboolean        use_24h,
                                   gboolean        show_seconds,
                                   gboolean        compact);

/* SHOPNO_CAL_CLOCK_SECOND when the format shows seconds, otherwise
 * SHOPNO_CAL_CLOCK_MINUTE */
guint  shopno_cal_format_interval (const gchar    *format);

/* Formats the time, or returns NULL when the format gives nothing, such
 * as a custom format with an unknown code */
gchar *shopno_cal_format_time     (GDateTime      *time,
                                   const gchar    *format);

G_END_DECLS

#endif /* !__SHOPNO_CAL_FORMAT_H__ */
