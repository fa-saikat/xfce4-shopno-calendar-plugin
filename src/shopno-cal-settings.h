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

#ifndef __SHOPNO_CAL_SETTINGS_H__
#define __SHOPNO_CAL_SETTINGS_H__

#include <glib-object.h>

G_BEGIN_DECLS

/* What the panel button shows. Stored in xfconf as a number, so the
 * values must not change. */
typedef enum
{
  SHOPNO_CAL_LAYOUT_TWO_LINES = 0, /* time over date, like Windows 11 */
  SHOPNO_CAL_LAYOUT_ONE_LINE  = 1, /* weekday, date and time */
  SHOPNO_CAL_LAYOUT_TIME      = 2, /* time only */
  SHOPNO_CAL_LAYOUT_CUSTOM    = 3, /* the custom-format property */
} ShopnoCalLayout;

#define SHOPNO_CAL_LAYOUT_LAST SHOPNO_CAL_LAYOUT_CUSTOM

/* The first day of the week in the calendar. Stored in xfconf. */
typedef enum
{
  SHOPNO_CAL_WEEK_START_LOCALE   = 0, /* as in the user's region */
  SHOPNO_CAL_WEEK_START_MONDAY   = 1,
  SHOPNO_CAL_WEEK_START_SUNDAY   = 2,
  SHOPNO_CAL_WEEK_START_SATURDAY = 3,
} ShopnoCalWeekStart;

#define SHOPNO_CAL_WEEK_START_LAST SHOPNO_CAL_WEEK_START_SATURDAY

/* The days tinted as the weekend. Stored in xfconf. */
typedef enum
{
  SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY = 0,
  SHOPNO_CAL_WEEKEND_FRIDAY_SATURDAY = 1, /* Bangladesh, much of the Middle East */
  SHOPNO_CAL_WEEKEND_FRIDAY          = 2,
  SHOPNO_CAL_WEEKEND_SUNDAY          = 3,
  SHOPNO_CAL_WEEKEND_NONE            = 4,
} ShopnoCalWeekend;

#define SHOPNO_CAL_WEEKEND_LAST SHOPNO_CAL_WEEKEND_NONE

#define SHOPNO_CAL_DEFAULT_CUSTOM_FORMAT "%a %e %b, %H:%M"

/* The system's Date & Time settings, from gnome-system-tools or MATE */
#define SHOPNO_CAL_DEFAULT_TIME_SETTINGS_COMMAND "time-admin"

#define SHOPNO_TYPE_CAL_SETTINGS (shopno_cal_settings_get_type ())
G_DECLARE_FINAL_TYPE (ShopnoCalSettings, shopno_cal_settings, SHOPNO, CAL_SETTINGS, GObject)

ShopnoCalSettings *shopno_cal_settings_new               (void);

ShopnoCalLayout    shopno_cal_settings_get_layout        (ShopnoCalSettings *settings);
const gchar       *shopno_cal_settings_get_custom_format (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_use_24h       (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_show_seconds  (ShopnoCalSettings *settings);
ShopnoCalWeekStart shopno_cal_settings_get_week_start    (ShopnoCalSettings *settings);
ShopnoCalWeekend   shopno_cal_settings_get_weekend       (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_week_numbers  (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_day_details   (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_show_tooltip  (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_copy_button   (ShopnoCalSettings *settings);
const gchar       *shopno_cal_settings_get_time_settings_command (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_timer         (ShopnoCalSettings *settings);

G_END_DECLS

#endif /* !__SHOPNO_CAL_SETTINGS_H__ */
