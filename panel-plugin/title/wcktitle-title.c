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

#include <libxfce4panel/libxfce4panel.h>

#include <common/ui_style.h>
#include <common/wck-plugin.h>
#include <common/wck-utils.h>

#include "wcktitle.h"
#include "wcktitle-title.h"

void reload_wnck_title (WckTitlePlugin *wtp)
{
    /* disconnect controlled window name signal handler */
    wck_signal_handler_disconnect (G_OBJECT (wtp->win->controlwindow), wtp->cnh);

    reload_wnck (wtp->win, wtp->prefs->only_maximized, wtp->prefs->only_current_display, wtp);
}


static gboolean is_window_on_active_workspace_and_no_other_maximized_windows_above(WnckWindow *window)
{
    WnckWorkspace *workspace;
    WnckScreen *screen;
    GList *windows;
    GList *bottom_window;

    if (window_is_desktop(window)) {
        return TRUE;
    }

    workspace = wnck_window_get_workspace(window);
    screen = wnck_workspace_get_screen(workspace);

    if (wnck_screen_get_active_workspace(screen) != workspace) {
        return FALSE;
    }

    windows = wnck_screen_get_windows_stacked(screen);
    if (!windows) {
        return TRUE;
    }

    bottom_window = g_list_first(windows);
    for (GList *top_window = g_list_last(windows);
         top_window->data != window && top_window != bottom_window;
         top_window = top_window->prev) {
        if (wnck_window_is_maximized((WnckWindow *)top_window->data)) {
            return FALSE;
        }
    }

    return TRUE;
}


static gchar *
get_title_color (WnckWindow *controlwindow, WckTitlePlugin *wtp)
{
    if (wnck_window_is_active (controlwindow))
    {
        /* window focused */
        /*~ gtk_widget_set_sensitive(GTK_WIDGET(wckp->title), TRUE); */
        return wtp->prefs->active_text_color;
    }

    if (is_window_on_active_workspace_and_no_other_maximized_windows_above (controlwindow))
    {
        /* window unfocused */
        /*~ gtk_widget_set_sensitive(GTK_WIDGET(wckp->title), FALSE); */
        return wtp->prefs->inactive_text_color;
    }

    return NULL;
}


/* Triggers when controlwindow's name changes */
/* Warning! This function is called very often, so it should only do the most necessary things! */
static void
on_name_changed (WnckWindow *controlwindow, WckTitlePlugin *wtp)
{
    if (controlwindow
        && wnck_window_get_pid(controlwindow)  /* if active window has been closed, pid is 0 */
        && (!window_is_desktop (controlwindow)
            || wtp->prefs->show_on_desktop))
    {
        const gchar *title_color;
        const gchar *title_text;
        gchar *title_markup;

        title_color = get_title_color (controlwindow, wtp);
        if (!title_color)
        {
            return;
        }

        title_text = wnck_window_get_name (controlwindow);

        /* Set tooltips */
        if (wtp->prefs->show_tooltips)
        {
            gtk_widget_set_tooltip_text (GTK_WIDGET (wtp->title), title_text);
        }

        /* get application and instance names */
        if (wtp->prefs->full_name && !wtp->prefs->two_lines)
        {
            title_markup = g_markup_printf_escaped ("<span font=\"%s\" color=\"%s\">%s</span>", wtp->prefs->title_font, title_color, title_text);
        }
        else {
            /* split title text */
            gchar **part = g_strsplit (title_text, " - ", 0);
            const gint n = g_strv_length (part);
            gchar *title = g_strdup (part[n - 1]);

            if (n > 1 && wtp->prefs->two_lines)
            {
                gchar *subtitle;

                if (wtp->prefs->full_name)
                {
                    g_free (part[n - 1]);
                    part[n - 1] = NULL;
                    subtitle = g_strjoinv (" - ", part);
                }
                else
                {
                    subtitle = g_strdup (part[0]);
                }

                title_markup = g_markup_printf_escaped ("<span font=\"%s\" color=\"%s\">%s</span><span font=\"%s\" color=\"%s\">\n%s</span>",
                                                        wtp->prefs->title_font, title_color, title, wtp->prefs->subtitle_font, title_color, subtitle);
                g_free (subtitle);
            }
            else
            {
                title_markup = g_markup_printf_escaped ("<span font=\"%s\" color=\"%s\">%s</span>", wtp->prefs->title_font, title_color, title);
            }

            g_free (title);
            g_strfreev (part);
        }

        gtk_label_set_markup(wtp->title, title_markup);

        if (wtp->prefs->title_alignment == LEFT)
        {
            gtk_label_set_justify (wtp->title, GTK_JUSTIFY_LEFT);
        }
        else if (wtp->prefs->title_alignment == CENTER)
        {
            gtk_label_set_justify (wtp->title, GTK_JUSTIFY_CENTER);
        }
        else if (wtp->prefs->title_alignment == RIGHT)
        {
            gtk_label_set_justify (wtp->title, GTK_JUSTIFY_RIGHT);
        }

        g_free (title_markup);
    }
    else
    {
        /* hide text */
        gtk_label_set_text (wtp->title, "");
    }
}


void on_wck_state_changed (WnckWindow *controlwindow, gpointer data)
{
    WckTitlePlugin *wtp = data;

    on_name_changed (controlwindow, wtp);
}


void on_control_window_changed (WnckWindow *controlwindow, WnckWindow *previous, gpointer data)
{
    WckTitlePlugin *wtp = data;

    /* disconect previous window title signal */
    wck_signal_handler_disconnect (G_OBJECT(previous), wtp->cnh);

    on_wck_state_changed (controlwindow, wtp);

    if (!controlwindow
        || (window_is_desktop (controlwindow)
            && !wtp->prefs->show_on_desktop))
    {
        if (gtk_widget_get_visible (GTK_WIDGET (wtp->box)))
            gtk_widget_hide (GTK_WIDGET (wtp->box));
    }
    else
    {
        if (!gtk_widget_get_visible (GTK_WIDGET (wtp->box)))
            gtk_widget_show_all (GTK_WIDGET (wtp->box));
    }

    if (controlwindow && !window_is_desktop (controlwindow))
    {
        wtp->cnh = g_signal_connect (G_OBJECT (controlwindow), "name-changed",
                                     G_CALLBACK (on_name_changed), wtp);
    }
}


void resize_title (WckTitlePlugin *wtp)
{
    switch (wtp->prefs->size_mode)
    {
        case SHRINK:
            gtk_label_set_max_width_chars (wtp->title, wtp->prefs->title_size);
            break;
        case EXPAND:
            gtk_label_set_max_width_chars (wtp->title, TITLE_SIZE_MAX);
            break;
        default:
            gtk_label_set_width_chars (wtp->title, wtp->prefs->title_size);
            break;
    }
}


void set_title_padding (WckTitlePlugin *wtp)
{
    gtk_widget_set_margin_start (wtp->box, wtp->prefs->title_padding);
    gtk_widget_set_margin_end (wtp->box, wtp->prefs->title_padding);
    gtk_box_set_spacing (GTK_BOX (wtp->box), wtp->prefs->title_padding);
}


void set_title_alignment (WckTitlePlugin *wtp)
{
    gtk_label_set_xalign (wtp->title, wtp->prefs->title_alignment / 2.0);
    gtk_label_set_yalign (wtp->title, 0.5);
}


gboolean on_title_pressed (GtkWidget *title, GdkEventButton *event, WckTitlePlugin *wtp)
{

    if (!wtp->win->controlwindow)
        return FALSE;

    if (event->button == 1
        && !window_is_desktop (wtp->win->controlwindow))
    {
        /* double/tripple click */
        if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
        {
            toggle_maximize (wtp->win->controlwindow);
        }
        else /* left-click */
        {
            wnck_window_activate (wtp->win->controlwindow, gtk_get_current_event_time());
        }
        return TRUE;
    }

    if (event->button == 3)
    {
        /* right-click */
        wnck_window_activate (wtp->win->controlwindow, gtk_get_current_event_time());

        /* let the panel show the menu */
        return TRUE;
    }

    return FALSE;
}


gboolean on_title_released (GtkWidget *title, GdkEventButton *event, WckTitlePlugin *wtp)
{
    if (!wtp->win->controlwindow)
        return FALSE;

    if (event->button == 2)
    {
        /* middle-click */
        wnck_window_close(wtp->win->controlwindow, GDK_CURRENT_TIME);
        return TRUE;
    }

    return FALSE;
}


static void
set_title_colors (WckTitlePlugin *wtp)
{
    /* get plugin widget style */
    g_free (wtp->prefs->active_text_color);
    g_free (wtp->prefs->inactive_text_color);
    wtp->prefs->active_text_color = get_ui_color (GTK_WIDGET (wtp->plugin), GTK_STATE_FLAG_NORMAL);
    wtp->prefs->inactive_text_color = mix_bg_fg (GTK_WIDGET (wtp->plugin), GTK_STATE_FLAG_NORMAL, wtp->prefs->inactive_text_alpha / 100.0, wtp->prefs->inactive_text_shade / 100.0);
}


static void
apply_wm_settings (WckTitlePlugin *wtp)
{
    gchar *wm_theme = xfconf_channel_get_string (wtp->wm_channel, "/general/theme", NULL);

    if (G_LIKELY(wm_theme))
    {
        gchar *wm_title_font = xfconf_channel_get_string (wtp->wm_channel, "/general/title_font", wtp->prefs->title_font);

        g_free (wtp->prefs->title_font);
        wtp->prefs->title_font = wm_title_font;

        on_name_changed (wtp->win->controlwindow, wtp);

        g_free (wm_theme);
    }
}


static void
on_x_channel_property_changed (XfconfChannel *x_channel, const gchar *property_name, const GValue *value, WckTitlePlugin *wtp)
{
    if (g_str_has_prefix(property_name, "/Net/") == TRUE)
    {
        const gchar *name = &property_name[5];
        switch (G_VALUE_TYPE(value))
        {
            case G_TYPE_STRING:
                if (!strcmp (name, "ThemeName"))
                {
                    set_title_colors (wtp);
                    on_name_changed (wtp->win->controlwindow, wtp);
                }
                break;
            default:
                g_warning("The property '%s' is not supported", property_name);
                break;
        }
    }
}


static void
on_xfwm_channel_property_changed (XfconfChannel *wm_channel, const gchar *property_name, const GValue *value, WckTitlePlugin *wtp)
{
    if (g_str_has_prefix(property_name, "/general/") == TRUE)
    {
        const gchar *name = &property_name[9];
        switch (G_VALUE_TYPE(value))
        {
            case G_TYPE_STRING:
                if (!strcmp (name, "title_font"))
                {
                    apply_wm_settings (wtp);
                }
                else if (!strcmp (name, "theme"))
                {
                    init_title (wtp);
                    reload_wnck_title (wtp);
                }
                break;
            default:
                g_warning("The property '%s' is not supported", property_name);
                break;
        }
    }
}


void init_title (WckTitlePlugin *wtp)
{
    set_title_colors (wtp);
    resize_title (wtp);

    gtk_label_set_ellipsize (wtp->title, PANGO_ELLIPSIZE_END);

    if (wtp->prefs->size_mode != SHRINK)
    {
        set_title_alignment (wtp);
    }

    /* get the xfwm4 chanel */
    wtp->wm_channel = wck_properties_get_channel (G_OBJECT (wtp->plugin), "xfwm4");

    /* try to set title settings from the xfwm4 theme */
    if (wtp->wm_channel && wtp->prefs->sync_wm_font)
    {
        apply_wm_settings (wtp);
        g_signal_connect (wtp->wm_channel, "property-changed",
                          G_CALLBACK (on_xfwm_channel_property_changed), wtp);
    }

    set_title_padding (wtp);

    /* get the xsettings chanel to update the gtk theme */
    wtp->x_channel = wck_properties_get_channel (G_OBJECT (wtp->plugin), "xsettings");

    if (wtp->x_channel)
        g_signal_connect (wtp->x_channel, "property-changed",
                          G_CALLBACK (on_x_channel_property_changed), wtp);
}
