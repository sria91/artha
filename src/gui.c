/**
 * Artha - Free cross-platform open thesaurus
 * Copyright (C) 2009  Sundaram Ramaswamy, legends2k@yahoo.com
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
 * along with Artha; if not, see <http://www.gnu.org/licenses/>.
 */

/* gui.c: GUI Code */


// TODO: When any of the relatives list is double clicked, it should go 
// to the word's defn., as usual, but should highlight the sense which 
// linked it there. Params reqd. are id and sense, in normal case, they 
// can be unset (-1) so that no highlighting is made also scroll to the 
// highlighted sense in both double/click cases. Write a function which 
// does the mark positioning and highlighting, which can be called in 
// both click (list select) and double click. When the user presses 
// right/left, expand/collapse accordingly


#include "gui.h"


static int x_error_handler(Display *dpy, XErrorEvent *xevent);
static void key_grab(Display *dpy, guint keyval);
static void key_ungrab(Display *dpy, guint keyval);
#ifdef NOTIFY
static void notifier_clicked(NotifyNotification *notify, gchar *actionID, gpointer user_data);
static void mnuChkNotify_toggled(GtkCheckMenuItem *chkMenu, gpointer user_data);
#endif
static GdkFilterReturn hotkey_pressed(GdkXEvent *xevent, GdkEvent *event, gpointer user_data);
static void status_icon_activate(GtkStatusIcon *status_icon, gpointer user_data);
static void status_icon_popup(GtkStatusIcon *status_icon, guint button, guint active_time, gpointer user_data);
static void about_response_handle(GtkDialog *about_dialog, gint response_id);
static void about_activate(GtkWidget *menu_item, gpointer user_data);
static void quit_activate(GtkWidget *menu_item, gpointer user_data);
static guint8 get_frequency(guint sense_count);
static void relatives_clear_all(GtkBuilder *gui_builder);
static void antonyms_load(GSList *antonyms, GtkBuilder *gui_builder);
static guint8 append_term(gchar *target, gchar *term, gboolean is_heading);
static void build_tree(GNode *node, GtkTreeStore *tree_store, GtkTreeIter *parent_iter);
static void trees_load(GSList *properties, GtkBuilder *gui_builder, WNIRequestFlags id);
static void list_relatives_load(GSList *properties, GtkBuilder *gui_builder, WNIRequestFlags id);
static void domains_load(GSList *properties, GtkBuilder *gui_builder);
static guint8 get_attribute_pos();
static void relatives_load(GtkBuilder *gui_builder, gboolean reset_tabs);
static void btnSearch_click(GtkButton *button, gpointer user_data);
static void cboQuery_changed(GtkComboBox *cboQuery, gpointer user_data);
static gboolean txtDefn_button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean txtDefn_button_released(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void txtDefn_selected(GtkWidget *widget, GtkSelectionData *sel_data, guint time, gpointer user_data);
static gboolean wndMain_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static void expander_clicked(GtkExpander *expander, gpointer user_data);
static void query_list_updated(GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data);
static gboolean relative_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean relative_keyed(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static void create_stores_renderers(GtkBuilder *gui_builder);
static void mode_toggled(GtkToggleToolButton *toggle_button, gpointer user_data);
static void btnNext_clicked(GtkToolButton *toolbutton, gpointer user_data);
static void btnPrev_clicked(GtkToolButton *toolbutton, gpointer user_data);
static void setup_toolbar(GtkBuilder *gui_builder);
static GtkMenu *create_popup_menu(GtkBuilder *gui_builder);
static void create_text_view_tags(GtkBuilder *gui_builder);
static gboolean load_preferences(GtkWindow *parent, gint8 hotkey_index);
static void save_preferences(gint8 hotkey_index);
static void about_email_hook(GtkAboutDialog *about_dialog, const gchar *link, gpointer user_data);
static void about_url_hook(GtkAboutDialog *about_dialog, const gchar *link, gpointer user_data);
static void destructor(Display *dpy, guint keyval);



static int x_error_handler(Display *dpy, XErrorEvent *xevent)
{
	if(BadAccess == xevent->error_code)
	{
		g_warning("Hot key combo already occupied!\n");
	}
	else
		g_error("X Server Error: %d\n", xevent->error_code);

	x_error = True;
	return -1;
}

static void key_grab(Display *dpy, guint keyval)
{
	Window root = {0};
	guint modmask = 0;

	root = XDefaultRootWindow(dpy);

	if(root)
	{
		G_DEBUG("Root Window Obtained, Mods set!!!\n");

		// Hint: XGetModifierMapping(dpy); use it in settings for getting the user's mods
		modmask |= ControlMask | Mod1Mask;		// Make sure if ALT is always Mod1Mask, as per GDK doc., it looks so

		XGrabKey(dpy, XKeysymToKeycode(dpy, keyval), modmask, root, True, GrabModeAsync, GrabModeAsync);				//Key Combo
		XGrabKey(dpy, XKeysymToKeycode(dpy, keyval), LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);		//Key Combo with Caps
		XGrabKey(dpy, XKeysymToKeycode(dpy, keyval), Mod2Mask | modmask, root, True, GrabModeAsync, GrabModeAsync);		//Key Combo with Num
		XGrabKey(dpy, XKeysymToKeycode(dpy, keyval), Mod2Mask | LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);	//Key Combo with both locks
		// Other than Num and Caps lock, check for Scroll lock too. xmodmap -pm gives you the mapping of masks and keys.
		//Source: http://linuxtechie.wordpress.com/2008/04/07/getting-scroll-lock-to-work-in-ubuntu/

		/*XGrabKey(dpy, XKeysymToKeycode(dpy, XK_w), modmask, root, True, GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, XKeysymToKeycode(dpy, XK_w), LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);*/
	}
}

static void key_ungrab(Display *dpy, guint keyval)
{
	Window root = {0};
	guint modmask = 0;

	root = XDefaultRootWindow(dpy);
	
	modmask |= ControlMask | Mod1Mask;

	XUngrabKey(dpy, XKeysymToKeycode(dpy, keyval), modmask, root);
	XUngrabKey(dpy, XKeysymToKeycode(dpy, keyval), LockMask | modmask, root);
	XUngrabKey(dpy, XKeysymToKeycode(dpy, keyval), Mod2Mask | modmask, root);
	XUngrabKey(dpy, XKeysymToKeycode(dpy, keyval), Mod2Mask | LockMask | modmask, root);

	/*XUngrabKey(dpy, XKeysymToKeycode(dpy, XK_w), modmask, root);
	XUngrabKey(dpy, XKeysymToKeycode(dpy, XK_w), LockMask | modmask, root);*/
}

#ifdef NOTIFY
static void notifier_clicked(NotifyNotification *notify, gchar *actionID, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
	GtkWidget *cboQuery = GTK_WIDGET(gtk_builder_get_object(gui_builder, COMBO_QUERY));

	if(!g_strcmp0(actionID, "lookup"))
	{
		gtk_window_present(window);
		gtk_widget_grab_focus(GTK_WIDGET(cboQuery));
	}
	
}
#endif

static GdkFilterReturn hotkey_pressed(GdkXEvent *xevent, GdkEvent *event, gpointer user_data)
{
	XEvent *xe = (XEvent*) xevent;
	gchar *selection = NULL;
	GtkBuilder *gui_builder = NULL;
	GtkComboBoxEntry *cboQuery = NULL;
	GtkEntry *txtQuery = NULL;
	GtkButton *btnSearch = NULL;
	GtkWindow *window = NULL;

	// Since you have not registerd filter_add for NULL, no need to check the key code, you will get a call only when its the hotkey, else it won't be called
	// Try this by removing below if and try to print debug text
	if(xe->type == KeyPress)// && XKeysymToKeycode(dpy, XStringToKeysym("w")) == xe->xkey.keycode && (xe->xkey.state & ControlMask) && (xe->xkey.state & Mod1Mask))
	{
		gui_builder = GTK_BUILDER(user_data);
		selection = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
		cboQuery = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));

		if(selection)
		{
			txtQuery = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cboQuery)));
			btnSearch = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));

			if(0 != g_ascii_strncasecmp(gtk_entry_get_text(txtQuery), selection, g_utf8_strlen(selection, -1) + 1))
			{
				gtk_entry_set_text(txtQuery, selection);
				//gtk_editable_set_position(GTK_EDITABLE(txtQuery), -1);
			}
			g_free(selection);

			gtk_button_clicked(btnSearch);
		}
		else
		{
			// see if in case notify is selected, should we popup or we should just notify "No selection made!"
			window = GTK_WINDOW(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
			gtk_window_present(window);
			gtk_widget_grab_focus(GTK_WIDGET(cboQuery));
		}
		return GDK_FILTER_REMOVE;
	}

	return GDK_FILTER_CONTINUE;
}

#ifdef NOTIFY
static void mnuChkNotify_toggled(GtkCheckMenuItem *chkMenu, gpointer user_data)
{
	notifier_enabled = gtk_check_menu_item_get_active(chkMenu)?TRUE:FALSE;
}
#endif

static void status_icon_activate(GtkStatusIcon *status_icon, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
	GtkComboBoxEntry *cboQuery = NULL;
//	XEvent xe = {0};
//	GdkEvent event = {0};

	if(GTK_WIDGET_VISIBLE(window))
		gtk_widget_hide(GTK_WIDGET(window));
	else
	{
		// Code to watch clipboard when popping up
		// Should be enabled when the 'Watch Clipboard' settings is put up in Pref. window
		/*xe.type = KeyPress;
		event.key.time = GDK_CURRENT_TIME;
		hotkey_pressed(&xe, &event, gui_builder);*/
		gtk_window_present(window);
		cboQuery = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));
		gtk_widget_grab_focus(GTK_WIDGET(cboQuery));
	}
}

static void status_icon_popup(GtkStatusIcon *status_icon, guint button, guint active_time, gpointer user_data)
{
	GtkMenu *menu = GTK_MENU(user_data);
	gtk_menu_popup(menu, NULL, NULL, gtk_status_icon_position_menu, status_icon, button, active_time);
}

static void about_response_handle(GtkDialog *about_dialog, gint response_id)
{
	switch(response_id)
	{
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_DELETE_EVENT:
			gtk_widget_destroy(GTK_WIDGET(about_dialog));
			break;
		case ARTHA_RESPONSE_REPORT_BUG:
			if(fp_show_uri)
				fp_show_uri(NULL, "http://launchpad.net/artha/+filebug", GDK_CURRENT_TIME, NULL);
			break;
		default:
			g_warning("About Dialog: Unhandled response_id: %d!\n", response_id);
			break;
	}
}

static void about_activate(GtkWidget *menu_item, gpointer user_data)
{
	GtkWidget *about_dialog = gtk_about_dialog_new();
	
	g_object_set(about_dialog, "license", STRING_LICENCE, "copyright", STRING_COPYRIGHT, 
	"comments", STRING_ABOUT, "authors", strv_authors, "version", PACKAGE_VERSION, 
	"wrap-license", TRUE, "website-label", STRING_WEBSITE_LABEL, "website", STRING_WEBSITE, NULL);

	if(fp_show_uri)
		gtk_dialog_add_button(GTK_DIALOG(about_dialog), BUTTON_TEXT_BUG, ARTHA_RESPONSE_REPORT_BUG);

	g_signal_connect(about_dialog, "response", G_CALLBACK(about_response_handle), about_dialog);
	
	gtk_dialog_run(GTK_DIALOG(about_dialog));
}

static void quit_activate(GtkWidget *menu_item, gpointer user_data)
{
	G_DEBUG("Destroy called!\n");

	if(last_search)
		g_free(last_search);

	// be responsible, give back the OS what you got from it :)	
	if(results)
	{
		wni_free(&results);
		results = NULL;
	}

	gtk_main_quit();
}

static guint8 get_frequency(guint sense_count)
{
	guint8 frequency = 0;

	if (sense_count == 0) frequency = 0;
	if (sense_count == 1) frequency = 1;
	if (sense_count == 2) frequency = 2;
	if (sense_count >= 3 && sense_count <= 4) frequency = 3;
	if (sense_count >= 5 && sense_count <= 8) frequency = 4;
	if (sense_count >= 9 && sense_count <= 16) frequency = 5;
	if (sense_count >= 17 && sense_count <= 32) frequency = 6;
	if (sense_count > 32 ) frequency = 7;

	return frequency;
}

static void relatives_clear_all(GtkBuilder *gui_builder)
{
	guint8 i = 0;
	GtkTreeView *temp_tree_view = NULL;
	GtkTreeStore *temp_tree_store = NULL;
	GtkWidget *relative_tab = NULL;

	for(i = 0; i < TOTAL_RELATIVES; i++)
	{
		temp_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[i]));
		relative_tab = gtk_widget_get_parent(GTK_WIDGET(temp_tree_view));

		if(GTK_WIDGET_VISIBLE(relative_tab))
		{
			temp_tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(temp_tree_view));
			if(temp_tree_store)
				gtk_tree_store_clear(temp_tree_store);
		}
		else
			gtk_widget_show(relative_tab);
		/*temp_tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(temp_tree_view));
		if(temp_tree_store)
			gtk_tree_store_clear(temp_tree_store);

		if(!GTK_WIDGET_VISIBLE(relative_tab))
			gtk_widget_show(relative_tab);*/
	}
}

static void antonyms_load(GSList *antonyms, GtkBuilder *gui_builder)
{
	WNIAntonymItem *temp_antonym_item = NULL;
	GSList *implications = NULL;
	GtkTreeView *tree_antonyms = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[TREE_ANTONYMS]));
	GtkTreeStore *antonyms_tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_antonyms));
	GtkTreeIter iter = {0}, sub_iter = {0}, direct_iter = {0}, indirect_iter = {0};
	GtkTreePath *expand_path = NULL;
	gboolean direct_deleted = FALSE;

	g_object_ref(antonyms_tree_store);
	gtk_tree_view_set_model(tree_antonyms, NULL);

	if(advanced_mode)
	{
		gtk_tree_store_append(antonyms_tree_store, &direct_iter, NULL);
		gtk_tree_store_set(antonyms_tree_store, &direct_iter, 0, "Direct Antonyms", 1, PANGO_WEIGHT_SEMIBOLD, -1);
		gtk_tree_store_append(antonyms_tree_store, &indirect_iter, NULL);
		gtk_tree_store_set(antonyms_tree_store, &indirect_iter, 0, "Indirect Antonyms (via similar terms)", 1, PANGO_WEIGHT_SEMIBOLD, -1);
	}

	while(antonyms)
	{
		temp_antonym_item = antonyms->data;
		if(temp_antonym_item)
		{
			if(DIRECT_ANT == temp_antonym_item->relation)
			{
				if(advanced_mode)
					gtk_tree_store_append(antonyms_tree_store, &iter, &direct_iter);
				else
					gtk_tree_store_append(antonyms_tree_store, &iter, NULL);
				gtk_tree_store_set(antonyms_tree_store, &iter, 0, temp_antonym_item->term, 1, PANGO_WEIGHT_NORMAL, -1);
			}
			else
			{
				gtk_tree_store_append(antonyms_tree_store, &iter, &indirect_iter);
				gtk_tree_store_set(antonyms_tree_store, &iter, 0, temp_antonym_item->term, 1, PANGO_WEIGHT_NORMAL, -1);
			}
			implications = temp_antonym_item->implications;
			while(implications)
			{
				gtk_tree_store_append(antonyms_tree_store, &sub_iter, &iter);
				gtk_tree_store_set(antonyms_tree_store, &sub_iter, 0, ((WNIImplication*)implications->data)->term, 1, PANGO_WEIGHT_NORMAL, -1);
				implications = g_slist_next(implications);
			}
		}
		antonyms = g_slist_next(antonyms);
	}

	gtk_tree_view_set_model(tree_antonyms, GTK_TREE_MODEL(antonyms_tree_store));
	g_object_unref(antonyms_tree_store);

	if(advanced_mode)
	{
		// if only Direct is present, then Indirect will be deleted, only indirect_iter becomes invalid
		// if only Indirect is present, then Direct will be deleted, and the indirect delete won't work, because the indirect_iter will get invalid
		// if both are present, none deleted, no iters become invalid
		// if both are not present, we won't be here in the first place ;)
		// notice the NOT op. for assigning direct_deleted, which is imp. coz it says true when there are no children under Direct i.e. Direct was deleted
		if((direct_deleted = !gtk_tree_model_iter_has_child(GTK_TREE_MODEL(antonyms_tree_store), &direct_iter)))
			gtk_tree_store_remove(antonyms_tree_store, &direct_iter);
		if(!gtk_tree_model_iter_has_child(GTK_TREE_MODEL(antonyms_tree_store), &indirect_iter))
			gtk_tree_store_remove(antonyms_tree_store, &indirect_iter);

		// expand one level
		expand_path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_expand_row(tree_antonyms, expand_path, direct_deleted);	// if direct list was deleted, then this is the indirect list that we are dealing, so expand, else don't
		gtk_tree_path_free(expand_path);

		// if there is a list in this path "1", then it should be indirect
		expand_path = gtk_tree_path_new_from_indices(1, -1);
		gtk_tree_view_expand_row(tree_antonyms, expand_path, TRUE);
		gtk_tree_path_free(expand_path);
	}
}

static guint8 append_term(gchar *target, gchar *term, gboolean is_heading)
{
	guint8 i = 0;

	while(term[i] != '\0')
	{
		*target = term[i++];
		target++;
	}
	*target = (is_heading)? ':' : ',';
	++target;
	*target = ' ';

	return i + 2;
}

static void build_tree(GNode *node, GtkTreeStore *tree_store, GtkTreeIter *parent_iter)
{
	GNode *child = NULL;
	GSList *temp_list = NULL;
	WNIImplication *imp = NULL;
	GtkTreeIter tree_iter = {0};
	guint8 i = 0;
	//guint8 type_no = 0;
	gchar terms[MAX_CONCAT_STR] = "";

	if((child = node->children))
	do
	{
		i = 0;
		//type_no = 0;
		terms[0] = '\0';

		//type_no = ((WNITreeList*)child->data)->type;
		//if((type_no >= ISMEMBERPTR && type_no <= HASPARTPTR) || (type_no >= INSTANCE && type_no <= INSTANCES))
		//{
		//	if(type_no >= ISMEMBERPTR && type_no <= HASPARTPTR)
		//	{
		//		type_no -= ISMEMBERPTR;
		//		i += append_term(terms, list_type[type_no], TRUE);
		//	}
		//	else if(type_no >= INSTANCE && type_no <= INSTANCES)
		//	{
		//		type_no -= INSTANCE;
		//		i += append_term(terms, hypernym_type[type_no], TRUE);
		//	}
		//}

		temp_list = ((WNITreeList*)child->data)->word_list;
		while(temp_list)
		{
			imp = (WNIImplication*) temp_list->data;
			i += append_term(&terms[i], imp->term, FALSE);

			temp_list = g_slist_next(temp_list);
		}
		terms[i - 2] = '\0';

		gtk_tree_store_append(tree_store, &tree_iter, parent_iter);
		gtk_tree_store_set(tree_store, &tree_iter, 0, terms, 1, PANGO_WEIGHT_NORMAL, -1);

		build_tree(child, tree_store, &tree_iter);

	} while((child = child->next));
}

/*
	build_tree shall not be called for a simple mode generation of tree type lists like 
	hypernyms, hyponyms, holonyms, meronyms & pertainyms. But make sure a duplicate check is 
	done when creating the list, since they may arise coz of no duplicate check in case of 
	tree list creation. e.g. hypernym tree Sense 3 of register (v). In fact, when in simple 
	mode, hypernyms should never be displayed, since Similar does the same job already :)
*/

static void trees_load(GSList *properties, GtkBuilder *gui_builder, WNIRequestFlags id)
{
	GNode *tree = NULL;
	GtkTreeView *tree_view = NULL;
	GtkTreeStore *tree_store = NULL;
	GtkTreeIter iter = {0};
	WNIImplication *imp = NULL;
	GSList *temp_list = NULL;
	gchar terms[MAX_CONCAT_STR] = "";
	guint8 i = 0;
	//guint8 type_no = 0;
	
	if(!advanced_mode && WORDNET_INTERFACE_HYPERNYMS == id) return;

	for(i = 0; 1 != id; (id = id >> 1), i++);

	tree_view = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[i]));
	tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));

	g_object_ref(tree_store);
	gtk_tree_view_set_model(tree_view, NULL);

	while(properties)
	{
		tree = (GNode*) properties->data;
		if(tree)
		{
			tree = tree->children;
			while(tree)
			{
				i = 0;
				//type_no = 0;
				terms[0] = '\0';

				//type_no = ((WNITreeList*)tree->data)->type;
				//if((type_no >= ISMEMBERPTR && type_no <= HASPARTPTR) || (type_no >= INSTANCE && type_no <= INSTANCES))
				//{
				//	if(type_no >= ISMEMBERPTR && type_no <= HASPARTPTR)
				//	{
				//		type_no -= ISMEMBERPTR;
				//		i += append_term(terms, list_type[type_no], TRUE);
				//	}
				//	else if(type_no >= INSTANCE && type_no <= INSTANCES)
				//	{
				//		type_no -= INSTANCE;
				//		i += append_term(terms, hypernym_type[type_no], TRUE);
				//	}
				//}

				temp_list = ((WNITreeList*)tree->data)->word_list;
				if(advanced_mode)
				{
					while(temp_list)
					{
						imp = (WNIImplication*) temp_list->data;
						i += append_term(&terms[i], imp->term, FALSE);

						temp_list = g_slist_next(temp_list);
					}
					terms[i - 2] = '\0';

					gtk_tree_store_append(tree_store, &iter, NULL);
					gtk_tree_store_set(tree_store, &iter, 0, terms, 1, PANGO_WEIGHT_NORMAL, -1);
				
					build_tree(tree, tree_store, &iter);
				}
				else
				{
					while(temp_list)
					{
						imp = (WNIImplication*) temp_list->data;
						
						gtk_tree_store_append(tree_store, &iter, NULL);
						gtk_tree_store_set(tree_store, &iter, 0, imp->term, 1, PANGO_WEIGHT_NORMAL, -1);
						
						temp_list = g_slist_next(temp_list);
					}
				}
				
				tree = tree->next;
			}
		}

		properties = g_slist_next(properties);
	}

	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(tree_store));
	g_object_unref(tree_store);
}

static void list_relatives_load(GSList *properties, GtkBuilder *gui_builder, WNIRequestFlags id)
{
	guint16 i = 0;
	guint text_weight = PANGO_WEIGHT_NORMAL;
	WNIPropertyItem *temp_property = NULL;
	GtkTreeView *tree_view = NULL;
	GtkTreeStore *tree_store = NULL;
	GtkTreeIter iter = {0};

	for(i = 0; 1 != id; (id = id >> 1), i++);

	tree_view = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[i]));
	tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));

	g_object_ref(tree_store);
	gtk_tree_view_set_model(tree_view, NULL);

	while(properties)
	{
		temp_property = properties->data;
		if(temp_property)
		{
			gtk_tree_store_append(tree_store, &iter, NULL);
			text_weight = PANGO_WEIGHT_NORMAL;

			// check if synonym has more than one mapping
			if(NULL != g_slist_nth(temp_property->mapping, 2))	// we try to get the 2nd term instead of checking the length; parsing the whole list is insane
				text_weight = PANGO_WEIGHT_BOLD;

			gtk_tree_store_set(tree_store, &iter, 0, temp_property->term, 1, text_weight, -1);
		}
		properties = g_slist_next(properties);
	}

	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(tree_store));
	g_object_unref(tree_store);
}

static void domains_load(GSList *properties, GtkBuilder *gui_builder)
{
	guint8 i = 0;
	WNIClassItem *class_item = NULL;
	GtkTreeView *tree_class = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[TREE_DOMAIN]));
	GtkTreeStore *class_tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_class));
	GtkTreeIter iter = {0}, class_iter[DOMAINS_COUNT] = {};

	g_object_ref(class_tree_store);
	gtk_tree_view_set_model(tree_class, NULL);

	for(i = 0; i < DOMAINS_COUNT; i++)
	{
		gtk_tree_store_append(class_tree_store, &class_iter[i], NULL);
		gtk_tree_store_set(class_tree_store, &class_iter[i], 0, domain_types[i], 1, PANGO_WEIGHT_SEMIBOLD, -1);
	}

	while(properties)
	{
		class_item = (WNIClassItem*) properties->data;
		if(class_item)
		{
			gtk_tree_store_append(class_tree_store, &iter, &class_iter[class_item->type - CLASSIF_CATEGORY]);
			gtk_tree_store_set(class_tree_store, &iter, 0, class_item->term, 1, PANGO_WEIGHT_NORMAL, -1);
		}
		properties = g_slist_next(properties);
	}
	
	for(i = 0; i < DOMAINS_COUNT; i++)
	{
		if(!gtk_tree_model_iter_has_child(GTK_TREE_MODEL(class_tree_store), &class_iter[i]))
			gtk_tree_store_remove(class_tree_store, &class_iter[i]);
	}

	gtk_tree_view_set_model(tree_class, GTK_TREE_MODEL(class_tree_store));
	g_object_unref(class_tree_store);

	gtk_tree_view_expand_all(tree_class);
}

static guint8 get_attribute_pos()
{
	GSList *temp_results = results, *def_list = NULL;
	WNINym *temp_nym = NULL;
	WNIPropertyItem *prop_item = NULL;
	WNIPropertyMapping *prop_mapping = NULL;
	WNIDefinitionItem *def_item = NULL;
		
	while(temp_results)
	{
		temp_nym = (WNINym*) temp_results->data;
		if(WORDNET_INTERFACE_ATTRIBUTES == temp_nym->id)
		{
			prop_item = (WNIPropertyItem*) (((WNIProperties*) temp_nym->data)->properties_list)->data;
			prop_mapping = (WNIPropertyMapping*) prop_item->mapping->data;
			break;
		}
		temp_results = g_slist_next(temp_results);
	}

	temp_results = results;
	while(temp_results)
	{
		temp_nym = (WNINym*) temp_results->data;
		if(WORDNET_INTERFACE_OVERVIEW == temp_nym->id)
		{
			def_list = ((WNIOverview*) temp_nym->data)->definitions_list;
			while(def_list)
			{
				if(def_list)
				{
					def_item = (WNIDefinitionItem*) def_list->data;
					if(def_item->id == prop_mapping->id)
					{
						return def_item->pos;
					}
				}
				def_list = g_slist_next(def_list);
			}

			break;
		}
		
		temp_results = g_slist_next(temp_results);
	}
	
	return 0;	// control should never reach here
}

static void relatives_load(GtkBuilder *gui_builder, gboolean reset_tabs)
{
	GSList *temp_results = results;
	WNINym *temp_nym = NULL;

	guint8 i = 0;
	gboolean store_empty = TRUE, page_set = FALSE;
	GtkTreeView *temp_tree_view = NULL;
	GtkTreeStore *temp_tree_store = NULL;
	GtkWidget *relative_tab = NULL, *expander = NULL;
	GtkTreeIter temp_iter = {0};
	
	GtkLabel *tab_label = NULL;

	// instead of invalidate rect redraw, we just toggle visibility
	expander = GTK_WIDGET(gtk_builder_get_object(gui_builder, EXPANDER));
	gtk_widget_hide(expander);

	while(temp_results)
	{
		temp_nym = (WNINym*) temp_results->data;
		switch(temp_nym->id)
		{
			case WORDNET_INTERFACE_OVERVIEW:
			{
				list_relatives_load(((WNIOverview*) temp_nym->data)->synonyms_list, gui_builder, temp_nym->id);
				break;
			}
			case WORDNET_INTERFACE_ANTONYMS:
			{
				antonyms_load(((WNIProperties*) temp_nym->data)->properties_list, gui_builder);
				break;
			}
			case WORDNET_INTERFACE_ATTRIBUTES:
			case WORDNET_INTERFACE_DERIVATIONS:
			case WORDNET_INTERFACE_CAUSES:
			case WORDNET_INTERFACE_ENTAILS:
			case WORDNET_INTERFACE_SIMILAR:
			{
				list_relatives_load(((WNIProperties*) temp_nym->data)->properties_list, gui_builder, temp_nym->id);
				break;
			}
			case WORDNET_INTERFACE_PERTAINYMS:
			case WORDNET_INTERFACE_HYPERNYMS:
			case WORDNET_INTERFACE_HYPONYMS:
			case WORDNET_INTERFACE_HOLONYMS:
			case WORDNET_INTERFACE_MERONYMS:
			{
				trees_load(((WNIProperties*) temp_nym->data)->properties_list, gui_builder, temp_nym->id);
				break;
			}
			case WORDNET_INTERFACE_CLASS:
			{
				domains_load(((WNIProperties*) temp_nym->data)->properties_list, gui_builder);
				break;
			}
			default:
				break;
		}
		temp_results = g_slist_next(temp_results);
		i++;
	}

	// set tab visibility
	for(i = 0; i < TOTAL_RELATIVES; i++)
	{
		temp_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[i]));
		relative_tab = gtk_widget_get_parent(GTK_WIDGET(temp_tree_view));

		temp_tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(temp_tree_view));
		if(temp_tree_store)
		{
			store_empty = !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(temp_tree_store), &temp_iter);
		}
		if(store_empty || NULL == temp_tree_store)
			gtk_widget_hide(relative_tab);
		else if(reset_tabs && !page_set)
		{
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(gui_builder, NOTEBOOK)), i);
			page_set = TRUE;
		}
		
		// see if the term has attributes in first place (check visibility for that)
		// if available, set the label based on the POS
		if(GTK_WIDGET_VISIBLE(relative_tab) && TREE_ATTRIBUTES == i)
		{
			tab_label = GTK_LABEL(gtk_builder_get_object(gui_builder, LABEL_ATTRIBUTES));
			if(get_attribute_pos() == ADJ)
				gtk_label_set_text(tab_label, LABEL_TEXT_ATTRIBUTE_OF);
			else
				gtk_label_set_text(tab_label, LABEL_TEXT_ATTRIBUTES);
		}
	}

	// instead of invalidate rect redraw, we just toggle visibility
	gtk_widget_show(expander);

	// not required in all systems. At home (with compiz) it works perfectly fine without this code
	// redraw the window after visibility changes
	//if(GTK_WIDGET(gtk_builder_get_object(gui_builder, EXPANDER))->window != NULL)		// if its NULL, it means window is invisible, needn't redraw
		//gdk_window_invalidate_rect(GTK_WIDGET(gtk_builder_get_object(gui_builder, EXPANDER))->window, NULL, TRUE);
}

static void btnSearch_click(GtkButton *button, gpointer user_data)
{
	gchar *search_str = NULL, *lemma = NULL;
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkComboBox *cboQuery = GTK_COMBO_BOX(gtk_builder_get_object(gui_builder, COMBO_QUERY));
	GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(gui_builder, WINDOW_MAIN));

	GtkListStore *query_list_store = NULL;
	GtkTreeIter query_list_iter = {0};

	GtkTextView *txtDefinitions = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter cur = {0};
	GtkTextMark *freq_marker = NULL;

	GSList *def_list = NULL, *definitions_list = NULL, *example = NULL;
	WNIDefinitionItem *def_item = NULL;
	WNIDefinition *defn = NULL;

	gint16 count = 0;
	const gchar *str_new_line = "\r\n";
	gchar *str_list_item = NULL;
	gchar str_count[5] = "";

#ifdef NOTIFY
	GError *err = NULL;
	gchar *definition = NULL;
#endif

	gboolean results_set = FALSE;

	GtkStatusbar *status_bar = GTK_STATUSBAR(gtk_builder_get_object(gui_builder, STATUSBAR));
	static guint msg_context_id = 0;
	gchar status_msg[MAX_STATUS_MSG] = "";
	guint16 total_results = 0;

	txtDefinitions = GTK_TEXT_VIEW(gtk_builder_get_object(gui_builder, TEXT_VIEW_DEFINITIONS));
	buffer = gtk_text_view_get_buffer(txtDefinitions);

	search_str = g_strstrip(gtk_combo_box_get_active_text(cboQuery));

	if(search_str)
		count = g_utf8_strlen(search_str, -1);
	
	if(count > 0)
	{

#ifdef NOTIFY
			notify_notification_close(notifier, NULL);
			notify_notification_clear_actions(notifier);
#endif

		G_MESSAGE("'%s' queried!\n", search_str);

		if(last_search)
			total_results = g_ascii_strncasecmp(last_search, search_str, count + 1);

		if((last_search != NULL && total_results != 0) || (total_results == 0 && !last_search_successful) || last_search == NULL)
		{
			gtk_statusbar_pop(status_bar, msg_context_id);

			G_MESSAGE("'%s' requested from WNI!\n", search_str);

			wni_request_nyms(search_str, &results, WORDNET_INTERFACE_ALL, advanced_mode);

			// clear prior text in definitons text view & relatives
			gtk_text_buffer_set_text(buffer, "", -1);
			relatives_clear_all(gui_builder);

			if(results)
			{
				total_results = 0;
				msg_context_id = 0;

				G_MESSAGE("Results successful!\n");

				def_list = ((WNIOverview*)((WNINym*)results->data)->data)->definitions_list;
				while(def_list)
				{
					if(def_list->data)
					{
						def_item = (WNIDefinitionItem*) def_list->data;
						definitions_list = def_item->definitions;

						gtk_text_buffer_get_end_iter(buffer, &cur);
						gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, def_item->lemma, -1, "lemma", NULL);
						gtk_text_buffer_insert(buffer, &cur, " ~ ", -1);
						gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, partnames[def_item->pos], -1, "pos", NULL);

						freq_marker = gtk_text_buffer_create_mark(buffer, "familiarity_marker", &cur, TRUE);

						gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, str_new_line, -1, "pos", NULL);
				
						count = 1;
						while(definitions_list)
						{
							defn = (WNIDefinition*) definitions_list->data;
							g_snprintf(str_count, 5, "%2d. ", count);

							gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, str_count, -1, "counter", NULL);
							gtk_text_buffer_insert(buffer, &cur, defn->definition, -1);
						
							example = defn->examples;

							definitions_list = g_slist_next(definitions_list);
							if(example || definitions_list) gtk_text_buffer_insert(buffer, &cur, str_new_line, -1);

							while(example)
							{
								gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, example->data, -1, "example", NULL);
								example = g_slist_next(example);
								if(example || definitions_list) gtk_text_buffer_insert(buffer, &cur, str_new_line, -1);
							}
							count++;
						}

						gtk_text_buffer_get_iter_at_mark(buffer, &cur, freq_marker);
						gtk_text_buffer_insert(buffer, &cur, "    ", -1);
						gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, familiarity[get_frequency(count - 1)], -1, familiarity[get_frequency(count - 1)], NULL);
					}
					def_list = g_slist_next(def_list);
					if(def_list)
					{
						gtk_text_buffer_get_end_iter(buffer, &cur);
						gtk_text_buffer_insert(buffer, &cur, str_new_line, -1);
						//gtk_text_buffer_insert(buffer, &cur, str_new_line, -1);
					}

					total_results += count - 1;
					msg_context_id++;
				}

				// scroll to top code goes here
				/*gtk_text_buffer_get_start_iter(buffer, &cur);
				gtk_text_buffer_add_mark(buffer, freq_marker, &cur);
				gtk_text_view_scroll_to_mark(txtDefinitions, freq_marker, 0, FALSE, 0, 0);*/

				g_snprintf(status_msg, MAX_STATUS_MSG, "Search complete. Results returned: %d sense(s) in %d POS(s)!", total_results, msg_context_id);
				msg_context_id = gtk_statusbar_get_context_id(status_bar, "search_successful");
				gtk_statusbar_push(status_bar, msg_context_id, status_msg);

				lemma = ((WNIDefinitionItem*)((WNIOverview*)((WNINym*)results->data)->data)->definitions_list->data)->lemma;

				query_list_store = GTK_LIST_STORE(gtk_combo_box_get_model(cboQuery));
				if(query_list_store)
				{
					count = 0;
					gtk_tree_model_get_iter_first(GTK_TREE_MODEL(query_list_store), &query_list_iter);

					while(gtk_list_store_iter_is_valid(query_list_store, &query_list_iter))
					{
						gtk_tree_model_get(GTK_TREE_MODEL(query_list_store), &query_list_iter, 0, &str_list_item, -1);
						if(g_strcmp0(lemma, str_list_item) == 0)
						{
							// While in here, if you set a particular index item as active, 
							// it again calls the btnSearch_click from within as an after effect of cboQuery "changed" signal
							//gtk_combo_box_set_active(cboQuery, count);

							// move it to top, is it reqd. for history impl.? No, as it only makes 
							// Prev. key confusing by cycling thru' the same 2 items again & again.
							//gtk_list_store_move_after(query_list_store, &query_list_iter, NULL);
							
							// flag cleared to not append it again
							count = -1;
							break;
						}
						g_free(str_list_item);
						gtk_tree_model_iter_next(GTK_TREE_MODEL(query_list_store), &query_list_iter);
						count++;
					}

					if(count != -1)
					{
						gtk_list_store_prepend(query_list_store, &query_list_iter);
						gtk_list_store_set(query_list_store, &query_list_iter, 0, lemma, -1);
					}
				}

				relatives_load(gui_builder, TRUE);

				if(last_search) g_free(last_search);
				last_search = search_str;
			
				results_set = TRUE;
			}
		}

		if(results)
		{
			if(!GTK_WIDGET_VISIBLE(window))
			{
#ifdef NOTIFY
				if(notifier_enabled && notifier)
				{
					if(NULL == lemma)
						lemma = ((WNIDefinitionItem*)((WNIOverview*)((WNINym*)results->data)->data)->definitions_list->data)->lemma;

					definition = g_strconcat("<b><i>", partnames[((WNIDefinitionItem*)((WNIOverview*)((WNINym*)results->data)->data)->definitions_list->data)->pos], 
					"</i></b>. ", ((WNIDefinition*)((WNIDefinitionItem*)((WNIOverview*)((WNINym*)results->data)->data)->definitions_list->data)->definitions->data)->definition, NULL);

					notify_notification_add_action(notifier, "lookup", "Detailed Lookup", NOTIFY_ACTION_CALLBACK(notifier_clicked), gui_builder, NULL);
					if(definition)
					{
						notify_notification_update(notifier, lemma, definition, "gtk-dialog-info");
						if(FALSE == notify_notification_show(notifier, &err))
						{
							g_warning("%s\n", err->message);
						}
						g_free(definition);
					}
				}
				else
				{
#endif
					gtk_window_present(window);
#ifdef NOTIFY
				}
#endif
			}

			last_search_successful = TRUE;
		}
		else
		{
#ifdef NOTIFY
			if(!GTK_WIDGET_VISIBLE(window) && notifier_enabled && notifier)
			{
				notify_notification_update(notifier, NOTIFY_QUERY_FAIL_TITLE, NOTIFY_QUERY_FAIL_BODY, "gtk-dialog-warning");
				if(FALSE == notify_notification_show(notifier, &err))
				{
					g_warning("%s\n", err->message);
				}
			}
			else
			{
#endif
				gtk_text_buffer_set_text(buffer, "", -1);
				gtk_text_buffer_get_iter_at_offset(buffer, &cur, 0);
				gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, STR_QUERY_FAILED, -1, "pos", NULL);
				gtk_window_present(window);

				msg_context_id = gtk_statusbar_get_context_id(status_bar, "search_failed");
				gtk_statusbar_push(status_bar, msg_context_id, STR_STATUS_QUERY_FAILED);
#ifdef NOTIFY
			}
#endif
			last_search_successful = FALSE;
		}
		
		gtk_editable_select_region(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(cboQuery))), 0, -1);
		gtk_widget_grab_focus(GTK_WIDGET(cboQuery));
	}

	if(search_str && !results_set)
	{
		g_free(search_str);
	}
}

static void cboQuery_changed(GtkComboBox *cboQuery, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkButton *btnSearch = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));
	GtkToolbar *tool_bar = GTK_TOOLBAR(gtk_builder_get_object(gui_builder, TOOLBAR));
	GtkToolItem *toolbar_prev = NULL, *toolbar_next = NULL;
	GtkTreeModel *tree_model = NULL;
	gint16 selected_item = gtk_combo_box_get_active(cboQuery);
	guint total_items = 0;

	if(selected_item != -1)
	{
		gtk_button_clicked(btnSearch);
		tree_model = gtk_combo_box_get_model(cboQuery);
		total_items = gtk_tree_model_iter_n_children(tree_model, NULL);

		if(total_items > 1)
		{
			toolbar_next = gtk_toolbar_get_nth_item(tool_bar, 1);
			toolbar_prev = gtk_toolbar_get_nth_item(tool_bar, 0);
			
			if(0 == selected_item)
			{
				gtk_widget_set_sensitive(GTK_WIDGET(toolbar_prev), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(toolbar_next), FALSE);
			}
			else if(total_items == selected_item + 1)
			{
				gtk_widget_set_sensitive(GTK_WIDGET(toolbar_prev), FALSE);
				gtk_widget_set_sensitive(GTK_WIDGET(toolbar_next), TRUE);
			}
			else 
			{
				gtk_widget_set_sensitive(GTK_WIDGET(toolbar_next), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(toolbar_prev), TRUE);
			}
		}
	}
}

static gboolean txtDefn_button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if(GDK_2BUTTON_PRESS == event->type)
		was_double_click = TRUE;
	return FALSE;
}

static gboolean txtDefn_button_released(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	// call query if it was a double click or a CTRL + selection
	if(was_double_click || (GDK_CONTROL_MASK & event->state))
	{
		/* And request the "TARGETS" target for the primary selection */
		gtk_selection_convert(widget, GDK_SELECTION_PRIMARY, GDK_TARGET_STRING, event->time);
		was_double_click = FALSE;
	}
	return FALSE;
}

static void txtDefn_selected(GtkWidget *widget, GtkSelectionData *sel_data, guint time, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkComboBoxEntry *cboQuery = NULL;
	GtkEntry *txtQuery = NULL;
	GtkButton *btnSearch = NULL;
	gchar *selection = (gchar*) gtk_selection_data_get_text(sel_data);

	if(selection)
	{
		cboQuery = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));
		txtQuery = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cboQuery)));
		btnSearch = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));
	
		if(0 != g_strcmp0(gtk_entry_get_text(txtQuery), selection))
		{
			gtk_entry_set_text(txtQuery, selection);
			//gtk_editable_set_position(GTK_EDITABLE(txtQuery), -1);
		}
		g_free(selection);

		gtk_button_clicked(btnSearch);
	}
}

static gboolean wndMain_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	GtkComboBox *cboQuery = GTK_COMBO_BOX(user_data);
	gboolean popped_down = FALSE;

	if(GDK_Escape == event->keyval)
	{
		g_object_get(cboQuery, "popup-shown", &popped_down, NULL);
		if(!popped_down)
		{
			gtk_widget_hide(widget);
			return TRUE;
		}
	}
	return FALSE;
}

static void expander_clicked(GtkExpander *expander, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkPaned *vpaned = GTK_PANED(gtk_builder_get_object(gui_builder, "vpaned1"));
	gtk_paned_set_position(vpaned, -1);		// unset the pane's position so that it goes to the original state
}

static void query_list_updated(GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkToolbar *tool_bar = GTK_TOOLBAR(gtk_builder_get_object(gui_builder, TOOLBAR));
	GtkToolItem *toolbar_prev = gtk_toolbar_get_nth_item(tool_bar, 0);
	GtkToolItem *toolbar_next = gtk_toolbar_get_nth_item(tool_bar, 1);
	GtkTreeIter temp_iter = {0};

	if(gtk_tree_model_iter_nth_child(tree_model, &temp_iter, NULL, 1))
	{
		gtk_widget_set_sensitive(GTK_WIDGET(toolbar_prev), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(toolbar_next), FALSE);
	}
}

static gboolean relative_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkTreeView *tree_view = NULL;
	GtkTreeModel *tree_store = NULL;
	GtkTreeIter iter = {0};
	gchar *term = NULL;
	GtkComboBoxEntry *cboQuery = NULL;
	GtkEntry *txtQuery = NULL;
	GtkButton *btnSearch = NULL;

	if(GDK_2BUTTON_PRESS == event->type)
	{
		tree_view = GTK_TREE_VIEW(widget);
		if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(tree_view), &tree_store, &iter))
		{
			gtk_tree_model_get(tree_store, &iter, 0, &term, -1);
			
			cboQuery = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));
			txtQuery = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cboQuery)));
			btnSearch = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));
	
			gtk_entry_set_text(txtQuery, term);

			g_free(term);

			gtk_button_clicked(btnSearch);

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean relative_keyed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkTreeView *tree_view = NULL;
	GtkTreeModel *tree_store = NULL;
	GtkTreeIter iter = {0};
	gchar *term = NULL;
	GtkComboBoxEntry *cboQuery = NULL;
	GtkEntry *txtQuery = NULL;
	GtkButton *btnSearch = NULL;

	if(GDK_Return == event->keyval)
	{
		tree_view = GTK_TREE_VIEW(widget);
		if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(tree_view), &tree_store, &iter))
		{
			gtk_tree_model_get(tree_store, &iter, 0, &term, -1);
			
			cboQuery = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));
			txtQuery = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cboQuery)));
			btnSearch = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));
	
			gtk_entry_set_text(txtQuery, term);

			g_free(term);

			gtk_button_clicked(btnSearch);

			return TRUE;
		}
	}

	return FALSE;
}

static void create_stores_renderers(GtkBuilder *gui_builder)
{
	guint8 i = 0;
	GtkComboBox *cboQuery = NULL;
	GtkListStore *list_store_query = NULL;

	GtkTreeView *tree_view = NULL;
	GtkTreeStore *tree_store = NULL;
	GtkCellRenderer *tree_renderer = NULL;
	gchar *col_name = NULL;
	
	// combo box data store
	cboQuery = GTK_COMBO_BOX(gtk_builder_get_object(gui_builder, COMBO_QUERY));
	list_store_query = gtk_list_store_new(1, G_TYPE_STRING);
	g_signal_connect(GTK_TREE_MODEL(list_store_query), "row-inserted", G_CALLBACK(query_list_updated), gui_builder);
	gtk_combo_box_set_model(cboQuery, GTK_TREE_MODEL(list_store_query));
	gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(cboQuery), 0);
	g_object_unref(list_store_query);
	
	for(i = 0; i < TOTAL_RELATIVES; i++)
	{
		tree_view = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[i]));
		tree_renderer = gtk_cell_renderer_text_new();
		col_name = relative_tree[i];
		col_name = col_name + 4;	// skip the 1st 4 chars "tree" in relative_tree string array
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, col_name, tree_renderer, "text", 0, "weight", 1, NULL);
		tree_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_UINT);
		gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(tree_store));
		g_object_unref(tree_store);

		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);

		g_signal_connect(tree_view, "button-press-event", G_CALLBACK(relative_clicked), gui_builder);
		g_signal_connect(tree_view, "key-release-event", G_CALLBACK(relative_keyed), gui_builder);
	}
}

static void mode_toggled(GtkToggleToolButton *toggle_button, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);

	advanced_mode = gtk_toggle_tool_button_get_active(toggle_button);

	if(last_search && last_search_successful)
	{
		G_MESSAGE("Re-requesting with changed mode: %d\n", advanced_mode);

		wni_request_nyms(last_search, &results, WORDNET_INTERFACE_ALL, advanced_mode);
		relatives_clear_all(gui_builder);
		relatives_load(gui_builder, FALSE);
	}
}

static void btnNext_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkComboBox *cboQuery = GTK_COMBO_BOX(gtk_builder_get_object(gui_builder, COMBO_QUERY));
	gint16 active_item = gtk_combo_box_get_active(cboQuery);
	GtkTreeModel *tree_model = NULL;

	if(-1 == active_item)
	{
		tree_model = gtk_combo_box_get_model(cboQuery);
		active_item = gtk_tree_model_iter_n_children(tree_model, NULL);
	}

	gtk_combo_box_set_active(cboQuery, active_item - 1);
}

static void btnPrev_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkComboBox *cboQuery = GTK_COMBO_BOX(gtk_builder_get_object(gui_builder, COMBO_QUERY));
	gint16 active_item = gtk_combo_box_get_active(cboQuery);

	if(-1 == active_item)
	{
		active_item = 0;
	}

	gtk_combo_box_set_active(cboQuery, active_item + 1);
}

static void setup_toolbar(GtkBuilder *gui_builder)
{
	GtkToolbar *toolbar = NULL;
	GtkToolItem *toolbar_quit = NULL;
	GtkToolItem *toolbar_about = NULL;
	GtkToolItem *toolbar_mode = NULL;
	GtkToolItem *toolbar_prev = NULL;
	GtkToolItem *toolbar_next = NULL;
	GtkToolItem *toolbar_sep = NULL;

	// toolbar code starts here
	toolbar = GTK_TOOLBAR(gtk_builder_get_object(gui_builder, TOOLBAR));
	
	toolbar_quit = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_quit), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_quit), "Q_uit");
	gtk_tool_item_set_tooltip_text(toolbar_quit, QUIT_TOOLITEM_TOOLTIP);
	g_signal_connect(toolbar_quit, "clicked", G_CALLBACK(quit_activate), NULL);
	gtk_toolbar_insert(toolbar, toolbar_quit, 0);

	toolbar_sep = gtk_separator_tool_item_new();
	gtk_toolbar_insert(toolbar, toolbar_sep, 0);

	toolbar_about = gtk_tool_button_new_from_stock(GTK_STOCK_ABOUT);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_about), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_about), "_About");
	gtk_tool_item_set_tooltip_text(toolbar_about, ABOUT_TOOLITEM_TOOLTIP);
	g_signal_connect(toolbar_about, "clicked", G_CALLBACK(about_activate), NULL);
	gtk_toolbar_insert(toolbar, toolbar_about, 0);

	toolbar_sep = gtk_separator_tool_item_new();
	gtk_toolbar_insert(toolbar, toolbar_sep, 0);

	toolbar_mode = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_DIALOG_INFO);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_mode), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_mode), "_Detailed");
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolbar_mode), advanced_mode);
	gtk_tool_item_set_tooltip_text(toolbar_mode, MODE_TOOLITEM_TOOLTIP);
	g_signal_connect(toolbar_mode, "toggled", G_CALLBACK(mode_toggled), gui_builder);
	gtk_toolbar_insert(toolbar, toolbar_mode, 0);

	toolbar_sep = gtk_separator_tool_item_new();
	gtk_toolbar_insert(toolbar, toolbar_sep, 0);

	toolbar_next = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_next), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_next), "_Next");
	gtk_tool_item_set_tooltip_text(toolbar_next, NEXT_TOOLITEM_TOOLTIP);
	gtk_widget_set_sensitive(GTK_WIDGET(toolbar_next), FALSE);
	g_signal_connect(toolbar_next, "clicked", G_CALLBACK(btnNext_clicked), gui_builder);
	gtk_toolbar_insert(toolbar, toolbar_next, 0);

	toolbar_prev = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_prev), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_prev), "_Previous");
	gtk_tool_item_set_tooltip_text(toolbar_prev, PREV_TOOLITEM_TOOLTIP);
	gtk_widget_set_sensitive(GTK_WIDGET(toolbar_prev), FALSE);
	g_signal_connect(toolbar_prev, "clicked", G_CALLBACK(btnPrev_clicked), gui_builder);
	gtk_toolbar_insert(toolbar, toolbar_prev, 0);

	gtk_widget_show_all(GTK_WIDGET(toolbar));
}

static GtkMenu *create_popup_menu(GtkBuilder *gui_builder)
{
	GtkMenu *menu = NULL;
#ifdef NOTIFY
	GtkSeparatorMenuItem *mnuSeparator = NULL;
	GtkCheckMenuItem *mnuChkNotify = NULL;
#endif
	GtkImageMenuItem *mnuQuit = NULL;
	
	// Initialize popup menu/sub menus
	menu = GTK_MENU(gtk_menu_new());

#ifdef NOTIFY
	// if there was no error in registering for a hot key, then setup a notifications menu
	if(False == x_error)
	{
		mnuChkNotify = GTK_CHECK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic("_Notifications"));
		// load the settings value
		gtk_check_menu_item_set_active(mnuChkNotify, notifier_enabled);

		mnuSeparator = GTK_SEPARATOR_MENU_ITEM(gtk_separator_menu_item_new());
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(mnuChkNotify));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(mnuSeparator));
		g_signal_connect(mnuChkNotify, "toggled", G_CALLBACK(mnuChkNotify_toggled), NULL);
	}
#endif

	mnuQuit = GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(mnuQuit));

	g_signal_connect(mnuQuit, "activate", G_CALLBACK(quit_activate), NULL);

	gtk_widget_show_all(GTK_WIDGET(menu));

	return menu;
}

static void create_text_view_tags(GtkBuilder *gui_builder)
{
	guint8 i = 0;
	GtkTextView *txtDefn = NULL;
	GtkTextBuffer *txtDefn_buffer = NULL;

	// create text view tags
	txtDefn = GTK_TEXT_VIEW(gtk_builder_get_object(gui_builder, TEXT_VIEW_DEFINITIONS));
	txtDefn_buffer = gtk_text_view_get_buffer(txtDefn);
	gtk_text_buffer_create_tag(txtDefn_buffer, "lemma", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(txtDefn_buffer, "pos", "foreground", "red", "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_buffer_create_tag(txtDefn_buffer, "counter", "left_margin", 15, "weight", PANGO_WEIGHT_BOLD, "foreground", "gray", NULL);
	gtk_text_buffer_create_tag(txtDefn_buffer, "example", "foreground", "blue", "font", "FreeSerif Italic 11", "left_margin", 45, NULL);

	g_signal_connect(txtDefn, "button-press-event", G_CALLBACK(txtDefn_button_pressed), gui_builder);					
	g_signal_connect(txtDefn, "button-release-event", G_CALLBACK(txtDefn_button_released), gui_builder);
	g_signal_connect(txtDefn, "selection-received", G_CALLBACK(txtDefn_selected), gui_builder);
	
	for(i = 0; i < FAMILIARITY_COUNT; i++)
	{
		gtk_text_buffer_create_tag(txtDefn_buffer, familiarity[i], "background", freq_colors[i], 
		"foreground", (i < 4)? "white" : "black", "font", "Monospace 9", NULL);
	}
}

static gboolean load_preferences(GtkWindow *parent, gint8 hotkey_index)
{
	gchar *conf_file_path = NULL;
	GKeyFile *key_file = NULL;
	GtkWidget *welcome_dialog = NULL;
	gchar *welcome_message = NULL;
	gchar welcome_title[] = WELCOME_TITLE;
	gint prev_hotkey_index = 0;
	gboolean first_run = FALSE;

	gchar *version = NULL;
	gchar **version_numbers = NULL;
	gchar **current_version_numbers = NULL;
	guint8 i = 0;

	conf_file_path = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S, PACKAGE_TARNAME, ".conf", NULL);
	key_file = g_key_file_new();

	if(g_key_file_load_from_file(key_file, conf_file_path, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		prev_hotkey_index = g_key_file_get_integer(key_file, GROUP_SETTINGS, KEY_HOTKEY_INDEX, NULL);
		advanced_mode = g_key_file_get_boolean(key_file, GROUP_SETTINGS, KEY_MODE, NULL);
#ifdef NOTIFY
		notifier_enabled = g_key_file_get_boolean(key_file, GROUP_SETTINGS, KEY_NOTIFICATIONS, NULL);
#endif

		version = g_key_file_get_string(key_file, GROUP_SETTINGS, KEY_VERSION, NULL);

		if(version)
		{
			if(0 != g_strcmp0(version, PACKAGE_VERSION))
			{
				// version number is split here. Major, Minor, Micro
				version_numbers = g_strsplit(version, ".", 3);
				current_version_numbers = g_strsplit(PACKAGE_VERSION, ".", 3);
				
				for(i = 0; i < 3; i++)
				{
					if(g_ascii_digit_value(version_numbers[i][0]) < g_ascii_digit_value(current_version_numbers[i][0]))
					{
						first_run = TRUE;
						break;
					}
				}
				
				g_strfreev(version_numbers);
				g_strfreev(current_version_numbers);
			}
			g_free(version);
		}

		// if previously set hot key got changed, or unset, notify user
		// or also hot key value couldn't be retrived from prefs file
		if(hotkey_index != prev_hotkey_index)
		{
			// hot key was set earlier and now couldn't be set
			if(True == x_error)
			{
				welcome_message = g_strconcat(first_run ? WELCOME_UPGRADED : WELCOME_TITLE, "\n\n", WELCOME_NOHOTKEY, WELCOME_MANUAL, NULL);
				welcome_dialog = gtk_message_dialog_new_with_markup(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, welcome_message, NULL);
			}
			
			// hot key was earlier either unset or was diff.
			else if(False == x_error)
			{
				welcome_message = g_strconcat(first_run ? WELCOME_UPGRADED : WELCOME_TITLE,  "\n\n", WELCOME_NOTE_HOTKEY_CHANGED, WELCOME_HOTKEY_INFO, 
#ifdef NOTIFY
				WELCOME_NOTIFY,
#endif
				WELCOME_MANUAL, NULL);

				// subtract hot key value by 32 to get upper case char
				welcome_dialog = gtk_message_dialog_new_with_markup(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, welcome_message, hot_key_vals[hotkey_index - 1] - 32);
			}			
		}
		else
		{
			// Artha was updated to a newer version
			if(first_run)
			{
				welcome_message = g_strconcat(WELCOME_UPGRADED, "\n\n", WELCOME_HOTKEY_NORMAL, WELCOME_HOTKEY_INFO,
#ifdef NOTIFY
				WELCOME_NOTIFY,
#endif
				WELCOME_MANUAL, NULL);

				welcome_dialog = gtk_message_dialog_new_with_markup(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, welcome_message, hot_key_vals[hotkey_index - 1] - 32, NULL);
			}
		}
	}
	else
	{
		first_run = TRUE;

		G_DEBUG("Preference file not found @ %s!\n", conf_file_path);
		G_DEBUG("User runs Artha for the first time!");
		
		if(False == x_error)
		{
			welcome_message = g_strconcat(WELCOME_HOTKEY_NORMAL, WELCOME_HOTKEY_INFO, 
#ifdef NOTIFY
			WELCOME_NOTIFY,
#endif
			WELCOME_MANUAL,
			NULL);

			// subtract hot key value by 32 to get upper case char
			welcome_dialog = gtk_message_dialog_new_with_markup(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, welcome_message, hot_key_vals[hotkey_index - 1] - 32);
		}
		else
		{
			welcome_message = g_strconcat(WELCOME_NOHOTKEY, WELCOME_MANUAL, NULL);

			welcome_dialog = gtk_message_dialog_new_with_markup(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, welcome_message, NULL);
		}

		// The user might never close the app. and just shut down the system. Conf file won't be written to disk in that case
		// Hence save the conf file when its a first run with the defaults
		save_preferences(hotkey_index);
	}
	
	// If some welcome message is set, show the message box and free it
	if(NULL != welcome_message)
	{
		g_object_set(welcome_dialog, "title", welcome_title, NULL);

		gtk_dialog_run(GTK_DIALOG(welcome_dialog));
		gtk_widget_destroy(welcome_dialog);

		g_free(welcome_message);
	}

	g_key_file_free(key_file);
	g_free(conf_file_path);
	
	return first_run;
}

static void save_preferences(gint8 hotkey_index)
{
	gchar *conf_file_path = NULL;
	GKeyFile *key_file = NULL;
	gchar *file_contents = NULL;
	gsize file_len = 0;

	conf_file_path = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S, PACKAGE_TARNAME, ".conf", NULL);
	key_file = g_key_file_new();
	
	g_key_file_set_comment(key_file, NULL, NULL, SETTINGS_COMMENT, NULL);
	
	g_key_file_set_string(key_file, GROUP_SETTINGS, KEY_VERSION, PACKAGE_VERSION);
	g_key_file_set_integer(key_file, GROUP_SETTINGS, KEY_HOTKEY_INDEX, hotkey_index);
	g_key_file_set_boolean(key_file, GROUP_SETTINGS, KEY_MODE, advanced_mode);

#ifdef NOTIFY
	g_key_file_set_boolean(key_file, GROUP_SETTINGS, KEY_NOTIFICATIONS, notifier_enabled);
#endif
	
	file_contents = g_key_file_to_data(key_file, &file_len, NULL);
	
	if(!g_file_set_contents(conf_file_path, file_contents, file_len, NULL))
		g_warning("Failed to save preferences to %s\n", conf_file_path);
	
	g_free(file_contents);

	g_key_file_free(key_file);
	g_free(conf_file_path);
}

static void about_email_hook(GtkAboutDialog *about_dialog, const gchar *link, gpointer user_data)
{
	GError *error = NULL;
	gchar *final_uri = NULL;

	final_uri = g_strconcat(MAILTO_PREFIX, link, NULL);

	if(!fp_show_uri(gtk_widget_get_screen(GTK_WIDGET(about_dialog)), final_uri, GDK_CURRENT_TIME, &error))
	{
		g_warning("Email gtk_show_uri error: %s\n", error->message);
	}

	g_free(final_uri);
}

static void about_url_hook(GtkAboutDialog *about_dialog, const gchar *link, gpointer user_data)
{
	GError *error = NULL;

	if(!fp_show_uri(gtk_widget_get_screen(GTK_WIDGET(about_dialog)), link, GDK_CURRENT_TIME, &error))
	{
		g_warning("URL gtk_show_uri error: %s\n", error->message);
	}
}

static void destructor(Display *dpy, guint keyval)
{
	// If XKeyGrab succeeded, make sure you XUnKeyGrab
	if(False == x_error)
	{
		// Unregister the hot key callback with XServer
		gdk_window_remove_filter(gdk_get_default_root_window(), hotkey_pressed, NULL);
		key_ungrab(dpy, keyval);
	}

#ifdef NOTIFY
	notify_notification_close(notifier, NULL);
	notify_notification_clear_actions(notifier);

	if(notifier)
	{
		g_object_unref(G_OBJECT(notifier));
		notifier = NULL;
	}
	notify_uninit();
#endif
}

int main(int argc, char *argv[])
{
	Display *dpy = NULL;

	GtkBuilder *gui_builder = NULL;
	GtkWidget *window = NULL, *btnSearch = NULL, *cboQuery = NULL;
	GtkStatusIcon *statusIcon = NULL;
	
	GtkMenu *popup_menu = NULL;

	GtkExpander *expander = NULL;
	
	GdkPixbuf *app_icon = NULL;
	
	gchar *ui_file_path = NULL, *icon_file_path = NULL;
	
	GModule *app_mod = NULL;
	gpointer func_pointer = NULL;

	gboolean first_run = FALSE;
	gint8 selected_key = 0;


	if(!g_thread_supported())
	{
		G_DEBUG("Calling g_thread_init()!\n");
		g_thread_init(NULL);
#ifdef NOTIFY
		// do this! if removed, when the user selects text in our own txt entry, and press hotkey, app. hangs
		dbus_g_thread_init();
#endif
	}

	g_set_application_name(PACKAGE_NAME);

	if(gtk_init_check(&argc, &argv))
	{

		gui_builder = gtk_builder_new();
		if(gui_builder)
		{
			ui_file_path = g_build_filename(APP_DIR, UI_FILE, NULL);
			if(gtk_builder_add_from_file(gui_builder, ui_file_path, NULL))
			{
				window = GTK_WIDGET(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
				if(window)
				{
					// Most Important: do not use the XServer's XGetDisplay, you will have to do XNextEvent (blocking) to 
					// get the event call, so get GDK's display and its X's display equivalent
					if (! ( dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default()) ) )
					{
						g_error("Can't open Display %s!\n", gdk_display_get_name(gdk_display_get_default()));
						return -1;
					}

					G_DEBUG("XDisplay Opened!\n");

					XSynchronize(dpy, True);		// Without calling this you get the error call back dealyed, much delayed!
					XSetErrorHandler(x_error_handler);	// Set the error handler for setting the flag

					while(selected_key < (sizeof(hot_key_vals) / sizeof(guint)) )
					{
						key_grab(dpy, hot_key_vals[selected_key]);	// Register with XServer for hotkey callback

						if(False == x_error)
						{
							// subtract hot key value by 32 to get the char in upper case
							G_DEBUG("Hotkey (Ctrl + Alt + %c) register succeeded!\n", hot_key_vals[selected_key] - 32);

							// Add a filter function to get low level event handling, like X events.
							// For Param1 use gdk_get_default_root_window() instead of NULL or window, so that
							// only when hotkey combo is pressed the filter func. will be called, unlike
							// others, where it will be called for all events beforehand GTK handles
							gdk_window_add_filter(gdk_get_default_root_window(), hotkey_pressed, gui_builder);
							
							break;
						}
						else
							x_error = False;
						
						selected_key++;
					}
					
					if(selected_key == (sizeof(hot_key_vals) / sizeof(guint)) ) x_error = True;

					if(True == x_error) selected_key = -2;

					first_run = load_preferences(GTK_WINDOW(window), selected_key + 1);

					// create popup menu
					popup_menu = create_popup_menu(gui_builder);

					icon_file_path = g_build_filename(ICON_DIR, ICON_FILE, NULL);
					if(!g_file_test(icon_file_path, G_FILE_TEST_IS_REGULAR))
					{
						g_error("Error loading icon file!\n%s not found!\n", ui_file_path);
					}
					
					// setup status icon
					statusIcon = gtk_status_icon_new_from_file(icon_file_path);
					gtk_status_icon_set_tooltip(statusIcon, "Artha ~ The Open Thesaurus");
					gtk_status_icon_set_visible(statusIcon, TRUE);
					
					g_free(icon_file_path);
					
					g_signal_connect(statusIcon, "activate", G_CALLBACK(status_icon_activate), gui_builder);
					g_signal_connect(statusIcon, "popup-menu", G_CALLBACK(status_icon_popup), GTK_WIDGET(popup_menu));

					// using the status icon, app. icon is also set
					g_object_get(statusIcon, "pixbuf", &app_icon, NULL);
					gtk_window_set_default_icon(app_icon);
					g_object_unref(app_icon);

#ifdef NOTIFY
					notify_init(PACKAGE_NAME);
					notifier = notify_notification_new_with_status_icon("Word", "Definition of the <b>word</b> in <u>Wordnet</u>", "gtk-dialog-info", statusIcon);
					notify_notification_set_urgency(notifier, NOTIFY_URGENCY_LOW);
					notify_notification_set_hint_byte(notifier, "persistent", (guchar) FALSE);
#endif
					btnSearch = GTK_WIDGET(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));
					g_signal_connect(btnSearch, "clicked", G_CALLBACK(btnSearch_click), gui_builder);

					cboQuery = GTK_WIDGET(gtk_builder_get_object(gui_builder, COMBO_QUERY));
					g_signal_connect(cboQuery, "changed", G_CALLBACK(cboQuery_changed), gui_builder);


					create_text_view_tags(gui_builder);

					setup_toolbar(gui_builder);

					// do main window specific connects
					g_signal_connect(window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
					
					// connect to Escape Key
					g_signal_connect(window, "key-press-event", G_CALLBACK(wndMain_key_press), cboQuery);

					
					expander = GTK_EXPANDER(gtk_builder_get_object(gui_builder, EXPANDER));
					g_signal_connect(expander, "activate", G_CALLBACK(expander_clicked), gui_builder);

					gtk_widget_grab_focus(GTK_WIDGET(cboQuery));

					G_DEBUG("GUI loaded successfully!\n");
					
					create_stores_renderers(gui_builder);

					app_mod = g_module_open(NULL, G_MODULE_BIND_LAZY);
					if(app_mod)
					{
						if(g_module_symbol(app_mod, "gtk_show_uri", &func_pointer))
						{
							fp_show_uri = (ShowURIFunc) &func_pointer;

							gtk_about_dialog_set_email_hook(about_email_hook, fp_show_uri, NULL);
							gtk_about_dialog_set_url_hook(about_url_hook, fp_show_uri, NULL);
						}
						else
							G_DEBUG("Cannot find gtk_show_uri function symbol!\n");

						g_module_close(app_mod);
					}
					else
						g_warning("Cannot open module!\n");

					// set via GUI.ui
					//gtk_window_set_startup_id(GTK_WINDOW(window), APP_ID);

					// To show or not to show? on startup - better show it the very first time, when the user sets the option
					// start up hided, then don't show it
					if(first_run) gtk_widget_show_all(window);

					
					gtk_main();

					save_preferences(selected_key + 1);
	
					gtk_widget_destroy(window);
					
					destructor(dpy, hot_key_vals[selected_key]);
				}
				else
				{
					g_error("Error loading GUI!\n. Corrupted data in UI file!\n");
				}
			}
			else
			{
				g_error("Error loading GUI!\n%s not found!\n", ui_file_path);
			}
			g_free(ui_file_path);
		}
		else
		{
			g_error("Error creating GtkBuilder!\n");
		}
	}
	else
	{
		g_error("Error initializing GUI!\n");
	}

	return 0;
}

