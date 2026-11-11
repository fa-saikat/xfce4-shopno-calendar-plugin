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

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>

#include "shopno-cal-clock.h"
#include "shopno-cal-details.h"
#include "shopno-cal-dialog.h"
#include "shopno-cal-flyout.h"
#include "shopno-cal-format.h"
#include "shopno-cal-month.h"
#include "shopno-cal-settings.h"

/* Shown when a custom format gives nothing */
#define FALLBACK_FORMAT "%H:%M"

/* Space between the flyout and the panel, and the screen edges (as in
 * Shopno Menu) */
#define POPUP_GAP 12

typedef struct _ShopnoCalPluginClass ShopnoCalPluginClass;
typedef struct _ShopnoCalPlugin      ShopnoCalPlugin;

struct _ShopnoCalPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _ShopnoCalPlugin
{
  XfcePanelPlugin    __parent__;

  ShopnoCalSettings *settings;
  XfconfChannel     *channel;
  ShopnoCalClock    *clock;
  gchar             *format;

  GtkWidget         *button;
  GtkWidget         *label;
  GtkWidget         *flyout;

  /* The day the tooltip shows, to notice midnight */
  GDate              tooltip_day;
};

/* Resident: the settings and clock types are static GTypes, which do
 * not survive the module being unloaded */
XFCE_PANEL_DEFINE_PLUGIN_RESIDENT (ShopnoCalPlugin, shopno_cal_plugin)



/*
 * Settings storage
 */

static void
shopno_cal_plugin_bind (ShopnoCalPlugin *self,
                        const gchar     *name,
                        GType            type)
{
  gchar *path = g_strconcat (xfce_panel_plugin_get_property_base (XFCE_PANEL_PLUGIN (self)), "/", name, NULL);

  xfconf_g_property_bind (self->channel, path, type, self->settings, name);
  g_free (path);
}



static void
shopno_cal_plugin_load_settings (ShopnoCalPlugin *self)
{
  GError *error = NULL;

  if (!xfconf_init (&error))
    {
      g_warning ("Failed to connect to xfconf, settings will not be saved: %s", error->message);
      g_error_free (error);
      return;
    }

  self->channel = xfconf_channel_get ("xfce4-panel");

  shopno_cal_plugin_bind (self, "layout", G_TYPE_UINT);
  shopno_cal_plugin_bind (self, "custom-format", G_TYPE_STRING);
  shopno_cal_plugin_bind (self, "use-24h", G_TYPE_BOOLEAN);
  shopno_cal_plugin_bind (self, "show-seconds", G_TYPE_BOOLEAN);
  shopno_cal_plugin_bind (self, "week-start", G_TYPE_UINT);
  shopno_cal_plugin_bind (self, "weekend", G_TYPE_UINT);
  shopno_cal_plugin_bind (self, "week-numbers", G_TYPE_BOOLEAN);
  shopno_cal_plugin_bind (self, "day-details", G_TYPE_BOOLEAN);
  shopno_cal_plugin_bind (self, "show-tooltip", G_TYPE_BOOLEAN);
  shopno_cal_plugin_bind (self, "copy-button", G_TYPE_BOOLEAN);
}



/*
 * Clock label
 */

static void
shopno_cal_plugin_update_label (ShopnoCalPlugin *self)
{
  GDateTime *now = shopno_cal_clock_get_now (self->clock);
  gchar     *text;

  text = shopno_cal_format_time (now, self->format);
  if (text == NULL)
    text = shopno_cal_format_time (now, FALLBACK_FORMAT);

  /* Skip the relayout when only the clock ticked */
  if (g_strcmp0 (text, gtk_label_get_text (GTK_LABEL (self->label))) != 0)
    gtk_label_set_text (GTK_LABEL (self->label), text);

  g_free (text);
  g_date_time_unref (now);
}



/* Every second while the flyout shows its seconds, otherwise as often as
 * the panel format needs */
static void
shopno_cal_plugin_update_interval (ShopnoCalPlugin *self)
{
  guint interval = shopno_cal_format_interval (self->format);

  if (self->flyout != NULL && gtk_widget_get_visible (self->flyout))
    interval = SHOPNO_CAL_CLOCK_SECOND;

  shopno_cal_clock_set_interval (self->clock, interval);
}



static void
shopno_cal_plugin_update_format (ShopnoCalPlugin *self)
{
  XfcePanelPluginMode mode = xfce_panel_plugin_get_mode (XFCE_PANEL_PLUGIN (self));

  g_free (self->format);
  self->format = shopno_cal_format_panel (shopno_cal_settings_get_layout (self->settings),
                                          shopno_cal_settings_get_custom_format (self->settings),
                                          shopno_cal_settings_get_use_24h (self->settings),
                                          shopno_cal_settings_get_show_seconds (self->settings),
                                          mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR);

  shopno_cal_plugin_update_interval (self);
  shopno_cal_plugin_update_label (self);
}



static void
shopno_cal_plugin_update_orientation (ShopnoCalPlugin *self)
{
  XfcePanelPluginMode mode = xfce_panel_plugin_get_mode (XFCE_PANEL_PLUGIN (self));

  /* A vertical panel turns its labels; a deskbar keeps them upright and
   * stacks the clock instead (see shopno_cal_format_panel) */
  gtk_label_set_angle (GTK_LABEL (self->label), mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL ? 270 : 0);

  /* Windows aligns the two lines to the right, next to the screen edge */
  gtk_label_set_justify (GTK_LABEL (self->label),
                         mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL ? GTK_JUSTIFY_RIGHT : GTK_JUSTIFY_CENTER);

  shopno_cal_plugin_update_format (self);
}



/*
 * Tooltip
 */

static void
shopno_cal_plugin_today (ShopnoCalPlugin *self,
                         GDate           *today)
{
  GDateTime *now = shopno_cal_clock_get_now (self->clock);

  g_date_clear (today, 1);
  g_date_set_dmy (today, g_date_time_get_day_of_month (now),
                  g_date_time_get_month (now), g_date_time_get_year (now));
  g_date_time_unref (now);
}



/* Built when the pointer rests on the clock, so it is never out of date.
 * Not over the open flyout, which shows all of it already. */
static gboolean
shopno_cal_plugin_query_tooltip (GtkWidget       *button,
                                 gint             x,
                                 gint             y,
                                 gboolean         keyboard_mode,
                                 GtkTooltip      *tooltip,
                                 ShopnoCalPlugin *self)
{
  GDate  today;
  gchar *markup;

  if (!shopno_cal_settings_get_show_tooltip (self->settings)
      || (self->flyout != NULL && gtk_widget_get_visible (self->flyout)))
    return FALSE;

  shopno_cal_plugin_today (self, &today);
  markup = shopno_cal_details_tooltip (&today,
                                       shopno_cal_month_week_start (shopno_cal_settings_get_week_start (self->settings)));
  gtk_tooltip_set_markup (tooltip, markup);
  g_free (markup);

  return TRUE;
}



/* A tooltip shown at midnight moves on to the new day */
static void
shopno_cal_plugin_update_tooltip (ShopnoCalPlugin *self)
{
  GDate today;

  shopno_cal_plugin_today (self, &today);

  if (!g_date_valid (&self->tooltip_day) || g_date_compare (&today, &self->tooltip_day) != 0)
    {
      self->tooltip_day = today;
      gtk_widget_trigger_tooltip_query (self->button);
    }
}



/*
 * Flyout
 */

static void
shopno_cal_plugin_flyout_visibility (ShopnoCalPlugin *self)
{
  gboolean visible = gtk_widget_get_visible (self->flyout);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->button), visible);
  xfce_panel_plugin_block_autohide (XFCE_PANEL_PLUGIN (self), visible);
  shopno_cal_plugin_update_interval (self);
}



static void
shopno_cal_plugin_add_gap (ShopnoCalPlugin *self,
                           gint            *x,
                           gint            *y)
{
  GdkDisplay     *display = gtk_widget_get_display (self->button);
  GdkMonitor     *monitor;
  GdkRectangle    geometry;
  GtkRequisition  size;
  gboolean        horizontal = TRUE;

  /* Move the flyout away from the panel, on the side it opens towards */
  switch (xfce_panel_plugin_arrow_type (XFCE_PANEL_PLUGIN (self)))
    {
    case GTK_ARROW_UP:
      *y -= POPUP_GAP;
      break;

    case GTK_ARROW_DOWN:
      *y += POPUP_GAP;
      break;

    case GTK_ARROW_LEFT:
      *x -= POPUP_GAP;
      horizontal = FALSE;
      break;

    case GTK_ARROW_RIGHT:
      *x += POPUP_GAP;
      horizontal = FALSE;
      break;

    default:
      break;
    }

  /* and keep it clear of the screen edges along the panel */
  monitor = gdk_display_get_monitor_at_window (display, gtk_widget_get_window (self->button));
  if (monitor == NULL)
    return;

  gdk_monitor_get_geometry (monitor, &geometry);
  gtk_widget_get_preferred_size (self->flyout, NULL, &size);

  if (horizontal)
    *x = MAX (geometry.x + POPUP_GAP, MIN (*x, geometry.x + geometry.width - size.width - POPUP_GAP));
  else
    *y = MAX (geometry.y + POPUP_GAP, MIN (*y, geometry.y + geometry.height - size.height - POPUP_GAP));
}



static void
shopno_cal_plugin_get_position (ShopnoCalPlugin *self,
                                gint            *x,
                                gint            *y)
{
  xfce_panel_plugin_position_widget (XFCE_PANEL_PLUGIN (self), self->flyout, self->button, x, y);
  shopno_cal_plugin_add_gap (self, x, y);
}



/* The flyout grows or shrinks while open when a setting such as the week
 * numbers changes; keep it next to the button and on the screen */
static void
shopno_cal_plugin_flyout_resized (ShopnoCalPlugin *self)
{
  gint x, y;
  gint old_x, old_y;

  if (!gtk_widget_get_visible (self->flyout))
    return;

  shopno_cal_plugin_get_position (self, &x, &y);
  gtk_window_get_position (GTK_WINDOW (self->flyout), &old_x, &old_y);

  if (x != old_x || y != old_y)
    gtk_window_move (GTK_WINDOW (self->flyout), x, y);
}



static void
shopno_cal_plugin_popup (ShopnoCalPlugin *self)
{
  gint x, y;

  if (self->flyout == NULL)
    {
      self->flyout = shopno_cal_flyout_new (self->settings, self->clock);
      g_signal_connect_swapped (self->flyout, "show",
                                G_CALLBACK (shopno_cal_plugin_flyout_visibility), self);
      g_signal_connect_swapped (self->flyout, "hide",
                                G_CALLBACK (shopno_cal_plugin_flyout_visibility), self);
      g_signal_connect_swapped (self->flyout, "size-allocate",
                                G_CALLBACK (shopno_cal_plugin_flyout_resized), self);
    }

  gtk_window_set_screen (GTK_WINDOW (self->flyout), gtk_widget_get_screen (self->button));

  shopno_cal_plugin_get_position (self, &x, &y);
  shopno_cal_flyout_popup (SHOPNO_CAL_FLYOUT (self->flyout), x, y);
}



static gboolean
shopno_cal_plugin_button_press_event (GtkWidget       *button,
                                      GdkEventButton  *event,
                                      ShopnoCalPlugin *self)
{
  /* Leave Ctrl+click and other buttons to the panel */
  if (event->button != 1
      || event->type != GDK_BUTTON_PRESS
      || (event->state & GDK_CONTROL_MASK) != 0)
    return GDK_EVENT_PROPAGATE;

  if (self->flyout != NULL && gtk_widget_get_visible (self->flyout))
    gtk_widget_hide (self->flyout);
  else
    shopno_cal_plugin_popup (self);

  return GDK_EVENT_STOP;
}



/*
 * XfcePanelPlugin
 */

static void
shopno_cal_plugin_construct (XfcePanelPlugin *plugin)
{
  ShopnoCalPlugin *self = (ShopnoCalPlugin *) plugin;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  xfce_panel_plugin_menu_show_configure (plugin);
  xfce_panel_plugin_menu_show_about (plugin);

  shopno_cal_plugin_load_settings (self);

  g_signal_connect_swapped (self->settings, "notify",
                            G_CALLBACK (shopno_cal_plugin_update_format), self);
  g_signal_connect_swapped (self->clock, "tick",
                            G_CALLBACK (shopno_cal_plugin_update_label), self);
  g_signal_connect_swapped (self->clock, "tick",
                            G_CALLBACK (shopno_cal_plugin_update_tooltip), self);

  shopno_cal_plugin_update_orientation (self);
}



static void
shopno_cal_plugin_free_data (XfcePanelPlugin *plugin)
{
  ShopnoCalPlugin *self = (ShopnoCalPlugin *) plugin;

  if (self->flyout != NULL)
    {
      g_signal_handlers_disconnect_by_data (self->flyout, self);
      gtk_widget_destroy (self->flyout);
      self->flyout = NULL;
    }

  g_signal_handlers_disconnect_by_data (self->clock, self);
  g_object_unref (self->clock);

  g_signal_handlers_disconnect_by_data (self->settings, self);
  if (self->channel != NULL)
    {
      xfconf_g_property_unbind_all (self->settings);
      xfconf_shutdown ();
    }
  g_object_unref (self->settings);

  g_free (self->format);
}



static gboolean
shopno_cal_plugin_size_changed (XfcePanelPlugin *plugin,
                                gint             size)
{
  size /= xfce_panel_plugin_get_nrows (plugin);

  /* The button grows along the panel to fit the text */
  if (xfce_panel_plugin_get_orientation (plugin) == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

  return TRUE;
}



static void
shopno_cal_plugin_mode_changed (XfcePanelPlugin     *plugin,
                                XfcePanelPluginMode  mode)
{
  shopno_cal_plugin_update_orientation ((ShopnoCalPlugin *) plugin);
}



static void
shopno_cal_plugin_configure_plugin (XfcePanelPlugin *plugin)
{
  ShopnoCalPlugin *self = (ShopnoCalPlugin *) plugin;
  GtkWidget       *dialog;

  dialog = shopno_cal_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                  self->settings, self->clock);

  xfce_panel_plugin_block_menu (plugin);
  g_signal_connect_swapped (dialog, "destroy", G_CALLBACK (xfce_panel_plugin_unblock_menu), plugin);
  gtk_widget_show (dialog);
}



static void
shopno_cal_plugin_about (XfcePanelPlugin *plugin)
{
  const gchar *authors[] = { "FA Saikat <fahimabrar.saikat@gmail.com>", NULL };

  gtk_show_about_dialog (NULL,
                         "program-name", _("Shopno Calendar"),
                         "version", PACKAGE_VERSION,
                         "comments", _("A clock and calendar flyout for the Xfce panel"),
                         "website", "https://github.com/fa-saikat/xfce4-shopno-calendar-plugin",
                         "license-type", GTK_LICENSE_GPL_2_0,
                         "authors", authors,
                         "logo-icon-name", "shopno-calendar",
                         NULL);
}



static void
shopno_cal_plugin_class_init (ShopnoCalPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);

  plugin_class->construct = shopno_cal_plugin_construct;
  plugin_class->free_data = shopno_cal_plugin_free_data;
  plugin_class->size_changed = shopno_cal_plugin_size_changed;
  plugin_class->mode_changed = shopno_cal_plugin_mode_changed;
  plugin_class->configure_plugin = shopno_cal_plugin_configure_plugin;
  plugin_class->about = shopno_cal_plugin_about;
}



static void
shopno_cal_plugin_init (ShopnoCalPlugin *self)
{
  PangoAttrList *attrs;

  self->settings = shopno_cal_settings_new ();
  self->clock = shopno_cal_clock_new ();

  self->button = xfce_panel_create_toggle_button ();
  gtk_widget_set_name (self->button, "shopno-calendar-button");
  g_signal_connect (self->button, "button-press-event",
                    G_CALLBACK (shopno_cal_plugin_button_press_event), self);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (self), self->button);
  gtk_widget_set_has_tooltip (self->button, TRUE);
  g_signal_connect (self->button, "query-tooltip",
                    G_CALLBACK (shopno_cal_plugin_query_tooltip), self);
  g_date_clear (&self->tooltip_day, 1);
  gtk_container_add (GTK_CONTAINER (self), self->button);

  self->label = gtk_label_new (NULL);

  /* Digits of equal width, so the clock does not wobble as it ticks */
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_font_features_new ("tnum"));
  gtk_label_set_attributes (GTK_LABEL (self->label), attrs);
  pango_attr_list_unref (attrs);

  gtk_container_add (GTK_CONTAINER (self->button), self->label);

  gtk_widget_show_all (self->button);
}
