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

#define SHOPNO_CAL_DEFAULT_CUSTOM_FORMAT "%a %e %b, %H:%M"

#define SHOPNO_TYPE_CAL_SETTINGS (shopno_cal_settings_get_type ())
G_DECLARE_FINAL_TYPE (ShopnoCalSettings, shopno_cal_settings, SHOPNO, CAL_SETTINGS, GObject)

ShopnoCalSettings *shopno_cal_settings_new               (void);

ShopnoCalLayout    shopno_cal_settings_get_layout        (ShopnoCalSettings *settings);
const gchar       *shopno_cal_settings_get_custom_format (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_use_24h       (ShopnoCalSettings *settings);
gboolean           shopno_cal_settings_get_show_seconds  (ShopnoCalSettings *settings);

G_END_DECLS

#endif /* !__SHOPNO_CAL_SETTINGS_H__ */
