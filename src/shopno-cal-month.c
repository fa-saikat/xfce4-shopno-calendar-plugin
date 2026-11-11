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



GDateWeekday
shopno_cal_month_week_start (ShopnoCalWeekStart week_start)
{
  switch (week_start)
    {
    case SHOPNO_CAL_WEEK_START_MONDAY:
      return G_DATE_MONDAY;

    case SHOPNO_CAL_WEEK_START_SUNDAY:
      return G_DATE_SUNDAY;

    case SHOPNO_CAL_WEEK_START_SATURDAY:
      return G_DATE_SATURDAY;

    case SHOPNO_CAL_WEEK_START_LOCALE:
    default:
      return shopno_cal_month_locale_week_start ();
    }
}



gboolean
shopno_cal_month_is_weekend (ShopnoCalWeekend weekend,
                             GDateWeekday     weekday)
{
  switch (weekend)
    {
    case SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY:
      return weekday == G_DATE_SATURDAY || weekday == G_DATE_SUNDAY;

    case SHOPNO_CAL_WEEKEND_FRIDAY_SATURDAY:
      return weekday == G_DATE_FRIDAY || weekday == G_DATE_SATURDAY;

    case SHOPNO_CAL_WEEKEND_FRIDAY:
      return weekday == G_DATE_FRIDAY;

    case SHOPNO_CAL_WEEKEND_SUNDAY:
      return weekday == G_DATE_SUNDAY;

    case SHOPNO_CAL_WEEKEND_NONE:
    default:
      return FALSE;
    }
}



guint
shopno_cal_month_row_week (const GDate *start)
{
  GDate monday = *start;

  g_return_val_if_fail (g_date_valid (start), 0);

  g_date_add_days (&monday, (G_DATE_MONDAY - g_date_get_weekday (start) + 7) % 7);

  return g_date_get_iso8601_week_of_year (&monday);
}



guint
shopno_cal_month_day_week (const GDate  *day,
                           GDateWeekday  week_start)
{
  GDate start = *day;

  g_return_val_if_fail (g_date_valid (day), 0);
  g_return_val_if_fail (g_date_valid_weekday (week_start), 0);

  g_date_subtract_days (&start, (g_date_get_weekday (day) - week_start + 7) % 7);

  return shopno_cal_month_row_week (&start);
}



guint
shopno_cal_month_working_days (const GDate      *first,
                               const GDate      *last,
                               ShopnoCalWeekend  weekend)
{
  GDateWeekday weekday;
  guint        days;
  guint        per_week = 0;
  guint        count;
  guint        i;

  g_return_val_if_fail (g_date_valid (first) && g_date_valid (last), 0);

  if (g_date_compare (first, last) > 0)
    {
      const GDate *swap = first;
      first = last;
      last = swap;
    }

  days = g_date_days_between (first, last) + 1;

  /* Any seven days in a row hold each weekday once; count the whole weeks
   * at once and walk only the days left over */
  for (weekday = G_DATE_MONDAY; weekday <= G_DATE_SUNDAY; weekday++)
    per_week += !shopno_cal_month_is_weekend (weekend, weekday);

  count = days / 7 * per_week;
  weekday = g_date_get_weekday (first);

  for (i = 0; i < days % 7; i++)
    count += !shopno_cal_month_is_weekend (weekend, (weekday - 1 + i) % 7 + 1);

  return count;
}



gchar *
shopno_cal_month_weekday_name (GDateWeekday weekday,
                               gboolean     abbreviated)
{
  GDateTime *day;
  gchar     *name;

  g_return_val_if_fail (g_date_valid_weekday (weekday), NULL);

  /* 2024-01-01 is a Monday */
  day = g_date_time_new_utc (2024, 1, weekday, 0, 0, 0);
  name = g_date_time_format (day, abbreviated ? "%a" : "%A");
  g_date_time_unref (day);

  return name;
}
