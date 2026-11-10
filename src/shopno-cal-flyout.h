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

#ifndef __SHOPNO_CAL_FLYOUT_H__
#define __SHOPNO_CAL_FLYOUT_H__

#include <gtk/gtk.h>

#include "shopno-cal-clock.h"
#include "shopno-cal-settings.h"

G_BEGIN_DECLS

#define SHOPNO_TYPE_CAL_FLYOUT (shopno_cal_flyout_get_type ())
G_DECLARE_FINAL_TYPE (ShopnoCalFlyout, shopno_cal_flyout, SHOPNO, CAL_FLYOUT, GtkWindow)

/* The calendar that opens from the panel clock. It shows seconds, so keep
 * the clock ticking every second while it is visible. */
GtkWidget *shopno_cal_flyout_new            (ShopnoCalSettings *settings,
                                             ShopnoCalClock    *clock);

/* Shows the current month and moves the window to x, y */
void       shopno_cal_flyout_popup          (ShopnoCalFlyout   *flyout,
                                             gint               x,
                                             gint               y);

void       shopno_cal_flyout_show_today     (ShopnoCalFlyout   *flyout);

void       shopno_cal_flyout_set_grab_input (ShopnoCalFlyout   *flyout,
                                             gboolean           grab_input);

G_END_DECLS

#endif /* !__SHOPNO_CAL_FLYOUT_H__ */
