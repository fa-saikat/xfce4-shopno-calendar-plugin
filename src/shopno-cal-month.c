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

#ifdef HAVE_NL_TIME_FIRST_WEEKDAY
#include <langinfo.h>
#endif

#include "shopno-cal-month.h"



/* glibc keeps the week start as an offset from a reference day. Read the
 * same way as GtkCalendar does. */
GDateWeekday
shopno_cal_month_locale_week_start (void)
{
#ifdef HAVE_NL_TIME_FIRST_WEEKDAY
  union { unsigned int word; char *string; } langinfo;
  guint first_weekday;
  guint origin;

  langinfo.string = nl_langinfo (_NL_TIME_WEEK_1STDAY);
  origin = langinfo.word;
  first_weekday = (guchar) nl_langinfo (_NL_TIME_FIRST_WEEKDAY)[0];

  if (first_weekday >= 1 && first_weekday <= 7)
    {
      /* 1997-11-30 is a Sunday, 1997-12-01 a Monday */
      if (origin == 19971130)
        return (first_weekday + 5) % 7 + 1;
      if (origin == 19971201)
        return first_weekday;
    }
#endif

  return G_DATE_MONDAY;
}



void
shopno_cal_month_grid_start (GDateYear     year,
                             GDateMonth    month,
                             GDateWeekday  week_start,
                             GDate        *start)
{
  guint back;

  g_return_if_fail (g_date_valid_dmy (1, month, year));
  g_return_if_fail (g_date_valid_weekday (week_start));

  g_date_clear (start, 1);
  g_date_set_dmy (start, 1, month, year);

  back = (g_date_get_weekday (start) - week_start + 7) % 7;
  g_date_subtract_days (start, back);
}



gchar *
shopno_cal_month_weekday_name (GDateWeekday weekday)
{
  GDateTime *day;
  gchar     *name;

  g_return_val_if_fail (g_date_valid_weekday (weekday), NULL);

  /* 2024-01-01 is a Monday */
  day = g_date_time_new_utc (2024, 1, weekday, 0, 0, 0);
  name = g_date_time_format (day, "%a");
  g_date_time_unref (day);

  return name;
}
