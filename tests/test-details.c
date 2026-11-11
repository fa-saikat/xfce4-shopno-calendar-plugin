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

/* Run by meson test with LC_ALL=C.UTF-8 */

#include "config.h"

#include <locale.h>

#include <pango/pango.h>

#include "shopno-cal-details.h"



static GDate
date (GDateYear  year,
      GDateMonth month,
      GDateDay   day)
{
  GDate date;

  g_date_clear (&date, 1);
  g_date_set_dmy (&date, day, month, year);

  return date;
}



static void
check_day (GDate        day,
           GDateWeekday week_start,
           const gchar *title,
           const gchar *facts)
{
  /* Tuesday 10 November 2026 */
  GDate  today = date (2026, G_DATE_NOVEMBER, 10);
  gchar *text;

  text = shopno_cal_details_day_title (&day, &today);
  g_assert_cmpstr (text, ==, title);
  g_free (text);

  text = shopno_cal_details_day (&day, &today, week_start);
  g_assert_cmpstr (text, ==, facts);
  g_free (text);
}



static void
test_day (void)
{
  check_day (date (2026, G_DATE_NOVEMBER, 10), G_DATE_MONDAY,
             "Today", "Week 46 · Day 314 of 365");
  check_day (date (2026, G_DATE_DECEMBER, 3), G_DATE_MONDAY,
             "Thursday, 3 December 2026", "In 23 days · Week 49 · Day 337 of 365");
  check_day (date (2026, G_DATE_NOVEMBER, 11), G_DATE_MONDAY,
             "Wednesday, 11 November 2026", "Tomorrow · Week 46 · Day 315 of 365");
  check_day (date (2026, G_DATE_NOVEMBER, 9), G_DATE_MONDAY,
             "Monday, 9 November 2026", "Yesterday · Week 46 · Day 313 of 365");
  check_day (date (2026, G_DATE_NOVEMBER, 2), G_DATE_MONDAY,
             "Monday, 2 November 2026", "8 days ago · Week 45 · Day 306 of 365");

  /* The week of the row the day is in */
  check_day (date (2026, G_DATE_NOVEMBER, 8), G_DATE_MONDAY,
             "Sunday, 8 November 2026", "2 days ago · Week 45 · Day 312 of 365");
  check_day (date (2026, G_DATE_NOVEMBER, 8), G_DATE_SUNDAY,
             "Sunday, 8 November 2026", "2 days ago · Week 46 · Day 312 of 365");

  /* A leap year */
  check_day (date (2028, G_DATE_DECEMBER, 31), G_DATE_MONDAY,
             "Sunday, 31 December 2028", "In 782 days · Week 52 · Day 366 of 366");
}



static void
check_range_title (GDate        first,
                   GDate        last,
                   const gchar *expected)
{
  gchar *text = shopno_cal_details_range_title (&first, &last);

  g_assert_cmpstr (text, ==, expected);
  g_free (text);
}



static void
test_range_title (void)
{
  /* The year once when both days are in it, in either order */
  check_range_title (date (2026, G_DATE_NOVEMBER, 10), date (2026, G_DATE_DECEMBER, 2),
                     "10 Nov → 2 Dec 2026");
  check_range_title (date (2026, G_DATE_DECEMBER, 2), date (2026, G_DATE_NOVEMBER, 10),
                     "10 Nov → 2 Dec 2026");
  check_range_title (date (2026, G_DATE_DECEMBER, 28), date (2027, G_DATE_JANUARY, 4),
                     "28 Dec 2026 → 4 Jan 2027");
}



static void
check_range (GDate             first,
             GDate             last,
             ShopnoCalWeekend  weekend,
             const gchar      *expected)
{
  gchar *text = shopno_cal_details_range (&first, &last, weekend);

  g_assert_cmpstr (text, ==, expected);
  g_free (text);
}



static void
test_range (void)
{
  GDate tuesday = date (2026, G_DATE_NOVEMBER, 10);

  check_range (tuesday, date (2026, G_DATE_DECEMBER, 2), SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY,
               "22 days apart (3 weeks, 1 day) · 17 working days including both");
  /* Either order */
  check_range (date (2026, G_DATE_DECEMBER, 2), tuesday, SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY,
               "22 days apart (3 weeks, 1 day) · 17 working days including both");
  check_range (tuesday, date (2026, G_DATE_DECEMBER, 2), SHOPNO_CAL_WEEKEND_FRIDAY,
               "22 days apart (3 weeks, 1 day) · 20 working days including both");

  /* Whole weeks, under a week, and one */
  check_range (tuesday, date (2026, G_DATE_NOVEMBER, 24), SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY,
               "14 days apart (2 weeks) · 11 working days including both");
  check_range (tuesday, date (2026, G_DATE_NOVEMBER, 17), SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY,
               "7 days apart (1 week) · 6 working days including both");
  check_range (tuesday, date (2026, G_DATE_NOVEMBER, 14), SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY,
               "4 days apart · 4 working days including both");
  check_range (date (2026, G_DATE_NOVEMBER, 14), date (2026, G_DATE_NOVEMBER, 15),
               SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY,
               "1 day apart · 0 working days including both");
  check_range (date (2026, G_DATE_NOVEMBER, 13), date (2026, G_DATE_NOVEMBER, 14),
               SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY,
               "1 day apart · 1 working day including both");
}



static void
test_tooltip (void)
{
  GDate  today = date (2026, G_DATE_NOVEMBER, 10);
  gchar *markup;

  markup = shopno_cal_details_tooltip (&today, G_DATE_MONDAY);
  g_assert_cmpstr (markup, ==,
                   "<b>Tuesday, 10 November 2026</b>\n"
                   "<span alpha=\"60%\">Week 46 · Day 314 of 365</span>");
  g_assert_true (pango_parse_markup (markup, -1, 0, NULL, NULL, NULL, NULL));
  g_free (markup);

  /* Sunday 8 November is in the row numbered 46 when the week starts on
   * Sunday, as in the flyout */
  today = date (2026, G_DATE_NOVEMBER, 8);
  markup = shopno_cal_details_tooltip (&today, G_DATE_SUNDAY);
  g_assert_cmpstr (markup, ==,
                   "<b>Sunday, 8 November 2026</b>\n"
                   "<span alpha=\"60%\">Week 46 · Day 312 of 365</span>");
  g_free (markup);
}



int
main (int    argc,
      char **argv)
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/details/day", test_day);
  g_test_add_func ("/details/range-title", test_range_title);
  g_test_add_func ("/details/range", test_range);
  g_test_add_func ("/details/tooltip", test_tooltip);

  return g_test_run ();
}
