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
#include "shopno-cal-format.h"
#include "shopno-cal-settings.h"

/* Shown when a custom format gives nothing */
#define FALLBACK_FORMAT "%H:%M"

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

  shopno_cal_clock_set_interval (self->clock, shopno_cal_format_interval (self->format));
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
 * XfcePanelPlugin
 */

static void
shopno_cal_plugin_construct (XfcePanelPlugin *plugin)
{
  ShopnoCalPlugin *self = (ShopnoCalPlugin *) plugin;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  xfce_panel_plugin_menu_show_about (plugin);

  shopno_cal_plugin_load_settings (self);

  g_signal_connect_swapped (self->settings, "notify",
                            G_CALLBACK (shopno_cal_plugin_update_format), self);
  g_signal_connect_swapped (self->clock, "tick",
                            G_CALLBACK (shopno_cal_plugin_update_label), self);

  shopno_cal_plugin_update_orientation (self);
}



static void
shopno_cal_plugin_free_data (XfcePanelPlugin *plugin)
{
  ShopnoCalPlugin *self = (ShopnoCalPlugin *) plugin;

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
                         "logo-icon-name", "x-office-calendar",
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
  plugin_class->about = shopno_cal_plugin_about;
}



static void
shopno_cal_plugin_init (ShopnoCalPlugin *self)
{
  PangoAttrList *attrs;

  self->settings = shopno_cal_settings_new ();
  self->clock = shopno_cal_clock_new ();

  self->button = xfce_panel_create_button ();
  gtk_widget_set_name (self->button, "shopno-calendar-button");
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (self), self->button);
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
