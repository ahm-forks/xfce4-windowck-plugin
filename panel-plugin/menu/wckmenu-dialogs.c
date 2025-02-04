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

#include "wckmenu.h"
#include "wckmenu-icon.h"
#include "wckmenu-dialogs.h"
#include "wckmenu-dialogs_ui.h"


static void
on_only_maximized_toggled (GtkRadioButton *only_maximized, WckMenuPlugin *wmp)
{
    wmp->prefs->only_maximized = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(only_maximized));
    reload_wnck_icon (wmp);
}

static void
on_only_current_display_toggled (GtkToggleButton *only_current_display, WckMenuPlugin *wmp)
{
    wmp->prefs->only_current_display = gtk_toggle_button_get_active (only_current_display);
    reload_wnck_icon (wmp);
}


static void
on_show_on_desktop_toggled (GtkToggleButton *show_on_desktop, WckMenuPlugin *wmp)
{
    wmp->prefs->show_on_desktop = gtk_toggle_button_get_active(show_on_desktop);

    if (wmp->icon->symbol)
        gtk_widget_set_sensitive (wmp->icon->symbol, TRUE);
    reload_wnck_icon (wmp);
}


static void
on_show_app_icon_toggled (GtkToggleButton *show_app_icon, WckMenuPlugin *wmp)
{
    wmp->prefs->show_app_icon = gtk_toggle_button_get_active(show_app_icon);

    reset_symbol (wmp);

    if (!wmp->prefs->show_app_icon)
        wck_signal_handler_disconnect (G_OBJECT(wmp->win->controlwindow), wmp->cih);

    on_wck_state_changed (wmp->win->controlwindow, wmp);
}


static GtkWidget *
build_properties_area (WckMenuPlugin *wmp, const gchar *buffer, gsize length)
{
    GError *error = NULL;

    if (wmp->prefs->builder)
        g_object_unref(G_OBJECT (wmp->prefs->builder));

    wmp->prefs->builder = gtk_builder_new();

    if (gtk_builder_add_from_string(wmp->prefs->builder, buffer, length, &error)) {
        GObject *area = gtk_builder_get_object(wmp->prefs->builder, "vbox0");

        if (G_LIKELY (area != NULL)) {
            GtkRadioButton *only_maximized, *active_window;
            GtkToggleButton *show_on_desktop;
            GtkToggleButton *only_current_display;
            GtkToggleButton *show_app_icon;

            only_maximized = GTK_RADIO_BUTTON (wck_dialog_get_widget (wmp->prefs->builder, "only_maximized"));
            active_window = GTK_RADIO_BUTTON (wck_dialog_get_widget (wmp->prefs->builder, "active_window"));
            if (G_LIKELY (only_maximized != NULL && active_window != NULL)) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(only_maximized), wmp->prefs->only_maximized);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_window), !wmp->prefs->only_maximized);
                g_signal_connect(only_maximized, "toggled", G_CALLBACK(on_only_maximized_toggled), wmp);
            }

            show_on_desktop = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wmp->prefs->builder, "show_on_desktop"));
            if (G_LIKELY (show_on_desktop != NULL)) {
                gtk_toggle_button_set_active(show_on_desktop, wmp->prefs->show_on_desktop);
                g_signal_connect(show_on_desktop, "toggled", G_CALLBACK(on_show_on_desktop_toggled), wmp);
            }

            only_current_display = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wmp->prefs->builder, "only_current_display"));
            if (G_LIKELY (only_current_display != NULL)) {
                gtk_toggle_button_set_active (only_current_display, wmp->prefs->only_current_display);
                g_signal_connect (only_current_display, "toggled",
                                  G_CALLBACK (on_only_current_display_toggled), wmp);
            }

            show_app_icon = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wmp->prefs->builder, "show_app_icon"));
            if (G_LIKELY (show_app_icon != NULL)) {
                gtk_toggle_button_set_active(show_app_icon, wmp->prefs->show_app_icon);
                g_signal_connect(show_app_icon, "toggled", G_CALLBACK(on_show_app_icon_toggled), wmp);
            }

            return GTK_WIDGET(area);
        } else {
            g_set_error_literal (&error, 0, 0, "No widget with the name \"vbox0\" found");
        }
    }

    g_critical("Failed to construct the builder for plugin %s-%d: %s.", xfce_panel_plugin_get_name (wmp->plugin), xfce_panel_plugin_get_unique_id (wmp->plugin), error->message);
    g_error_free(error);
    g_object_unref (G_OBJECT (wmp->prefs->builder));

    return NULL;
}


static void
wckmenu_configure_response (GtkWidget *dialog, gint response, WckMenuPlugin *wmp)
{
    wck_configure_response (wmp->plugin, dialog, response, (WckSettingsCb) wckmenu_settings_save, wmp->prefs);
}


void
wckmenu_configure (XfcePanelPlugin *plugin, WckMenuPlugin *wmp)
{
    GtkWidget *ca;

    ca = build_properties_area (wmp, wckmenu_dialogs_ui, wckmenu_dialogs_ui_length);

    wck_configure_dialog (plugin, WCKMENU_ICON, ca, G_CALLBACK(wckmenu_configure_response), wmp);
}
