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

#include <glib/gi18n-lib.h>

#include "shopno-cal-details.h"
#include "shopno-cal-month.h"



/* "Today", "In 23 days", "3 days ago" */
static gchar *
shopno_cal_details_relative (const GDate *day,
                             const GDate *today)
{
  gint days = g_date_days_between (today, day);

  switch (days)
    {
    case 0:
      return g_strdup (_("Today"));

    case 1:
      return g_strdup (_("Tomorrow"));

    case -1:
      return g_strdup (_("Yesterday"));

    default:
      if (days > 0)
        return g_strdup_printf (ngettext ("In %d day", "In %d days", days), days);
      return g_strdup_printf (ngettext ("%d day ago", "%d days ago", -days), -days);
    }
}



gchar *
shopno_cal_details_day (const GDate  *day,
                        const GDate  *today,
                        GDateWeekday  week_start)
{
  gchar *relative;
  gchar *text;

  g_return_val_if_fail (g_date_valid (day), NULL);
  g_return_val_if_fail (g_date_valid (today), NULL);

  relative = shopno_cal_details_relative (day, today);
  /* TRANSLATORS: the line under the calendar for the selected day, such
   * as "In 23 days · Week 49 · Day 337 of 365". %s is "Today", "In 23
   * days" or "3 days ago", then the week number, the day of the year and
   * the days in the year. */
  text = g_strdup_printf (_("%s · Week %u · Day %u of %u"),
                          relative,
                          shopno_cal_month_day_week (day, week_start),
                          g_date_get_day_of_year (day),
                          g_date_is_leap_year (g_date_get_year (day)) ? 366 : 365);
  g_free (relative);

  return text;
}



gchar *
shopno_cal_details_range (const GDate      *first,
                          const GDate      *last,
                          ShopnoCalWeekend  weekend)
{
  guint  days;
  guint  weeks;
  guint  working;
  gchar *apart;
  gchar *span = NULL;
  gchar *part;
  gchar *rest;
  gchar *text;

  g_return_val_if_fail (g_date_valid (first), NULL);
  g_return_val_if_fail (g_date_valid (last), NULL);

  days = ABS (g_date_days_between (first, last));
  weeks = days / 7;
  working = shopno_cal_month_working_days (first, last, weekend);

  apart = g_strdup_printf (ngettext ("%u day apart", "%u days apart", days), days);

  /* "3 weeks, 1 day", or "3 weeks"; nothing under a week */
  if (weeks > 0)
    {
      span = g_strdup_printf (ngettext ("%u week", "%u weeks", weeks), weeks);
      if (days % 7 > 0)
        {
          rest = g_strdup_printf (ngettext ("%u day", "%u days", days % 7), days % 7);
          /* TRANSLATORS: weeks and days, such as "3 weeks, 1 day" */
          part = g_strdup_printf (_("%s, %s"), span, rest);
          g_free (rest);
          g_free (span);
          span = part;
        }
    }

  part = g_strdup_printf (ngettext ("%u working day including both",
                                    "%u working days including both", working),
                          working);

  if (span != NULL)
    /* TRANSLATORS: the line under the calendar for two days, such as
     * "22 days apart (3 weeks, 1 day) · 17 working days including both" */
    text = g_strdup_printf (_("%s (%s) · %s"), apart, span, part);
  else
    /* TRANSLATORS: the line under the calendar for two days less than a
     * week apart, such as "4 days apart · 3 working days including both" */
    text = g_strdup_printf (_("%s · %s"), apart, part);

  g_free (part);
  g_free (span);
  g_free (apart);

  return text;
}
