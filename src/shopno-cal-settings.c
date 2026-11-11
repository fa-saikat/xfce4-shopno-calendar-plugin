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

#include <langinfo.h>
#include <string.h>

#include "shopno-cal-settings.h"

enum
{
  PROP_0,
  PROP_LAYOUT,
  PROP_CUSTOM_FORMAT,
  PROP_USE_24H,
  PROP_SHOW_SECONDS,
  PROP_WEEK_START,
  PROP_WEEKEND,
  PROP_WEEK_NUMBERS,
  PROP_DAY_DETAILS,
  PROP_SHOW_TOOLTIP,
  PROP_COPY_BUTTON,
  PROP_TIME_SETTINGS_COMMAND,
  PROP_TIMER,
  N_PROPERTIES
};

struct _ShopnoCalSettings
{
  GObject  __parent__;

  guint    layout;
  gchar   *custom_format;
  gboolean use_24h;
  gboolean show_seconds;
  guint    week_start;
  guint    weekend;
  gboolean week_numbers;
  gboolean day_details;
  gboolean show_tooltip;
  gboolean copy_button;
  gchar   *time_settings_command;
  gboolean timer;
};

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE (ShopnoCalSettings, shopno_cal_settings, G_TYPE_OBJECT)



/* 12 or 24-hour time follows the locale until the user picks one, like
 * Windows does with the regional format */
static gboolean
shopno_cal_settings_locale_uses_24h (void)
{
  const gchar *format = nl_langinfo (T_FMT);

  return format == NULL
         || (strstr (format, "%I") == NULL
             && strstr (format, "%l") == NULL
             && strstr (format, "%r") == NULL);
}



static void
shopno_cal_settings_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ShopnoCalSettings *self = SHOPNO_CAL_SETTINGS (object);

  switch (prop_id)
    {
    case PROP_LAYOUT:
      g_value_set_uint (value, self->layout);
      break;

    case PROP_CUSTOM_FORMAT:
      g_value_set_string (value, self->custom_format);
      break;

    case PROP_USE_24H:
      g_value_set_boolean (value, self->use_24h);
      break;

    case PROP_SHOW_SECONDS:
      g_value_set_boolean (value, self->show_seconds);
      break;

    case PROP_WEEK_START:
      g_value_set_uint (value, self->week_start);
      break;

    case PROP_WEEKEND:
      g_value_set_uint (value, self->weekend);
      break;

    case PROP_WEEK_NUMBERS:
      g_value_set_boolean (value, self->week_numbers);
      break;

    case PROP_DAY_DETAILS:
      g_value_set_boolean (value, self->day_details);
      break;

    case PROP_SHOW_TOOLTIP:
      g_value_set_boolean (value, self->show_tooltip);
      break;

    case PROP_COPY_BUTTON:
      g_value_set_boolean (value, self->copy_button);
      break;

    case PROP_TIME_SETTINGS_COMMAND:
      g_value_set_string (value, self->time_settings_command);
      break;

    case PROP_TIMER:
      g_value_set_boolean (value, self->timer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
shopno_cal_settings_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ShopnoCalSettings *self = SHOPNO_CAL_SETTINGS (object);
  const gchar       *str;

  switch (prop_id)
    {
    case PROP_LAYOUT:
      if (self->layout != g_value_get_uint (value))
        {
          self->layout = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_CUSTOM_FORMAT:
      str = g_value_get_string (value);
      if (str == NULL)
        str = SHOPNO_CAL_DEFAULT_CUSTOM_FORMAT;
      if (g_strcmp0 (self->custom_format, str) != 0)
        {
          g_free (self->custom_format);
          self->custom_format = g_strdup (str);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_USE_24H:
      if (self->use_24h != g_value_get_boolean (value))
        {
          self->use_24h = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_SHOW_SECONDS:
      if (self->show_seconds != g_value_get_boolean (value))
        {
          self->show_seconds = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_WEEK_START:
      if (self->week_start != g_value_get_uint (value))
        {
          self->week_start = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_WEEKEND:
      if (self->weekend != g_value_get_uint (value))
        {
          self->weekend = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_WEEK_NUMBERS:
      if (self->week_numbers != g_value_get_boolean (value))
        {
          self->week_numbers = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_DAY_DETAILS:
      if (self->day_details != g_value_get_boolean (value))
        {
          self->day_details = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_SHOW_TOOLTIP:
      if (self->show_tooltip != g_value_get_boolean (value))
        {
          self->show_tooltip = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_COPY_BUTTON:
      if (self->copy_button != g_value_get_boolean (value))
        {
          self->copy_button = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_TIME_SETTINGS_COMMAND:
      if (g_strcmp0 (self->time_settings_command, g_value_get_string (value)) != 0)
        {
          g_free (self->time_settings_command);
          self->time_settings_command = g_value_dup_string (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_TIMER:
      if (self->timer != g_value_get_boolean (value))
        {
          self->timer = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
shopno_cal_settings_finalize (GObject *object)
{
  ShopnoCalSettings *self = SHOPNO_CAL_SETTINGS (object);

  g_free (self->custom_format);
  g_free (self->time_settings_command);

  G_OBJECT_CLASS (shopno_cal_settings_parent_class)->finalize (object);
}



static void
shopno_cal_settings_class_init (ShopnoCalSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = shopno_cal_settings_get_property;
  object_class->set_property = shopno_cal_settings_set_property;
  object_class->finalize = shopno_cal_settings_finalize;

  properties[PROP_LAYOUT] =
    g_param_spec_uint ("layout", NULL, NULL,
                       0, SHOPNO_CAL_LAYOUT_LAST, SHOPNO_CAL_LAYOUT_TWO_LINES,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_CUSTOM_FORMAT] =
    g_param_spec_string ("custom-format", NULL, NULL,
                         SHOPNO_CAL_DEFAULT_CUSTOM_FORMAT,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /* The default is replaced with the locale's in init */
  properties[PROP_USE_24H] =
    g_param_spec_boolean ("use-24h", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_SHOW_SECONDS] =
    g_param_spec_boolean ("show-seconds", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_WEEK_START] =
    g_param_spec_uint ("week-start", NULL, NULL,
                       0, SHOPNO_CAL_WEEK_START_LAST, SHOPNO_CAL_WEEK_START_LOCALE,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_WEEKEND] =
    g_param_spec_uint ("weekend", NULL, NULL,
                       0, SHOPNO_CAL_WEEKEND_LAST, SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_WEEK_NUMBERS] =
    g_param_spec_boolean ("week-numbers", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_DAY_DETAILS] =
    g_param_spec_boolean ("day-details", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_SHOW_TOOLTIP] =
    g_param_spec_boolean ("show-tooltip", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_COPY_BUTTON] =
    g_param_spec_boolean ("copy-button", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /* Empty for SHOPNO_CAL_DEFAULT_TIME_SETTINGS_COMMAND */
  properties[PROP_TIME_SETTINGS_COMMAND] =
    g_param_spec_string ("time-settings-command", NULL, NULL,
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_TIMER] =
    g_param_spec_boolean ("timer", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}



static void
shopno_cal_settings_init (ShopnoCalSettings *self)
{
  self->layout = SHOPNO_CAL_LAYOUT_TWO_LINES;
  self->custom_format = g_strdup (SHOPNO_CAL_DEFAULT_CUSTOM_FORMAT);
  self->use_24h = shopno_cal_settings_locale_uses_24h ();
  self->show_seconds = FALSE;
  self->week_start = SHOPNO_CAL_WEEK_START_LOCALE;
  self->weekend = SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY;
  self->week_numbers = FALSE;
  self->day_details = TRUE;
  self->show_tooltip = TRUE;
  self->copy_button = TRUE;
  self->time_settings_command = g_strdup ("");
  self->timer = TRUE;
}



ShopnoCalSettings *
shopno_cal_settings_new (void)
{
  return g_object_new (SHOPNO_TYPE_CAL_SETTINGS, NULL);
}



ShopnoCalLayout
shopno_cal_settings_get_layout (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), SHOPNO_CAL_LAYOUT_TWO_LINES);

  return settings->layout;
}



const gchar *
shopno_cal_settings_get_custom_format (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), SHOPNO_CAL_DEFAULT_CUSTOM_FORMAT);

  return settings->custom_format;
}



gboolean
shopno_cal_settings_get_use_24h (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), TRUE);

  return settings->use_24h;
}



gboolean
shopno_cal_settings_get_show_seconds (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), FALSE);

  return settings->show_seconds;
}



ShopnoCalWeekStart
shopno_cal_settings_get_week_start (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), SHOPNO_CAL_WEEK_START_LOCALE);

  return settings->week_start;
}



ShopnoCalWeekend
shopno_cal_settings_get_weekend (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY);

  return settings->weekend;
}



gboolean
shopno_cal_settings_get_week_numbers (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), FALSE);

  return settings->week_numbers;
}



gboolean
shopno_cal_settings_get_day_details (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), TRUE);

  return settings->day_details;
}



gboolean
shopno_cal_settings_get_show_tooltip (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), TRUE);

  return settings->show_tooltip;
}



gboolean
shopno_cal_settings_get_copy_button (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), TRUE);

  return settings->copy_button;
}



/* The command of the settings button: the one set, or the default */
const gchar *
shopno_cal_settings_get_time_settings_command (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), SHOPNO_CAL_DEFAULT_TIME_SETTINGS_COMMAND);

  if (settings->time_settings_command == NULL || *settings->time_settings_command == '\0')
    return SHOPNO_CAL_DEFAULT_TIME_SETTINGS_COMMAND;

  return settings->time_settings_command;
}



gboolean
shopno_cal_settings_get_timer (ShopnoCalSettings *settings)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), TRUE);

  return settings->timer;
}
