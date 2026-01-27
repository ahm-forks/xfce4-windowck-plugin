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

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include <string.h>
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include <libxfce4ui/libxfce4ui.h>

#include "wck-plugin.h"

XfconfChannel *
wck_properties_get_channel (GObject *object_for_weak_ref, const gchar *channel_name)
{
  GError        *error = NULL;
  XfconfChannel *channel;

  g_return_val_if_fail (G_IS_OBJECT (object_for_weak_ref), NULL);

  if (!xfconf_init (&error))
    {
      g_critical ("Failed to initialize Xfconf: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  //~ channel = xfconf_channel_get (XFCE_PANEL_CHANNEL_NAME);
  channel = xfconf_channel_get (channel_name);
  g_object_weak_ref (object_for_weak_ref, (GWeakNotify) xfconf_shutdown, NULL);

  return channel;
}


WckConf *
wck_conf_new (XfcePanelPlugin *plugin)
{
    WckConf *conf = g_slice_new0 (WckConf);

    conf->channel = wck_properties_get_channel (G_OBJECT (plugin), "xfce4-panel");
    conf->property_base = xfce_panel_plugin_get_property_base (plugin);

    return conf;
}


gboolean
wck_conf_get_bool (const WckConf *conf, const gchar *setting, gboolean default_value)
{
    gchar    *property;
    gboolean  value;

    property = g_strconcat (conf->property_base, setting, NULL);
    value = xfconf_channel_get_bool (conf->channel, property, default_value);
    g_free (property);

    return value;
}


gint
wck_conf_get_int (const WckConf *conf, const gchar *setting, gint default_value)
{
    gchar *property;
    gint   value;

    property = g_strconcat (conf->property_base, setting, NULL);
    value = xfconf_channel_get_int (conf->channel, property, default_value);
    g_free (property);

    return value;
}


gchar *
wck_conf_get_string (const WckConf *conf, const gchar *setting, const gchar *default_value)
{
    gchar *property;
    gchar *value;

    property = g_strconcat (conf->property_base, setting, NULL);
    value = xfconf_channel_get_string (conf->channel, property, default_value);
    g_free (property);

    return value;
}


void
wck_conf_set_bool (const WckConf *conf, const gchar *setting, gboolean value)
{
    gchar *property;

    property = g_strconcat (conf->property_base, setting, NULL);
    xfconf_channel_set_bool (conf->channel, property, value);
    g_free (property);
}


void
wck_conf_set_int (const WckConf *conf, const gchar *setting, gint value)
{
    gchar *property;

    property = g_strconcat (conf->property_base, setting, NULL);
    xfconf_channel_set_int (conf->channel, property, value);
    g_free (property);
}


void
wck_conf_set_string (const WckConf *conf, const gchar *setting, const gchar *value)
{
    gchar *property;

    property = g_strconcat (conf->property_base, setting, NULL);
    xfconf_channel_set_string (conf->channel, property, value);
    g_free (property);
}


void
wck_about (XfcePanelPlugin *plugin, const gchar *icon_name)
{
    const gchar *authors[] =
    {
        "Alessio Piccoli <alepic@geckoblu.net>",
        "Cedric Leporcq <cedl38@gmail.com>",
        "Felix Krull <f_krull@gmx.de>",
        "Pavel Zlámal <zlamal@cesnet.cz>",
        "",
        "This code is derived from",
        "Window Applets https://www.gnome-look.org/p/1115400 by Andrej Belcijan.",
        NULL
    };

    /* TODO: add translators. */
    gtk_show_about_dialog (NULL,
                           "logo-icon-name", icon_name,
                           "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
                           "version", VERSION_FULL,
                           "program-name", xfce_panel_plugin_get_display_name (plugin),
                           "comments", xfce_panel_plugin_get_comment (plugin),
                           "website", PACKAGE_URL,
                           "copyright", "Copyright \302\251 2003-" COPYRIGHT_YEAR " The Xfce development team",
                           "authors", authors,
                           NULL);
}


GtkWidget *show_refresh_item (XfcePanelPlugin *plugin)
{
    GtkWidget *refresh;
    refresh = xfce_gtk_image_menu_item_new_from_icon_name (_("_Refresh"), NULL, NULL, NULL, NULL, "view-refresh", NULL);
    xfce_panel_plugin_menu_insert_item(plugin, GTK_MENU_ITEM(refresh));
    gtk_widget_show (refresh);

    return refresh;
}


GtkWidget *
wck_dialog_get_widget (GtkBuilder *builder, const gchar *name)
{
    GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (builder, name));

    if (G_UNLIKELY (widget == NULL))
        DBG ("No widget with the name \"%s\" found", name);

    return widget;
}

void
wck_configure_dialog (XfcePanelPlugin *plugin, const gchar *icon_name, GtkWidget *ca, GCallback response_cb, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    const gchar *name;

    if ((dialog = g_object_get_data (G_OBJECT (plugin), "dialog")) != NULL)
    {
        gtk_window_present (GTK_WINDOW (dialog));
        return;
    }

    /* create the dialog */
    name = xfce_panel_plugin_get_display_name (plugin);
    dialog = xfce_titled_dialog_new_with_mixed_buttons (_(name),
                                                        GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        "help-browser", _("Help"), GTK_RESPONSE_HELP,
                                                        "window-close", _("_Close"), GTK_RESPONSE_OK,
                                                        NULL);

    /* center dialog on the screen */
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

    /* set dialog icon */
    gtk_window_set_icon_name (GTK_WINDOW (dialog), icon_name);

    /* link the dialog to the plugin, so we can destroy it when the plugin
    * is closed, but the dialog is still open */
    g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

    /* connect the response signal to the dialog */
    g_signal_connect (G_OBJECT (dialog), "response",
                      response_cb, data);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog) );

    if (G_LIKELY (ca != NULL))
        gtk_container_add (GTK_CONTAINER (content_area), ca);
    else
        DBG("Failed to create content area");

    /* show the entire dialog */
    gtk_widget_show (dialog);
}


void
wck_configure_response (XfcePanelPlugin *plugin, GtkWidget *dialog, gint response, WckSettingsCb save_settings, gpointer data)
{
    if (response == GTK_RESPONSE_HELP)
    {
        gboolean result;

#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
        result = g_spawn_command_line_async ("xfce-open --launch WebBrowser " PACKAGE_URL, NULL);
#else
        result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PACKAGE_URL, NULL);
#endif

        if (G_UNLIKELY (result == FALSE))
            g_warning (_("Unable to open the following url: %s"), PACKAGE_URL);
    }
    else
    {
        /* remove the dialog data from the plugin */
        g_object_set_data (G_OBJECT (plugin), "dialog", NULL);

        /* save the plugin */
        save_settings (data);

        /* destroy the properties dialog */
        gtk_widget_destroy (dialog);
    }
}


gboolean
wck_check_x11_windowing (GdkScreen *screen)
{
#ifdef GDK_WINDOWING_X11
    gboolean x11_windowing = GDK_IS_X11_DISPLAY (gdk_display_get_default ());
#else
    gboolean x11_windowing = FALSE;
#endif
    if (!x11_windowing)
    {
        GtkWidget *dialog = xfce_message_dialog_new (NULL, NULL, "dialog-error",
                                                     _("Unsupported windowing environment"), NULL,
                                                     _("_OK"), GTK_RESPONSE_OK, NULL);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return FALSE;
    }

    return TRUE;
}
