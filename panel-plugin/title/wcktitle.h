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

#ifndef __WCKTITLE_H__
#define __WCKTITLE_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <xfconf/xfconf.h>

#include <common/wck-plugin.h>
#include <common/wck-utils.h>

#define WCKTITLE_ICON "wcktitle-plugin"

G_BEGIN_DECLS

typedef enum Alignment
{
    LEFT = 0,
    CENTER = 1,
    RIGHT = 2
} Alignment;

typedef enum SizeMode
{
    SHRINK = 0,
    FIXED = 1,
    EXPAND = 2
} SizeMode;

typedef struct {
    WckConf *conf;
    GtkBuilder *builder;
    gboolean only_maximized;        // [T/F] Only track maximized windows
    gboolean show_on_desktop;       // [T/F] Show the plugin on desktop
    gboolean only_current_display;  // [T/F] Only control windows of current display
    gboolean full_name;             // [T/F] Show full name
    gboolean two_lines;             // [T/F] Display the title on two lines
    gboolean show_tooltips;         // [T/F] Show tooltips

    SizeMode size_mode;             // Size mode [SHRINK, FIXED, EXPAND]
    gint title_size;                // Title size in chars
    gint title_padding;             // Title padding in pixels
    Alignment title_alignment;      // Title alignment [LEFT, CENTER, RIGHT]

    gboolean sync_wm_font;          // [T/F] Try to use xfwm4 active theme if possible.
    gchar *title_font;              // Title font
    gchar *subtitle_font;           // Subtitle font
    gint inactive_text_alpha;       // Title inactive alpha
    gint inactive_text_shade;       // Title inactive shade
    gchar *active_text_color;       // active text color
    gchar *inactive_text_color;     // inactive text color
} WckTitlePreferences;

/* plugin structure */
typedef struct {
    XfcePanelPlugin *plugin;

    /* Widgets */
    GtkWidget *ebox;
    GtkWidget *box;
    GtkLabel *title;

    WckTitlePreferences     *prefs;
    WckUtils *win;

    gulong cnh;                     // controlled window name handler id

    XfconfChannel *wm_channel;      // window manager chanel
    XfconfChannel *x_channel;       // xsettings chanel
} WckTitlePlugin;

void wcktitle_settings_save (WckTitlePreferences *prefs);

G_END_DECLS

#endif /* __WCKTITLE_H__ */
