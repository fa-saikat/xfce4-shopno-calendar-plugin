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

#include "shopno-cal-clock.h"
#include "shopno-cal-format.h"



gchar *
shopno_cal_format_panel (ShopnoCalLayout layout,
                         const gchar    *custom_format,
                         gboolean        use_24h,
                         gboolean        show_seconds,
                         gboolean        compact)
{
  gchar *time;
  gchar *format;

  if (layout == SHOPNO_CAL_LAYOUT_CUSTOM)
    return g_strdup (custom_format != NULL ? custom_format : SHOPNO_CAL_DEFAULT_CUSTOM_FORMAT);

  /* %-l is the hour without padding; %p is empty in locales without AM
   * and PM, and the space before it is stripped when formatting */
  time = g_strconcat (use_24h ? "%H:%M" : "%-l:%M",
                      show_seconds ? ":%S" : "",
                      use_24h ? "" : " %p",
                      NULL);

  switch (layout)
    {
    case SHOPNO_CAL_LAYOUT_TWO_LINES:
      /* %x is the locale's short date, as on Windows */
      format = g_strconcat (time, compact ? "\n%-d %b" : "\n%x", NULL);
      break;

    case SHOPNO_CAL_LAYOUT_ONE_LINE:
      format = g_strconcat (compact ? "%a\n%-d %b\n" : "%a %-d %b  ", time, NULL);
      break;

    case SHOPNO_CAL_LAYOUT_TIME:
    default:
      format = g_strdup (time);
      break;
    }

  g_free (time);

  return format;
}



/* The codes that change every second, from Xfce's Clock plugin
 * (clock_time_interval_from_format in plugins/clock/clock-time.c) */
guint
shopno_cal_format_interval (const gchar *format)
{
  const gchar *p;

  if (format == NULL)
    return SHOPNO_CAL_CLOCK_MINUTE;

  for (p = format; *p != '\0'; p++)
    {
      if (*p != '%' || p[1] == '\0')
        continue;

      p++;

      /* Skip GLib's padding and alternative-digit modifiers, as in %-S */
      while (*p == '-' || *p == '_' || *p == '0' || *p == '^' || *p == '#' || *p == 'E' || *p == 'O')
        p++;

      switch (*p)
        {
        case 'c':
        case 'N':
        case 'r':
        case 's':
        case 'S':
        case 'T':
        case 'X':
          return SHOPNO_CAL_CLOCK_SECOND;

        case '\0':
          return SHOPNO_CAL_CLOCK_MINUTE;

        default:
          break;
        }
    }

  return SHOPNO_CAL_CLOCK_MINUTE;
}



gchar *
shopno_cal_format_time (GDateTime   *time,
                        const gchar *format)
{
  gchar   *text;
  gchar  **lines;
  gchar   *result;
  gboolean empty = TRUE;

  g_return_val_if_fail (time != NULL, NULL);

  if (format == NULL)
    return NULL;

  text = g_date_time_format (time, format);
  if (text == NULL)
    return NULL;

  lines = g_strsplit (text, "\n", -1);
  for (gint i = 0; lines[i] != NULL; i++)
    {
      g_strstrip (lines[i]);
      if (*lines[i] != '\0')
        empty = FALSE;
    }

  result = empty ? NULL : g_strjoinv ("\n", lines);

  g_strfreev (lines);
  g_free (text);

  return result;
}
