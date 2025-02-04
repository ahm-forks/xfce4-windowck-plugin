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

#include "wcktitle.h"
#include "wcktitle-dialogs.h"
#include "wcktitle-title.h"

#define SETTING_FULL_NAME           "/full-name"
#define SETTING_TWO_LINES           "/two-lines"
#define SETTING_SHOW_TOOLTIPS       "/show-tooltips"
#define SETTING_SIZE_MODE           "/size-mode"
#define SETTING_TITLE_SIZE          "/title-size"
#define SETTING_TITLE_ALIGNMENT     "/title-alignment"
#define SETTING_TITLE_PADDING       "/title-padding"
#define SETTING_SYNC_WM_FONT        "/sync-wm-font"
#define SETTING_TITLE_FONT          "/title-font"
#define SETTING_SUBTITLE_FONT       "/subtitle-font"
#define SETTING_INACTIVE_TEXT_ALPHA "/inactive-text-alpha"
#define SETTING_INACTIVE_TEXT_SHADE "/inactive-text-shade"

/* default settings */
#define DEFAULT_FULL_NAME TRUE
#define DEFAULT_TWO_LINES FALSE
#define DEFAULT_SHOW_TOOLTIPS TRUE
#define DEFAULT_SIZE_MODE FIXED
#define DEFAULT_TITLE_SIZE 80
#define DEFAULT_TITLE_ALIGNMENT CENTER
#define DEFAULT_TITLE_PADDING 3
#define DEFAULT_SYNC_WM_FONT TRUE
#define DEFAULT_TITLE_FONT "sans 10"
#define DEFAULT_SUBTITLE_FONT "sans 10"
#define DEFAULT_INACTIVE_TEXT_ALPHA 60
#define DEFAULT_INACTIVE_TEXT_SHADE 110

void
wcktitle_settings_save (WckTitlePreferences *prefs)
{
    wck_conf_set_bool (prefs->conf, SETTING_ONLY_MAXIMIZED, prefs->only_maximized);
    wck_conf_set_bool (prefs->conf, SETTING_SHOW_ON_DESKTOP, prefs->show_on_desktop);
    wck_conf_set_bool (prefs->conf, SETTING_FULL_NAME, prefs->full_name);
    wck_conf_set_bool (prefs->conf, SETTING_TWO_LINES, prefs->two_lines);
    wck_conf_set_bool (prefs->conf, SETTING_SHOW_TOOLTIPS, prefs->show_tooltips);
    wck_conf_set_int (prefs->conf, SETTING_SIZE_MODE, prefs->size_mode);
    wck_conf_set_int (prefs->conf, SETTING_TITLE_SIZE, prefs->title_size);
    wck_conf_set_bool (prefs->conf, SETTING_SYNC_WM_FONT, prefs->sync_wm_font);
    if (prefs->title_font)
        wck_conf_set_string (prefs->conf, SETTING_TITLE_FONT, prefs->title_font);

    if (prefs->subtitle_font)
        wck_conf_set_string (prefs->conf, SETTING_SUBTITLE_FONT, prefs->subtitle_font);

    wck_conf_set_int (prefs->conf, SETTING_TITLE_ALIGNMENT, prefs->title_alignment);
    wck_conf_set_int (prefs->conf, SETTING_TITLE_PADDING, prefs->title_padding);
    wck_conf_set_int (prefs->conf, SETTING_INACTIVE_TEXT_ALPHA, prefs->inactive_text_alpha);
    wck_conf_set_int (prefs->conf, SETTING_INACTIVE_TEXT_SHADE, prefs->inactive_text_shade);
}

static void
wcktitle_save (G_GNUC_UNUSED XfcePanelPlugin *plugin, WckTitlePlugin *wtp)
{
    wcktitle_settings_save (wtp->prefs);
}


static void
wcktitle_settings_load (WckTitlePreferences *prefs)
{
    prefs->only_maximized = wck_conf_get_bool (prefs->conf, SETTING_ONLY_MAXIMIZED, DEFAULT_ONLY_MAXIMIZED);
    prefs->show_on_desktop = wck_conf_get_bool (prefs->conf, SETTING_SHOW_ON_DESKTOP, DEFAULT_SHOW_ON_DESKTOP);
    prefs->full_name = wck_conf_get_bool (prefs->conf, SETTING_FULL_NAME, DEFAULT_FULL_NAME);
    prefs->two_lines = wck_conf_get_bool (prefs->conf, SETTING_TWO_LINES, DEFAULT_TWO_LINES);
    prefs->show_tooltips = wck_conf_get_bool (prefs->conf, SETTING_SHOW_TOOLTIPS, DEFAULT_SHOW_TOOLTIPS);
    prefs->size_mode = wck_conf_get_int (prefs->conf, SETTING_SIZE_MODE, DEFAULT_SIZE_MODE);
    prefs->title_size = wck_conf_get_int (prefs->conf, SETTING_TITLE_SIZE, DEFAULT_TITLE_SIZE);
    prefs->sync_wm_font = wck_conf_get_bool (prefs->conf, SETTING_SYNC_WM_FONT, DEFAULT_SYNC_WM_FONT);
    prefs->title_font = wck_conf_get_string (prefs->conf, SETTING_TITLE_FONT, DEFAULT_TITLE_FONT);
    prefs->subtitle_font = wck_conf_get_string (prefs->conf, SETTING_SUBTITLE_FONT, DEFAULT_SUBTITLE_FONT);
    prefs->title_alignment = wck_conf_get_int (prefs->conf, SETTING_TITLE_ALIGNMENT, DEFAULT_TITLE_ALIGNMENT);
    prefs->title_padding = wck_conf_get_int (prefs->conf, SETTING_TITLE_PADDING, DEFAULT_TITLE_PADDING);
    prefs->inactive_text_alpha = wck_conf_get_int (prefs->conf, SETTING_INACTIVE_TEXT_ALPHA, DEFAULT_INACTIVE_TEXT_ALPHA);
    prefs->inactive_text_shade = wck_conf_get_int (prefs->conf, SETTING_INACTIVE_TEXT_SHADE, DEFAULT_INACTIVE_TEXT_SHADE);
}


static WckTitlePreferences *
wcktitle_read (XfcePanelPlugin *plugin)
{
    /* allocate memory for the preferences structure */
    WckTitlePreferences *prefs = g_slice_new0 (WckTitlePreferences);

    prefs->conf = wck_conf_new (plugin);
    wcktitle_settings_load (prefs);

    return prefs;
}


static WckTitlePlugin *
wcktitle_new (XfcePanelPlugin *plugin)
{
    WckTitlePlugin *wtp;

    GtkOrientation orientation;

    /* allocate memory for the plugin structure */
    wtp = g_slice_new0 (WckTitlePlugin);

    /* pointer to plugin */
    wtp->plugin = plugin;

    /* read the user settings */
    wtp->prefs = wcktitle_read (wtp->plugin);

    /* get the current orientation */
    orientation = xfce_panel_plugin_get_orientation(plugin);

    /* not needed for shrink mode */
    if (wtp->prefs->size_mode != SHRINK)
        xfce_panel_plugin_set_shrink (plugin, TRUE);

    /* create some panel widgets */
    wtp->ebox = gtk_event_box_new ();
    gtk_event_box_set_visible_window (GTK_EVENT_BOX (wtp->ebox), FALSE);
    gtk_widget_set_name (wtp->ebox, "XfceWckTitlePlugin");

    wtp->box = gtk_box_new (orientation, 2);
    gtk_box_set_homogeneous (GTK_BOX (wtp->box), FALSE);
    gtk_widget_set_halign (wtp->box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (wtp->box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top (wtp->box, DEFAULT_TITLE_PADDING);
    gtk_widget_set_margin_bottom (wtp->box, DEFAULT_TITLE_PADDING);

    /* some WckTitlePlugin widgets */
    wtp->title = GTK_LABEL (gtk_label_new (""));
    gtk_box_pack_start (GTK_BOX (wtp->box), GTK_WIDGET (wtp->title), TRUE, TRUE, 0);

    gtk_container_add (GTK_CONTAINER (wtp->ebox), GTK_WIDGET (wtp->box));

    /* show widgets */
    gtk_widget_show (wtp->ebox);
    gtk_widget_show (wtp->box);
    gtk_widget_show (GTK_WIDGET (wtp->title));

    return wtp;
}


static void
wcktitle_free (XfcePanelPlugin *plugin, WckTitlePlugin *wtp)
{
    GtkWidget *dialog;

    disconnect_wnck (wtp->win);

    /* check if the dialog is still open. if so, destroy it */
    dialog = g_object_get_data(G_OBJECT (plugin), "dialog");
    if (G_UNLIKELY (dialog != NULL))
        gtk_widget_destroy(dialog);

    /* destroy the panel widgets */
    gtk_widget_destroy (wtp->box);

    /* free the plugin structure */
    g_slice_free (WckUtils, wtp->win);
    g_slice_free (WckTitlePreferences, wtp->prefs);
    g_slice_free (WckTitlePlugin, wtp);
}


static void
wcktitle_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation, WckTitlePlugin *wtp)
{
    /* change the orienation of the box */
    gtk_orientable_set_orientation (GTK_ORIENTABLE (wtp->box), orientation);
}


static void
wcktitle_screen_position_changed (XfcePanelPlugin *plugin, XfceScreenPosition *position, WckTitlePlugin *wtp)
{
    if (wtp->prefs->size_mode != SHRINK)
    {
        xfce_panel_plugin_set_shrink (plugin, FALSE);
        gtk_label_set_width_chars (wtp->title, 1);
        xfce_panel_plugin_set_shrink (plugin, TRUE);
        resize_title (wtp);
    }
}


static gboolean
wcktitle_size_changed (XfcePanelPlugin *plugin, gint size, WckTitlePlugin *wtp)
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


static void
on_refresh_item_activated (GtkMenuItem *refresh, WckTitlePlugin *wtp)
{
    init_title (wtp);
    reload_wnck_title (wtp);
}


static void
wcktitle_construct (XfcePanelPlugin *plugin)
{
    WckTitlePlugin *wtp;
    GtkWidget *refresh;

    /* setup transation domain */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    /* abort on non X11 environments */
    if (wck_abort_non_x11_windowing (plugin))
        return;

    /* create the plugin */
    wtp = wcktitle_new (plugin);

    /* add the ebox to the panel */
    gtk_container_add (GTK_CONTAINER (plugin), wtp->ebox);

    /* show the panel's right-click menu on this ebox */
    xfce_panel_plugin_add_action_widget (plugin, wtp->ebox);

    /* Set event handling (title clicks) */
    g_signal_connect (G_OBJECT (wtp->ebox), "button-press-event",
                      G_CALLBACK (on_title_pressed), wtp);

    g_signal_connect (G_OBJECT (wtp->ebox), "button-release-event",
                      G_CALLBACK (on_title_released), wtp);

    /* connect plugin signals */
    g_signal_connect (G_OBJECT (plugin), "free-data",
                      G_CALLBACK (wcktitle_free), wtp);

    g_signal_connect (G_OBJECT (plugin), "save",
                      G_CALLBACK (wcktitle_save), wtp);

    g_signal_connect (G_OBJECT (plugin), "size-changed",
                      G_CALLBACK (wcktitle_size_changed), wtp);

    g_signal_connect (G_OBJECT (plugin), "screen-position-changed",
                      G_CALLBACK (wcktitle_screen_position_changed), wtp);

    g_signal_connect (G_OBJECT (plugin), "orientation-changed",
                      G_CALLBACK (wcktitle_orientation_changed), wtp);

    /* show the configure menu item and connect signal */
    xfce_panel_plugin_menu_show_configure(plugin);
    g_signal_connect (G_OBJECT (plugin), "configure-plugin",
                      G_CALLBACK (wcktitle_configure), wtp);

    /* show the about menu item and connect signal */
    xfce_panel_plugin_menu_show_about(plugin);
    g_signal_connect (G_OBJECT (plugin), "about",
                      G_CALLBACK (wck_about), WCKTITLE_ICON);

    /* add custom menu items */
    refresh = show_refresh_item (plugin);
    g_signal_connect (G_OBJECT (refresh), "activate",
                      G_CALLBACK (on_refresh_item_activated), wtp);

    /* start tracking title text */
    wtp->win = g_slice_new0 (WckUtils);
    wtp->win->get_plugin = NULL;
    init_wnck (wtp->win, wtp->prefs->only_maximized, FALSE, wtp);

    /* start tracking title size */
    init_title (wtp);
}


/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER (wcktitle_construct);
