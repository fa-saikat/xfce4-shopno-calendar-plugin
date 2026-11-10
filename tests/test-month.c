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

#include "shopno-cal-month.h"



static void
check_start (GDateYear     year,
             GDateMonth    month,
             GDateWeekday  week_start,
             GDateYear     start_year,
             GDateMonth    start_month,
             GDateDay      start_day)
{
  GDate start;

  shopno_cal_month_grid_start (year, month, week_start, &start);

  g_assert_cmpuint (g_date_get_year (&start), ==, start_year);
  g_assert_cmpuint (g_date_get_month (&start), ==, start_month);
  g_assert_cmpuint (g_date_get_day (&start), ==, start_day);
  g_assert_cmpuint (g_date_get_weekday (&start), ==, week_start);
}



static void
test_grid_start (void)
{
  /* November 2026 begins on a Sunday */
  check_start (2026, G_DATE_NOVEMBER, G_DATE_SUNDAY, 2026, G_DATE_NOVEMBER, 1);
  check_start (2026, G_DATE_NOVEMBER, G_DATE_MONDAY, 2026, G_DATE_OCTOBER, 26);
  check_start (2026, G_DATE_NOVEMBER, G_DATE_SATURDAY, 2026, G_DATE_OCTOBER, 31);

  /* January 2027 begins on a Friday, so the grid starts in last year */
  check_start (2027, G_DATE_JANUARY, G_DATE_MONDAY, 2026, G_DATE_DECEMBER, 28);
  check_start (2027, G_DATE_JANUARY, G_DATE_FRIDAY, 2027, G_DATE_JANUARY, 1);

  /* Six weeks always reach the end of the month: March 2025 begins on a
   * Saturday and has 31 days */
  check_start (2025, G_DATE_MARCH, G_DATE_SUNDAY, 2025, G_DATE_FEBRUARY, 23);
}



static void
test_weekday_names (void)
{
  gchar *name;

  name = shopno_cal_month_weekday_name (G_DATE_MONDAY, TRUE);
  g_assert_cmpstr (name, ==, "Mon");
  g_free (name);

  name = shopno_cal_month_weekday_name (G_DATE_SUNDAY, TRUE);
  g_assert_cmpstr (name, ==, "Sun");
  g_free (name);

  name = shopno_cal_month_weekday_name (G_DATE_SATURDAY, FALSE);
  g_assert_cmpstr (name, ==, "Saturday");
  g_free (name);
}



static void
test_locale_week_start (void)
{
  /* The C locale starts the week on Sunday */
#ifdef HAVE_NL_TIME_FIRST_WEEKDAY
  g_assert_cmpuint (shopno_cal_month_locale_week_start (), ==, G_DATE_SUNDAY);
#else
  g_assert_cmpuint (shopno_cal_month_locale_week_start (), ==, G_DATE_MONDAY);
#endif
}



static void
test_week_start (void)
{
  g_assert_cmpuint (shopno_cal_month_week_start (SHOPNO_CAL_WEEK_START_MONDAY), ==, G_DATE_MONDAY);
  g_assert_cmpuint (shopno_cal_month_week_start (SHOPNO_CAL_WEEK_START_SUNDAY), ==, G_DATE_SUNDAY);
  g_assert_cmpuint (shopno_cal_month_week_start (SHOPNO_CAL_WEEK_START_SATURDAY), ==, G_DATE_SATURDAY);
  g_assert_cmpuint (shopno_cal_month_week_start (SHOPNO_CAL_WEEK_START_LOCALE),
                    ==, shopno_cal_month_locale_week_start ());
}



static void
test_weekend (void)
{
  GDateWeekday day;
  guint        count;

  g_assert_true (shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY, G_DATE_SATURDAY));
  g_assert_true (shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY, G_DATE_SUNDAY));
  g_assert_false (shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY, G_DATE_FRIDAY));

  g_assert_true (shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_FRIDAY_SATURDAY, G_DATE_FRIDAY));
  g_assert_true (shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_FRIDAY_SATURDAY, G_DATE_SATURDAY));
  g_assert_false (shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_FRIDAY_SATURDAY, G_DATE_SUNDAY));

  g_assert_true (shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_FRIDAY, G_DATE_FRIDAY));
  g_assert_true (shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_SUNDAY, G_DATE_SUNDAY));

  for (day = G_DATE_MONDAY, count = 0; day <= G_DATE_SUNDAY; day++)
    count += shopno_cal_month_is_weekend (SHOPNO_CAL_WEEKEND_NONE, day);
  g_assert_cmpuint (count, ==, 0);
}



static void
check_row_week (GDateYear  year,
                GDateMonth month,
                GDateDay   day,
                guint      expected)
{
  GDate start;

  g_date_clear (&start, 1);
  g_date_set_dmy (&start, day, month, year);
  g_assert_cmpuint (shopno_cal_month_row_week (&start), ==, expected);
}



static void
test_row_week (void)
{
  /* Monday 9 November 2026 is in ISO week 46 */
  check_row_week (2026, G_DATE_NOVEMBER, 9, 46);
  /* A Sunday row: Monday the 9th decides */
  check_row_week (2026, G_DATE_NOVEMBER, 8, 46);
  /* A Saturday row: Monday the 9th decides */
  check_row_week (2026, G_DATE_NOVEMBER, 7, 46);

  /* Around the new year: Monday 28 December 2026 is in week 53, and
   * Monday 4 January 2027 in week 1 */
  check_row_week (2026, G_DATE_DECEMBER, 28, 53);
  check_row_week (2027, G_DATE_JANUARY, 3, 1);
  /* Monday 29 December 2025 is already in week 1 of 2026 */
  check_row_week (2025, G_DATE_DECEMBER, 29, 1);
}



int
main (int    argc,
      char **argv)
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/month/grid-start", test_grid_start);
  g_test_add_func ("/month/weekday-names", test_weekday_names);
  g_test_add_func ("/month/locale-week-start", test_locale_week_start);
  g_test_add_func ("/month/week-start", test_week_start);
  g_test_add_func ("/month/weekend", test_weekend);
  g_test_add_func ("/month/row-week", test_row_week);

  return g_test_run ();
}
