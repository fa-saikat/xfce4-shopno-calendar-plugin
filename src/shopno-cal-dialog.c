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
#include <libxfce4ui/libxfce4ui.h>

#include "shopno-cal-dialog.h"
#include "shopno-cal-format.h"

#define BINDING_FLAGS (G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE)

#define FORMAT_CODES_URL "https://docs.gtk.org/glib/method.DateTime.format.html"

/* In the order of ShopnoCalLayout */
static const gchar *layouts[] =
{
  N_("Time over date"),
  N_("Date and time on one line"),
  N_("Time only"),
  N_("Custom format"),
};

G_STATIC_ASSERT (G_N_ELEMENTS (layouts) == SHOPNO_CAL_LAYOUT_LAST + 1);

/* The codes most people need; the link lists the rest */
static const struct
{
  const gchar *code;
  const gchar *meaning;
}
codes[] =
{
  { "%H", N_("hour, 00–23") },
  { "%-l", N_("hour, 1–12") },
  { "%M", N_("minute") },
  { "%S", N_("second") },
  { "%p", N_("AM or PM") },
  { "%a", N_("weekday, short") },
  { "%A", N_("weekday") },
  { "%-d", N_("day of the month") },
  { "%b", N_("month, short") },
  { "%B", N_("month") },
  { "%Y", N_("year") },
  { "%x", N_("date, as in your region") },
  { "%n", N_("new line") },
};



static GtkWidget *
shopno_cal_dialog_heading_new (const gchar *text)
{
  GtkWidget *label = gtk_label_new (NULL);
  gchar     *markup = g_markup_printf_escaped ("<b>%s</b>", text);

  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  g_free (markup);

  return label;
}



/*
 * Preview
 */

static void
shopno_cal_dialog_update_preview (GtkWidget *preview)
{
  ShopnoCalSettings *settings = g_object_get_data (G_OBJECT (preview), "settings");
  ShopnoCalClock    *clock = g_object_get_data (G_OBJECT (preview), "clock");
  GtkStyleContext   *context = gtk_widget_get_style_context (preview);
  GDateTime         *now;
  gchar             *format;
  gchar             *text;

  format = shopno_cal_format_panel (shopno_cal_settings_get_layout (settings),
                                    shopno_cal_settings_get_custom_format (settings),
                                    shopno_cal_settings_get_use_24h (settings),
                                    shopno_cal_settings_get_show_seconds (settings),
                                    FALSE);
  now = shopno_cal_clock_get_now (clock);
  text = shopno_cal_format_time (now, format);

  /* The panel falls back to the time when a custom format shows nothing */
  if (text != NULL)
    {
      gtk_label_set_text (GTK_LABEL (preview), text);
      gtk_label_set_justify (GTK_LABEL (preview), GTK_JUSTIFY_RIGHT);
      gtk_style_context_remove_class (context, "warning");
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (preview), _("This format shows nothing, so the panel shows the time."));
      gtk_label_set_justify (GTK_LABEL (preview), GTK_JUSTIFY_LEFT);
      gtk_style_context_add_class (context, "warning");
    }

  g_free (text);
  g_free (format);
  g_date_time_unref (now);
}



static GtkWidget *
shopno_cal_dialog_preview_new (ShopnoCalSettings *settings,
                               ShopnoCalClock    *clock)
{
  GtkWidget     *preview = gtk_label_new (NULL);
  PangoAttrList *attrs = pango_attr_list_new ();

  /* Larger, as on the panel, with digits of equal width */
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_LARGE));
  pango_attr_list_insert (attrs, pango_attr_font_features_new ("tnum"));
  gtk_label_set_attributes (GTK_LABEL (preview), attrs);
  pango_attr_list_unref (attrs);

  gtk_label_set_xalign (GTK_LABEL (preview), 0.0);
  gtk_label_set_selectable (GTK_LABEL (preview), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (preview), TRUE);
  gtk_label_set_max_width_chars (GTK_LABEL (preview), 40);

  g_object_set_data_full (G_OBJECT (preview), "settings", g_object_ref (settings), g_object_unref);
  g_object_set_data_full (G_OBJECT (preview), "clock", g_object_ref (clock), g_object_unref);

  g_signal_connect_object (settings, "notify", G_CALLBACK (shopno_cal_dialog_update_preview),
                           preview, G_CONNECT_SWAPPED);
  g_signal_connect_object (clock, "tick", G_CALLBACK (shopno_cal_dialog_update_preview),
                           preview, G_CONNECT_SWAPPED);

  shopno_cal_dialog_update_preview (preview);

  return preview;
}



/*
 * Format codes
 */

static GtkWidget *
shopno_cal_dialog_codes_new (void)
{
  GtkWidget *grid = gtk_grid_new ();
  GtkWidget *label;
  gchar     *markup;
  guint      rows = (G_N_ELEMENTS (codes) + 1) / 2;

  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 8);

  /* Two columns of code and meaning */
  for (guint i = 0; i < G_N_ELEMENTS (codes); i++)
    {
      guint column = (i / rows) * 2;
      guint row = i % rows;

      markup = g_markup_printf_escaped ("<small><tt>%s</tt></small>", codes[i].code);
      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_widget_set_margin_start (label, column > 0 ? 12 : 0);
      gtk_grid_attach (GTK_GRID (grid), label, column, row, 1, 1);
      g_free (markup);

      markup = g_markup_printf_escaped ("<small>%s</small>", _(codes[i].meaning));
      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
      gtk_grid_attach (GTK_GRID (grid), label, column + 1, row, 1, 1);
      g_free (markup);
    }

  markup = g_strdup_printf ("<small><a href=\"%s\">%s</a></small>", FORMAT_CODES_URL, _("All codes"));
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, rows, 4, 1);
  g_free (markup);

  return grid;
}



/*
 * Bindings
 */

static gboolean
shopno_cal_dialog_is_custom (GBinding     *binding,
                             const GValue *from,
                             GValue       *to,
                             gpointer      data)
{
  g_value_set_boolean (to, g_value_get_uint (from) == SHOPNO_CAL_LAYOUT_CUSTOM);
  return TRUE;
}



static gboolean
shopno_cal_dialog_is_not_custom (GBinding     *binding,
                                 const GValue *from,
                                 GValue       *to,
                                 gpointer      data)
{
  g_value_set_boolean (to, g_value_get_uint (from) != SHOPNO_CAL_LAYOUT_CUSTOM);
  return TRUE;
}



static void
shopno_cal_dialog_bind_custom (ShopnoCalSettings *settings,
                               GtkWidget         *widget,
                               gboolean           custom)
{
  g_object_bind_property_full (settings, "layout", widget, "sensitive", G_BINDING_SYNC_CREATE,
                               custom ? shopno_cal_dialog_is_custom : shopno_cal_dialog_is_not_custom,
                               NULL, NULL, NULL);
}



GtkWidget *
shopno_cal_dialog_new (GtkWindow         *parent,
                       ShopnoCalSettings *settings,
                       ShopnoCalClock    *clock)
{
  GtkWidget *dialog;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *combo;
  GtkWidget *entry;
  GtkWidget *check;
  GtkWidget *codes_grid;
  gint       row = 0;

  g_return_val_if_fail (SHOPNO_IS_CAL_SETTINGS (settings), NULL);
  g_return_val_if_fail (SHOPNO_IS_CAL_CLOCK (clock), NULL);

  dialog = xfce_titled_dialog_new_with_mixed_buttons (_("Shopno Calendar"), parent,
                                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                                      "window-close-symbolic", _("_Close"), GTK_RESPONSE_CLOSE,
                                                      NULL);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "preferences-system-time");
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), grid, TRUE, TRUE, 0);

  /* Panel clock */
  gtk_grid_attach (GTK_GRID (grid), shopno_cal_dialog_heading_new (_("Panel clock")), 0, row++, 2, 1);

  label = gtk_label_new_with_mnemonic (_("_Layout:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_widget_set_margin_start (label, 12);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  combo = gtk_combo_box_text_new ();
  for (guint i = 0; i < G_N_ELEMENTS (layouts); i++)
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _(layouts[i]));
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  g_object_bind_property (settings, "layout", combo, "active", BINDING_FLAGS);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row++, 1, 1);

  label = gtk_label_new_with_mnemonic (_("_Format:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_widget_set_margin_start (label, 12);
  shopno_cal_dialog_bind_custom (settings, label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 28);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  g_object_bind_property (settings, "custom-format", entry, "text", BINDING_FLAGS);
  shopno_cal_dialog_bind_custom (settings, entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row++, 1, 1);

  codes_grid = shopno_cal_dialog_codes_new ();
  shopno_cal_dialog_bind_custom (settings, codes_grid, TRUE);
  gtk_grid_attach (GTK_GRID (grid), codes_grid, 1, row++, 1, 1);

  check = gtk_check_button_new_with_mnemonic (_("_24-hour time"));
  gtk_widget_set_margin_start (check, 12);
  g_object_bind_property (settings, "use-24h", check, "active", BINDING_FLAGS);
  shopno_cal_dialog_bind_custom (settings, check, FALSE);
  gtk_grid_attach (GTK_GRID (grid), check, 0, row++, 2, 1);

  check = gtk_check_button_new_with_mnemonic (_("Show _seconds"));
  gtk_widget_set_margin_start (check, 12);
  g_object_bind_property (settings, "show-seconds", check, "active", BINDING_FLAGS);
  shopno_cal_dialog_bind_custom (settings, check, FALSE);
  gtk_grid_attach (GTK_GRID (grid), check, 0, row++, 2, 1);

  /* Preview */
  label = shopno_cal_dialog_heading_new (_("Preview"));
  gtk_widget_set_margin_top (label, 12);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row++, 2, 1);

  label = shopno_cal_dialog_preview_new (settings, clock);
  gtk_widget_set_margin_start (label, 12);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row++, 2, 1);

  gtk_widget_show_all (grid);

  return dialog;
}
