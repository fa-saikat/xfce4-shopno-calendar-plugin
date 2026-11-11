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

/* What the copy menu of the flyout offers, in its order */
typedef enum
{
  SHOPNO_CAL_COPY_ISO_DATE,  /* 2026-11-10 */
  SHOPNO_CAL_COPY_LOCAL_DATE, /* 10/11/2026, from the locale */
  SHOPNO_CAL_COPY_LONG_DATE, /* Tuesday, 10 November 2026 */
  SHOPNO_CAL_COPY_DATE_TIME, /* 2026-11-10 14:05 */
  SHOPNO_CAL_COPY_UNIX_TIME, /* 1794319509 */
  SHOPNO_CAL_COPY_N_FORMATS
} ShopnoCalCopyFormat;

/* The name of a copy format in the menu, translated */
const gchar *shopno_cal_format_copy_name (ShopnoCalCopyFormat format);

/* The time in a copy format, never NULL */
gchar       *shopno_cal_format_copy      (GDateTime          *time,
                                          ShopnoCalCopyFormat format);

G_END_DECLS

#endif /* !__SHOPNO_CAL_FORMAT_H__ */
