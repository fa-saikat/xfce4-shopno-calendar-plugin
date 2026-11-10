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

/*
 * Opens the calendar flyout without the panel, so layout and CSS changes
 * can be tried without reinstalling the plugin. Settings are not saved.
 * Build with -Dpreview=true.
 */

#include "config.h"

#include <locale.h>

#include <gtk/gtk.h>

#include "shopno-cal-clock.h"
#include "shopno-cal-flyout.h"
#include "shopno-cal-settings.h"

static gboolean opt_12h = FALSE;
static gboolean opt_24h = FALSE;
static gboolean opt_no_grab = FALSE;

static const GOptionEntry entries[] =
{
  { "12h", 0, 0, G_OPTION_ARG_NONE, &opt_12h, "Use 12-hour time", NULL },
  { "24h", 0, 0, G_OPTION_ARG_NONE, &opt_24h, "Use 24-hour time", NULL },
  { "no-grab", 0, 0, G_OPTION_ARG_NONE, &opt_no_grab, "Do not grab the keyboard and pointer", NULL },
  { NULL }
};



int
main (int    argc,
      char **argv)
{
  ShopnoCalSettings *settings;
  ShopnoCalClock    *clock;
  GtkWidget         *flyout;
  GError            *error = NULL;

  setlocale (LC_ALL, "");

  if (!gtk_init_with_args (&argc, &argv, NULL, entries, NULL, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      return 1;
    }

  settings = shopno_cal_settings_new ();
  if (opt_12h || opt_24h)
    g_object_set (settings, "use-24h", opt_24h, NULL);

  clock = shopno_cal_clock_new ();
  shopno_cal_clock_set_interval (clock, SHOPNO_CAL_CLOCK_SECOND);

  flyout = shopno_cal_flyout_new (settings, clock);
  shopno_cal_flyout_set_grab_input (SHOPNO_CAL_FLYOUT (flyout), !opt_no_grab);
  g_signal_connect (flyout, "hide", G_CALLBACK (gtk_main_quit), NULL);

  shopno_cal_flyout_popup (SHOPNO_CAL_FLYOUT (flyout), 100, 100);
  gtk_main ();

  gtk_widget_destroy (flyout);
  g_object_unref (clock);
  g_object_unref (settings);

  return 0;
}
