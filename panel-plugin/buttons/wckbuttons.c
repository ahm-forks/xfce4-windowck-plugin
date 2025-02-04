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
 *  Copyright (C) 2013 Cedric Leporcq  <cedl38@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include <common/wck-plugin.h>

#include "wckbuttons.h"
#include "wckbuttons-dialogs.h"
#include "wckbuttons-theme.h"

#define SETTING_SYNC_WM_THEME "/sync-wm-theme"
#define SETTING_BUTTON_LAYOUT "/button-layout"
#define SETTING_THEME         "/theme"

/* default settings */
#define DEFAULT_SYNC_WM_THEME TRUE
#define DEFAULT_BUTTON_LAYOUT "HMC"

#define BUTTONS_SIGNALS_CONNECT(name, id) \
        g_signal_connect (G_OBJECT (wbp->button[id]->eventbox), "button-press-event", G_CALLBACK (name##_button_pressed), wbp); \
        g_signal_connect (G_OBJECT (wbp->button[id]->eventbox), "button-release-event", G_CALLBACK (name##_button_release), wbp); \
        g_signal_connect (G_OBJECT (wbp->button[id]->eventbox), "enter-notify-event", G_CALLBACK (name##_button_hover_enter), wbp); \
        g_signal_connect (G_OBJECT (wbp->button[id]->eventbox), "leave-notify-event", G_CALLBACK (name##_button_hover_leave), wbp);


/* prototypes */
static void
wckbuttons_construct (XfcePanelPlugin *plugin);


void
wckbuttons_settings_save (WckButtonsPreferences *prefs)
{
    wck_conf_set_bool (prefs->conf, SETTING_ONLY_MAXIMIZED, prefs->only_maximized);
    wck_conf_set_bool (prefs->conf, SETTING_SHOW_ON_DESKTOP, prefs->show_on_desktop);
    wck_conf_set_bool (prefs->conf, SETTING_ONLY_CURRDISPLAY, prefs->only_current_display);
    wck_conf_set_bool (prefs->conf, SETTING_SYNC_WM_THEME, prefs->sync_wm_theme);
    if (prefs->button_layout)
        wck_conf_set_string (prefs->conf, SETTING_BUTTON_LAYOUT, prefs->button_layout);

    if (prefs->theme)
        wck_conf_set_string (prefs->conf, SETTING_THEME, prefs->theme);
}

static void
wckbuttons_save (G_GNUC_UNUSED XfcePanelPlugin *plugin, WckButtonsPlugin *wbp)
{
    wckbuttons_settings_save (wbp->prefs);
}


static void
wckbuttons_settings_load (WckButtonsPreferences *prefs)
{
    gchar *button_layout;

    prefs->only_maximized = wck_conf_get_bool (prefs->conf, SETTING_ONLY_MAXIMIZED, DEFAULT_ONLY_MAXIMIZED);
    prefs->show_on_desktop = wck_conf_get_bool (prefs->conf, SETTING_SHOW_ON_DESKTOP, DEFAULT_SHOW_ON_DESKTOP);
    prefs->only_current_display = wck_conf_get_bool (prefs->conf, SETTING_ONLY_CURRDISPLAY, DEFAULT_ONLY_CURRDISPLAY);
    prefs->sync_wm_theme = wck_conf_get_bool (prefs->conf, SETTING_SYNC_WM_THEME, DEFAULT_SYNC_WM_THEME);

    button_layout = wck_conf_get_string (prefs->conf, SETTING_BUTTON_LAYOUT, DEFAULT_BUTTON_LAYOUT);
    prefs->button_layout = button_layout_filter (button_layout, DEFAULT_BUTTON_LAYOUT);
    g_free (button_layout);

    prefs->theme = wck_conf_get_string (prefs->conf, SETTING_THEME, DEFAULT_THEME);
}


static WckButtonsPreferences *
wckbuttons_read (XfcePanelPlugin *plugin)
{
    /* allocate memory for the preferences structure */
    WckButtonsPreferences *prefs = g_slice_new0(WckButtonsPreferences);

    prefs->conf = wck_conf_new (plugin);
    wckbuttons_settings_load (prefs);

    return prefs;
}

static WindowButton *
window_button_new (WckButtonsPlugin *wbp)
{
    WindowButton *button = g_new0 (WindowButton, 1);

    button->eventbox = GTK_EVENT_BOX (gtk_event_box_new ());
    button->image = GTK_IMAGE (gtk_image_new ());

    gtk_widget_set_can_focus (GTK_WIDGET (button->eventbox), TRUE);

    gtk_container_add (GTK_CONTAINER (button->eventbox), GTK_WIDGET (button->image));
    gtk_event_box_set_visible_window (button->eventbox, FALSE);
    gtk_box_pack_start (GTK_BOX (wbp->box), GTK_WIDGET (button->eventbox), TRUE, TRUE, 0);

    /* Add hover events to eventboxes */
    gtk_widget_add_events (GTK_WIDGET (button->eventbox), GDK_ENTER_NOTIFY_MASK); //add the "enter" signal
    gtk_widget_add_events (GTK_WIDGET (button->eventbox), GDK_LEAVE_NOTIFY_MASK); //add the "leave" signal

    return button;
}

static GtkOrientation get_orientation (XfcePanelPlugin *plugin)
{
    XfcePanelPluginMode mode;
    mode = xfce_panel_plugin_get_mode (plugin);
    if  (mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
    return GTK_ORIENTATION_HORIZONTAL;
    else
    return xfce_panel_plugin_get_orientation (plugin);
}

static WckButtonsPlugin *
wckbuttons_new (XfcePanelPlugin *plugin)
{
    WckButtonsPlugin   *wbp;
    GtkOrientation  orientation;

    /* allocate memory for the plugin structure */
    wbp = g_slice_new0 (WckButtonsPlugin);

    /* pointer to plugin */
    wbp->plugin = plugin;

    /* read the user settings */
    wbp->prefs = wckbuttons_read (wbp->plugin);

    /* get the current orientation */
    orientation = get_orientation (plugin);

    /* create some panel widgets */
    wbp->ebox = gtk_event_box_new ();
    gtk_event_box_set_visible_window (GTK_EVENT_BOX(wbp->ebox), FALSE);
    gtk_widget_set_name (wbp->ebox, "XfceWckButtonsPlugin");

    wbp->box = gtk_box_new (orientation, 2);
    gtk_box_set_homogeneous (GTK_BOX (wbp->box), FALSE);

    /* create buttons */
    for (gint i = 0; i < BUTTONS; i++)
    {
        wbp->button[i] = window_button_new (wbp);
    }

    gtk_widget_show (wbp->ebox);
    gtk_widget_show (wbp->box);
    gtk_container_add (GTK_CONTAINER (wbp->ebox), wbp->box);

    return wbp;
}


static void
wckbuttons_free (XfcePanelPlugin *plugin, WckButtonsPlugin *wbp)
{
    GtkWidget *dialog;

    disconnect_wnck (wbp->win);

    /* check if the dialog is still open. if so, destroy it */
    dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
    if (G_UNLIKELY (dialog != NULL))
        gtk_widget_destroy (dialog);

    /* destroy the panel widgets */
    gtk_widget_destroy (wbp->box);

    /* cleanup the settings */
    if (G_LIKELY (wbp->prefs->button_layout != NULL))
        g_free (wbp->prefs->button_layout);

    /* free the plugin structure */
    g_slice_free (WckUtils, wbp->win);
    g_slice_free (WckButtonsPreferences, wbp->prefs);
    g_slice_free (WckButtonsPlugin, wbp);
}


static void
wckbuttons_orientation_changed (XfcePanelPlugin  *plugin,
                                GtkOrientation    orientation,
                                WckButtonsPlugin *wbp)
{
    /* change the orienation of the box */
    gtk_orientable_set_orientation (GTK_ORIENTABLE (wbp->box), get_orientation (wbp->plugin));
}


static gboolean
wckbuttons_size_changed (XfcePanelPlugin  *plugin,
                         gint              size,
                         WckButtonsPlugin *wbp)
{
    XfcePanelPluginMode mode;
    GtkOrientation orientation;

    /* get the orientation of the plugin */
    orientation = xfce_panel_plugin_get_orientation (plugin);
    mode = xfce_panel_plugin_get_mode (plugin);

    /* set the widget size */
    if (mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, -1);
    else if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
    else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

    /* we handled the orientation */
    return TRUE;
}

static WBImageButton
get_maximize_button_image (WckButtonsPlugin *wbp)
{
    return (wbp->win->controlwindow && wnck_window_is_maximized (wbp->win->controlwindow)) ? IMAGE_UNMAXIMIZE : IMAGE_MAXIMIZE;
}

void
on_wck_state_changed (WnckWindow *controlwindow, gpointer data)
{
    WckButtonsPlugin *wbp = data;
    WBImageButton image_button = get_maximize_button_image (wbp);
    WBImageState image_state;

    if (controlwindow && wnck_window_is_active (controlwindow))
        image_state = IMAGE_FOCUSED;
    else
        image_state = IMAGE_UNFOCUSED;

    /* update buttons images */
    gtk_image_set_from_pixbuf (wbp->button[MINIMIZE_BUTTON]->image, wbp->pixbufs[IMAGE_MINIMIZE][image_state]);
    gtk_image_set_from_pixbuf (wbp->button[MAXIMIZE_BUTTON]->image, wbp->pixbufs[image_button][image_state]);
    gtk_image_set_from_pixbuf (wbp->button[CLOSE_BUTTON]->image, wbp->pixbufs[IMAGE_CLOSE][image_state]);
}

void
on_control_window_changed (WnckWindow *controlwindow, WnckWindow *previous, gpointer data)
{
    WckButtonsPlugin *wbp = data;

    if (!controlwindow
        || (window_is_desktop (controlwindow)
            && !wbp->prefs->show_on_desktop))
    {
        if (gtk_widget_get_visible (GTK_WIDGET (wbp->box)))
            gtk_widget_hide (GTK_WIDGET (wbp->box));
    }
    else
    {
        const gboolean is_desktop = window_is_desktop (controlwindow);

        gtk_widget_set_sensitive (GTK_WIDGET (wbp->button[MINIMIZE_BUTTON]->eventbox), !is_desktop);
        gtk_widget_set_sensitive (GTK_WIDGET (wbp->button[MAXIMIZE_BUTTON]->eventbox), !is_desktop);
        gtk_widget_set_sensitive (GTK_WIDGET (wbp->button[CLOSE_BUTTON]->eventbox), TRUE);

        on_wck_state_changed (controlwindow, wbp);

        if (!gtk_widget_get_visible(GTK_WIDGET(wbp->box)))
            gtk_widget_show_all(GTK_WIDGET(wbp->box));
    }
}


/* Called when we click on a button */
static gboolean
on_button_pressed (GdkEventButton *event, WckButtonsPlugin *wbp, WBButton button, WBImageButton image_button)
{
    if (event->button != 1)
        return FALSE;

    gtk_image_set_from_pixbuf (wbp->button[button]->image, wbp->pixbufs[image_button][IMAGE_PRESSED]);

    return TRUE;
}


/* Makes the button 'glow' when the mouse enters it */
static gboolean
on_button_hover_enter (WckButtonsPlugin *wbp, WBButton button, WBImageButton image_button)
{
    gtk_image_set_from_pixbuf (wbp->button[button]->image, wbp->pixbufs[image_button][IMAGE_PRELIGHT]);

    return TRUE;
}


/* Makes the button stop 'glowing' when the mouse leaves it */
static gboolean
on_button_hover_leave (WckButtonsPlugin *wbp, WBButton button, WBImageButton image_button)
{
    WBImageState image_state = wnck_window_is_active (wbp->win->controlwindow) ? IMAGE_FOCUSED : IMAGE_UNFOCUSED;

    gtk_image_set_from_pixbuf (wbp->button[button]->image, wbp->pixbufs[image_button][image_state]);

    return TRUE;
}


/* Called when we release the click on a button */
static gboolean
on_minimize_button_release (GtkWidget        *event_box,
                            GdkEventButton   *event,
                            WckButtonsPlugin *wbp)
{
    if (event->button != 1) return FALSE;

    wnck_window_minimize (wbp->win->controlwindow);

    return TRUE;
}


/* Called when we click on a button */
static gboolean
on_minimize_button_pressed (GtkWidget        *event_box,
                            GdkEventButton   *event,
                            WckButtonsPlugin *wbp)
{
    return on_button_pressed (event, wbp, MINIMIZE_BUTTON, IMAGE_MINIMIZE);
}


/* Makes the button stop 'glowing' when the mouse leaves it */
static gboolean
on_minimize_button_hover_leave (GtkWidget        *widget,
                                GdkEventCrossing *event,
                                WckButtonsPlugin *wbp)
{
    return on_button_hover_leave (wbp, MINIMIZE_BUTTON, IMAGE_MINIMIZE);
}


/* Makes the button 'glow' when the mouse enters it */
static gboolean
on_minimize_button_hover_enter (GtkWidget        *widget,
                                GdkEventCrossing *event,
                                WckButtonsPlugin *wbp)
{
    return on_button_hover_enter (wbp, MINIMIZE_BUTTON, IMAGE_MINIMIZE);
}


/* Called when we release the click on a button */
static gboolean
on_maximize_button_release (GtkWidget        *event_box,
                            GdkEventButton   *event,
                            WckButtonsPlugin *wbp)
{
    if (event->button != 1) return FALSE;

    toggle_maximize (wbp->win->controlwindow);

    return TRUE;
}


/* Called when we click on a button */
static gboolean
on_maximize_button_pressed (GtkWidget        *event_box,
                            GdkEventButton   *event,
                            WckButtonsPlugin *wbp)
{
    WBImageButton image_button = get_maximize_button_image (wbp);

    return on_button_pressed (event, wbp, MAXIMIZE_BUTTON, image_button);
}


/* Makes the button stop 'glowing' when the mouse leaves it */
static gboolean
on_maximize_button_hover_leave (GtkWidget        *widget,
                                GdkEventCrossing *event,
                                WckButtonsPlugin *wbp)
{
    WBImageButton image_button = get_maximize_button_image (wbp);

    return on_button_hover_leave (wbp, MAXIMIZE_BUTTON, image_button);
}


/* Makes the button 'glow' when the mouse enters it */
static gboolean
on_maximize_button_hover_enter (GtkWidget        *widget,
                                GdkEventCrossing *event,
                                WckButtonsPlugin *wbp)
{
    WBImageButton image_button = get_maximize_button_image (wbp);

    return on_button_hover_enter (wbp, MAXIMIZE_BUTTON, image_button);
}


/* Called when we release the click on a button */
static gboolean
on_close_button_release (GtkWidget        *event_box,
                         GdkEventButton   *event,
                         WckButtonsPlugin *wbp)
{
    if (event->button != 1) return FALSE;

    wnck_window_close(wbp->win->controlwindow, GDK_CURRENT_TIME);

    return TRUE;
}


/* Called when we click on a button */
static gboolean
on_close_button_pressed (GtkWidget        *event_box,
                         GdkEventButton   *event,
                         WckButtonsPlugin *wbp)
{
    return on_button_pressed (event, wbp, CLOSE_BUTTON, IMAGE_CLOSE);
}


/* Makes the button stop 'glowing' when the mouse leaves it */
static gboolean
on_close_button_hover_leave (GtkWidget        *widget,
                             GdkEventCrossing *event,
                             WckButtonsPlugin *wbp)
{
    return on_button_hover_leave (wbp, CLOSE_BUTTON, IMAGE_CLOSE);
}


/* Makes the button 'glow' when the mouse enters it */
static gboolean
on_close_button_hover_enter (GtkWidget        *widget,
                             GdkEventCrossing *event,
                             WckButtonsPlugin *wbp)
{
    return on_button_hover_enter (wbp, CLOSE_BUTTON, IMAGE_CLOSE);
}


static void
on_refresh_item_activated (GtkMenuItem *refresh, WckButtonsPlugin *wbp)
{
    wbp->prefs = wckbuttons_read (wbp->plugin);
    init_theme (wbp);
    reload_wnck (wbp->win, wbp->prefs->only_maximized, wbp->prefs->only_current_display, wbp);
}

static XfcePanelPlugin* wckbuttons_get_plugin(gpointer wtp) {
    return ((WckButtonsPlugin *) wtp)->plugin;
}

static void
wckbuttons_construct (XfcePanelPlugin *plugin)
{
    WckButtonsPlugin *wbp;
    GtkWidget *refresh;

    /* setup transation domain */
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    /* abort on non X11 environments */
    if (wck_abort_non_x11_windowing (plugin))
        return;

    /* create the plugin */
    wbp = wckbuttons_new (plugin);

    /* add the ebox to the panel */
    gtk_container_add (GTK_CONTAINER (plugin), wbp->ebox);

    /* show the panel's right-click menu on this ebox */
    xfce_panel_plugin_add_action_widget (plugin, wbp->ebox);

    /* connect plugin signals */
    g_signal_connect (G_OBJECT (plugin), "free-data",
                      G_CALLBACK (wckbuttons_free), wbp);

    g_signal_connect (G_OBJECT (plugin), "save",
                      G_CALLBACK (wckbuttons_save), wbp);

    g_signal_connect (G_OBJECT (plugin), "size-changed",
                      G_CALLBACK (wckbuttons_size_changed), wbp);

    g_signal_connect (G_OBJECT (plugin), "orientation-changed",
                      G_CALLBACK (wckbuttons_orientation_changed), wbp);

    /* show the configure menu item and connect signal */
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (G_OBJECT (plugin), "configure-plugin",
                      G_CALLBACK (wckbuttons_configure), wbp);

    /* show the about menu item and connect signal */
    xfce_panel_plugin_menu_show_about (plugin);
    g_signal_connect (G_OBJECT (plugin), "about",
                      G_CALLBACK (wck_about), WCKBUTTONS_ICON);


    /* add custom menu items */
    refresh = show_refresh_item (plugin);
    g_signal_connect (G_OBJECT (refresh), "activate",
                      G_CALLBACK (on_refresh_item_activated), wbp);

    /* start tracking windows */
    wbp->win = g_slice_new0 (WckUtils);
    wbp->win->get_plugin = wckbuttons_get_plugin;
    init_wnck (wbp->win, wbp->prefs->only_maximized, wbp->prefs->only_current_display, wbp);

    /* get theme */
    init_theme (wbp);

    /* start tracking buttons events*/
    BUTTONS_SIGNALS_CONNECT (on_minimize, MINIMIZE_BUTTON);
    BUTTONS_SIGNALS_CONNECT (on_maximize, MAXIMIZE_BUTTON);
    BUTTONS_SIGNALS_CONNECT (on_close, CLOSE_BUTTON);
}


/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER (wckbuttons_construct);
