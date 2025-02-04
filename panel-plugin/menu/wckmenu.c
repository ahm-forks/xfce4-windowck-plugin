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
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include <common/wck-plugin.h>

#include "wckmenu.h"
#include "wckmenu-dialogs.h"
#include "wckmenu-icon.h"

#define ICON_PADDING 3

#define SETTING_SHOW_APP_ICON  "/show-app-icon"
#define SETTING_INACTIVE_ALPHA "/inactive-alpha"
#define SETTING_INACTIVE_SHADE "/inactive-shade"

/* default settings */
#define DEFAULT_SHOW_APP_ICON TRUE
#define DEFAULT_INACTIVE_ALPHA 60
#define DEFAULT_INACTIVE_SHADE 110

/* prototypes */
static void wckmenu_construct(XfcePanelPlugin *plugin);


void
wckmenu_settings_save (WckMenuPreferences *prefs)
{
    wck_conf_set_bool (prefs->conf, SETTING_ONLY_MAXIMIZED, prefs->only_maximized);
    wck_conf_set_bool (prefs->conf, SETTING_ONLY_CURRDISPLAY, prefs->only_current_display);
    wck_conf_set_bool (prefs->conf, SETTING_SHOW_ON_DESKTOP, prefs->show_on_desktop);
    wck_conf_set_bool (prefs->conf, SETTING_SHOW_APP_ICON, prefs->show_app_icon);
    wck_conf_set_int (prefs->conf, SETTING_INACTIVE_ALPHA, prefs->inactive_alpha);
    wck_conf_set_int (prefs->conf, SETTING_INACTIVE_SHADE, prefs->inactive_shade);
}

static void
wckmenu_save (G_GNUC_UNUSED XfcePanelPlugin *plugin, WckMenuPlugin *wmp)
{
    wckmenu_settings_save (wmp->prefs);
}


static void
wckmenu_settings_load (WckMenuPreferences *prefs)
{
    prefs->only_maximized = wck_conf_get_bool (prefs->conf, SETTING_ONLY_MAXIMIZED, DEFAULT_ONLY_MAXIMIZED);
    prefs->only_current_display = wck_conf_get_bool (prefs->conf, SETTING_ONLY_CURRDISPLAY, DEFAULT_ONLY_CURRDISPLAY);
    prefs->show_on_desktop = wck_conf_get_bool (prefs->conf, SETTING_SHOW_ON_DESKTOP, DEFAULT_SHOW_ON_DESKTOP);
    prefs->show_app_icon = wck_conf_get_bool (prefs->conf, SETTING_SHOW_APP_ICON, DEFAULT_SHOW_APP_ICON);
    prefs->inactive_alpha = wck_conf_get_int (prefs->conf, SETTING_INACTIVE_ALPHA, DEFAULT_INACTIVE_ALPHA);
    prefs->inactive_shade = wck_conf_get_int (prefs->conf, SETTING_INACTIVE_SHADE, DEFAULT_INACTIVE_SHADE);
}


static WckMenuPreferences *
wckmenu_read (XfcePanelPlugin *plugin)
{
    /* allocate memory for the preferences structure */
    WckMenuPreferences *prefs = g_slice_new0(WckMenuPreferences);

    prefs->conf = wck_conf_new (plugin);
    wckmenu_settings_load (prefs);

    return prefs;
}


void reset_symbol (WckMenuPlugin *wmp)
{
    if (wmp->icon->symbol)
    {
        gtk_widget_destroy (wmp->icon->symbol);
        wmp->icon->symbol = NULL;
    }

    if (wmp->prefs->show_app_icon)
        wmp->icon->symbol = gtk_image_new ();
    else
        wmp->icon->symbol = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_MENU);

    gtk_container_add (GTK_CONTAINER (wmp->icon->eventbox), wmp->icon->symbol);
    gtk_widget_show_all (GTK_WIDGET(wmp->icon->eventbox));
}


static WindowIcon *
window_icon_new (void)
{
    WindowIcon *icon = g_slice_new0 (WindowIcon);

    icon->eventbox = GTK_EVENT_BOX (gtk_event_box_new ());
    gtk_widget_set_can_focus (GTK_WIDGET (icon->eventbox), TRUE);
    gtk_event_box_set_visible_window (icon->eventbox, FALSE);

    icon->symbol = NULL;

    return icon;
}


static void
wckmenu_scale_factor (XfcePanelPlugin *plugin)
{
    gint scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (plugin));
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    /* TODO switch to wnck_handle_set_default_icon_size() and wnck_handle_set_default_mini_icon_size() */
    wnck_set_default_icon_size (WNCK_DEFAULT_ICON_SIZE * scale_factor);
    wnck_set_default_mini_icon_size (WNCK_DEFAULT_MINI_ICON_SIZE * scale_factor);
G_GNUC_END_IGNORE_DEPRECATIONS
}


static WckMenuPlugin *
wckmenu_new (XfcePanelPlugin *plugin)
{
    WckMenuPlugin *wmp;

    GtkOrientation orientation;

    /* allocate memory for the plugin structure */
    wmp = g_slice_new0 (WckMenuPlugin);

    /* pointer to plugin */
    wmp->plugin = plugin;

    /* read the user settings */
    wmp->prefs = wckmenu_read(wmp->plugin);

    /* get the current orientation */
    orientation = xfce_panel_plugin_get_orientation(plugin);

    /* not needed for shrink mode */
    xfce_panel_plugin_set_shrink (plugin, TRUE);

    /* create some panel widgets */
    wmp->ebox = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(wmp->ebox), FALSE);
    gtk_widget_set_name (wmp->ebox, "XfceWckMenuPlugin");

    wmp->box = gtk_box_new (orientation, 2);
    gtk_box_set_homogeneous (GTK_BOX (wmp->box), FALSE);
    gtk_widget_set_halign (wmp->box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (wmp->box, GTK_ALIGN_CENTER);

    gtk_widget_set_margin_top (wmp->box, ICON_PADDING);
    gtk_widget_set_margin_bottom (wmp->box, ICON_PADDING);
    gtk_widget_set_margin_start (wmp->box, ICON_PADDING);
    gtk_widget_set_margin_end (wmp->box, ICON_PADDING);

    /* some wckp widgets */
    wmp->icon = window_icon_new ();
    gtk_box_pack_start (GTK_BOX (wmp->box), GTK_WIDGET (wmp->icon->eventbox), FALSE, FALSE, 0);

    reset_symbol (wmp);

    gtk_container_add(GTK_CONTAINER(wmp->ebox), GTK_WIDGET(wmp->box));

    /* show widgets */
    gtk_widget_show(wmp->ebox);
    gtk_widget_show(wmp->box);

    /* adapt wnck default icon size when UI scale changes */
    wckmenu_scale_factor (plugin);
    g_signal_connect (plugin, "notify::scale-factor", G_CALLBACK (wckmenu_scale_factor), NULL);

    return wmp;
}


static void
wckmenu_free (XfcePanelPlugin *plugin, WckMenuPlugin *wmp)
{
    GtkWidget *dialog;

    disconnect_wnck (wmp->win);

    /* check if the dialog is still open. if so, destroy it */
    dialog = g_object_get_data(G_OBJECT (plugin), "dialog");
    if (G_UNLIKELY (dialog != NULL))
        gtk_widget_destroy(dialog);

    /* destroy the panel widgets */
    gtk_widget_destroy(wmp->box);

    /* free the plugin structure */
    g_slice_free(WindowIcon, wmp->icon);
    g_slice_free(WckUtils, wmp->win);
    g_slice_free(WckMenuPreferences, wmp->prefs);
    g_slice_free(WckMenuPlugin, wmp);
}


static void
wckmenu_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation, WckMenuPlugin *wmp)
{
    /* change the orienation of the box */
    gtk_orientable_set_orientation (GTK_ORIENTABLE (wmp->box), orientation);
}


static gboolean
wckmenu_size_changed (XfcePanelPlugin *plugin, gint size, WckMenuPlugin *wmp)
{
    GtkOrientation orientation;

    /* get the orientation of the plugin */
    orientation = xfce_panel_plugin_get_orientation(plugin);

    /* set the widget size */
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request(GTK_WIDGET (plugin), -1, size);
    else
        gtk_widget_set_size_request(GTK_WIDGET (plugin), size, -1);

    /* we handled the orientation */
    return TRUE;
}


static void on_refresh_item_activated (GtkMenuItem *refresh, WckMenuPlugin *wmp)
{
    init_icon_colors (wmp);
    reload_wnck_icon (wmp);
}


static void wckmenu_construct(XfcePanelPlugin *plugin)
{
    WckMenuPlugin *wmp;
    GtkWidget *refresh;

    /* setup transation domain */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    /* abort on non X11 environments */
    if (wck_abort_non_x11_windowing (plugin))
        return;

    /* create the plugin */
    wmp = wckmenu_new(plugin);

    /* add the ebox to the panel */
    gtk_container_add(GTK_CONTAINER (plugin), wmp->ebox);

    /* show the panel's right-click menu on this ebox */
    xfce_panel_plugin_add_action_widget(plugin, wmp->ebox);

    /* Set event handling (icon clicks) */
    g_signal_connect(G_OBJECT (wmp->icon->eventbox), "button-release-event", G_CALLBACK (on_icon_released), wmp);

    /* connect plugin signals */

    g_signal_connect(G_OBJECT (plugin), "free-data", G_CALLBACK (wckmenu_free), wmp);

    g_signal_connect(G_OBJECT (plugin), "save", G_CALLBACK (wckmenu_save), wmp);

    g_signal_connect(G_OBJECT (plugin), "size-changed", G_CALLBACK (wckmenu_size_changed), wmp);

    g_signal_connect(G_OBJECT (plugin), "orientation-changed", G_CALLBACK (wckmenu_orientation_changed), wmp);

    /* show the configure menu item and connect signal */
    xfce_panel_plugin_menu_show_configure(plugin);
    g_signal_connect(G_OBJECT (plugin), "configure-plugin", G_CALLBACK (wckmenu_configure), wmp);

    /* show the about menu item and connect signal */
    xfce_panel_plugin_menu_show_about(plugin);
    g_signal_connect (G_OBJECT (plugin), "about",
                    G_CALLBACK (wck_about), WCKMENU_ICON);

    /* add custom menu items */
    refresh = show_refresh_item (plugin);
    g_signal_connect (G_OBJECT (refresh), "activate", G_CALLBACK (on_refresh_item_activated), wmp);

    /* start tracking */
    wmp->win = g_slice_new0 (WckUtils);
    wmp->win->get_plugin = NULL;
    init_wnck (wmp->win, wmp->prefs->only_maximized, FALSE, wmp);

    /* start tracking icon color */
    init_icon_colors (wmp);
}


/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER(wckmenu_construct);
