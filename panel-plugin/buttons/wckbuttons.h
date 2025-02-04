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

#ifndef __WCKBUTTONS_H__
#define __WCKBUTTONS_H__

#include <libxfce4panel/libxfce4panel.h>
#include <xfconf/xfconf.h>

#include <common/wck-plugin.h>
#include <common/wck-utils.h>

#define WCKBUTTONS_ICON "wckbuttons-plugin"

G_BEGIN_DECLS

/* indexing of buttons */
typedef enum {
    MINIMIZE_BUTTON = 0,    // minimize button
    MAXIMIZE_BUTTON,    // maximize/unmaximize button
    CLOSE_BUTTON,       // close button

    BUTTONS             // number of buttons
} WBButton;

typedef enum {
    BUTTON_STATE_NORMAL =  0,
    BUTTON_STATE_PRELIGHT,
    BUTTON_STATE_PRESSED,

  BUTTON_STATES
} WBButtonSt;

/* we will index images for convenience */
typedef enum {
    IMAGE_MINIMIZE = 0,
    IMAGE_UNMAXIMIZE,
    IMAGE_MAXIMIZE,
    IMAGE_CLOSE,

    IMAGES_BUTTONS
} WBImageButton;

/* we will also index image states for convenience */
typedef enum {
    IMAGE_UNFOCUSED = 0,
    IMAGE_FOCUSED,
    IMAGE_PRELIGHT,
    IMAGE_PRESSED,

    IMAGES_STATES
} WBImageState;

typedef struct {
    WckConf *conf;
    GtkBuilder *builder;
    gboolean only_maximized;        // [T/F] Only track maximized windows
    gboolean show_on_desktop;       // [T/F] Show the plugin on desktop
    gboolean only_current_display;  // [T/F] Only control windows of current display
    gchar       *theme;             // Selected theme path
    gchar      *button_layout;      // Button layout ["XXX"] (example "HMC" : H=Hide, M=Maximize/unMaximize, C=Close)
    gboolean sync_wm_theme;         // [T/F] Try to use xfwm4 active theme if possible.
} WckButtonsPreferences;

/* Definition for our button */
typedef struct {
    GtkEventBox     *eventbox;
    GtkImage        *image;
} WindowButton;

/* plugin structure */
typedef struct {
    XfcePanelPlugin *plugin;

    /* Widgets */
    GtkWidget *ebox;
    GtkWidget *box;

    WindowButton  *button[BUTTONS]; // Array of buttons

    WckButtonsPreferences *prefs;   // Main properties
    WckUtils *win;

    GdkPixbuf *pixbufs[IMAGES_STATES][IMAGES_BUTTONS];
    XfconfChannel *wm_channel;      // window manager chanel
    XfconfChannel *x_channel;       // xsettings chanel
    gulong wph;                     // xfwm chanel property changed handler id
} WckButtonsPlugin;

void wckbuttons_settings_save (WckButtonsPreferences *prefs);

G_END_DECLS

#endif /* !__WCKBUTTONS_H__ */
