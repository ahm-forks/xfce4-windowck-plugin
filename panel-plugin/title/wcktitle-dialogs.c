/*  $Id$
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Copyright (C) 2013 Alessio Piccoli <alepic@geckoblu.net>
 *                     Cedric Leporcq  <cedl38@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <common/wck-plugin.h>

#include "wcktitle-title.h"
#include "wcktitle-dialogs.h"
#include "wcktitle-dialogs_ui.h"

#define TITLE_SIZE_MIN 3


static void
on_only_maximized_toggled (GtkRadioButton *only_maximized, WckTitlePlugin *wtp)
{
    wtp->prefs->only_maximized = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (only_maximized));
    reload_wnck_title (wtp);
}


static void
on_show_on_desktop_toggled (GtkToggleButton *show_on_desktop, WckTitlePlugin *wtp)
{
    wtp->prefs->show_on_desktop = gtk_toggle_button_get_active (show_on_desktop);

    reload_wnck_title (wtp);
}


static void
on_only_current_display_toggled (GtkToggleButton *only_current_display, WckTitlePlugin *wtp)
{
    wtp->prefs->only_current_display = gtk_toggle_button_get_active (only_current_display);

    reload_wnck_title (wtp);
}


static void
on_titlesize_changed (GtkSpinButton *titlesize, WckTitlePlugin *wtp)
{
    wtp->prefs->title_size = gtk_spin_button_get_value (titlesize);
    resize_title (wtp);
}


static void
set_titlesize_sensitive (const WckTitlePlugin *wtp, gboolean sensitive)
{
    GtkWidget *titlesize;
    GtkWidget *width_unit;

    titlesize = GTK_WIDGET (gtk_builder_get_object (wtp->prefs->builder, "titlesize"));
    if (G_LIKELY (titlesize != NULL))
    {
        gtk_widget_set_sensitive (titlesize, sensitive);
    }

    width_unit = GTK_WIDGET (gtk_builder_get_object (wtp->prefs->builder, "width_unit"));
    if (G_LIKELY (width_unit != NULL))
    {
        gtk_widget_set_sensitive (width_unit, sensitive);
    }
}


static void
on_size_mode_changed (GtkComboBox *size_mode, WckTitlePlugin *wtp)
{
    gint id;

    id = gtk_combo_box_get_active (size_mode);

    if (id < 0 || id > 2)
    {
        g_critical ("Trying to set a default size but got an invalid item");
        return;
    }

    wtp->prefs->size_mode = id;

    xfce_panel_plugin_set_shrink (wtp->plugin, wtp->prefs->size_mode != SHRINK);
    set_titlesize_sensitive (wtp, wtp->prefs->size_mode != EXPAND);

    /* dynamic resizing */
    /* don't work for title shrinking -> need to restart the applet */
    // TODO: make it working...
    resize_title (wtp);
}


static void
on_full_name_toggled (GtkToggleButton *full_name, WckTitlePlugin *wtp)
{
    wtp->prefs->full_name = gtk_toggle_button_get_active (full_name);
    on_wck_state_changed (wtp->win->controlwindow, wtp);
}


static void
on_two_lines_toggled (GtkToggleButton *two_lines, WckTitlePlugin *wtp)
{
    GtkWidget *sync_wm_font, *subtitle_font, *subtitle_font_label;

    sync_wm_font = GTK_WIDGET (gtk_builder_get_object (wtp->prefs->builder, "sync_wm_font"));
    subtitle_font = GTK_WIDGET (gtk_builder_get_object (wtp->prefs->builder, "subtitle_font"));
    subtitle_font_label = GTK_WIDGET (gtk_builder_get_object (wtp->prefs->builder, "subtitle_font_label"));
    wtp->prefs->two_lines = gtk_toggle_button_get_active (two_lines);

    on_wck_state_changed (wtp->win->controlwindow, wtp);

    if (wtp->prefs->two_lines)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sync_wm_font), FALSE);
    }

    gtk_widget_set_sensitive (subtitle_font, wtp->prefs->two_lines);
    gtk_widget_set_sensitive (subtitle_font_label, wtp->prefs->two_lines);
    gtk_widget_set_sensitive (sync_wm_font, !wtp->prefs->two_lines);
}


static void
on_sync_wm_font_toggled (GtkToggleButton *sync_wm_font, WckTitlePlugin *wtp)
{
    GtkFontButton *title_font;

    wtp->prefs->sync_wm_font = gtk_toggle_button_get_active (sync_wm_font);
    init_title (wtp);
    title_font = GTK_FONT_BUTTON(gtk_builder_get_object (wtp->prefs->builder, "title_font"));
    gtk_font_chooser_set_font (GTK_FONT_CHOOSER (title_font), wtp->prefs->title_font);
}


static gchar *
set_title_font (GtkLabel *title, GtkFontButton *font_button)
{
    gchar *font = g_strdup (gtk_font_chooser_get_font (GTK_FONT_CHOOSER (font_button)));
    PangoFontDescription *font_desc = pango_font_description_from_string (font);
    PangoAttribute *attr = pango_attr_font_desc_new (font_desc);
    PangoAttrList *attr_list = pango_attr_list_new ();

    pango_attr_list_insert (attr_list, attr);
    gtk_label_set_attributes (title, attr_list);

    pango_attr_list_unref (attr_list);
    pango_font_description_free (font_desc);
    return font;
}


static void
on_title_font_set (GtkFontButton *title_font, WckTitlePlugin *wtp)
{
    g_free (wtp->prefs->title_font);
    wtp->prefs->title_font = set_title_font (wtp->title, title_font);

    if (wtp->prefs->sync_wm_font)
        xfconf_channel_set_string (wtp->wm_channel, "/general/title_font", wtp->prefs->title_font);
}


static void
on_subtitle_font_set (GtkFontButton *subtitle_font, WckTitlePlugin *wtp)
{
    g_free (wtp->prefs->subtitle_font);
    wtp->prefs->subtitle_font = set_title_font (wtp->title, subtitle_font);
}


static void
on_title_alignment_changed (GtkComboBox *title_alignment, WckTitlePlugin *wtp)
{
    gint id;

    id = gtk_combo_box_get_active(title_alignment);

    if (id < 0 || id > 2)
    {
        g_critical ("Trying to set a default size but got an invalid item");
        return;
    }

    wtp->prefs->title_alignment = id;

    set_title_alignment (wtp);
    on_wck_state_changed (wtp->win->controlwindow, wtp);
}


static void
on_title_padding_changed (GtkSpinButton *title_padding, WckTitlePlugin *wtp)
{
    wtp->prefs->title_padding = gtk_spin_button_get_value (title_padding);
    set_title_padding (wtp);
}


static GtkWidget *
build_properties_area (WckTitlePlugin *wtp, const gchar *buffer, gsize length)
{
    GError *error = NULL;

    if (wtp->prefs->builder)
        g_object_unref(G_OBJECT (wtp->prefs->builder));

    wtp->prefs->builder = gtk_builder_new();

    if (gtk_builder_add_from_string (wtp->prefs->builder, buffer, length, &error)) {
        GObject *area = gtk_builder_get_object(wtp->prefs->builder, "vbox0");

        if (G_LIKELY (area != NULL))
        {
            GtkSpinButton *titlesize, *title_padding;
            GtkComboBox *size_mode, *title_alignment;
            GtkToggleButton *sync_wm_font;
            GtkRadioButton *only_maximized, *active_window;
            GtkToggleButton *show_on_desktop, *only_current_display, *full_name, *two_lines;
            GtkFontButton *title_font, *subtitle_font;
            GtkWidget *subtitle_font_label;

            only_maximized = GTK_RADIO_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "only_maximized"));
            active_window = GTK_RADIO_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "active_window"));
            if (G_LIKELY (only_maximized != NULL && active_window != NULL))
            {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_maximized), wtp->prefs->only_maximized);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (active_window), !wtp->prefs->only_maximized);
                g_signal_connect (only_maximized, "toggled",
                                  G_CALLBACK (on_only_maximized_toggled), wtp);
            }

            show_on_desktop = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "show_on_desktop"));
            if (G_LIKELY (show_on_desktop != NULL))
            {
                gtk_toggle_button_set_active (show_on_desktop, wtp->prefs->show_on_desktop);
                g_signal_connect (show_on_desktop, "toggled",
                                  G_CALLBACK (on_show_on_desktop_toggled), wtp);
            }

            only_current_display = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "only_current_display"));
            if (G_LIKELY (only_current_display != NULL))
            {
                gtk_toggle_button_set_active (only_current_display, wtp->prefs->only_current_display);
                g_signal_connect (only_current_display, "toggled",
                                  G_CALLBACK (on_only_current_display_toggled), wtp);
            }

            full_name = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "full_name"));
            if (G_LIKELY (full_name != NULL))
            {
                gtk_toggle_button_set_active (full_name, wtp->prefs->full_name);
                g_signal_connect (full_name, "toggled",
                                  G_CALLBACK (on_full_name_toggled), wtp);
            }

            two_lines = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "two_lines"));
            if (G_LIKELY (two_lines != NULL))
            {
                gtk_toggle_button_set_active (two_lines, wtp->prefs->two_lines);
                g_signal_connect (two_lines, "toggled",
                                  G_CALLBACK (on_two_lines_toggled), wtp);
            }

            titlesize = GTK_SPIN_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "titlesize"));
            if (G_LIKELY (titlesize != NULL))
            {
                gtk_spin_button_set_range (titlesize, TITLE_SIZE_MIN, TITLE_SIZE_MAX);
                gtk_spin_button_set_increments (titlesize, 1, 1);
                gtk_spin_button_set_value (titlesize, wtp->prefs->title_size);
                g_signal_connect (titlesize, "value-changed",
                                  G_CALLBACK (on_titlesize_changed), wtp);
            }

            sync_wm_font = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "sync_wm_font"));
            if (G_LIKELY (sync_wm_font != NULL))
            {
                if (wtp->wm_channel)
                {
                    gtk_toggle_button_set_active (sync_wm_font, wtp->prefs->sync_wm_font);
                    g_signal_connect (sync_wm_font, "toggled",
                                      G_CALLBACK (on_sync_wm_font_toggled), wtp);
                }
                else {
                    gtk_widget_set_sensitive (GTK_WIDGET(sync_wm_font), FALSE);
                }
            }

            title_font = GTK_FONT_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "title_font"));
            if (G_LIKELY (title_font != NULL))
            {
                gtk_font_chooser_set_font (GTK_FONT_CHOOSER (title_font), wtp->prefs->title_font);
                g_signal_connect (title_font, "font-set",
                                  G_CALLBACK (on_title_font_set), wtp);
            }

            subtitle_font = GTK_FONT_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "subtitle_font"));
            subtitle_font_label = wck_dialog_get_widget (wtp->prefs->builder, "subtitle_font_label");
            if (G_LIKELY (subtitle_font != NULL && subtitle_font_label != NULL))
            {
                gtk_font_chooser_set_font (GTK_FONT_CHOOSER (subtitle_font), wtp->prefs->subtitle_font);
                gtk_widget_set_sensitive (GTK_WIDGET(subtitle_font), wtp->prefs->two_lines);
                gtk_widget_set_sensitive (subtitle_font_label, wtp->prefs->two_lines);
                g_signal_connect (subtitle_font, "font-set",
                                  G_CALLBACK (on_subtitle_font_set), wtp);
            }

            title_alignment = GTK_COMBO_BOX (wck_dialog_get_widget (wtp->prefs->builder, "title_alignment"));
            if (G_LIKELY (title_alignment != NULL))
            {
                gtk_combo_box_set_active (title_alignment, wtp->prefs->title_alignment);
                g_signal_connect (title_alignment, "changed",
                                  G_CALLBACK (on_title_alignment_changed), wtp);
            }

            title_padding = GTK_SPIN_BUTTON (wck_dialog_get_widget (wtp->prefs->builder, "title_padding"));
            if (G_LIKELY (title_padding != NULL))
            {
                gtk_spin_button_set_range (title_padding, 0, 99);
                gtk_spin_button_set_increments (title_padding, 1, 1);
                gtk_spin_button_set_value (title_padding, wtp->prefs->title_padding);
                g_signal_connect (title_padding, "value-changed",
                                  G_CALLBACK (on_title_padding_changed), wtp);
            }

            size_mode = GTK_COMBO_BOX (wck_dialog_get_widget (wtp->prefs->builder, "size_mode"));
            if (G_LIKELY (size_mode != NULL))
            {
                gtk_combo_box_set_active (size_mode, wtp->prefs->size_mode);

                if (wtp->prefs->size_mode == EXPAND)
                {
                    set_titlesize_sensitive (wtp, FALSE);
                }

                g_signal_connect (size_mode, "changed",
                                  G_CALLBACK (on_size_mode_changed), wtp);
            }

            return GTK_WIDGET(area);
        }
        else {
            g_set_error_literal (&error, 0, 0, "No widget with the name \"vbox0\" found");
        }
    }

    g_critical ("Failed to construct the builder for plugin %s-%d: %s.", xfce_panel_plugin_get_name (wtp->plugin), xfce_panel_plugin_get_unique_id (wtp->plugin), error->message);
    g_error_free (error);
    g_object_unref (G_OBJECT (wtp->prefs->builder));

    return NULL;
}


static void
wcktitle_configure_response (GtkWidget *dialog, gint response, WckTitlePlugin *wtp)
{
    wck_configure_response (wtp->plugin, dialog, response, (WckSettingsCb) wcktitle_settings_save, wtp->prefs);
}


void
wcktitle_configure (XfcePanelPlugin *plugin, WckTitlePlugin *wtp)
{
    GtkWidget *ca;

    ca = build_properties_area (wtp, wcktitle_dialogs_ui, wcktitle_dialogs_ui_length);

    wck_configure_dialog (plugin, WCKTITLE_ICON, ca, G_CALLBACK (wcktitle_configure_response), wtp);
}
