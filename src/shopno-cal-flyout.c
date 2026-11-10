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

#include "shopno-cal-flyout.h"
#include "shopno-cal-month.h"

/* The pointer is often still grabbed by the panel button when the flyout
 * maps, so retry the seat grab a few times before giving up (from Shopno
 * Menu) */
#define GRAB_RETRY_INTERVAL 50
#define GRAB_MAX_RETRIES    10

enum
{
  PROP_0,
  PROP_SETTINGS,
  PROP_CLOCK,
  N_PROPERTIES
};

struct _ShopnoCalFlyout
{
  GtkWindow          __parent__;

  ShopnoCalSettings *settings;
  ShopnoCalClock    *clock;

  GtkWidget         *frame;
  GtkWidget         *time_label;
  GtkWidget         *seconds_label;
  GtkWidget         *date_button;
  GtkWidget         *title_label;
  GtkWidget         *weekday_labels[7];
  GtkWidget         *day_labels[SHOPNO_CAL_MONTH_WEEKS * 7];

  /* The month shown, and today, to notice midnight */
  GDateYear          year;
  GDateMonth         month;
  GDate              today;
  GDateWeekday       week_start;

  gboolean           grab_input;
  GdkSeat           *grab_seat;
  guint              grab_retry_id;
  guint              grab_retries;
};

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE (ShopnoCalFlyout, shopno_cal_flyout, GTK_TYPE_WINDOW)



static void
shopno_cal_flyout_load_css (GdkScreen *screen)
{
  static gboolean  loaded = FALSE;
  GtkCssProvider  *provider;

  if (loaded)
    return;

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, "/org/shopnoos/calendar/shopno-calendar.css");
  gtk_style_context_add_provider_for_screen (screen,
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  loaded = TRUE;
}



/* With a compositor the corners are rounded and the window manager draws
 * the shadow; without one the window is a plain box */
static void
shopno_cal_flyout_update_visual (ShopnoCalFlyout *self)
{
  GtkWidget       *widget = GTK_WIDGET (self);
  GdkScreen       *screen = gtk_widget_get_screen (widget);
  GdkVisual       *rgba_visual = gdk_screen_get_rgba_visual (screen);
  GtkStyleContext *context = gtk_widget_get_style_context (widget);
  gboolean         composited = rgba_visual != NULL && gdk_screen_is_composited (screen);
  GdkVisual       *visual = composited ? rgba_visual : gdk_screen_get_system_visual (screen);

  /* The visual can only change while unrealized */
  if (visual != gtk_widget_get_visual (widget))
    {
      if (gtk_widget_get_realized (widget))
        gtk_widget_unrealize (widget);
      gtk_widget_set_visual (widget, visual);
    }

  gtk_widget_set_app_paintable (widget, composited);

  if (composited)
    gtk_style_context_add_class (context, "composited");
  else
    gtk_style_context_remove_class (context, "composited");
}



static void
shopno_cal_flyout_set_class (GtkWidget   *widget,
                             const gchar *name,
                             gboolean     set)
{
  GtkStyleContext *context = gtk_widget_get_style_context (widget);

  if (set)
    gtk_style_context_add_class (context, name);
  else
    gtk_style_context_remove_class (context, name);
}



static gchar *
shopno_cal_flyout_format_date (const GDate *date,
                               const gchar *format)
{
  GDateTime *time;
  gchar     *text;

  time = g_date_time_new_utc (g_date_get_year (date), g_date_get_month (date),
                              g_date_get_day (date), 0, 0, 0);
  text = g_date_time_format (time, format);
  g_date_time_unref (time);

  return text;
}



/*
 * Header
 */

static void
shopno_cal_flyout_update_header (ShopnoCalFlyout *self,
                                 GDateTime       *now)
{
  gboolean  use_24h = shopno_cal_settings_get_use_24h (self->settings);
  gchar    *text;

  text = g_date_time_format (now, use_24h ? "%H:%M" : "%-l:%M");
  gtk_label_set_text (GTK_LABEL (self->time_label), text);
  g_free (text);

  /* %p is empty in locales without AM and PM */
  text = g_date_time_format (now, use_24h ? ":%S" : ":%S %p");
  g_strchomp (text);
  gtk_label_set_text (GTK_LABEL (self->seconds_label), text);
  g_free (text);

  /* TRANSLATORS: the date in the flyout header, see
   * https://docs.gtk.org/glib/method.DateTime.format.html */
  text = g_date_time_format (now, _("%A, %-d %B %Y"));
  gtk_button_set_label (GTK_BUTTON (self->date_button), text);
  g_free (text);
}



/*
 * Month grid
 */

static void
shopno_cal_flyout_update_weekdays (ShopnoCalFlyout *self)
{
  guint  i;
  gchar *name;

  for (i = 0; i < 7; i++)
    {
      name = shopno_cal_month_weekday_name ((self->week_start - 1 + i) % 7 + 1);
      gtk_label_set_text (GTK_LABEL (self->weekday_labels[i]), name);
      g_free (name);
    }
}



static void
shopno_cal_flyout_show_month (ShopnoCalFlyout *self,
                              GDateYear        year,
                              GDateMonth       month)
{
  GDate  date;
  GDate  first;
  gchar *text;
  gchar  number[4];
  guint  i;

  self->year = year;
  self->month = month;

  g_date_clear (&first, 1);
  g_date_set_dmy (&first, 1, month, year);
  /* TRANSLATORS: the month above the calendar grid, see
   * https://docs.gtk.org/glib/method.DateTime.format.html */
  text = shopno_cal_flyout_format_date (&first, _("%B %Y"));
  gtk_label_set_text (GTK_LABEL (self->title_label), text);
  g_free (text);

  shopno_cal_month_grid_start (year, month, self->week_start, &date);

  for (i = 0; i < G_N_ELEMENTS (self->day_labels); i++)
    {
      GtkWidget *label = self->day_labels[i];

      g_snprintf (number, sizeof (number), "%u", g_date_get_day (&date));
      gtk_label_set_text (GTK_LABEL (label), number);

      shopno_cal_flyout_set_class (label, "other-month", g_date_get_month (&date) != month);
      shopno_cal_flyout_set_class (label, "today", g_date_compare (&date, &self->today) == 0);

      g_date_add_days (&date, 1);
    }
}



static void
shopno_cal_flyout_update (ShopnoCalFlyout *self)
{
  GDateTime *now = shopno_cal_clock_get_now (self->clock);
  GDate      today;
  gboolean   showing_today;

  shopno_cal_flyout_update_header (self, now);

  g_date_clear (&today, 1);
  g_date_set_dmy (&today, g_date_time_get_day_of_month (now),
                  g_date_time_get_month (now), g_date_time_get_year (now));

  /* Midnight, or a new time zone: move the today square, and follow it
   * into the next month when the current month is shown */
  if (!g_date_valid (&self->today) || g_date_compare (&today, &self->today) != 0)
    {
      showing_today = !g_date_valid (&self->today)
                      || (self->year == g_date_get_year (&self->today)
                          && self->month == g_date_get_month (&self->today));
      self->today = today;

      if (showing_today)
        shopno_cal_flyout_show_month (self, g_date_get_year (&today), g_date_get_month (&today));
      else
        shopno_cal_flyout_show_month (self, self->year, self->month);
    }

  g_date_time_unref (now);
}



static void
shopno_cal_flyout_tick (ShopnoCalFlyout *self)
{
  if (gtk_widget_get_visible (GTK_WIDGET (self)))
    shopno_cal_flyout_update (self);
}



/*
 * Input grab, as in Shopno Menu
 */

static gboolean
shopno_cal_flyout_try_grab (gpointer data)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (data);
  GdkWindow       *window = gtk_widget_get_window (GTK_WIDGET (self));
  GdkSeat         *seat;

  seat = gdk_display_get_default_seat (gtk_widget_get_display (GTK_WIDGET (self)));

  /* owner_events is TRUE so widgets of this process, including the panel
   * button, keep receiving their own events while the flyout is open */
  if (gdk_seat_grab (seat, window, GDK_SEAT_CAPABILITY_ALL, TRUE,
                     NULL, NULL, NULL, NULL) == GDK_GRAB_SUCCESS)
    {
      self->grab_seat = seat;
      self->grab_retry_id = 0;
      return G_SOURCE_REMOVE;
    }

  if (++self->grab_retries >= GRAB_MAX_RETRIES)
    {
      g_warning ("Failed to grab input, the calendar will close on focus loss only");
      self->grab_retry_id = 0;
      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}



static void
shopno_cal_flyout_ungrab (ShopnoCalFlyout *self)
{
  g_clear_handle_id (&self->grab_retry_id, g_source_remove);

  if (self->grab_seat != NULL)
    {
      gdk_seat_ungrab (self->grab_seat);
      self->grab_seat = NULL;
    }
}



/*
 * GtkWidget
 */

static gboolean
shopno_cal_flyout_map_event (GtkWidget   *widget,
                             GdkEventAny *event)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (widget);

  if (self->grab_input)
    {
      self->grab_retries = 0;
      if (shopno_cal_flyout_try_grab (self) == G_SOURCE_CONTINUE)
        self->grab_retry_id = g_timeout_add (GRAB_RETRY_INTERVAL, shopno_cal_flyout_try_grab, self);
    }

  return GTK_WIDGET_CLASS (shopno_cal_flyout_parent_class)->map_event (widget, event);
}



static void
shopno_cal_flyout_hide (GtkWidget *widget)
{
  shopno_cal_flyout_ungrab (SHOPNO_CAL_FLYOUT (widget));

  GTK_WIDGET_CLASS (shopno_cal_flyout_parent_class)->hide (widget);
}



static gboolean
shopno_cal_flyout_button_press_event (GtkWidget      *widget,
                                      GdkEventButton *event)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (widget);
  GtkAllocation    alloc;
  gint             x, y;

  /* With the seat grabbed, clicks anywhere on screen arrive here. Close
   * when the click lands outside the visible frame. */
  gdk_window_get_origin (gtk_widget_get_window (widget), &x, &y);
  gtk_widget_get_allocation (self->frame, &alloc);
  x += alloc.x;
  y += alloc.y;

  if (event->x_root < x || event->x_root >= x + alloc.width
      || event->y_root < y || event->y_root >= y + alloc.height)
    {
      gtk_widget_hide (widget);
      return GDK_EVENT_STOP;
    }

  return GTK_WIDGET_CLASS (shopno_cal_flyout_parent_class)->button_press_event (widget, event);
}



static gboolean
shopno_cal_flyout_key_press_event (GtkWidget   *widget,
                                   GdkEventKey *event)
{
  if (event->keyval == GDK_KEY_Escape)
    {
      gtk_widget_hide (widget);
      return GDK_EVENT_STOP;
    }

  return GTK_WIDGET_CLASS (shopno_cal_flyout_parent_class)->key_press_event (widget, event);
}



static gboolean
shopno_cal_flyout_focus_out_event (GtkWidget     *widget,
                                   GdkEventFocus *event)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (widget);

  /* Fallback for when the input grab failed */
  if (self->grab_input && self->grab_seat == NULL)
    gtk_widget_hide (widget);

  return GTK_WIDGET_CLASS (shopno_cal_flyout_parent_class)->focus_out_event (widget, event);
}



/*
 * GObject
 */

static void
shopno_cal_flyout_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      self->settings = g_value_dup_object (value);
      break;

    case PROP_CLOCK:
      self->clock = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
shopno_cal_flyout_constructed (GObject *object)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (object);

  G_OBJECT_CLASS (shopno_cal_flyout_parent_class)->constructed (object);

  g_signal_connect_swapped (self->settings, "notify::use-24h",
                            G_CALLBACK (shopno_cal_flyout_tick), self);
  g_signal_connect_swapped (self->clock, "tick",
                            G_CALLBACK (shopno_cal_flyout_tick), self);

  shopno_cal_flyout_update (self);
}



static void
shopno_cal_flyout_dispose (GObject *object)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (object);

  shopno_cal_flyout_ungrab (self);

  if (self->settings != NULL)
    {
      g_signal_handlers_disconnect_by_data (self->settings, self);
      g_clear_object (&self->settings);
    }

  if (self->clock != NULL)
    {
      g_signal_handlers_disconnect_by_data (self->clock, self);
      g_clear_object (&self->clock);
    }

  G_OBJECT_CLASS (shopno_cal_flyout_parent_class)->dispose (object);
}



static void
shopno_cal_flyout_class_init (ShopnoCalFlyoutClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->set_property = shopno_cal_flyout_set_property;
  gobject_class->constructed = shopno_cal_flyout_constructed;
  gobject_class->dispose = shopno_cal_flyout_dispose;

  widget_class->map_event = shopno_cal_flyout_map_event;
  widget_class->hide = shopno_cal_flyout_hide;
  widget_class->button_press_event = shopno_cal_flyout_button_press_event;
  widget_class->key_press_event = shopno_cal_flyout_key_press_event;
  widget_class->focus_out_event = shopno_cal_flyout_focus_out_event;

  properties[PROP_SETTINGS] =
    g_param_spec_object ("settings", NULL, NULL, SHOPNO_TYPE_CAL_SETTINGS,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  properties[PROP_CLOCK] =
    g_param_spec_object ("clock", NULL, NULL, SHOPNO_TYPE_CAL_CLOCK,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}



static GtkWidget *
shopno_cal_flyout_label_new (const gchar *style_class)
{
  GtkWidget *label = gtk_label_new (NULL);

  gtk_style_context_add_class (gtk_widget_get_style_context (label), style_class);

  return label;
}



static void
shopno_cal_flyout_init (ShopnoCalFlyout *self)
{
  GtkWindow *window = GTK_WINDOW (self);
  GtkWidget *header;
  GtkWidget *time_box;
  GtkWidget *month;
  GtkWidget *grid;
  guint      i;

  self->grab_input = TRUE;
  self->week_start = shopno_cal_month_locale_week_start ();
  g_date_clear (&self->today, 1);

  shopno_cal_flyout_load_css (gtk_widget_get_screen (GTK_WIDGET (self)));

  gtk_window_set_title (window, _("Calendar"));
  gtk_window_set_decorated (window, FALSE);
  gtk_window_set_resizable (window, FALSE);
  gtk_window_set_skip_taskbar_hint (window, TRUE);
  gtk_window_set_skip_pager_hint (window, TRUE);
  gtk_window_set_keep_above (window, TRUE);
  gtk_window_stick (window);

  /* Like Shopno Menu: xfwm4 gives menu windows a menu shadow */
  gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_MENU);

  gtk_widget_add_events (GTK_WIDGET (self), GDK_BUTTON_PRESS_MASK);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)), "shopno-cal-flyout");
  shopno_cal_flyout_update_visual (self);

  self->frame = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->frame), "shopno-cal-frame");
  gtk_container_add (GTK_CONTAINER (self), self->frame);

  /* Header: the time, large, with smaller seconds, and the date */
  header = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (header), "shopno-cal-header");
  gtk_box_pack_start (GTK_BOX (self->frame), header, FALSE, FALSE, 0);

  time_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (header), time_box, FALSE, FALSE, 0);

  self->time_label = shopno_cal_flyout_label_new ("shopno-cal-time");
  gtk_widget_set_valign (self->time_label, GTK_ALIGN_BASELINE);
  gtk_box_pack_start (GTK_BOX (time_box), self->time_label, FALSE, FALSE, 0);

  self->seconds_label = shopno_cal_flyout_label_new ("shopno-cal-seconds");
  gtk_widget_set_valign (self->seconds_label, GTK_ALIGN_BASELINE);
  gtk_box_pack_start (GTK_BOX (time_box), self->seconds_label, FALSE, FALSE, 0);

  self->date_button = gtk_button_new_with_label ("");
  gtk_button_set_relief (GTK_BUTTON (self->date_button), GTK_RELIEF_NONE);
  gtk_widget_set_halign (self->date_button, GTK_ALIGN_START);
  gtk_widget_set_tooltip_text (self->date_button, _("Go to today"));
  gtk_style_context_add_class (gtk_widget_get_style_context (self->date_button), "shopno-cal-date");
  g_signal_connect_swapped (self->date_button, "clicked",
                            G_CALLBACK (shopno_cal_flyout_show_today), self);
  gtk_box_pack_start (GTK_BOX (header), self->date_button, FALSE, FALSE, 0);

  /* Month: its name over the grid of six weeks */
  month = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (month), "shopno-cal-month");
  gtk_box_pack_start (GTK_BOX (self->frame), month, FALSE, FALSE, 0);

  self->title_label = shopno_cal_flyout_label_new ("shopno-cal-title");
  gtk_label_set_xalign (GTK_LABEL (self->title_label), 0.0);
  gtk_box_pack_start (GTK_BOX (month), self->title_label, FALSE, FALSE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (grid), "shopno-cal-grid");
  gtk_box_pack_start (GTK_BOX (month), grid, FALSE, FALSE, 0);

  for (i = 0; i < 7; i++)
    {
      self->weekday_labels[i] = shopno_cal_flyout_label_new ("shopno-cal-weekday");
      gtk_grid_attach (GTK_GRID (grid), self->weekday_labels[i], i, 0, 1, 1);
    }

  for (i = 0; i < G_N_ELEMENTS (self->day_labels); i++)
    {
      self->day_labels[i] = shopno_cal_flyout_label_new ("shopno-cal-day");
      gtk_grid_attach (GTK_GRID (grid), self->day_labels[i], i % 7, i / 7 + 1, 1, 1);
    }

  shopno_cal_flyout_update_weekdays (self);

  gtk_widget_show_all (self->frame);
}



GtkWidget *
shopno_cal_flyout_new (ShopnoCalSettings *settings,
                       ShopnoCalClock    *clock)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), NULL);
  g_return_val_if_fail (SHOPNO_IS_CAL_CLOCK (clock), NULL);

  return g_object_new (SHOPNO_TYPE_CAL_FLYOUT, "settings", settings, "clock", clock, NULL);
}



void
shopno_cal_flyout_popup (ShopnoCalFlyout *flyout,
                         gint             x,
                         gint             y)
{
  g_return_if_fail (SHOPNO_IS_CAL_FLYOUT (flyout));

  /* The compositor may have been toggled since the last popup */
  if (!gtk_widget_get_visible (GTK_WIDGET (flyout)))
    shopno_cal_flyout_update_visual (flyout);

  /* Open on today, like Windows */
  shopno_cal_flyout_update (flyout);
  shopno_cal_flyout_show_today (flyout);

  gtk_window_move (GTK_WINDOW (flyout), x, y);
  gtk_window_present_with_time (GTK_WINDOW (flyout), gtk_get_current_event_time ());
}



void
shopno_cal_flyout_show_today (ShopnoCalFlyout *flyout)
{
  g_return_if_fail (SHOPNO_IS_CAL_FLYOUT (flyout));

  shopno_cal_flyout_show_month (flyout, g_date_get_year (&flyout->today),
                                g_date_get_month (&flyout->today));
}



/* Turning the grab off is only meant for the preview tool, so a flyout
 * opened on a developer's desktop does not capture all input */
void
shopno_cal_flyout_set_grab_input (ShopnoCalFlyout *flyout,
                                  gboolean         grab_input)
{
  g_return_if_fail (SHOPNO_IS_CAL_FLYOUT (flyout));

  flyout->grab_input = grab_input;
}
