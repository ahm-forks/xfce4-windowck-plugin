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

#include "wckmenu.h"
#include "wckmenu-icon.h"

void reload_wnck_icon (WckMenuPlugin *wmp)
{
    /* disconnect controlled window icon signal handler */
    wck_signal_handler_disconnect (G_OBJECT(wmp->win->controlwindow), wmp->cih);

    reload_wnck (wmp->win, wmp->prefs->only_maximized, FALSE, wmp);
}


static void on_icon_changed(WnckWindow *controlwindow, WckMenuPlugin *wmp)
{
    gint icon_size;

    if (!controlwindow)
    {
        gtk_image_clear (GTK_IMAGE (wmp->icon->symbol));
        return;
    }

    icon_size = xfce_panel_plugin_get_icon_size (wmp->plugin);
    if (wmp->prefs->show_on_desktop)
    {
        gtk_widget_set_sensitive (wmp->icon->symbol, TRUE);

        if (window_is_desktop (controlwindow))
        {
            if (!wnck_window_is_active(controlwindow))
                gtk_widget_set_sensitive (wmp->icon->symbol, FALSE);

            gtk_image_set_from_icon_name (GTK_IMAGE (wmp->icon->symbol), "go-home", GTK_ICON_SIZE_BUTTON);
            gtk_image_set_pixel_size (GTK_IMAGE (wmp->icon->symbol), icon_size);
        }
    }

    if (!window_is_desktop (controlwindow))
    {
        GdkPixbuf *pixbuf = NULL;
        GdkPixbuf *grayscale = NULL;
        cairo_surface_t *surface;
        gint scale_factor;

        /* This only returns a pointer - it SHOULDN'T be unrefed! */
        if (icon_size < WNCK_DEFAULT_ICON_SIZE)
            pixbuf = wnck_window_get_mini_icon(controlwindow);
        else
            pixbuf = wnck_window_get_icon(controlwindow);

        /* leave when there is no valid pixbuf */
        if (G_UNLIKELY (pixbuf == NULL))
        {
            gtk_image_clear (GTK_IMAGE (wmp->icon->symbol));
            return;
        }

        if (!wnck_window_is_active(controlwindow))
        {
            /* icon color is set to grayscale */
            grayscale = gdk_pixbuf_copy(pixbuf);
            gdk_pixbuf_saturate_and_pixelate(grayscale, grayscale, 0, FALSE);
            if (G_UNLIKELY (grayscale != NULL))
                pixbuf = grayscale;
        }

        scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (wmp->plugin));
        surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);
        gtk_image_set_from_surface (GTK_IMAGE (wmp->icon->symbol), surface);
        cairo_surface_destroy (surface);

        if (grayscale != NULL && grayscale != pixbuf)
            g_object_unref (G_OBJECT (grayscale));
    }
}


static void
set_icon_color (GtkWidget *widget, const gchar *color)
{
    GtkStyleContext *style_ctx = gtk_widget_get_style_context (widget);
    GtkCssProvider *provider;
    const gchar *data_name = "color_provider";
    gpointer current_provider = g_object_get_data (G_OBJECT (widget), data_name);
    gchar *style;

    if (current_provider)
        gtk_style_context_remove_provider (style_ctx, GTK_STYLE_PROVIDER (current_provider));

    provider = gtk_css_provider_new ();
    style = g_strdup_printf ("* { color: %s; }", color);
    gtk_css_provider_load_from_data (provider, style, strlen (style), NULL);
    g_free (style);
    gtk_style_context_add_provider (style_ctx, GTK_STYLE_PROVIDER (provider), G_MAXUINT);

    /* Store the provider inside widget */
    g_object_set_data_full (G_OBJECT (widget), data_name, provider, g_object_unref);
}


void on_wck_state_changed (WnckWindow *controlwindow, gpointer data)
{
    WckMenuPlugin *wmp = data;

    if (wmp->prefs->show_app_icon)
    {
        on_icon_changed (wmp->win->controlwindow, wmp);
    }
    else if (controlwindow
             && (!window_is_desktop (controlwindow)
                 || wmp->prefs->show_on_desktop))
    {
        set_icon_color (wmp->icon->symbol, wnck_window_is_active (controlwindow) ? wmp->prefs->active_color : wmp->prefs->inactive_color);
    }
}


void on_control_window_changed (WnckWindow *controlwindow, WnckWindow *previous, gpointer data)
{
    WckMenuPlugin *wmp = data;

    on_wck_state_changed (controlwindow, wmp);

    if (!controlwindow
        || (window_is_desktop (controlwindow)
            && !wmp->prefs->show_on_desktop))
    {
        if (gtk_widget_get_visible(GTK_WIDGET(wmp->box)))
            gtk_widget_hide(GTK_WIDGET(wmp->box));
    }
    else
    {
        if (!gtk_widget_get_visible(GTK_WIDGET(wmp->box)))
            gtk_widget_show_all(GTK_WIDGET(wmp->box));
    }

    if (controlwindow)
    {
        if (!window_is_desktop (controlwindow))
        {
            if (!gtk_widget_get_visible(GTK_WIDGET(wmp->icon->eventbox)))
                gtk_widget_show_all (GTK_WIDGET(wmp->icon->eventbox));
        }
        else if (wmp->prefs->show_on_desktop && !wmp->prefs->show_app_icon)
        {
            if (gtk_widget_get_visible(GTK_WIDGET(wmp->icon->eventbox)))
                gtk_widget_hide (GTK_WIDGET(wmp->icon->eventbox));
        }
    }

    if (wmp->prefs->show_app_icon)
    {
        wck_signal_handler_disconnect (G_OBJECT(previous), wmp->cih);

        if (controlwindow)
            wmp->cih = g_signal_connect(G_OBJECT(controlwindow), "icon-changed", G_CALLBACK(on_icon_changed), wmp);
    }
}


gboolean on_icon_released(GtkWidget *icon, GdkEventButton *event, WckMenuPlugin *wmp)
{
    GtkWidget *menu;

    if ((event->button != 1)
        || window_is_desktop (wmp->win->controlwindow))
        return FALSE;

    menu = wnck_action_menu_new (wmp->win->controlwindow);

    gtk_menu_attach_to_widget(GTK_MENU(menu), GTK_WIDGET(wmp->icon->eventbox), NULL);
    gtk_menu_popup_at_widget (GTK_MENU (menu), GTK_WIDGET(wmp->icon->eventbox),
                              GDK_GRAVITY_STATIC, GDK_GRAVITY_STATIC, NULL);

    return TRUE;
}


static void
set_icon_colors (WckMenuPlugin *wmp)
{
    /* get plugin widget style */
    g_free (wmp->prefs->active_color);
    wmp->prefs->active_color = get_ui_color (GTK_WIDGET(wmp->plugin), GTK_STATE_FLAG_NORMAL);

    g_free (wmp->prefs->inactive_color);
    wmp->prefs->inactive_color = mix_bg_fg (GTK_WIDGET(wmp->plugin), GTK_STATE_FLAG_NORMAL, wmp->prefs->inactive_alpha / 100.0, wmp->prefs->inactive_shade / 100.0);
}


static void
on_x_channel_property_changed (XfconfChannel *x_channel, const gchar *property_name, const GValue *value, WckMenuPlugin *wmp)
{
    if (g_str_has_prefix(property_name, "/Net/") == TRUE)
    {
        const gchar *name = &property_name[5];
        switch (G_VALUE_TYPE(value))
        {
            case G_TYPE_STRING:
                if (!strcmp (name, "ThemeName"))
                {
                    set_icon_colors (wmp);
                }
                break;
            default:
                g_warning("The property '%s' is not supported", property_name);
                break;
        }
    }
}


static void
on_xfwm_channel_property_changed (XfconfChannel *wm_channel, const gchar *property_name, const GValue *value, WckMenuPlugin *wmp)
{
    if (g_str_has_prefix(property_name, "/general/"))
    {
        const gchar *name = &property_name[9];
        switch (G_VALUE_TYPE(value))
        {
            case G_TYPE_STRING:
                if (!strcmp (name, "theme"))
                {
                    init_icon_colors (wmp);
                    reload_wnck_icon (wmp);
                }
                break;
            default:
                g_warning("The property '%s' is not supported", property_name);
                break;
        }
    }
}


void init_icon_colors (WckMenuPlugin *wmp)
{
    set_icon_colors (wmp);

    /* get the xfwm4 chanel */
    wmp->wm_channel = wck_properties_get_channel (G_OBJECT (wmp->plugin), "xfwm4");

    /* try to set settings from the xfwm4 theme */
    if (wmp->wm_channel)
    {
        g_signal_connect (wmp->wm_channel, "property-changed", G_CALLBACK (on_xfwm_channel_property_changed), wmp);
    }

    /* get the xsettings chanel to update the gtk theme */
    wmp->x_channel = wck_properties_get_channel (G_OBJECT (wmp->plugin), "xsettings");

    if (wmp->x_channel)
    {
        g_signal_connect (wmp->x_channel, "property-changed", G_CALLBACK (on_x_channel_property_changed), wmp);
    }
}
