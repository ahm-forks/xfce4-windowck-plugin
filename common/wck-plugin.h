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

#ifndef __WCK_PLUGIN_H__
#define __WCK_PLUGIN_H__

#include <xfconf/xfconf.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

#define SETTING_ONLY_MAXIMIZED  "/only-maximized"
#define SETTING_SHOW_ON_DESKTOP "/show-on-desktop"
#define SETTING_ONLY_CURRDISPLAY "/only-current-display"

/* default settings */
#define DEFAULT_ONLY_MAXIMIZED TRUE
#define DEFAULT_ONLY_CURRDISPLAY FALSE
#define DEFAULT_SHOW_ON_DESKTOP FALSE

typedef void (*WckSettingsCb) (gpointer prefs);

typedef struct {
    XfconfChannel *channel;
    const gchar   *property_base;
} WckConf;

XfconfChannel *
wck_properties_get_channel (GObject *object_for_weak_ref, const gchar *channel_name);
WckConf *wck_conf_new (XfcePanelPlugin *plugin);
gboolean wck_conf_get_bool (const WckConf *conf, const gchar *setting, gboolean default_value);
gint wck_conf_get_int (const WckConf *conf, const gchar *setting, gint default_value);
gchar *wck_conf_get_string (const WckConf *conf, const gchar *setting, const gchar *default_value);
void wck_conf_set_bool (const WckConf *conf, const gchar *setting, gboolean value);
void wck_conf_set_int (const WckConf *conf, const gchar *setting, gint value);
void wck_conf_set_string (const WckConf *conf, const gchar *setting, const gchar *value);

void wck_about (XfcePanelPlugin *plugin, const gchar *icon_name);
GtkWidget *show_refresh_item (XfcePanelPlugin *plugin);
GtkWidget *wck_dialog_get_widget (GtkBuilder *builder, const gchar *name);
void wck_configure_dialog (XfcePanelPlugin *plugin, const gchar *icon_name, GtkWidget *ca, GCallback response_cb, gpointer data);
void wck_configure_response (XfcePanelPlugin *plugin, GtkWidget *dialog, gint response, WckSettingsCb save_settings, gpointer data);
gboolean wck_abort_non_x11_windowing (XfcePanelPlugin *plugin);

G_END_DECLS

#endif /* __WCK_PLUGIN_H__ */
