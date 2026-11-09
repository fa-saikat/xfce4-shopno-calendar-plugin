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

/* Run by meson test with LC_ALL=C.UTF-8 and TZ=UTC */

#include "config.h"

#include <locale.h>

#include "shopno-cal-clock.h"
#include "shopno-cal-format.h"



/* Tuesday, 10 November 2026, 07:05:09 */
static GDateTime *
sample_time (void)
{
  return g_date_time_new_utc (2026, 11, 10, 7, 5, 9);
}



static void
check_panel (ShopnoCalLayout layout,
             gboolean        use_24h,
             gboolean        show_seconds,
             gboolean        compact,
             const gchar    *expected)
{
  GDateTime *time = sample_time ();
  gchar     *format = shopno_cal_format_panel (layout, NULL, use_24h, show_seconds, compact);
  gchar     *text = shopno_cal_format_time (time, format);

  g_assert_cmpstr (text, ==, expected);

  g_free (text);
  g_free (format);
  g_date_time_unref (time);
}



static void
test_layouts (void)
{
  check_panel (SHOPNO_CAL_LAYOUT_TWO_LINES, TRUE, FALSE, FALSE, "07:05\n11/10/26");
  check_panel (SHOPNO_CAL_LAYOUT_TWO_LINES, TRUE, FALSE, TRUE, "07:05\n10 Nov");
  check_panel (SHOPNO_CAL_LAYOUT_ONE_LINE, TRUE, FALSE, FALSE, "Tue 10 Nov  07:05");
  check_panel (SHOPNO_CAL_LAYOUT_ONE_LINE, TRUE, FALSE, TRUE, "Tue\n10 Nov\n07:05");
  check_panel (SHOPNO_CAL_LAYOUT_TIME, TRUE, FALSE, FALSE, "07:05");
  check_panel (SHOPNO_CAL_LAYOUT_TIME, TRUE, TRUE, FALSE, "07:05:09");
  check_panel (SHOPNO_CAL_LAYOUT_TIME, FALSE, FALSE, FALSE, "7:05 AM");
  check_panel (SHOPNO_CAL_LAYOUT_TIME, FALSE, TRUE, FALSE, "7:05:09 AM");
}



static void
test_custom (void)
{
  GDateTime *time = sample_time ();
  gchar     *format;
  gchar     *text;

  format = shopno_cal_format_panel (SHOPNO_CAL_LAYOUT_CUSTOM, "%A%n%Y", TRUE, TRUE, TRUE);
  g_assert_cmpstr (format, ==, "%A%n%Y");
  text = shopno_cal_format_time (time, format);
  g_assert_cmpstr (text, ==, "Tuesday\n2026");
  g_free (text);
  g_free (format);

  /* Nothing to show */
  g_assert_null (shopno_cal_format_time (time, ""));
  g_assert_null (shopno_cal_format_time (time, "  \n "));
  g_assert_null (shopno_cal_format_time (time, NULL));

  g_date_time_unref (time);
}



static void
test_interval (void)
{
  g_assert_cmpuint (shopno_cal_format_interval ("%H:%M"), ==, SHOPNO_CAL_CLOCK_MINUTE);
  g_assert_cmpuint (shopno_cal_format_interval ("%H:%M:%S"), ==, SHOPNO_CAL_CLOCK_SECOND);
  g_assert_cmpuint (shopno_cal_format_interval ("%-S"), ==, SHOPNO_CAL_CLOCK_SECOND);
  g_assert_cmpuint (shopno_cal_format_interval ("%T"), ==, SHOPNO_CAL_CLOCK_SECOND);
  g_assert_cmpuint (shopno_cal_format_interval ("100%%S"), ==, SHOPNO_CAL_CLOCK_MINUTE);
  g_assert_cmpuint (shopno_cal_format_interval ("%"), ==, SHOPNO_CAL_CLOCK_MINUTE);
  g_assert_cmpuint (shopno_cal_format_interval ("%-"), ==, SHOPNO_CAL_CLOCK_MINUTE);
  g_assert_cmpuint (shopno_cal_format_interval (NULL), ==, SHOPNO_CAL_CLOCK_MINUTE);
}



static void
on_tick (ShopnoCalClock *clock,
         GMainLoop      *loop)
{
  g_main_loop_quit (loop);
}



static gboolean
on_timeout (gpointer data)
{
  g_assert_not_reached ();
  return G_SOURCE_REMOVE;
}



/* With a one-second interval the first tick comes at the next second,
 * close to its start */
static void
test_clock_tick (void)
{
  ShopnoCalClock *clock = shopno_cal_clock_new ();
  GMainLoop      *loop = g_main_loop_new (NULL, FALSE);
  guint           timeout_id;
  gint64          offset;

  shopno_cal_clock_set_interval (clock, SHOPNO_CAL_CLOCK_SECOND);
  g_signal_connect (clock, "tick", G_CALLBACK (on_tick), loop);
  timeout_id = g_timeout_add (1500, on_timeout, NULL);

  g_main_loop_run (loop);

  offset = g_get_real_time () % G_USEC_PER_SEC;
  g_assert_cmpint (offset, <, 100000);

  g_source_remove (timeout_id);
  g_main_loop_unref (loop);
  g_object_unref (clock);
}



int
main (int    argc,
      char **argv)
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/format/layouts", test_layouts);
  g_test_add_func ("/format/custom", test_custom);
  g_test_add_func ("/format/interval", test_interval);
  g_test_add_func ("/clock/tick", test_clock_tick);

  return g_test_run ();
}
