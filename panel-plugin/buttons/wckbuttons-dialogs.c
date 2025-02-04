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

#include "theme.h"
#include "wckbuttons.h"
#include "wckbuttons-theme.h"
#include "wckbuttons-dialogs.h"
#include "wckbuttons-dialogs_ui.h"


enum
{
  COL_THEME_NAME,
  COL_THEME_RC,
  N_COLUMNS
};


static void
on_only_maximized_toggled (GtkRadioButton *only_maximized, WckButtonsPlugin *wbp)
{
    wbp->prefs->only_maximized = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (only_maximized));
    reload_wnck (wbp->win, wbp->prefs->only_maximized, wbp->prefs->only_current_display, wbp);
}

static void
on_only_current_display_toggled (GtkToggleButton *only_current_display, WckButtonsPlugin *wbp)
{
    wbp->prefs->only_current_display = gtk_toggle_button_get_active (only_current_display);
    reload_wnck (wbp->win, wbp->prefs->only_maximized, wbp->prefs->only_current_display, wbp);
}

static void
on_show_on_desktop_toggled (GtkToggleButton *show_on_desktop, WckButtonsPlugin *wbp)
{
    wbp->prefs->show_on_desktop = gtk_toggle_button_get_active (show_on_desktop);
    reload_wnck (wbp->win, wbp->prefs->only_maximized, wbp->prefs->only_current_display, wbp);
}


static void
on_button_layout_changed (GtkEditable *entry, WckButtonsPlugin *wbp)
{
    if (gtk_widget_get_sensitive (GTK_WIDGET(entry)))
    {
        const gchar *button_layout = gtk_entry_get_text (GTK_ENTRY(entry));
        wbp->prefs->button_layout = button_layout_filter (button_layout, wbp->prefs->button_layout);

        if (wbp->prefs->sync_wm_theme)
        {
            gchar *part;
            const gchar *layout;

            const gchar *wm_button_layout = xfconf_channel_get_string (wbp->wm_channel,
                                                                      "/general/button_layout",
                                                                      "O|HMC");

            /* get opposite part of the layout and concatenate it */
            part = opposite_layout_filter (wm_button_layout);
            if (wm_button_layout[0] == part[0])
                layout = g_strconcat (part, wbp->prefs->button_layout, NULL);
            else
                layout = g_strconcat (wbp->prefs->button_layout, part, NULL);

            xfconf_channel_set_string (wbp->wm_channel, "/general/button_layout", layout);
            g_free (part);
        }
        else
        {
            replace_buttons (wbp->prefs->button_layout, wbp);
            on_wck_state_changed (wbp->win->controlwindow, wbp);
        }
    }
}


static void
wckbuttons_theme_selection_changed (GtkTreeSelection *selection, WckButtonsPlugin *wbp)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        const gchar *theme;
        GtkWidget   *entry;

        gtk_tree_model_get (model, &iter, COL_THEME_NAME, &theme, -1);

        /* set the theme name */
        wbp->prefs->theme = g_strdup (theme);

        entry = GTK_WIDGET (gtk_builder_get_object (wbp->prefs->builder, "button_layout"));

        if (wbp->prefs->sync_wm_theme)
        {
            gchar *button_layout;

            xfconf_channel_set_string (wbp->wm_channel, "/general/theme", wbp->prefs->theme);
            button_layout = get_rc_button_layout (theme);

            if (button_layout)
            {
                gtk_widget_set_sensitive (entry, FALSE);
                gtk_entry_set_text (GTK_ENTRY(entry), button_layout);
            }
            else
            {
                gtk_entry_set_text (GTK_ENTRY(entry), wbp->prefs->button_layout);
                gtk_widget_set_sensitive (entry, TRUE);
            }
            g_free (button_layout);
        }
        else
        {
            load_theme (wbp->prefs->theme, wbp);
            replace_buttons (wbp->prefs->button_layout, wbp);
            on_wck_state_changed (wbp->win->controlwindow, wbp);
        }
    }
}


static void
wckbuttons_load_themes (GtkWidget *theme_name_treeview, WckButtonsPlugin *wbp)
{
    GtkTreeModel *model;
    GHashTable   *themes;
    gchar       **theme_dirs;

    themes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (theme_name_treeview));

    /* clear any previous row */
    gtk_list_store_clear (GTK_LIST_STORE (model));

    xfce_resource_push_path (XFCE_RESOURCE_THEMES, DATADIR G_DIR_SEPARATOR_S "themes");
    theme_dirs = xfce_resource_dirs (XFCE_RESOURCE_THEMES);
    xfce_resource_pop_path (XFCE_RESOURCE_THEMES);

    for (gint i = 0; theme_dirs[i] != NULL; ++i)
    {
        GDir         *dir;
        const gchar  *file;

        dir = g_dir_open (theme_dirs[i], 0, NULL);
        if (G_UNLIKELY (dir == NULL))
            continue;

        while ((file = g_dir_read_name (dir)) != NULL)
        {
            /* check if there is not already a theme with the
             * same name in the database */
            if (g_hash_table_lookup (themes, file) == NULL)
            {
                GtkTreeIter   iter;
                gchar        *themedir;

                if (wbp->prefs->sync_wm_theme)
                {
                    if (!test_theme_dir(file, "xfwm4", THEMERC))
                        continue;
                }

                themedir = get_unity_theme_dir (file, NULL);
                if (!themedir)
                    continue;

                g_hash_table_insert (themes, g_strdup (file), GINT_TO_POINTER (1));

                /* insert in the list store */
                gtk_list_store_append (GTK_LIST_STORE (model), &iter);
                gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                                    COL_THEME_NAME, file,
                                    COL_THEME_RC, g_path_get_basename (themedir), -1);

                if (G_UNLIKELY (g_str_equal (wbp->prefs->theme, file)))
                {
                    GtkTreePath *path = gtk_tree_model_get_path (model, &iter);

                    gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (theme_name_treeview)),
                                                  &iter);
                    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (theme_name_treeview), path, NULL, TRUE, 0.5, 0.5);

                    gtk_tree_path_free (path);
                }

                g_free (themedir);
            }
        }

        g_dir_close (dir);
    }

    g_strfreev (theme_dirs);
    g_hash_table_destroy (themes);
}


static gint
wckbuttons_theme_sort_func (GtkTreeModel *model,
                            GtkTreeIter  *iter1,
                            GtkTreeIter  *iter2,
                            void *unused)
{
  gchar *str1 = NULL;
  gchar *str2 = NULL;

  gtk_tree_model_get (model, iter1, 0, &str1, -1);
  gtk_tree_model_get (model, iter2, 0, &str2, -1);

  if (str1 == NULL) str1 = g_strdup ("");
  if (str2 == NULL) str2 = g_strdup ("");

  return g_utf8_collate (str1, str2);
}


static void
on_sync_wm_theme_toggled (GtkToggleButton *sync_wm_theme, WckButtonsPlugin *wbp)
{
    GtkWidget   *theme_name_treeview;

    theme_name_treeview = GTK_WIDGET (gtk_builder_get_object (wbp->prefs->builder,
                                                              "theme_name_treeview"));

    wbp->prefs->sync_wm_theme = gtk_toggle_button_get_active (sync_wm_theme);

    init_theme (wbp);
    wckbuttons_load_themes (theme_name_treeview, wbp);

    if (!wbp->prefs->sync_wm_theme)
    {
        GtkWidget    *entry;

        entry = GTK_WIDGET (gtk_builder_get_object (wbp->prefs->builder, "button_layout"));
        gtk_widget_set_sensitive (entry, TRUE);
        gtk_entry_set_text (GTK_ENTRY(entry), wbp->prefs->button_layout);
    }
}


static GtkWidget *
build_properties_area (WckButtonsPlugin *wbp, const gchar *buffer, gsize length)
{
    GError *error = NULL;

    wbp->prefs->builder = gtk_builder_new ();

    if (gtk_builder_add_from_string (wbp->prefs->builder, buffer, length, &error)) {
        GObject *area = gtk_builder_get_object (wbp->prefs->builder, "vbox0");

        if (G_LIKELY (area != NULL))
        {
            GtkRadioButton *only_maximized, *active_window;
            GtkToggleButton *show_on_desktop, *only_current_display, *sync_wm_theme;
            GtkWidget *theme_name_treeview;
            GtkEntry *button_layout;

            only_maximized = GTK_RADIO_BUTTON (wck_dialog_get_widget (wbp->prefs->builder,
                                                                      "only_maximized"));
            active_window = GTK_RADIO_BUTTON (wck_dialog_get_widget (wbp->prefs->builder,
                                                                     "active_window"));
            if (G_LIKELY (only_maximized != NULL && active_window != NULL))
            {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_maximized), wbp->prefs->only_maximized);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (active_window), !wbp->prefs->only_maximized);
                g_signal_connect (only_maximized, "toggled",
                                  G_CALLBACK (on_only_maximized_toggled), wbp);
            }

            show_on_desktop = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wbp->prefs->builder, "show_on_desktop"));
            if (G_LIKELY (show_on_desktop != NULL))
            {
                gtk_toggle_button_set_active (show_on_desktop, wbp->prefs->show_on_desktop);
                g_signal_connect (show_on_desktop, "toggled",
                                  G_CALLBACK (on_show_on_desktop_toggled), wbp);
            }

            only_current_display = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wbp->prefs->builder, "only_current_display"));
            if (G_LIKELY (only_current_display != NULL))
            {
                gtk_toggle_button_set_active (only_current_display, wbp->prefs->only_current_display);
                g_signal_connect (only_current_display, "toggled",
                                  G_CALLBACK (on_only_current_display_toggled), wbp);
            }

            /* Style widgets */

            /* theme name */
            theme_name_treeview = wck_dialog_get_widget (wbp->prefs->builder, "theme_name_treeview");
            if (G_LIKELY (theme_name_treeview != NULL))
            {
                GtkTreeSelection *selection;
                GtkCellRenderer *renderer;
                GtkListStore *list_store;

                list_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
                gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (list_store), COL_THEME_NAME,
                                                 wckbuttons_theme_sort_func,
                                                 NULL, NULL);
                gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (list_store), COL_THEME_NAME, GTK_SORT_ASCENDING);
                gtk_tree_view_set_model (GTK_TREE_VIEW (theme_name_treeview), GTK_TREE_MODEL (list_store));
                g_object_unref (G_OBJECT (list_store));

                renderer = gtk_cell_renderer_text_new ();
                gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (theme_name_treeview),
                                                             0, _("Directory"), renderer, "text", 1, NULL);
                gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (theme_name_treeview),
                                                             0, _("Themes usable"), renderer, "text", 0, NULL);

                selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (theme_name_treeview));
                g_signal_connect (selection, "changed",
                                  G_CALLBACK (wckbuttons_theme_selection_changed), wbp);
                gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

                wckbuttons_load_themes (theme_name_treeview, wbp);
            }

            sync_wm_theme = GTK_TOGGLE_BUTTON (wck_dialog_get_widget (wbp->prefs->builder, "sync_wm_theme"));
            if (G_LIKELY (sync_wm_theme != NULL))
            {
                if (wbp->wm_channel)
                {
                    gtk_toggle_button_set_active (sync_wm_theme, wbp->prefs->sync_wm_theme);
                    g_signal_connect (sync_wm_theme, "toggled",
                                      G_CALLBACK(on_sync_wm_theme_toggled), wbp);
                }
                else {
                    gtk_widget_set_sensitive (GTK_WIDGET(sync_wm_theme), FALSE);
                }
            }

            button_layout = GTK_ENTRY (wck_dialog_get_widget (wbp->prefs->builder, "button_layout"));
            if (G_LIKELY (button_layout != NULL))
            {
                gtk_entry_set_text (button_layout, wbp->prefs->button_layout);
                g_signal_connect (GTK_EDITABLE(button_layout), "changed",
                                  G_CALLBACK(on_button_layout_changed), wbp);
            }

            return GTK_WIDGET(area);
        }
        else {
            g_set_error_literal (&error, 0, 0, "No widget with the name \"vbox0\" found");
        }
    }

    g_critical ("Failed to construct the builder for plugin %s-%d: %s.",
                xfce_panel_plugin_get_name (wbp->plugin),
                xfce_panel_plugin_get_unique_id (wbp->plugin),
                error->message);
    g_error_free (error);
    g_object_unref (G_OBJECT (wbp->prefs->builder));

    return NULL;
}


static void
wckbuttons_configure_response (GtkWidget *dialog, gint response, WckButtonsPlugin *wbp)
{
    wck_configure_response (wbp->plugin, dialog, response, (WckSettingsCb) wckbuttons_settings_save, wbp->prefs);
}


void
wckbuttons_configure (XfcePanelPlugin *plugin, WckButtonsPlugin *wbp)
{
    GtkWidget *ca;

    ca = build_properties_area (wbp, wckbuttons_dialogs_ui, wckbuttons_dialogs_ui_length);

    wck_configure_dialog (plugin, WCKBUTTONS_ICON, ca, G_CALLBACK(wckbuttons_configure_response), wbp);
}
