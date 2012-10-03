/* hotkey_editor.c
 * Artha - Free cross-platform open thesaurus
 * Copyright (C) 2009, 2010  Sundaram Ramaswamy, legends2k@yahoo.com
 *
 * Artha is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Artha is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Artha; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/*
 * This code sets up the GtkCellRendererAccelMode for the tree view 
 * of hotkey changer dialog.
 */


#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#define STR_INVALID_HOTKEY "The shortcut \"%s\" cannot be used because \
it will become impossible to type using this key.\nPlease try with a \
combination key such as Control, Alt or Shift at the same time."

#define STR_DUPLICATE_HOTKEY "This key combination (%s) is perhaps already \
taken up by another application. Please choose a different one!"

#define STR_INVALID_HOTKEY_TITLE	"Invalid Hotkey"

/* enum for GtkTreeView */
enum
{
	HOTKEY_COLUMN = 0,
	NUM_COLUMNS
};

/* these keys should never be part of the hotkey combo */
static const guint forbidden_keyvals[] = 
{
	/* Navigation keys */
	GDK_Home,
	GDK_Left,
	GDK_Up,
	GDK_Right,
	GDK_Down,
	GDK_Page_Up,
	GDK_Page_Down,
	GDK_End,
	GDK_Tab,

	/* Return */
	GDK_KP_Enter,
	GDK_Return,

	GDK_space,
	GDK_Mode_switch,
	GDK_Delete,
	GDK_Print,
	GDK_Insert
};

gboolean grab_ungrab_with_ignorable_modifiers(GtkAccelKey *binding, gboolean grab);
extern GtkAccelKey app_hotkey;

static gboolean keyval_is_forbidden (guint keyval)
{
	guint i = 0;

	for (i = 0; i < G_N_ELEMENTS(forbidden_keyvals); i++)
	{
		if (keyval == forbidden_keyvals[i])
			return TRUE;
	}

	return FALSE;
}

static void accel_edited_callback(GtkCellRendererText *cell, const char *path_string, guint accel_key, 
				  GdkModifierType accel_mods, guint hardware_keycode, gpointer data)
{
	GtkTreeView *view = (GtkTreeView *)data;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter = {0};
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkAccelKey *key_entry = NULL, temp_key = {0};
	GtkWidget *msg_dialog = NULL;
	gchar *temp_str = NULL;
	gboolean reg_succeeded = FALSE;

	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (model, &iter, HOTKEY_COLUMN, &key_entry, -1);

	/* sanity check and check to see if the same key combo was pressed again */
	if(key_entry == NULL || 
	(key_entry->accel_key == accel_key && 
	key_entry->accel_mods == accel_mods && 
	key_entry->accel_flags == hardware_keycode))
		return;

	/* CapsLock isn't supported as a keybinding modifier, so keep it from confusing us */
	accel_mods &= ~GDK_LOCK_MASK;

	temp_key.accel_key = accel_key;
	temp_key.accel_flags = hardware_keycode;
	temp_key.accel_mods   = accel_mods;


	/* Check for unmodified keys */
	if(temp_key.accel_mods == 0 && temp_key.accel_key != 0)
	{
		if ((temp_key.accel_key >= GDK_a && temp_key.accel_key <= GDK_z)
		   || (temp_key.accel_key >= GDK_A && temp_key.accel_key <= GDK_Z)
		   || (temp_key.accel_key >= GDK_0 && temp_key.accel_key <= GDK_9)
		   || (temp_key.accel_key >= GDK_kana_fullstop && temp_key.accel_key <= GDK_semivoicedsound)
		   || (temp_key.accel_key >= GDK_Arabic_comma && temp_key.accel_key <= GDK_Arabic_sukun)
		   || (temp_key.accel_key >= GDK_Serbian_dje && temp_key.accel_key <= GDK_Cyrillic_HARDSIGN)
		   || (temp_key.accel_key >= GDK_Greek_ALPHAaccent && temp_key.accel_key <= GDK_Greek_omega)
		   || (temp_key.accel_key >= GDK_hebrew_doublelowline && temp_key.accel_key <= GDK_hebrew_taf)
		   || (temp_key.accel_key >= GDK_Thai_kokai && temp_key.accel_key <= GDK_Thai_lekkao)
		   || (temp_key.accel_key >= GDK_Hangul && temp_key.accel_key <= GDK_Hangul_Special)
		   || (temp_key.accel_key >= GDK_Hangul_Kiyeog && temp_key.accel_key <= GDK_Hangul_J_YeorinHieuh)
		   || keyval_is_forbidden (temp_key.accel_key))
		{
			temp_str = gtk_accelerator_get_label (accel_key, accel_mods);

			msg_dialog = gtk_message_dialog_new (
						GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
						GTK_MESSAGE_WARNING,
						GTK_BUTTONS_OK, 
						STR_INVALID_HOTKEY, 
						temp_str);
			g_object_set(msg_dialog, "title", STR_INVALID_HOTKEY_TITLE, NULL);

			g_free (temp_str); temp_str = NULL;
			gtk_dialog_run (GTK_DIALOG (msg_dialog));
			gtk_widget_destroy (msg_dialog);
			msg_dialog = NULL;

			return;
		}
	}

#ifdef G_OS_WIN32
	/* Vista onwards calling RegisterHotKey registers more than one hotkey
	with the same ID, avoid this by unregistering before registering
	a new one */
	if(!key_entry->accel_key || grab_ungrab_with_ignorable_modifiers(key_entry, FALSE))
	{
		reg_succeeded = grab_ungrab_with_ignorable_modifiers(&temp_key, TRUE);
		/* if the new hotkey couldn't be set, then reset the old hotkey */
		if(!reg_succeeded)
		{
			grab_ungrab_with_ignorable_modifiers(key_entry, TRUE);
		}
#else
	/* try registering the new key combo */
	if((reg_succeeded = grab_ungrab_with_ignorable_modifiers(&temp_key, TRUE)))
	{
		/* unregister the previous hotkey */
		grab_ungrab_with_ignorable_modifiers(key_entry, FALSE);
#endif
	}

	if(reg_succeeded)
	{
		/* set the value in the list store to the newly set key combo
		so that it gets reflected in the Accel Cell Renderer */
		*key_entry = temp_key;
	}
	else
	{
		temp_str = gtk_accelerator_get_label (accel_key, accel_mods);

		msg_dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
							GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							GTK_BUTTONS_OK, 
							STR_DUPLICATE_HOTKEY, 
							temp_str);
		g_object_set(msg_dialog, "title", STR_INVALID_HOTKEY_TITLE, NULL);

		g_free (temp_str); temp_str = NULL;
		gtk_dialog_run (GTK_DIALOG (msg_dialog));
		gtk_widget_destroy (msg_dialog);

		return;
	}
}

static void accel_cleared_callback (GtkCellRendererText *cell, const char *path_string, gpointer data)
{
	GtkTreeView *view = (GtkTreeView *) data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkAccelKey *key_entry = NULL;
	GtkTreeIter iter = {0};
	GtkTreeModel *model = NULL;

	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (model, &iter, HOTKEY_COLUMN, &key_entry, -1);

	g_assert(key_entry);

	/* unregister what's previously registered */
	grab_ungrab_with_ignorable_modifiers(key_entry, FALSE);

	key_entry->accel_key = key_entry->accel_mods = key_entry->accel_flags = 0;
}

static void accel_set_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GtkAccelKey *key_entry = NULL;

	gtk_tree_model_get (model, iter, HOTKEY_COLUMN, &key_entry, -1);

	/* if key entry is not NULL, set properties of Egg Cell Renderer */
	if(key_entry)
	{
		g_object_set (cell, "visible", TRUE, /*"editable", TRUE,*/ "accel-key", key_entry->accel_key, "accel-mods", key_entry->accel_mods,
				"keycode", key_entry->accel_flags, "style", PANGO_STYLE_NORMAL, NULL);
	}
	else
		g_object_set (cell, "visible", FALSE, NULL);
}

GtkWidget* create_hotkey_editor(void)
{
	GtkWidget *hotkey_tree_view = NULL;
	GtkCellRenderer *renderer = NULL;
	GtkListStore  *model = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkTreeIter iter = {0};

	/* create a tree view */
	hotkey_tree_view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(hotkey_tree_view), FALSE);
	g_object_set(hotkey_tree_view, "visible", TRUE, NULL);

	/* create the Egg Cell Renderer and connect to signals */
	renderer = (GtkCellRenderer*) gtk_cell_renderer_accel_new ();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "accel-edited", G_CALLBACK (accel_edited_callback), hotkey_tree_view);
	g_signal_connect (renderer, "accel-cleared", G_CALLBACK (accel_cleared_callback), hotkey_tree_view);

	/* create a column and set the created renderer - append the same to the tree view */
	column = gtk_tree_view_column_new_with_attributes ("Shortcut", renderer, NULL);
	/* since it's a custom renderer, setting the data to the cell needs to be custom as well */
	gtk_tree_view_column_set_cell_data_func (column, renderer, accel_set_func, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(hotkey_tree_view), column);

	/* create the list store for the tree */
	model = gtk_list_store_new (1, G_TYPE_POINTER);
	gtk_tree_view_set_model (GTK_TREE_VIEW (hotkey_tree_view), GTK_TREE_MODEL(model));

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, HOTKEY_COLUMN, &app_hotkey, -1);

	/* The tree view has acquired its own reference to the
	 *  model, so we can drop ours. That way the model will
	 *  be freed automatically when the tree view is destroyed */
	g_object_unref (model);

	g_object_set(hotkey_tree_view, "visible", TRUE, NULL);

	return hotkey_tree_view;
}

