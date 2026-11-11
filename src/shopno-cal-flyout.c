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

#include "shopno-cal-details.h"
#include "shopno-cal-flyout.h"
#include "shopno-cal-format.h"
#include "shopno-cal-month.h"

/* The pointer is often still grabbed by the panel button when the flyout
 * maps, so retry the seat grab a few times before giving up (from Shopno
 * Menu) */
#define GRAB_RETRY_INTERVAL 50
#define GRAB_MAX_RETRIES    10

/* Keeps every cell of every view a valid date */
#define MIN_YEAR 11
#define MAX_YEAR 9988

#define CELL_INDEX_KEY "shopno-cal-cell-index"

/* How long the footer says what was copied */
#define STATUS_TIMEOUT 2500

typedef enum
{
  VIEW_DAYS,
  VIEW_MONTHS,
  VIEW_YEARS,
} ShopnoCalView;

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
  GtkWidget         *title_button;
  GtkWidget         *title_label;
  GtkWidget         *previous_button;
  GtkWidget         *next_button;
  GtkWidget         *stack;
  GtkWidget         *day_grid;
  GtkWidget         *weekday_labels[7];
  GtkWidget         *week_labels[SHOPNO_CAL_MONTH_WEEKS];
  GtkWidget         *day_buttons[SHOPNO_CAL_MONTH_WEEKS * 7];
  GtkWidget         *month_buttons[12];
  GtkWidget         *year_buttons[12];
  GtkWidget         *details;
  GtkWidget         *details_stack;
  GtkWidget         *day_title;
  GtkWidget         *day_facts;
  GtkWidget         *range_title;
  GtkWidget         *range_facts;
  GtkWidget         *footer;
  GtkWidget         *status_label;
  GtkWidget         *copy_button;
  GtkWidget         *copy_rows[SHOPNO_CAL_COPY_N_FORMATS];
  GtkWidget         *copy_values[SHOPNO_CAL_COPY_N_FORMATS];
  guint              status_timeout_id;

  /* The view, the month or year it shows, and the selected day, with the
   * other end of a range when a second day is shift-clicked. Today is kept
   * to notice midnight. */
  ShopnoCalView      view;
  GDateYear          year;
  GDateMonth         month;
  GDate              grid_start;
  GDate              selected;
  GDate              range_end;
  GDate              today;
  GDateWeekday       week_start;
  ShopnoCalWeekend   weekend;
  gdouble            scroll_delta;

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
shopno_cal_flyout_set_accessible_name (GtkWidget   *widget,
                                       const gchar *name)
{
  AtkObject *accessible = gtk_widget_get_accessible (widget);

  if (accessible != NULL && name != NULL)
    atk_object_set_name (accessible, name);
}



/* The weekday of a column of the days view */
static GDateWeekday
shopno_cal_flyout_column_weekday (ShopnoCalFlyout *self,
                                  guint            column)
{
  return (self->week_start - 1 + column) % 7 + 1;
}



static void
shopno_cal_flyout_update_weekdays (ShopnoCalFlyout *self)
{
  GDateWeekday weekday;
  guint        i;
  gchar       *name;

  for (i = 0; i < 7; i++)
    {
      weekday = shopno_cal_flyout_column_weekday (self, i);

      name = shopno_cal_month_weekday_name (weekday, TRUE);
      gtk_label_set_text (GTK_LABEL (self->weekday_labels[i]), name);
      g_free (name);

      name = shopno_cal_month_weekday_name (weekday, FALSE);
      shopno_cal_flyout_set_accessible_name (self->weekday_labels[i], name);
      g_free (name);

      shopno_cal_flyout_set_class (self->weekday_labels[i], "weekend",
                                   shopno_cal_month_is_weekend (self->weekend, weekday));
    }
}



static void
shopno_cal_flyout_set_cell (GtkWidget   *button,
                            const gchar *text,
                            const gchar *accessible_name,
                            gboolean     outside,
                            gboolean     today,
                            gboolean     selected)
{
  gtk_button_set_label (GTK_BUTTON (button), text);
  shopno_cal_flyout_set_accessible_name (button, accessible_name);

  shopno_cal_flyout_set_class (button, "outside", outside);
  shopno_cal_flyout_set_class (button, "today", today);
  shopno_cal_flyout_set_class (button, "selected", selected);
}



/* The date of a cell of the days view */
static void
shopno_cal_flyout_day_date (ShopnoCalFlyout *self,
                            guint            index,
                            GDate           *date)
{
  *date = self->grid_start;
  g_date_add_days (date, index);
}



/* The day the keyboard moves from: the end of the range when there is
 * one, otherwise the selected day */
static const GDate *
shopno_cal_flyout_cursor (ShopnoCalFlyout *self)
{
  return g_date_valid (&self->range_end) ? &self->range_end : &self->selected;
}



/* Only one day can take the keyboard focus, so Tab moves past the grid in
 * one step and the arrow keys move inside it: the cursor day when it is
 * in the month shown, otherwise the month's first day */
static guint
shopno_cal_flyout_focus_day (ShopnoCalFlyout *self)
{
  const GDate *cursor = shopno_cal_flyout_cursor (self);
  GDate        first;
  gint         index;

  if (g_date_get_year (cursor) == self->year
      && g_date_get_month (cursor) == self->month)
    return g_date_days_between (&self->grid_start, cursor);

  g_date_clear (&first, 1);
  g_date_set_dmy (&first, 1, self->month, self->year);
  index = g_date_days_between (&self->grid_start, &first);

  return index;
}



static void
shopno_cal_flyout_refresh_days (ShopnoCalFlyout *self)
{
  GtkWidget *focus;
  GDate      date;
  GDate      range_first;
  GDate      range_last;
  gboolean   range = g_date_valid (&self->range_end);
  gchar     *name;
  gchar      number[4];
  guint      focus_day;
  guint      week;
  guint      i;

  shopno_cal_month_grid_start (self->year, self->month, self->week_start, &self->grid_start);
  focus_day = shopno_cal_flyout_focus_day (self);

  if (range)
    {
      range_first = self->selected;
      range_last = self->range_end;
      g_date_order (&range_first, &range_last);
    }

  for (i = 0; i < G_N_ELEMENTS (self->day_buttons); i++)
    {
      shopno_cal_flyout_day_date (self, i, &date);

      g_snprintf (number, sizeof (number), "%u", g_date_get_day (&date));
      name = shopno_cal_flyout_format_date (&date, _("%A, %-d %B %Y"));
      shopno_cal_flyout_set_cell (self->day_buttons[i], number, name,
                                  g_date_get_month (&date) != self->month,
                                  g_date_compare (&date, &self->today) == 0,
                                  g_date_compare (&date, &self->selected) == 0
                                  || (range && g_date_compare (&date, &self->range_end) == 0));
      /* The ends have the selection ring; the days between are filled */
      shopno_cal_flyout_set_class (self->day_buttons[i], "in-range",
                                   range
                                   && g_date_compare (&date, &range_first) > 0
                                   && g_date_compare (&date, &range_last) < 0);
      shopno_cal_flyout_set_class (self->day_buttons[i], "weekend",
                                   shopno_cal_month_is_weekend (self->weekend,
                                                                g_date_get_weekday (&date)));
      g_free (name);

      if (i % 7 == 0)
        {
          week = shopno_cal_month_row_week (&date);
          g_snprintf (number, sizeof (number), "%u", week);
          gtk_label_set_text (GTK_LABEL (self->week_labels[i / 7]), number);

          name = g_strdup_printf (_("Week %u"), week);
          shopno_cal_flyout_set_accessible_name (self->week_labels[i / 7], name);
          g_free (name);
        }

      gtk_widget_set_can_focus (self->day_buttons[i], i == focus_day);
    }

  /* GTK leaves the focus on a day that can no longer take it */
  focus = gtk_window_get_focus (GTK_WINDOW (self));
  if (focus != NULL && gtk_widget_get_parent (focus) == self->day_grid
      && focus != self->day_buttons[focus_day])
    gtk_widget_grab_focus (self->day_buttons[focus_day]);
}



static void
shopno_cal_flyout_refresh_months (ShopnoCalFlyout *self)
{
  GDate  date;
  gchar *text;
  gchar *name;
  guint  i;

  g_date_clear (&date, 1);

  for (i = 0; i < G_N_ELEMENTS (self->month_buttons); i++)
    {
      g_date_set_dmy (&date, 1, i + 1, self->year);
      text = shopno_cal_flyout_format_date (&date, "%b");
      name = shopno_cal_flyout_format_date (&date, _("%B %Y"));
      shopno_cal_flyout_set_cell (self->month_buttons[i], text, name, FALSE,
                                  self->year == g_date_get_year (&self->today)
                                  && i + 1 == g_date_get_month (&self->today),
                                  i + 1 == self->month);
      g_free (name);
      g_free (text);
    }
}



/* A decade, with the last year of the one before and the first year of
 * the one after, dimmed, to fill four rows of three */
static GDateYear
shopno_cal_flyout_decade (GDateYear year)
{
  return year - year % 10;
}



static void
shopno_cal_flyout_refresh_years (ShopnoCalFlyout *self)
{
  GDateYear decade = shopno_cal_flyout_decade (self->year);
  GDateYear year;
  gchar     text[8];
  guint     i;

  for (i = 0; i < G_N_ELEMENTS (self->year_buttons); i++)
    {
      year = decade - 1 + i;
      g_snprintf (text, sizeof (text), "%u", year);
      shopno_cal_flyout_set_cell (self->year_buttons[i], text, text,
                                  i == 0 || i == G_N_ELEMENTS (self->year_buttons) - 1,
                                  year == g_date_get_year (&self->today),
                                  year == self->year);

      /* Outside the years the calendar goes to */
      gtk_widget_set_sensitive (self->year_buttons[i], year >= MIN_YEAR && year <= MAX_YEAR);
    }
}



/* Where the up and down arrows lead: a month, a year or a decade away */
static void
shopno_cal_flyout_shift (ShopnoCalFlyout *self,
                         gint             delta,
                         GDateYear       *year,
                         GDateMonth      *month)
{
  gint months = self->year * 12 + (self->month - 1);
  gint y;

  switch (self->view)
    {
    case VIEW_DAYS:
      months += delta;
      break;

    case VIEW_MONTHS:
      months += delta * 12;
      break;

    case VIEW_YEARS:
      months += delta * 120;
      break;
    }

  months = CLAMP (months, MIN_YEAR * 12, MAX_YEAR * 12 + 11);
  y = months / 12;

  *year = y;
  *month = months % 12 + 1;
}



static gboolean
shopno_cal_flyout_can_shift (ShopnoCalFlyout *self,
                             gint             delta)
{
  GDateYear  year;
  GDateMonth month;

  shopno_cal_flyout_shift (self, delta, &year, &month);

  switch (self->view)
    {
    case VIEW_DAYS:
      return year != self->year || month != self->month;

    case VIEW_MONTHS:
      return year != self->year;

    case VIEW_YEARS:
      return shopno_cal_flyout_decade (year) != shopno_cal_flyout_decade (self->year);
    }

  return FALSE;
}



static void
shopno_cal_flyout_update_details (ShopnoCalFlyout *self)
{
  gboolean range = g_date_valid (&self->range_end);
  GDate    first;
  GDate    last;
  gchar   *text;

  text = shopno_cal_details_day_title (&self->selected, &self->today);
  gtk_label_set_text (GTK_LABEL (self->day_title), text);
  g_free (text);

  text = shopno_cal_details_day (&self->selected, &self->today, self->week_start);
  gtk_label_set_text (GTK_LABEL (self->day_facts), text);
  g_free (text);

  /* Without a range, the hidden range page holds a long sample, so the
   * stack keeps the height a range needs and the flyout does not jump when
   * one starts. 15 December 2026 to 25 February 2027 is 72 days. */
  if (range)
    {
      first = self->selected;
      last = self->range_end;
    }
  else
    {
      g_date_clear (&first, 1);
      g_date_set_dmy (&first, 15, G_DATE_DECEMBER, 2026);
      g_date_clear (&last, 1);
      g_date_set_dmy (&last, 25, G_DATE_FEBRUARY, 2027);
    }

  text = shopno_cal_details_range_title (&first, &last);
  gtk_label_set_text (GTK_LABEL (self->range_title), text);
  g_free (text);

  text = shopno_cal_details_range (&first, &last, self->weekend);
  gtk_label_set_text (GTK_LABEL (self->range_facts), text);
  g_free (text);

  gtk_stack_set_visible_child_name (GTK_STACK (self->details_stack), range ? "range" : "day");
}



static void
shopno_cal_flyout_refresh (ShopnoCalFlyout *self)
{
  GDate        first;
  gchar       *text = NULL;
  const gchar *previous = NULL;
  const gchar *next = NULL;
  const gchar *title_tip = NULL;
  GDateYear    decade;

  g_date_clear (&first, 1);
  g_date_set_dmy (&first, 1, self->month, self->year);

  switch (self->view)
    {
    case VIEW_DAYS:
      shopno_cal_flyout_refresh_days (self);
      /* TRANSLATORS: the month above the calendar grid, see
       * https://docs.gtk.org/glib/method.DateTime.format.html */
      text = shopno_cal_flyout_format_date (&first, _("%B %Y"));
      previous = _("Previous month");
      next = _("Next month");
      title_tip = _("Choose a month");
      gtk_stack_set_visible_child_name (GTK_STACK (self->stack), "days");
      break;

    case VIEW_MONTHS:
      shopno_cal_flyout_refresh_months (self);
      text = shopno_cal_flyout_format_date (&first, "%Y");
      previous = _("Previous year");
      next = _("Next year");
      title_tip = _("Choose a year");
      gtk_stack_set_visible_child_name (GTK_STACK (self->stack), "months");
      break;

    case VIEW_YEARS:
      shopno_cal_flyout_refresh_years (self);
      decade = shopno_cal_flyout_decade (self->year);
      /* TRANSLATORS: the decade above the grid of years, such as
       * "2020 – 2029" */
      text = g_strdup_printf (_("%u – %u"), decade, decade + 9);
      previous = _("Previous decade");
      next = _("Next decade");
      gtk_stack_set_visible_child_name (GTK_STACK (self->stack), "years");
      break;
    }

  gtk_label_set_text (GTK_LABEL (self->title_label), text);
  g_free (text);

  shopno_cal_flyout_update_details (self);

  /* The years are as far as it zooms out */
  gtk_widget_set_sensitive (self->title_button, self->view != VIEW_YEARS);
  gtk_widget_set_tooltip_text (self->title_button, title_tip);

  gtk_widget_set_tooltip_text (self->previous_button, previous);
  shopno_cal_flyout_set_accessible_name (self->previous_button, previous);
  gtk_widget_set_sensitive (self->previous_button, shopno_cal_flyout_can_shift (self, -1));

  gtk_widget_set_tooltip_text (self->next_button, next);
  shopno_cal_flyout_set_accessible_name (self->next_button, next);
  gtk_widget_set_sensitive (self->next_button, shopno_cal_flyout_can_shift (self, 1));
}



/* Moves the keyboard focus to the selected cell of the view */
static void
shopno_cal_flyout_focus_selected (ShopnoCalFlyout *self)
{
  GtkWidget *widget = NULL;

  switch (self->view)
    {
    case VIEW_DAYS:
      widget = self->day_buttons[shopno_cal_flyout_focus_day (self)];
      break;

    case VIEW_MONTHS:
      widget = self->month_buttons[self->month - 1];
      break;

    case VIEW_YEARS:
      widget = self->year_buttons[self->year - shopno_cal_flyout_decade (self->year) + 1];
      break;
    }

  gtk_widget_grab_focus (widget);
}



static void
shopno_cal_flyout_set_view (ShopnoCalFlyout *self,
                            ShopnoCalView    view)
{
  self->view = view;
  shopno_cal_flyout_refresh (self);
  shopno_cal_flyout_focus_selected (self);
}



static void
shopno_cal_flyout_navigate (ShopnoCalFlyout *self,
                            gint             delta)
{
  if (!shopno_cal_flyout_can_shift (self, delta))
    return;

  shopno_cal_flyout_shift (self, delta, &self->year, &self->month);
  shopno_cal_flyout_refresh (self);
}



/* Keeps a day within the years the calendar goes to */
static void
shopno_cal_flyout_clamp_day (GDate *date)
{
  GDate first;
  GDate last;

  g_date_clear (&first, 1);
  g_date_set_dmy (&first, 1, G_DATE_JANUARY, MIN_YEAR);
  g_date_clear (&last, 1);
  g_date_set_dmy (&last, 31, G_DATE_DECEMBER, MAX_YEAR);

  g_date_clamp (date, &first, &last);
}



/* Shows the month of a day in the days view */
static void
shopno_cal_flyout_show_day (ShopnoCalFlyout *self,
                            const GDate     *date)
{
  self->year = g_date_get_year (date);
  self->month = g_date_get_month (date);
  shopno_cal_flyout_set_view (self, VIEW_DAYS);
}



/* Selects a day, ending any range, and shows its month */
static void
shopno_cal_flyout_select_day (ShopnoCalFlyout *self,
                              const GDate     *date)
{
  self->selected = *date;
  shopno_cal_flyout_clamp_day (&self->selected);
  g_date_clear (&self->range_end, 1);

  shopno_cal_flyout_show_day (self, &self->selected);
}



/* Makes a day the other end of a range from the selected day, to count
 * the days between, and shows its month. Without the details there is
 * nothing to count, so the day is only selected. */
static void
shopno_cal_flyout_extend_to (ShopnoCalFlyout *self,
                             const GDate     *date)
{
  GDate end = *date;

  if (!shopno_cal_settings_get_day_details (self->settings))
    {
      shopno_cal_flyout_select_day (self, date);
      return;
    }

  shopno_cal_flyout_clamp_day (&end);

  /* Back on the selected day, the range is gone */
  if (g_date_compare (&end, &self->selected) == 0)
    g_date_clear (&self->range_end, 1);
  else
    self->range_end = end;

  shopno_cal_flyout_show_day (self, &end);
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

  /* Midnight, or a new time zone: move the today square, the selection
   * when it was on today, and follow into the next month when the current
   * month is shown */
  if (!g_date_valid (&self->today) || g_date_compare (&today, &self->today) != 0)
    {
      showing_today = !g_date_valid (&self->today)
                      || (self->view == VIEW_DAYS
                          && self->year == g_date_get_year (&self->today)
                          && self->month == g_date_get_month (&self->today));

      if (!g_date_valid (&self->selected) || g_date_compare (&self->selected, &self->today) == 0)
        self->selected = today;

      if (g_date_valid (&self->range_end) && g_date_compare (&self->range_end, &self->selected) == 0)
        g_date_clear (&self->range_end, 1);

      self->today = today;

      if (showing_today)
        {
          self->year = g_date_get_year (&today);
          self->month = g_date_get_month (&today);
        }

      shopno_cal_flyout_refresh (self);
    }

  g_date_time_unref (now);
}



static void
shopno_cal_flyout_week_settings_changed (ShopnoCalFlyout *self)
{
  gboolean week_numbers = shopno_cal_settings_get_week_numbers (self->settings);
  guint    i;

  self->week_start = shopno_cal_month_week_start (shopno_cal_settings_get_week_start (self->settings));
  self->weekend = shopno_cal_settings_get_weekend (self->settings);

  for (i = 0; i < G_N_ELEMENTS (self->week_labels); i++)
    gtk_widget_set_visible (self->week_labels[i], week_numbers);

  shopno_cal_flyout_update_weekdays (self);

  if (g_date_valid (&self->today))
    shopno_cal_flyout_refresh (self);
}



/* Footer */

static void
shopno_cal_flyout_update_footer (ShopnoCalFlyout *self)
{
  gboolean copy = shopno_cal_settings_get_copy_button (self->settings);

  gtk_widget_set_visible (self->copy_button, copy);
  gtk_widget_set_visible (self->footer, copy);
}



static void
shopno_cal_flyout_clear_status (ShopnoCalFlyout *self)
{
  g_clear_handle_id (&self->status_timeout_id, g_source_remove);
  gtk_label_set_text (GTK_LABEL (self->status_label), NULL);
}



static gboolean
shopno_cal_flyout_status_timeout (gpointer data)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (data);

  self->status_timeout_id = 0;
  shopno_cal_flyout_clear_status (self);

  return G_SOURCE_REMOVE;
}



/* The values of the copy menu, kept current while it is open */
static void
shopno_cal_flyout_update_copy_values (ShopnoCalFlyout *self)
{
  GDateTime *now = shopno_cal_clock_get_now (self->clock);
  GtkWidget *row;
  gchar     *value;
  gchar     *name;
  guint      i;

  for (i = 0; i < SHOPNO_CAL_COPY_N_FORMATS; i++)
    {
      value = shopno_cal_format_copy (now, i);
      gtk_label_set_text (GTK_LABEL (self->copy_values[i]), value);

      /* TRANSLATORS: a row of the copy menu for screen readers, such as
       * "Copy ISO date, 2026-11-10" */
      name = g_strdup_printf (_("Copy %s, %s"), shopno_cal_format_copy_name (i), value);
      row = gtk_widget_get_ancestor (self->copy_values[i], GTK_TYPE_BUTTON);
      shopno_cal_flyout_set_accessible_name (row, name);
      g_free (name);
      g_free (value);
    }

  g_date_time_unref (now);
}



/* Lays out the rows of the copy menu with the name and the value side by
 * side, as in the mockup, or the name over the value when that would be
 * wider than the flyout: GTK cuts a popover off at its window's edges,
 * and a long date in a large font does not fit beside its name */
static void
shopno_cal_flyout_fit_copy_menu (ShopnoCalFlyout *self)
{
  GtkPopover      *popover = gtk_menu_button_get_popover (GTK_MENU_BUTTON (self->copy_button));
  GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (popover));
  GtkBorder        padding;
  GtkBorder        border;
  gint             width;
  gboolean         stacked;
  guint            i;

  for (i = 0; i < SHOPNO_CAL_COPY_N_FORMATS; i++)
    {
      gtk_orientable_set_orientation (GTK_ORIENTABLE (self->copy_rows[i]), GTK_ORIENTATION_HORIZONTAL);
      gtk_box_set_spacing (GTK_BOX (self->copy_rows[i]), 12);
      gtk_label_set_xalign (GTK_LABEL (self->copy_values[i]), 1.0);
    }

  /* The popover's own width also counts the room for its shadow, which may
   * reach the window's edge; its padding and border may not */
  gtk_widget_get_preferred_width (gtk_bin_get_child (GTK_BIN (popover)), NULL, &width);
  gtk_style_context_get_padding (context, gtk_style_context_get_state (context), &padding);
  gtk_style_context_get_border (context, gtk_style_context_get_state (context), &border);
  width += padding.left + padding.right + border.left + border.right;

  /* Keep a few pixels clear of the flyout's sides */
  stacked = width > gtk_widget_get_allocated_width (GTK_WIDGET (self)) - 8;

  if (stacked)
    for (i = 0; i < SHOPNO_CAL_COPY_N_FORMATS; i++)
      {
        gtk_orientable_set_orientation (GTK_ORIENTABLE (self->copy_rows[i]), GTK_ORIENTATION_VERTICAL);
        gtk_box_set_spacing (GTK_BOX (self->copy_rows[i]), 2);
        gtk_label_set_xalign (GTK_LABEL (self->copy_values[i]), 0.0);
      }

  shopno_cal_flyout_set_class (GTK_WIDGET (popover), "stacked", stacked);
}



static void
shopno_cal_flyout_copy_menu_shown (ShopnoCalFlyout *self)
{
  shopno_cal_flyout_update_copy_values (self);
  shopno_cal_flyout_fit_copy_menu (self);
}



static gboolean
shopno_cal_flyout_copy_menu_open (ShopnoCalFlyout *self)
{
  GtkPopover *popover = gtk_menu_button_get_popover (GTK_MENU_BUTTON (self->copy_button));

  return popover != NULL && gtk_widget_get_visible (GTK_WIDGET (popover));
}



static void
shopno_cal_flyout_copy_clicked (GtkWidget       *row,
                                ShopnoCalFlyout *self)
{
  guint        index = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (row), CELL_INDEX_KEY));
  const gchar *value = gtk_label_get_text (GTK_LABEL (self->copy_values[index]));
  gchar       *text;

  /* The panel keeps running, so the text stays on the clipboard after the
   * flyout closes */
  gtk_clipboard_set_text (gtk_widget_get_clipboard (row, GDK_SELECTION_CLIPBOARD), value, -1);

  gtk_popover_popdown (gtk_menu_button_get_popover (GTK_MENU_BUTTON (self->copy_button)));

  /* TRANSLATORS: in the flyout footer after copying, such as
   * "Copied 2026-11-10" */
  text = g_strdup_printf (_("Copied %s"), value);
  g_clear_handle_id (&self->status_timeout_id, g_source_remove);
  gtk_label_set_text (GTK_LABEL (self->status_label), text);
  self->status_timeout_id = g_timeout_add (STATUS_TIMEOUT, shopno_cal_flyout_status_timeout, self);
  g_free (text);
}



static void
shopno_cal_flyout_day_details_changed (ShopnoCalFlyout *self)
{
  gboolean day_details = shopno_cal_settings_get_day_details (self->settings);

  gtk_widget_set_visible (self->details, day_details);

  /* A range cannot be seen or counted without the details */
  if (!day_details && g_date_valid (&self->range_end))
    {
      g_date_clear (&self->range_end, 1);
      if (g_date_valid (&self->today))
        shopno_cal_flyout_refresh (self);
    }
}



static void
shopno_cal_flyout_tick (ShopnoCalFlyout *self)
{
  if (gtk_widget_get_visible (GTK_WIDGET (self)))
    {
      shopno_cal_flyout_update (self);
      if (shopno_cal_flyout_copy_menu_open (self))
        shopno_cal_flyout_update_copy_values (self);
    }
}



/*
 * Navigation
 */

static guint
shopno_cal_flyout_cell_index (GtkWidget *button)
{
  return GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button), CELL_INDEX_KEY));
}



static void
shopno_cal_flyout_day_clicked (GtkWidget       *button,
                               ShopnoCalFlyout *self)
{
  GdkModifierType state = 0;
  GDate           date;

  /* A dimmed day goes to its month */
  shopno_cal_flyout_day_date (self, shopno_cal_flyout_cell_index (button), &date);

  /* Shift-click counts the days from the selected day */
  if (gtk_get_current_event_state (&state) && (state & GDK_SHIFT_MASK) != 0)
    shopno_cal_flyout_extend_to (self, &date);
  else
    shopno_cal_flyout_select_day (self, &date);
}



static void
shopno_cal_flyout_month_clicked (GtkWidget       *button,
                                 ShopnoCalFlyout *self)
{
  self->month = shopno_cal_flyout_cell_index (button) + 1;
  shopno_cal_flyout_set_view (self, VIEW_DAYS);
}



static void
shopno_cal_flyout_year_clicked (GtkWidget       *button,
                                ShopnoCalFlyout *self)
{
  self->year = shopno_cal_flyout_decade (self->year) - 1 + shopno_cal_flyout_cell_index (button);
  shopno_cal_flyout_set_view (self, VIEW_MONTHS);
}



static void
shopno_cal_flyout_title_clicked (ShopnoCalFlyout *self)
{
  if (self->view == VIEW_DAYS)
    shopno_cal_flyout_set_view (self, VIEW_MONTHS);
  else if (self->view == VIEW_MONTHS)
    shopno_cal_flyout_set_view (self, VIEW_YEARS);
}



static void
shopno_cal_flyout_previous_clicked (ShopnoCalFlyout *self)
{
  shopno_cal_flyout_navigate (self, -1);
}



static void
shopno_cal_flyout_next_clicked (ShopnoCalFlyout *self)
{
  shopno_cal_flyout_navigate (self, 1);
}



/* One step per notch of a wheel, and per line's worth of a touchpad
 * swipe */
static gboolean
shopno_cal_flyout_scroll_event (GtkWidget       *widget,
                                GdkEventScroll  *event,
                                ShopnoCalFlyout *self)
{
  gdouble dx, dy;

  switch (event->direction)
    {
    case GDK_SCROLL_UP:
      shopno_cal_flyout_navigate (self, -1);
      break;

    case GDK_SCROLL_DOWN:
      shopno_cal_flyout_navigate (self, 1);
      break;

    case GDK_SCROLL_SMOOTH:
      gdk_event_get_scroll_deltas ((GdkEvent *) event, &dx, &dy);

      /* Start over when the direction changes */
      if ((dy > 0) != (self->scroll_delta > 0))
        self->scroll_delta = 0;
      self->scroll_delta += dy;

      while (self->scroll_delta >= 1.0)
        {
          shopno_cal_flyout_navigate (self, 1);
          self->scroll_delta -= 1.0;
        }
      while (self->scroll_delta <= -1.0)
        {
          shopno_cal_flyout_navigate (self, -1);
          self->scroll_delta += 1.0;
        }
      break;

    default:
      break;
    }

  return GDK_EVENT_STOP;
}



/* Arrow keys, Page Up and Page Down on a day: move the selection, into the
 * next or previous month when needed. With Shift, move the end of a range
 * instead. */
static gboolean
shopno_cal_flyout_day_key (ShopnoCalFlyout *self,
                           GtkWidget       *focus,
                           guint            keyval,
                           gboolean         extend)
{
  GDate    date;
  gboolean rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;

  shopno_cal_flyout_day_date (self, shopno_cal_flyout_cell_index (focus), &date);

  switch (keyval)
    {
    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
      if (rtl)
        g_date_add_days (&date, 1);
      else
        g_date_subtract_days (&date, 1);
      break;

    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
      if (rtl)
        g_date_subtract_days (&date, 1);
      else
        g_date_add_days (&date, 1);
      break;

    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      g_date_subtract_days (&date, 7);
      break;

    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      g_date_add_days (&date, 7);
      break;

    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:
      g_date_subtract_months (&date, 1);
      break;

    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down:
      g_date_add_months (&date, 1);
      break;

    default:
      return GDK_EVENT_PROPAGATE;
    }

  if (extend)
    shopno_cal_flyout_extend_to (self, &date);
  else
    shopno_cal_flyout_select_day (self, &date);

  return GDK_EVENT_STOP;
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
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (widget);

  shopno_cal_flyout_ungrab (self);

  /* Open the next time without the menu or an old message */
  gtk_popover_popdown (gtk_menu_button_get_popover (GTK_MENU_BUTTON (self->copy_button)));
  shopno_cal_flyout_clear_status (self);

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
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (widget);
  GtkWidget       *focus = gtk_window_get_focus (GTK_WINDOW (widget));
  gboolean         on_day = focus != NULL && gtk_widget_get_parent (focus) == self->day_grid;
  GdkModifierType  modifiers = event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_SHIFT_MASK);

  /* The copy menu has the keyboard; Escape closes only the menu */
  if (shopno_cal_flyout_copy_menu_open (self))
    return GTK_WIDGET_CLASS (shopno_cal_flyout_parent_class)->key_press_event (widget, event);

  /* Shift with the keys that move between days moves the end of a range */
  if (modifiers == GDK_SHIFT_MASK && on_day
      && shopno_cal_flyout_day_key (self, focus, event->keyval, TRUE))
    return GDK_EVENT_STOP;

  if (modifiers != 0)
    return GTK_WIDGET_CLASS (shopno_cal_flyout_parent_class)->key_press_event (widget, event);

  switch (event->keyval)
    {
    case GDK_KEY_Escape:
      gtk_widget_hide (widget);
      return GDK_EVENT_STOP;

    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
      shopno_cal_flyout_show_today (self);
      return GDK_EVENT_STOP;

    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:
      if (!on_day)
        {
          shopno_cal_flyout_navigate (self, -1);
          return GDK_EVENT_STOP;
        }
      break;

    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down:
      if (!on_day)
        {
          shopno_cal_flyout_navigate (self, 1);
          return GDK_EVENT_STOP;
        }
      break;

    default:
      break;
    }

  if (on_day && shopno_cal_flyout_day_key (self, focus, event->keyval, FALSE))
    return GDK_EVENT_STOP;

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
  g_signal_connect_swapped (self->settings, "notify::week-start",
                            G_CALLBACK (shopno_cal_flyout_week_settings_changed), self);
  g_signal_connect_swapped (self->settings, "notify::weekend",
                            G_CALLBACK (shopno_cal_flyout_week_settings_changed), self);
  g_signal_connect_swapped (self->settings, "notify::week-numbers",
                            G_CALLBACK (shopno_cal_flyout_week_settings_changed), self);
  shopno_cal_flyout_week_settings_changed (self);
  g_signal_connect_swapped (self->settings, "notify::day-details",
                            G_CALLBACK (shopno_cal_flyout_day_details_changed), self);
  shopno_cal_flyout_day_details_changed (self);
  g_signal_connect_swapped (self->settings, "notify::copy-button",
                            G_CALLBACK (shopno_cal_flyout_update_footer), self);
  shopno_cal_flyout_update_footer (self);
  g_signal_connect_swapped (self->clock, "tick",
                            G_CALLBACK (shopno_cal_flyout_tick), self);

  shopno_cal_flyout_update (self);
}



static void
shopno_cal_flyout_dispose (GObject *object)
{
  ShopnoCalFlyout *self = SHOPNO_CAL_FLYOUT (object);

  shopno_cal_flyout_ungrab (self);
  g_clear_handle_id (&self->status_timeout_id, g_source_remove);

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



static GtkWidget *
shopno_cal_flyout_button_new (const gchar *style_class)
{
  GtkWidget *button = gtk_button_new_with_label ("");

  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_style_context_add_class (gtk_widget_get_style_context (button), style_class);

  return button;
}



static GtkWidget *
shopno_cal_flyout_arrow_new (const gchar *icon_name)
{
  GtkWidget *button = gtk_button_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);

  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_style_context_add_class (gtk_widget_get_style_context (button), "shopno-cal-arrow");

  return button;
}



static GtkWidget *
shopno_cal_flyout_cell_new (const gchar *style_class,
                            guint        index)
{
  GtkWidget *button = shopno_cal_flyout_button_new (style_class);

  gtk_widget_set_hexpand (button, TRUE);
  gtk_widget_set_vexpand (button, TRUE);
  g_object_set_data (G_OBJECT (button), CELL_INDEX_KEY, GUINT_TO_POINTER (index));

  return button;
}



static GtkWidget *
shopno_cal_flyout_grid_new (const gchar *style_class)
{
  GtkWidget *grid = gtk_grid_new ();

  gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
  gtk_style_context_add_class (gtk_widget_get_style_context (grid), "shopno-cal-grid");
  gtk_style_context_add_class (gtk_widget_get_style_context (grid), style_class);

  return grid;
}



static GtkWidget *
shopno_cal_flyout_details_label_new (const gchar *style_class)
{
  GtkWidget *label = shopno_cal_flyout_label_new (style_class);

  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 1);

  return label;
}



/* The copy menu: a row per format, with its name and the value to copy */
static GtkWidget *
shopno_cal_flyout_copy_menu_new (ShopnoCalFlyout *self)
{
  GtkWidget *popover = gtk_popover_new (self->copy_button);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *row;
  GtkWidget *row_box;
  GtkWidget *label;
  guint      i;

  gtk_style_context_add_class (gtk_widget_get_style_context (popover), "shopno-cal-copy-menu");
  gtk_popover_set_position (GTK_POPOVER (popover), GTK_POS_TOP);
  gtk_container_add (GTK_CONTAINER (popover), box);

  for (i = 0; i < SHOPNO_CAL_COPY_N_FORMATS; i++)
    {
      row = shopno_cal_flyout_button_new ("shopno-cal-copy-item");
      gtk_container_remove (GTK_CONTAINER (row), gtk_bin_get_child (GTK_BIN (row)));
      g_object_set_data (G_OBJECT (row), CELL_INDEX_KEY, GUINT_TO_POINTER (i));
      g_signal_connect (row, "clicked", G_CALLBACK (shopno_cal_flyout_copy_clicked), self);
      gtk_box_pack_start (GTK_BOX (box), row, FALSE, FALSE, 0);

      row_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_container_add (GTK_CONTAINER (row), row_box);
      self->copy_rows[i] = row_box;

      label = shopno_cal_flyout_label_new ("shopno-cal-copy-name");
      gtk_label_set_text (GTK_LABEL (label), shopno_cal_format_copy_name (i));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (row_box), label, TRUE, TRUE, 0);

      self->copy_values[i] = shopno_cal_flyout_label_new ("shopno-cal-copy-value");
      gtk_label_set_xalign (GTK_LABEL (self->copy_values[i]), 1.0);
      gtk_box_pack_end (GTK_BOX (row_box), self->copy_values[i], FALSE, FALSE, 0);
    }

  /* Current values each time it opens, laid out to fit */
  g_signal_connect_swapped (popover, "show",
                            G_CALLBACK (shopno_cal_flyout_copy_menu_shown), self);
  gtk_widget_show_all (box);

  return popover;
}



/* A page of the details: a title, a line of facts, and a hint */
static GtkWidget *
shopno_cal_flyout_details_page_new (GtkWidget   **title,
                                    GtkWidget   **facts,
                                    const gchar  *hint)
{
  GtkWidget *page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *label;

  *title = shopno_cal_flyout_details_label_new ("shopno-cal-details-title");
  gtk_box_pack_start (GTK_BOX (page), *title, FALSE, FALSE, 0);

  *facts = shopno_cal_flyout_details_label_new ("shopno-cal-details-facts");
  gtk_box_pack_start (GTK_BOX (page), *facts, FALSE, FALSE, 0);

  label = shopno_cal_flyout_details_label_new ("shopno-cal-details-hint");
  gtk_label_set_text (GTK_LABEL (label), hint);
  gtk_box_pack_start (GTK_BOX (page), label, FALSE, FALSE, 0);

  return page;
}



static void
shopno_cal_flyout_init (ShopnoCalFlyout *self)
{
  GtkWindow *window = GTK_WINDOW (self);
  GtkWidget *header;
  GtkWidget *time_box;
  GtkWidget *month;
  GtkWidget *nav;
  GtkWidget *events;
  GtkWidget *grid;
  GtkWidget *page;
  guint      i;

  self->grab_input = TRUE;
  self->view = VIEW_DAYS;
  self->week_start = shopno_cal_month_locale_week_start ();
  self->weekend = SHOPNO_CAL_WEEKEND_SATURDAY_SUNDAY;
  g_date_clear (&self->today, 1);
  g_date_clear (&self->selected, 1);
  g_date_clear (&self->range_end, 1);
  g_date_clear (&self->grid_start, 1);

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

  /* Month: the title, which zooms out, and the arrows, over a stack of
   * the days, months and years views */
  month = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (month), "shopno-cal-month");
  gtk_box_pack_start (GTK_BOX (self->frame), month, FALSE, FALSE, 0);

  nav = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (nav), "shopno-cal-nav");
  gtk_box_pack_start (GTK_BOX (month), nav, FALSE, FALSE, 0);

  /* The whole width left of the arrows, so a short title is easy to hit */
  self->title_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (self->title_button), GTK_RELIEF_NONE);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->title_button), "shopno-cal-title");
  self->title_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (self->title_label), 0.0);
  gtk_container_add (GTK_CONTAINER (self->title_button), self->title_label);
  g_signal_connect_swapped (self->title_button, "clicked",
                            G_CALLBACK (shopno_cal_flyout_title_clicked), self);
  gtk_box_pack_start (GTK_BOX (nav), self->title_button, TRUE, TRUE, 0);

  self->previous_button = shopno_cal_flyout_arrow_new ("go-up-symbolic");
  g_signal_connect_swapped (self->previous_button, "clicked",
                            G_CALLBACK (shopno_cal_flyout_previous_clicked), self);
  gtk_box_pack_start (GTK_BOX (nav), self->previous_button, FALSE, FALSE, 0);

  self->next_button = shopno_cal_flyout_arrow_new ("go-down-symbolic");
  g_signal_connect_swapped (self->next_button, "clicked",
                            G_CALLBACK (shopno_cal_flyout_next_clicked), self);
  gtk_box_pack_start (GTK_BOX (nav), self->next_button, FALSE, FALSE, 0);

  /* The wheel changes the month, year or decade over any view */
  events = gtk_event_box_new ();
  gtk_widget_add_events (events, GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
  g_signal_connect (events, "scroll-event", G_CALLBACK (shopno_cal_flyout_scroll_event), self);
  gtk_box_pack_start (GTK_BOX (month), events, FALSE, FALSE, 0);

  self->stack = gtk_stack_new ();
  gtk_stack_set_homogeneous (GTK_STACK (self->stack), TRUE);
  gtk_stack_set_transition_type (GTK_STACK (self->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (self->stack), 120);
  gtk_container_add (GTK_CONTAINER (events), self->stack);

  /* Days: weekday names over six weeks, with an optional column of week
   * numbers on the left. The days share the width; the week numbers take
   * only what they need. */
  self->day_grid = shopno_cal_flyout_grid_new ("shopno-cal-days");
  gtk_grid_set_column_homogeneous (GTK_GRID (self->day_grid), FALSE);
  gtk_stack_add_named (GTK_STACK (self->stack), self->day_grid, "days");

  for (i = 0; i < 7; i++)
    {
      self->weekday_labels[i] = shopno_cal_flyout_label_new ("shopno-cal-weekday");
      gtk_grid_attach (GTK_GRID (self->day_grid), self->weekday_labels[i], i + 1, 0, 1, 1);
    }

  for (i = 0; i < G_N_ELEMENTS (self->week_labels); i++)
    {
      self->week_labels[i] = shopno_cal_flyout_label_new ("shopno-cal-week-number");
      gtk_widget_set_no_show_all (self->week_labels[i], TRUE);
      gtk_grid_attach (GTK_GRID (self->day_grid), self->week_labels[i], 0, i + 1, 1, 1);
    }

  for (i = 0; i < G_N_ELEMENTS (self->day_buttons); i++)
    {
      self->day_buttons[i] = shopno_cal_flyout_cell_new ("shopno-cal-day", i);
      g_signal_connect (self->day_buttons[i], "clicked",
                        G_CALLBACK (shopno_cal_flyout_day_clicked), self);
      gtk_grid_attach (GTK_GRID (self->day_grid), self->day_buttons[i], i % 7 + 1, i / 7 + 1, 1, 1);
    }

  /* Months of the year, and years of the decade, in rows of four */
  grid = shopno_cal_flyout_grid_new ("shopno-cal-months");
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);
  gtk_stack_add_named (GTK_STACK (self->stack), grid, "months");

  for (i = 0; i < G_N_ELEMENTS (self->month_buttons); i++)
    {
      self->month_buttons[i] = shopno_cal_flyout_cell_new ("shopno-cal-cell", i);
      g_signal_connect (self->month_buttons[i], "clicked",
                        G_CALLBACK (shopno_cal_flyout_month_clicked), self);
      gtk_grid_attach (GTK_GRID (grid), self->month_buttons[i], i % 4, i / 4, 1, 1);
    }

  grid = shopno_cal_flyout_grid_new ("shopno-cal-years");
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);
  gtk_stack_add_named (GTK_STACK (self->stack), grid, "years");

  for (i = 0; i < G_N_ELEMENTS (self->year_buttons); i++)
    {
      self->year_buttons[i] = shopno_cal_flyout_cell_new ("shopno-cal-cell", i);
      g_signal_connect (self->year_buttons[i], "clicked",
                        G_CALLBACK (shopno_cal_flyout_year_clicked), self);
      gtk_grid_attach (GTK_GRID (grid), self->year_buttons[i], i % 4, i / 4, 1, 1);
    }

  /* Details: a section under the calendar with the selected day, or the
   * days between two, over a line of facts and a hint. The two are pages
   * of a stack as tall as the taller one. Long lines wrap to the width of
   * the calendar instead of widening the flyout. */
  self->details = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->details), "shopno-cal-details");
  gtk_box_pack_start (GTK_BOX (self->frame), self->details, FALSE, FALSE, 0);

  self->details_stack = gtk_stack_new ();
  gtk_stack_set_homogeneous (GTK_STACK (self->details_stack), TRUE);
  gtk_box_pack_start (GTK_BOX (self->details), self->details_stack, FALSE, FALSE, 0);

  page = shopno_cal_flyout_details_page_new (&self->day_title, &self->day_facts,
                                             _("Shift-click another date to count the days between."));
  gtk_stack_add_named (GTK_STACK (self->details_stack), page, "day");

  page = shopno_cal_flyout_details_page_new (&self->range_title, &self->range_facts,
                                             _("Click any date to clear."));
  gtk_stack_add_named (GTK_STACK (self->details_stack), page, "range");

  /* Footer: a status on the left, and the copy button */
  self->footer = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->footer), "shopno-cal-footer");
  gtk_box_pack_end (GTK_BOX (self->frame), self->footer, FALSE, FALSE, 0);

  self->status_label = shopno_cal_flyout_label_new ("shopno-cal-footer-status");
  gtk_label_set_xalign (GTK_LABEL (self->status_label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (self->status_label), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (self->footer), self->status_label, TRUE, TRUE, 0);

  self->copy_button = gtk_menu_button_new ();
  gtk_button_set_relief (GTK_BUTTON (self->copy_button), GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (self->copy_button),
                        gtk_image_new_from_icon_name ("edit-copy-symbolic", GTK_ICON_SIZE_BUTTON));
  gtk_style_context_add_class (gtk_widget_get_style_context (self->copy_button), "shopno-cal-arrow");
  gtk_widget_set_valign (self->copy_button, GTK_ALIGN_CENTER);
  gtk_widget_set_tooltip_text (self->copy_button, _("Copy date or time"));
  shopno_cal_flyout_set_accessible_name (self->copy_button, _("Copy date or time"));
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (self->copy_button),
                               shopno_cal_flyout_copy_menu_new (self));
  gtk_box_pack_end (GTK_BOX (self->footer), self->copy_button, FALSE, FALSE, 0);

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

  shopno_cal_flyout_select_day (flyout, &flyout->today);
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
