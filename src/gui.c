/* gui.c
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
 * GUI Code
 */


#include "gui.h"


static int x_error_handler(Display *dpy, XErrorEvent *xevent);
static void notification_toggled(GObject *obj, gpointer user_data);
static gchar* strip_invalid_edges(gchar *selection);
static GdkFilterReturn hotkey_pressed(GdkXEvent *xevent, GdkEvent *event, gpointer user_data);
static void status_icon_activate(GtkStatusIcon *status_icon, gpointer user_data);
static void status_icon_popup(GtkStatusIcon *status_icon, guint button, guint active_time, gpointer user_data);
static void about_response_handle(GtkDialog *about_dialog, gint response_id, gpointer user_data);
static void about_activate(GtkToolButton *menu_item, gpointer user_data);
static gboolean quit_activate(GObject *obj, gpointer user_data);
static guint8 get_frequency(guint sense_count);
static void relatives_clear_all(GtkBuilder *gui_builder);
static void antonyms_load(GSList *antonyms, GtkBuilder *gui_builder);
static guint16 append_term(gchar *target, gchar *term, gboolean is_heading);
static void build_tree(GNode *node, GtkTreeStore *tree_store, GtkTreeIter *parent_iter);
static void trees_load(GSList *properties, GtkBuilder *gui_builder, WNIRequestFlags id);
static void list_relatives_load(GSList *properties, GtkBuilder *gui_builder, WNIRequestFlags id);
static void domains_load(GSList *properties, GtkBuilder *gui_builder);
static guint8 get_attribute_pos();
static void relatives_load(GtkBuilder *gui_builder, gboolean reset_tabs);
static gchar* wildmat_to_regex(gchar *wildmat);
static void set_regex_results(gchar *wildmat_exp, GtkBuilder *gui_builder);
static gboolean is_wildmat_expr();
static void button_search_click(GtkButton *button, gpointer user_data);
static void combo_query_changed(GtkComboBox *combo_query, gpointer user_data);
static gboolean text_view_button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean text_view_button_released(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void text_view_selection_made(GtkWidget *widget, GtkSelectionData *sel_data, guint time, gpointer user_data);
static gboolean window_main_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static void expander_clicked(GtkExpander *expander, gpointer user_data);
static void query_list_updated(GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data);
static GtkTextMark *highlight_definition(guint8 id, guint16 sense, GtkTextView *text_view);
static void highlight_senses_from_synonyms(guint16 relative_index, GtkTextView *text_view);
static void highlight_senses_from_antonyms(guint8 category, guint16 relative_index, guint8 sub_level, GtkTextView *text_view);
static void highlight_senses_from_domain(guint8 category, guint16 relative_index, GtkTextView *text_view);
static void highlight_senses_from_relative_lists(WNIRequestFlags id, guint16 relative_index, GtkTextView *text_view);
static void relative_selection_changed(GtkTreeView *tree_view, gpointer user_data);
static void relative_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);
static void create_stores_renderers(GtkBuilder *gui_builder);
static void mode_toggled(GtkToggleToolButton *toggle_button, gpointer user_data);
static void button_next_clicked(GtkToolButton *toolbutton, gpointer user_data);
static void button_prev_clicked(GtkToolButton *toolbutton, gpointer user_data);
static gboolean combo_query_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
static void setup_toolbar(GtkBuilder *gui_builder, GtkWidget *hotkey_editor_dialog);
static GtkMenu *create_popup_menu(GtkBuilder *gui_builder);
static void create_text_view_tags(GtkBuilder *gui_builder);
static gboolean load_preferences(GtkWindow *parent);
static void save_preferences();
static void about_email_hook(GtkAboutDialog *about_dialog, const gchar *link, gpointer user_data);
static void about_url_hook(GtkAboutDialog *about_dialog, const gchar *link, gpointer user_data);
static gboolean wordnet_terms_load(GtkBuilder *gui_builder);
static void lookup_ignorable_modifiers(void);
gboolean grab_ungrab_with_ignorable_modifiers (GtkAccelKey *binding, gboolean grab);
static gboolean register_unregister_hotkey(gboolean first_run, gboolean setup_hotkey);
static void setup_hotkey_editor(GtkBuilder *gui_builder, GtkWidget **hotkey_editor_dialog);
static void show_hotkey_editor(GtkToolButton *toolbutton, gpointer user_data);
static void destructor(GtkBuilder *gui_builder);
static void show_message_dlg(GtkWidget *parent_window, MessageResposeCode msg_code);


static int x_error_handler(Display *dpy, XErrorEvent *xevent)
{
	if(BadAccess == xevent->error_code)
	{
		g_warning("Hotkey combo already occupied!\n");
	}
	else
		g_error("X Server Error: %d\n", xevent->error_code);

	x_error = True;
	return -1;
}

/*
	This will be called for both notify check pop-up menu and notify tool bar button
	Hence the first argument is GObject. Since both have the propery "active" as its
	current state, this common function will suffice for notification toggling. 
*/
static void notification_toggled(GObject *obj, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkToolbar *toolbar = GTK_TOOLBAR(gtk_builder_get_object(gui_builder, TOOLBAR));
	GtkToolItem *toolbar_notify = gtk_toolbar_get_nth_item(toolbar, notify_toolbar_index);
	gboolean prev_state_of_notification = notifier_enabled;		// flag to prevent showing hotkey-not-set alert twice

	g_object_get(obj, "active", &notifier_enabled, NULL);

	g_object_set(G_OBJECT(toolbar_notify), "active", notifier_enabled, NULL);
	//gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolbar_notify), notifier_enabled);
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(toolbar_notify), notifier_enabled ? GTK_STOCK_YES : GTK_STOCK_NO);

	gtk_check_menu_item_set_active(menu_notify, notifier_enabled);

	save_preferences();

	/* if not hotkey is set; then there's no point in enabling notifications
	   intimate this to the user */
	if(!hotkey_set && notifier_enabled && prev_state_of_notification != notifier_enabled)
		show_message_dlg(NULL, MSG_HOTKEY_NOTSET);
}

static gchar* strip_invalid_edges(gchar *selection)
{
	guint i = 0;
	gboolean alphanum_encounterd = FALSE;

	if(selection)
	{
		while(selection[i] != '\0')
		{
			if(!g_ascii_isalnum(selection[i]) && selection[i] != '-' && selection[i] != '_' && 
			selection[i] != ' ' && selection[i] != '\'' && selection[i] != '.')
			{
				if(alphanum_encounterd)
				{
					selection[i] = '\0';
					break;
				}
				else
				{
					selection[i] = ' ';
				}
			}
			else
			{
				// before setting the alphanum_encountered, make sure the current char isn't other valid ones
				if(!alphanum_encounterd && selection[i] != '-' && selection[i] != '_' && selection[i] != ' ' && 
				selection[i] != '\'' && selection[i] != '.')
					alphanum_encounterd = TRUE;
			}
			i++;
		}
	}
	
	return selection;
}

static GdkFilterReturn hotkey_pressed(GdkXEvent *xevent, GdkEvent *event, gpointer user_data)
{
	XEvent *xe = (XEvent*) xevent;
	gchar *selection = NULL;
	GtkBuilder *gui_builder = NULL;
	GtkComboBoxEntry *combo_query = NULL;
	GtkEntry *text_entry_query = NULL;
	GtkButton *button_search = NULL;
	GtkWindow *window = NULL;

	// Since you have not registerd filter_add for NULL, no need to check the key code, you will get a call only when its the hotkey, else it won't be called
	// Try this by removing below if and try to print debug text
	if(xe->type == KeyPress)
	{
		hotkey_processing = TRUE;
		last_hotkey_time = xe->xkey.time;

		gui_builder = GTK_BUILDER(user_data);
		combo_query = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));
		
		// get the clipboard text and pass it to query
		selection = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));

		if(selection)
		{
			text_entry_query = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_query)));
			button_search = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));

			if(0 != g_ascii_strncasecmp(gtk_entry_get_text(text_entry_query), selection, g_utf8_strlen(selection, -1) + 1))
			{
				gtk_entry_set_text(text_entry_query, selection);
				//gtk_editable_set_position(GTK_EDITABLE(text_entry_query), -1);
			}
			g_free(selection);

			gtk_button_clicked(button_search);
		}
		else
		{
			// see if in case notify is selected, should we popup or we should just notify "No selection made!"
			window = GTK_WINDOW(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
			tomboy_window_present_hardcore(window);
			gtk_widget_grab_focus(GTK_WIDGET(combo_query));
		}

		hotkey_processing = FALSE;
		return GDK_FILTER_REMOVE;
	}

	return GDK_FILTER_CONTINUE;
}

static void status_icon_activate(GtkStatusIcon *status_icon, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
	GtkComboBoxEntry *combo_query = NULL;

	if(GTK_WIDGET_VISIBLE(window))
		gtk_widget_hide(GTK_WIDGET(window));
	else
	{

		/* close notifications, if any */
		if(notifier)
			notify_notification_close(notifier, NULL);

		tomboy_window_present_hardcore(window);
		combo_query = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));
		gtk_widget_grab_focus(GTK_WIDGET(combo_query));
	}
}

static void status_icon_popup(GtkStatusIcon *status_icon, guint button, guint active_time, gpointer user_data)
{
	GtkMenu *menu = GTK_MENU(user_data);
	gtk_menu_popup(menu, NULL, NULL, gtk_status_icon_position_menu, status_icon, button, active_time);
}

static void about_response_handle(GtkDialog *about_dialog, gint response_id, gpointer user_data)
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
				fp_show_uri(NULL, STR_BUG_WEBPAGE, GDK_CURRENT_TIME, NULL);
			break;
		default:
			g_warning("About Dialog: Unhandled response_id: %d!\n", response_id);
			break;
	}
}

static void about_activate(GtkToolButton *menu_item, gpointer user_data)
{
	GtkWidget *about_dialog = gtk_about_dialog_new();

	g_object_set(G_OBJECT(about_dialog), "license", STR_LICENCE, "copyright", STR_COPYRIGHT, 
	"comments", STR_ABOUT, "authors", strv_authors, "version", PACKAGE_VERSION, 
	"wrap-license", TRUE, "website-label", STR_WEBSITE_LABEL, "website", STR_WEBSITE, NULL);

	if(fp_show_uri)
		gtk_dialog_add_button(GTK_DIALOG(about_dialog), STR_REPORT_BUG, ARTHA_RESPONSE_REPORT_BUG);

	g_signal_connect(about_dialog, "response", G_CALLBACK(about_response_handle), NULL);
	
	gtk_dialog_run(GTK_DIALOG(about_dialog));
}

/* Multiple signals have this as the registered call back function.
   The most important of them is "delete-event" of the main window 
   which expects the return value of this function to 'gboolean' 
   and not 'void' like other signals do. Hence it has gboolean as 
   it's return type.
*/
static gboolean quit_activate(GObject *obj, gpointer user_data)
{
	G_DEBUG("Destroy called!\n");

	gtk_main_quit();
	return TRUE;
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
	gboolean direct_deleted = FALSE, has_indirect = FALSE;

	g_assert(antonyms_tree_store);

	if(advanced_mode)
	{
		gtk_tree_store_append(antonyms_tree_store, &direct_iter, NULL);
		gtk_tree_store_set(antonyms_tree_store, &direct_iter, 0, STR_ANTONYM_HEADER_DIRECT, 1, PANGO_WEIGHT_SEMIBOLD, 2, NULL, -1);
		gtk_tree_store_append(antonyms_tree_store, &indirect_iter, NULL);
		gtk_tree_store_set(antonyms_tree_store, &indirect_iter, 0, STR_ANTONYM_HEADER_INDIRECT, 1, PANGO_WEIGHT_SEMIBOLD, 2, NULL, -1);
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
				gtk_tree_store_set(antonyms_tree_store, &iter, 0, temp_antonym_item->term, 1, PANGO_WEIGHT_NORMAL, 2, NULL, -1);
			}
			else
			{				
				iter = indirect_iter;
				has_indirect = TRUE;
			}
			implications = temp_antonym_item->implications;
			while(implications)
			{
				gtk_tree_store_append(antonyms_tree_store, &sub_iter, &iter);
				gtk_tree_store_set(antonyms_tree_store, &sub_iter, 0, ((WNIImplication*)implications->data)->term, 
				1, PANGO_WEIGHT_NORMAL, 2, (DIRECT_ANT == temp_antonym_item->relation)?NULL:temp_antonym_item->term, -1);
				implications = g_slist_next(implications);
			}
		}
		antonyms = g_slist_next(antonyms);
	}

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

		// if direct list was deleted, then this is the indirect list that we are dealing, so expand, else don't
		gtk_tree_view_expand_row(tree_antonyms, expand_path, direct_deleted);
		gtk_tree_path_free(expand_path);

		// if there is a list in this path "1", then it should be indirect
		expand_path = gtk_tree_path_new_from_indices(1, -1);
		gtk_tree_view_expand_row(tree_antonyms, expand_path, TRUE);
		gtk_tree_path_free(expand_path);
	}
	
	gtk_tree_view_set_headers_visible(tree_antonyms, has_indirect);
}

static guint16 append_term(gchar *target, gchar *term, gboolean is_heading)
{
	guint16 i = 0;

	while(term[i] != '\0')
	{
		*target = term[i++];
		target++;
	}
	*target = (is_heading)? ':' : ',';
	++target;
	*target = ' ';

	return (i + 2);
}

static void build_tree(GNode *node, GtkTreeStore *tree_store, GtkTreeIter *parent_iter)
{
	GNode *child = NULL;
	GSList *temp_list = NULL;
	WNIImplication *imp = NULL;
	GtkTreeIter tree_iter = {0};
	guint32 i = 0;
	gchar terms[MAX_CONCAT_STR] = "";

	if((child = g_node_first_child(node)))
	do
	{
		i = 0;
		terms[0] = '\0';

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

	} while((child = g_node_next_sibling(child)));
}

static void trees_load(GSList *properties, GtkBuilder *gui_builder, WNIRequestFlags id)
{
	GNode *tree = NULL;
	GtkTreeView *tree_view = NULL;
	GtkTreeStore *tree_store = NULL;
	GtkTreeIter iter = {0};
	WNIImplication *imp = NULL;
	GSList *temp_list = NULL;
	gchar terms[MAX_CONCAT_STR] = "";
	guint16 i = 0;
	
	if(!advanced_mode && WORDNET_INTERFACE_HYPERNYMS == id) return;

	for(i = 0; 1 != id; (id = id >> 1), i++);

	tree_view = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[i]));
	tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));
	g_assert(tree_store);

	while(properties)
	{
		tree = (GNode*) properties->data;
		if(tree)
		{
			tree = g_node_first_child(tree);
			while(tree)
			{
				i = 0;
				terms[0] = '\0';

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
				
				tree = g_node_next_sibling(tree);
			}
		}

		properties = g_slist_next(properties);
	}
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
	g_assert(tree_store);

	while(properties)
	{
		temp_property = (WNIPropertyItem*) properties->data;
		if(temp_property)
		{
			gtk_tree_store_append(tree_store, &iter, NULL);
			text_weight = PANGO_WEIGHT_NORMAL;

			// check if synonym has more than one mapping
			// we try to get the 2nd term (I.e. index 1) instead of checking the length; parsing the whole list is insane
			if(NULL != g_slist_nth(temp_property->mapping, 1))
				text_weight = PANGO_WEIGHT_BOLD;

			gtk_tree_store_set(tree_store, &iter, 0, temp_property->term, 1, text_weight, -1);
		}
		properties = g_slist_next(properties);
	}
}

static void domains_load(GSList *properties, GtkBuilder *gui_builder)
{
	guint8 i = 0;
	WNIClassItem *class_item = NULL;
	GtkTreeView *tree_class = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[TREE_DOMAIN]));
	GtkTreeStore *class_tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_class));
	GtkTreeIter iter = {0}, class_iter[DOMAINS_COUNT] = {{0}};

	g_assert(class_tree_store);

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

	// if the expander was contracted automatically, then expand it back again
	if(!gtk_expander_get_expanded(GTK_EXPANDER(expander)) && auto_contract)
		gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
}

static gchar* wildmat_to_regex(gchar *wildmat)
{
	guint16 i = 0;
	gchar ch = 0;
	GString *regex = g_string_new("^");

	/* Conversion Rules are simple at this point.
	   1. To get the proper term from start to end, make sure the beginning and end of the regex is ^ and $ respectively
	   2. E.g. in 'a*b', * is applied to a in regex, while in wildmat, 'a' & 'b' is sure included and * is anything b/w them
	   so in regex make it as 'a.*b', which will make * applied to . (dot means any char. except newline in regex)
	   3. A dot/period in regex acts as a Joker (?) in wildmat, so make a direct conv.
	   4. Other chars go in as such, its the same in both for stuff like [r|m|s]{m,n} or [a-m]+, etc. */

	while((ch = wildmat[i++]) != '\0')
	{
		if(ch == '*')
			regex = g_string_append(regex, ".*");
		else if(ch == '?')
			regex = g_string_append_c(regex, '.');
		else
			regex = g_string_append_c(regex, ch);
		
	}
	
	regex = g_string_append_c(regex, '$');
	regex = g_string_append_c(regex, '\0');

	return g_string_free(regex, FALSE);
}

static void set_regex_results(gchar *wildmat_exp, GtkBuilder *gui_builder)
{
	gchar *regex_pattern = NULL, *lemma = NULL;
	GRegex *regex = NULL;
	GMatchInfo *match_info = NULL;
	guint32 count = 0;
	GtkTextView *text_view = NULL;
	GtkTextBuffer *text_buffer = NULL;
	GtkTextIter cur = {0};
	GtkStatusbar *status_bar = GTK_STATUSBAR(gtk_builder_get_object(gui_builder, STATUSBAR));
	gchar status_msg[MAX_STATUS_MSG] = "";


	// convert the wilmat expr. to PERL regex
	if(wildmat_exp)
		regex_pattern = wildmat_to_regex(wildmat_exp);

	if(regex_pattern)
	{
		text_view = GTK_TEXT_VIEW(gtk_builder_get_object(gui_builder, TEXT_VIEW_DEFINITIONS));
		text_buffer = gtk_text_view_get_buffer(text_view);

		// clear text and get the iter
		gtk_text_buffer_set_text(text_buffer, "", -1);
		gtk_text_buffer_get_start_iter(text_buffer, &cur);

		// set heading that Regex mode is now in action
		gtk_text_buffer_insert_with_tags_by_name(text_buffer, &cur, STR_REGEX_DETECTED, -1, TAG_LEMMA, NULL);
		gtk_text_buffer_insert(text_buffer, &cur, NEW_LINE, -1);

		// compile a GRegex
		regex = g_regex_new(regex_pattern, G_REGEX_MULTILINE|G_REGEX_CASELESS|G_REGEX_UNGREEDY|G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY, NULL);

		if(regex)
		{
			// do the actual regex matching and get the count
			g_regex_match(regex, wordnet_terms->str, G_REGEX_MATCH_NOTEMPTY, &match_info);
			count = g_match_info_get_match_count(match_info);

			// make a header for the search results to follow
			if(count > 0)
			{
				gtk_text_buffer_insert_with_tags_by_name(text_buffer, &cur, STR_SUGGEST_MATCHES, -1, TAG_MATCH, NULL);
			}

			// initialize count to 0 for the actual count to be displayed in the status bar
			count = 0;

			// insert the matches one by one
			while(g_match_info_matches(match_info))
			{

				lemma = g_match_info_fetch(match_info, 0);
				gtk_text_buffer_insert(text_buffer, &cur, NEW_LINE, -1);
				gtk_text_buffer_insert_with_tags_by_name(text_buffer, &cur, lemma, -1, TAG_SUGGESTION, NULL);
				g_free (lemma);
				g_match_info_next (match_info, NULL);
				count++;
			}

			// free all the initialised data
			g_match_info_free (match_info);
			g_regex_unref(regex);

			// set the status bar accordingly with the count
			gtk_statusbar_pop(status_bar, status_msg_context_id);
			g_snprintf(status_msg, MAX_STATUS_MSG, STR_STATUS_REGEX, count, count > 0?STR_STATUS_LOOKUP_HINT:"");
			status_msg_context_id = gtk_statusbar_get_context_id(status_bar, STATUS_DESC_REGEX_RESULTS);
			gtk_statusbar_push(status_bar, status_msg_context_id, status_msg);
		}

		// if regex was invlaid (it would be NULL)
		// or if the expr. fetched no results, set regex failed message
		if(NULL == regex || 0 == count)
		{
			gtk_text_buffer_insert_with_tags_by_name(text_buffer, &cur, STR_REGEX_FAILED, -1, TAG_SUGGESTION, NULL);
		}

		g_free(regex_pattern);
	}
}

static gboolean is_wildmat_expr(gchar *expr)
{
	guint16 i = 0;
	gchar ch = 0;

	if(expr)
	{
		// if the passed expr. has any of the char below deem it as a wildmat expr.
		while((ch = expr[i++]) != '\0')
		{
			if(ch == '*' || ch == '?' || ch == '[' || ch == ']' || ch =='{' || ch == '}' || ch == '+')
				return TRUE;
		}
	}

	return FALSE;
}

static void button_search_click(GtkButton *button, gpointer user_data)
{
	gchar *search_str = NULL, *lemma = NULL, **suggestions = NULL;
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkComboBox *combo_query = GTK_COMBO_BOX(gtk_builder_get_object(gui_builder, COMBO_QUERY));
	GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
	GtkExpander *expander = GTK_EXPANDER(gtk_builder_get_object(gui_builder, EXPANDER));

	GtkListStore *query_list_store = NULL;
	GtkTreeIter query_list_iter = {0};

	GtkTextView *text_view = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter cur = {0};
	GtkTextMark *freq_marker = NULL;

	GSList *def_list = NULL, *definitions_list = NULL, *example = NULL;
	WNIDefinitionItem *def_item = NULL;
	WNIDefinition *defn = NULL;

	gint16 count = 0;
	gchar *str_list_item = NULL;
	gchar str_count[MAX_SENSE_DIGITS] = "";

	GError *err = NULL;
	gchar *definition = NULL;

	gboolean results_set = FALSE;
	gchar *regex_text = NULL;

	GtkStatusbar *status_bar = GTK_STATUSBAR(gtk_builder_get_object(gui_builder, STATUSBAR));
	gchar status_msg[MAX_STATUS_MSG] = "";
	guint16 total_results = 0;
	guint8 total_pos = 0;



	text_view = GTK_TEXT_VIEW(gtk_builder_get_object(gui_builder, TEXT_VIEW_DEFINITIONS));
	buffer = gtk_text_view_get_buffer(text_view);


	// Check if the fed string is a wildmat expr.
	// If true, call the regex mod. to set the results and return
	regex_text = g_strstrip(gtk_combo_box_get_active_text(combo_query));
	if((results_set = is_wildmat_expr(regex_text)))
	{
		// clear previously set relatives
		// clear search successful flag; without this when the prev. looked up
		// word is again dbl clicked from the results, it won't jump to the word
		relatives_clear_all(gui_builder);
		last_search_successful = FALSE;

		// contract the expander, since relatives aren't required here
		if(gtk_expander_get_expanded(expander))
		{
			gtk_expander_set_expanded(expander, FALSE);
			auto_contract = TRUE;
		}

		gtk_statusbar_pop(status_bar, status_msg_context_id);
		
		if(wordnet_terms)
		{
			// set the status bar to inform that the search is on going
			g_snprintf(status_msg, MAX_STATUS_MSG, STR_STATUS_SEARCHING);
			status_msg_context_id = gtk_statusbar_get_context_id(status_bar, STATUS_DESC_REGEX_SEARCHING);
			gtk_statusbar_push(status_bar, status_msg_context_id, status_msg);

			// if visible, update the statusbar; the window will usually go on a freeze
			// without updating the statusbar, until GDK is idle
			// this call forces GDK to first redraw the statusbar and proceed
			if(GTK_WIDGET_VISIBLE(window))
				gdk_window_process_updates(((GtkWidget*)status_bar)->window, FALSE);

			set_regex_results(regex_text, gui_builder);
		}
		else
		{
			// report that the search index (index.sense) is missing
			g_snprintf(status_msg, MAX_STATUS_MSG, STR_STATUS_REGEX_FILE_MISSING);
			status_msg_context_id = gtk_statusbar_get_context_id(status_bar, STATUS_DESC_REGEX_ERROR);
			gtk_statusbar_push(status_bar, status_msg_context_id, status_msg);

			gtk_text_buffer_set_text(buffer, "", -1);
			gtk_text_buffer_get_end_iter(buffer, &cur);

			lemma = g_strdup_printf(STR_REGEX_FILE_MISSING, SetSearchdir());
			if(lemma)
			{
				gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, lemma, -1, TAG_POS, NULL);

				g_free(lemma);
				lemma = NULL;
			}
		}
	}

	g_free(regex_text);

	// If it was a regex and its results are now furnished, so return
	if(results_set) return;


	// from here on normal lookup starts
	search_str = g_strstrip(strip_invalid_edges(gtk_combo_box_get_active_text(combo_query)));

	if(search_str)
		count = g_utf8_strlen(search_str, -1);
	
	if(count > 0)
	{
		if(notifier)
		{
			notify_notification_close(notifier, NULL);
		}

		G_MESSAGE("'%s' queried!\n", search_str);

		if(last_search)
			total_results = g_ascii_strncasecmp(last_search, search_str, count + 1);

		if((last_search != NULL && total_results != 0) || (total_results == 0 && !last_search_successful) || last_search == NULL)
		{
			// set the status bar and update the window
			gtk_statusbar_pop(status_bar, status_msg_context_id);
			g_snprintf(status_msg, MAX_STATUS_MSG, STR_STATUS_SEARCHING);
			status_msg_context_id = gtk_statusbar_get_context_id(status_bar, STATUS_DESC_REGEX_SEARCHING);
			gtk_statusbar_push(status_bar, status_msg_context_id, status_msg);
			
			// if visible, repaint the statusbar after setting the status message, for it to get reflected
			if(GTK_WIDGET_VISIBLE(window))
				gdk_window_process_updates(((GtkWidget*)status_bar)->window, FALSE);

			G_MESSAGE("'%s' requested from WNI!\n", search_str);

			wni_request_nyms(search_str, &results, WORDNET_INTERFACE_ALL, advanced_mode);

			// clear prior text in definitons text view & relatives
			gtk_text_buffer_set_text(buffer, "", -1);
			relatives_clear_all(gui_builder);

			if(results)
			{
				total_results = 0; total_pos = 0;

				G_MESSAGE("Results successful!\n");

				def_list = ((WNIOverview*)((WNINym*)results->data)->data)->definitions_list;
				while(def_list)
				{
					if(def_list->data)
					{
						def_item = (WNIDefinitionItem*) def_list->data;
						definitions_list = def_item->definitions;

						gtk_text_buffer_get_end_iter(buffer, &cur);
						gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, def_item->lemma, -1, TAG_LEMMA, NULL);
						gtk_text_buffer_insert(buffer, &cur, " ~ ", -1);
						gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, partnames[def_item->pos], -1, TAG_POS, NULL);

						freq_marker = gtk_text_buffer_create_mark(buffer, NULL, &cur, TRUE);

						gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, NEW_LINE, -1, TAG_POS, NULL);
				
						count = 1;
						while(definitions_list)
						{
							defn = (WNIDefinition*) definitions_list->data;
							
							total_results++;
							g_snprintf(str_count, MAX_SENSE_DIGITS, "%3d", total_results);
							gtk_text_buffer_create_mark(buffer, str_count, &cur, TRUE);

							g_snprintf(str_count, MAX_SENSE_DIGITS, "%2d. ", count);
							
							gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, str_count, -1, TAG_COUNTER, NULL);
							gtk_text_buffer_insert(buffer, &cur, defn->definition, -1);
						
							example = defn->examples;

							definitions_list = g_slist_next(definitions_list);
							if(example || definitions_list) gtk_text_buffer_insert(buffer, &cur, NEW_LINE, -1);

							while(example)
							{
								gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, example->data, -1, TAG_EXAMPLE, NULL);
								example = g_slist_next(example);
								// for each example come to appear in a new line enabled this line and 
								// comment the next two buffer inserts of "; " and new line
								//if(example || definitions_list) gtk_text_buffer_insert(buffer, &cur, NEW_LINE, -1);

								if(example)
									gtk_text_buffer_insert(buffer, &cur, "; ", -1);
								else if(definitions_list)
									gtk_text_buffer_insert(buffer, &cur, NEW_LINE, -1);
							}
							count++;
						}

						gtk_text_buffer_get_iter_at_mark(buffer, &cur, freq_marker);

						gtk_text_buffer_insert(buffer, &cur, "    ", -1);
						gtk_text_buffer_insert_with_tags_by_name(buffer, 
											&cur, 
											familiarity[get_frequency(count - 1)], 
											-1, 
											familiarity[get_frequency(count - 1)], 
											NULL);
					}
					def_list = g_slist_next(def_list);
					if(def_list)
					{
						gtk_text_buffer_get_end_iter(buffer, &cur);
						gtk_text_buffer_insert(buffer, &cur, NEW_LINE, -1);
					}

					total_pos++;
				}

				/* scroll to the text view top to show the first definition
				gtk_text_buffer_get_start_iter(buffer, &cur);
				gtk_text_view_scroll_to_iter(text_view, &cur, 0.0, FALSE, 0.0, 0.0); */

				gtk_statusbar_pop(status_bar, status_msg_context_id);
				g_snprintf(status_msg, MAX_STATUS_MSG, STR_STATUS_QUERY_SUCCESS, total_results, total_pos);
				status_msg_context_id = gtk_statusbar_get_context_id(status_bar, STATUS_DESC_SEARCH_SUCCESS);
				gtk_statusbar_push(status_bar, status_msg_context_id, status_msg);

				lemma = ((WNIDefinitionItem*)((WNIOverview*)((WNINym*)results->data)->data)->definitions_list->data)->lemma;

				query_list_store = GTK_LIST_STORE(gtk_combo_box_get_model(combo_query));
				if(query_list_store)
				{
					count = 0;
					gtk_tree_model_get_iter_first(GTK_TREE_MODEL(query_list_store), &query_list_iter);

					while(gtk_list_store_iter_is_valid(query_list_store, &query_list_iter))
					{
						gtk_tree_model_get(GTK_TREE_MODEL(query_list_store), &query_list_iter, 0, &str_list_item, -1);
						if(0 == wni_strcmp0(lemma, str_list_item))
						{
							// While in here, if you set a particular index item as active, 
							// it again calls the button_search_click from within as an after effect of combo_query "changed" signal
							//gtk_combo_box_set_active(combo_query, count);

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
						history_count++;
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
			/* if mod notify is present & if notifications are enabled and window is invisible then show notification  */
			if(notifier && notifier_enabled && (!GTK_WIDGET_VISIBLE(window)))
			{
				if(NULL == lemma)
					lemma = ((WNIDefinitionItem*)((WNIOverview*)((WNINym*)results->data)->data)->definitions_list->data)->lemma;

				definition = g_strconcat("<b><i>", 
				partnames[((WNIDefinitionItem*)((WNIOverview*)((WNINym*)results->data)->data)->definitions_list->data)->pos], 
				"</i></b>. ", ((WNIDefinition*)((WNIDefinitionItem*)((WNIOverview*)((WNINym*)results->data)->data)->definitions_list->data)->definitions->data)->definition, NULL);

				if(definition)
				{
					notify_notification_update(notifier, lemma, definition, "gtk-dialog-info");
					if(FALSE == notify_notification_show(notifier, &err))
					{
						g_warning("%s\n", err->message);
						g_error_free(err);
					}
					g_free(definition);
				}
			}
			else
				tomboy_window_present_hardcore(window);

			last_search_successful = TRUE;
		}
		else
		{
			if(notifier && notifier_enabled && (!GTK_WIDGET_VISIBLE(window)))
			{
				notify_notification_update(notifier, search_str, STR_STATUS_QUERY_FAILED, "gtk-dialog-warning");
				if(FALSE == notify_notification_show(notifier, &err))
				{
					g_warning("%s\n", err->message);
					g_error_free(err);
				}
			}
			else
			{
				gtk_text_buffer_set_text(buffer, "", -1);
				gtk_text_buffer_get_start_iter(buffer, &cur);
				gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, STR_QUERY_FAILED, -1, TAG_POS, NULL);

				if(mod_suggest)
				{
					suggestions = suggestions_get(search_str);
					if(suggestions)
					{
						total_results = 0;
					
						gtk_text_buffer_insert(buffer, &cur, NEW_LINE, -1);
						gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, STR_SUGGEST_MATCHES, -1, TAG_MATCH, NULL);

						while(suggestions[total_results])
						{
							gtk_text_buffer_insert(buffer, &cur, NEW_LINE, -1);
							gtk_text_buffer_insert_with_tags_by_name(buffer, &cur, 
												suggestions[total_results++], 
												-1, TAG_SUGGESTION, NULL);
						}

						g_strfreev(suggestions);

						// contract the expander, since relatives aren't required here
						if(gtk_expander_get_expanded(expander))
						{
							gtk_expander_set_expanded(expander, FALSE);
							auto_contract = TRUE;
						}
					}
				}

				tomboy_window_present_hardcore(window);

				gtk_statusbar_pop(status_bar, status_msg_context_id);
				status_msg_context_id = gtk_statusbar_get_context_id(status_bar, STATUS_DESC_SEARCH_FAILURE);
				gtk_statusbar_push(status_bar, status_msg_context_id, STR_STATUS_QUERY_FAILED);
			}

			last_search_successful = FALSE;
		}
		
		gtk_editable_select_region(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(combo_query))), 0, -1);
		gtk_widget_grab_focus(GTK_WIDGET(combo_query));
	}

	if(search_str && !results_set)
	{
		g_free(search_str);
	}
}

static void combo_query_changed(GtkComboBox *combo_query, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkButton *button_search = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));
	GtkToolbar *tool_bar = GTK_TOOLBAR(gtk_builder_get_object(gui_builder, TOOLBAR));
	GtkToolItem *toolbar_prev = NULL, *toolbar_next = NULL;
	GtkTreeModel *tree_model = NULL;
	gint selected_item = gtk_combo_box_get_active(combo_query);
	gint total_items = 0;

	if(selected_item != -1)
	{
		gtk_button_clicked(button_search);
		tree_model = gtk_combo_box_get_model(combo_query);
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

static gboolean text_view_button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if(GDK_2BUTTON_PRESS == event->type)
		was_double_click = TRUE;

	return FALSE;
}

static gboolean text_view_button_released(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkTextView *defn_text_view = GTK_TEXT_VIEW(gtk_builder_get_object(gui_builder, TEXT_VIEW_DEFINITIONS));
	GtkTextBuffer *defn_text_buffer = gtk_text_view_get_buffer(defn_text_view);
	GtkTextMark *insert_mark = NULL, *selection_mark = NULL;
	GtkTextIter ins_iter = {0}, sel_iter = {0};
	gchar *trial_text = NULL;
	guint8 i = 0;
	gboolean iter_move_success = FALSE;

	// set the selection if it was a double click or a CTRL + mouse release
	if(was_double_click || (GDK_CONTROL_MASK & event->state))
	{
		// if it was a double click, try selecting diff. combinations
		// for the sake of compound lemmas like 'work out', 'in a nutshell', etc.
		if(was_double_click)
		{
			was_double_click = FALSE;

			if(gtk_text_buffer_get_has_selection(defn_text_buffer))
			{
				// get the 'selection_bound' and 'insert' markers
				selection_mark = gtk_text_buffer_get_selection_bound(defn_text_buffer);
				insert_mark = gtk_text_buffer_get_mark(defn_text_buffer, "insert");

				for(i = 0; ((i < 6) && (!iter_move_success)); i++)
				{
					// on every iteration of the loop, set the iters to the actual sel. marks
					gtk_text_buffer_get_iter_at_mark(defn_text_buffer, &ins_iter, insert_mark);
					gtk_text_buffer_get_iter_at_mark(defn_text_buffer, &sel_iter, selection_mark);

					switch(i)
					{
						case 0:
							// try selecting two words after the actual sel.
							if(!gtk_text_iter_forward_visible_word_ends(&sel_iter, 2))
								gtk_text_iter_forward_to_end(&sel_iter);
							iter_move_success = TRUE;
							break;
						case 1:
							// try selecting two words before the actual sel.
							if(gtk_text_iter_backward_visible_word_starts(&ins_iter, 2))
								iter_move_success = TRUE;
							break;
						case 2:
							// try selecting one word before and after the actual sel.
							if((gtk_text_iter_forward_visible_word_end(&sel_iter)) && (gtk_text_iter_backward_visible_word_start(&ins_iter)))
								iter_move_success = TRUE;
							break;
						case 3:
							// try selecting one word next to the actual sel.
							if(!gtk_text_iter_forward_visible_word_end(&sel_iter))
								gtk_text_iter_forward_to_end(&sel_iter);
							iter_move_success = TRUE;
							break;
						case 4:
							// try selecting one word before the actual sel.
							if(gtk_text_iter_backward_visible_word_start(&ins_iter))
								iter_move_success = TRUE;
							break;
						case 5:
							// fallback to just the actual selection
							iter_move_success = TRUE;
							break;
					}

					if(iter_move_success)
					{
						// obtain the text for the set iters
						trial_text = gtk_text_buffer_get_text(defn_text_buffer, &ins_iter, &sel_iter, FALSE);
						
						// search the trial selection in WordNet, on a successful lookup iter_move_success is set
						iter_move_success = wni_request_nyms(trial_text, NULL, 0, FALSE);

						// free the obtained text
						g_free(trial_text);
						trial_text = NULL;
					}
				}


				// set the 'insert' and 'selection_bound' markers respectivly to the newly placed iters
				gtk_text_buffer_move_mark(defn_text_buffer, insert_mark, &ins_iter);
				gtk_text_buffer_move_mark(defn_text_buffer, selection_mark, &sel_iter);
			}
		}

		// request the "TARGETS" target for the primary selection
		gtk_selection_convert(widget, GDK_SELECTION_PRIMARY, GDK_TARGET_STRING, event->time);
	}

	return FALSE;
}

static void text_view_selection_made(GtkWidget *widget, GtkSelectionData *sel_data, guint time, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkComboBoxEntry *combo_query = NULL;
	GtkEntry *txtQuery = NULL;
	GtkButton *button_search = NULL;
	gchar *selection = (gchar*) gtk_selection_data_get_text(sel_data);

	if(selection)
	{
		combo_query = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));
		txtQuery = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_query)));
		button_search = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));
	
		if(0 != wni_strcmp0(gtk_entry_get_text(txtQuery), selection))
		{
			gtk_entry_set_text(txtQuery, selection);
			//gtk_editable_set_position(GTK_EDITABLE(txtQuery), -1);
		}
		g_free(selection);

		gtk_button_clicked(button_search);
	}
}

static gboolean window_main_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	GtkComboBox *combo_query = GTK_COMBO_BOX(user_data);
	gboolean popped_down = FALSE;
	gboolean deleted = FALSE;

	if(GDK_Escape == event->keyval)
	{
		g_object_get(combo_query, "popup-shown", &popped_down, NULL);
		if(!popped_down)
		{
			g_signal_emit_by_name(widget, "delete-event", G_TYPE_BOOLEAN, &deleted);
			return TRUE;
		}
	}
	return FALSE;
}

static void expander_clicked(GtkExpander *expander, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkPaned *vpaned = GTK_PANED(gtk_builder_get_object(gui_builder, VPANE));

	gtk_paned_set_position(vpaned, -1);		// unset the pane's position so that it goes to the original state

	// if the user manually expands/contracts it, reset the auto flag
	auto_contract = FALSE;
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

static GtkTextMark* highlight_definition(guint8 id, guint16 sense, GtkTextView *text_view)
{
	guint16 defn_no = 0;
	GSList *wni_results = results, *definitions_list = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextMark *mark = NULL;
	GtkTextIter start = {0}, end = {0};
	gchar str_index[MAX_SENSE_DIGITS] = "";

	while(wni_results)
	{
		if(WORDNET_INTERFACE_OVERVIEW == ((WNINym*)wni_results->data)->id)
		{
			definitions_list = ((WNIOverview*)((WNINym*)wni_results->data)->data)->definitions_list;
			
			while(definitions_list)
			{
				if(id == ((WNIDefinitionItem*)definitions_list->data)->id)
				{
					defn_no += sense + 1;
					buffer = gtk_text_view_get_buffer(text_view);

					g_snprintf(str_index, MAX_SENSE_DIGITS, "%3d", defn_no);
					mark = gtk_text_buffer_get_mark(buffer, str_index);
					if(mark)
					{
						gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);
						gtk_text_buffer_get_iter_at_offset(buffer, &end, gtk_text_iter_get_offset(&start) + 3);
						gtk_text_buffer_apply_tag_by_name(buffer, TAG_HIGHLIGHT, &start, &end);
					}
					
					break;
				}
				else
					defn_no += ((WNIDefinitionItem*)definitions_list->data)->count;

				definitions_list = g_slist_next(definitions_list);
			}
			
			break;
		}
		wni_results = g_slist_next(wni_results);
	}
	
	return mark;
}

static void highlight_senses_from_synonyms(guint16 relative_index, GtkTextView *text_view)
{
	GSList *wni_results = results;
	GSList *synonyms_list = NULL;
	GSList *synonym_mapping = NULL;
	GtkTextMark *highlighted_mark = NULL, *scroll_mark = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter = {0};
	guint16 last_offset = 0, current_offset = 0;

	while(wni_results)
	{
		if(WORDNET_INTERFACE_OVERVIEW == ((WNINym*)wni_results->data)->id)
		{
			synonyms_list = ((WNIOverview*) ((WNINym*)wni_results->data)->data)->synonyms_list;
			for(; relative_index; relative_index--, synonyms_list = g_slist_next(synonyms_list));
			synonym_mapping = ((WNIPropertyItem*) synonyms_list->data)->mapping;
			
			buffer = gtk_text_view_get_buffer(text_view);
			gtk_text_buffer_get_end_iter(buffer, &iter);
			// store the last line number
			last_offset = gtk_text_iter_get_line(&iter);

			while(synonym_mapping)
			{
				highlighted_mark = highlight_definition(((WNISynonymMapping*) synonym_mapping->data)->id, ((WNISynonymMapping*) synonym_mapping->data)->sense, text_view);
				
				gtk_text_buffer_get_iter_at_mark(buffer, &iter, highlighted_mark);

				// see if this mark is at a higher line
				current_offset = gtk_text_iter_get_line(&iter);

				if(current_offset <= last_offset)
				{
					last_offset = current_offset;
					scroll_mark = highlighted_mark;
				}

				synonym_mapping = g_slist_next(synonym_mapping);
			}

			gtk_text_view_scroll_to_mark(text_view, scroll_mark, 0.0, TRUE, 0, 0);

			break;
		}

		wni_results = g_slist_next(wni_results);
	}
}

static void highlight_senses_from_antonyms(guint8 category, guint16 relative_index, guint8 sub_level, GtkTextView *text_view)
{
	GSList *wni_results = results;
	GSList *antonyms_list = NULL;
	GSList *antonym_mapping = NULL;
	WNIAntonymItem *antonym_item = NULL;
	guint16 direct_count = 0, indirect_count = 0, count = 0;
	GtkTextBuffer *buffer = NULL;
	GtkTextMark *scroll_mark = NULL, *highlighted_mark = NULL;
	GtkTextIter iter = {0}, end = {0};

	while(wni_results)
	{
		if(WORDNET_INTERFACE_ANTONYMS == ((WNINym*)wni_results->data)->id)
		{
			antonyms_list = ((WNIProperties*) ((WNINym*) wni_results->data)->data)->properties_list;

			while(antonyms_list)
			{
				antonym_item = (WNIAntonymItem*) antonyms_list->data;

				if(antonym_item->relation <= DIRECT_ANT)
					direct_count++;
				else
				{
					count = g_slist_length(antonym_item->implications);
					indirect_count += count;
					
					if(indirect_count > relative_index)
						indirect_count = relative_index + 1;
				}

				if((category <= DIRECT_ANT && direct_count == relative_index + 1) || 
				(category == INDIRECT_ANT && indirect_count == relative_index + 1))
				{
					antonym_mapping = antonym_item->mapping;

					buffer = gtk_text_view_get_buffer(text_view);
					gtk_text_buffer_get_end_iter(buffer, &end);

					// store the last line number
					direct_count = gtk_text_iter_get_line(&end);

					while(antonym_mapping)
					{
						if(sub_level)
						{
							count += ((WNIAntonymMapping*) antonym_mapping->data)->related_word_count;
							if(sub_level > count)
							{
								antonym_mapping = g_slist_next(antonym_mapping);
								continue;
							}
						}

						highlighted_mark = highlight_definition(((WNIAntonymMapping*) antonym_mapping->data)->id, 
						((WNIAntonymMapping*) antonym_mapping->data)->sense, text_view);
						
						gtk_text_buffer_get_iter_at_mark(buffer, &iter, highlighted_mark);

						// see if this mark is at a higher line
						indirect_count = gtk_text_iter_get_line(&iter);

						if(indirect_count <= direct_count)
						{
							direct_count = indirect_count;
							scroll_mark = highlighted_mark;
						}
						
						antonym_mapping = g_slist_next(antonym_mapping);
						
						if(sub_level)
						{
							if(sub_level <= count) break;
						}
					}

					gtk_text_view_scroll_to_mark(text_view, scroll_mark, 0.0, TRUE, 0, 0);

					break;
				}

				antonyms_list = g_slist_next(antonyms_list);
			}

			break;
		}
		
		wni_results = g_slist_next(wni_results);
	}
}

static void highlight_senses_from_domain(guint8 category, guint16 relative_index, GtkTextView *text_view)
{
	GSList *wni_results = results;
	GSList *class_list = NULL;
	WNIClassItem *class_item = NULL;
	guint16 count = 0;

	while(wni_results)
	{
		if(WORDNET_INTERFACE_CLASS == ((WNINym*) wni_results->data)->id)
		{
			class_list = ((WNIProperties*) ((WNINym*) wni_results->data)->data)->properties_list;

			while(class_list)
			{
				class_item = (WNIClassItem*)class_list->data;
				if(category == class_item->type - CLASSIF_CATEGORY)
					count++;

				if(count == relative_index + 1)
				{
					gtk_text_view_scroll_to_mark(text_view, highlight_definition(class_item->id, class_item->sense, text_view), 0.0, TRUE, 0, 0);
					break;
				}

				class_list = g_slist_next(class_list);
			}

			break;
		}

		wni_results = g_slist_next(wni_results);
	}
}

static void highlight_senses_from_relative_lists(WNIRequestFlags id, guint16 relative_index, GtkTextView *text_view)
{
	GSList *wni_results = results;
	GSList *properties_list = NULL;
	GSList *property_mapping = NULL;
	WNIPropertyMapping *mapping = NULL;
	GtkTextMark *highlighted_mark = NULL, *scroll_mark = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter = {0};
	guint16 last_offset = 0, current_offset = 0;

	while(wni_results)
	{
		if(id == ((WNINym*)wni_results->data)->id)
		{
			properties_list = ((WNIProperties*)((WNINym*)wni_results->data)->data)->properties_list;

			for(; relative_index; relative_index--, properties_list = g_slist_next(properties_list));
			property_mapping = ((WNIPropertyItem*) properties_list->data)->mapping;
			
			buffer = gtk_text_view_get_buffer(text_view);
			gtk_text_buffer_get_end_iter(buffer, &iter);
			last_offset = gtk_text_iter_get_line(&iter);
			
			while(property_mapping)
			{
				mapping = (WNIPropertyMapping*) property_mapping->data;
				highlighted_mark = highlight_definition(mapping->id, mapping->sense, text_view);
				
				gtk_text_buffer_get_iter_at_mark(buffer, &iter, highlighted_mark);

				// see if this mark is at a higher line
				current_offset = gtk_text_iter_get_line(&iter);

				if(current_offset <= last_offset)
				{
					last_offset = current_offset;
					scroll_mark = highlighted_mark;
				}

				property_mapping = g_slist_next(property_mapping);
			}
			
			gtk_text_view_scroll_to_mark(text_view, scroll_mark, 0.0, TRUE, 0, 0);

			break;
		}
		wni_results = g_slist_next(wni_results);
	}
}

static void relative_selection_changed(GtkTreeView *tree_view, gpointer user_data)
{
	GtkTreeIter iter = {0};
	GtkTreeModel *tree_store = NULL;
	gchar *term = NULL, *str_path = NULL, *str_category = NULL, *str_demark = NULL;
	gchar str_root[3] = "";
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkNotebook *notebook = GTK_NOTEBOOK(gtk_builder_get_object(gui_builder, NOTEBOOK));
	GtkTextView *text_view = GTK_TEXT_VIEW(gtk_builder_get_object(gui_builder, TEXT_VIEW_DEFINITIONS));
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start = {0}, end = {0};
	guint16 sub_level = 0;

	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(tree_view), &tree_store, &iter))
	{
		gtk_tree_model_get(tree_store, &iter, 0, &term, -1);
		str_path = gtk_tree_model_get_string_from_iter(tree_store, &iter);
		
		buffer = gtk_text_view_get_buffer(text_view);
		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
		gtk_text_buffer_remove_tag_by_name(buffer, TAG_HIGHLIGHT, &start, &end);

		if(term && str_path)
		{
			switch(gtk_notebook_get_current_page(notebook))
			{
				case TREE_SYNONYMS:
					highlight_senses_from_synonyms(g_ascii_strtoull(str_path, NULL, 10), text_view);
					break;
				case TREE_ANTONYMS:
					// to make sure no highlighting is done for categories, check for ':' in the path
					// Categories (Direct/Indirect) won't have a parent, hence only one char. will be 
					// there in the path; if not, a word has got selected
					if(advanced_mode && (str_demark = g_strstr_len(str_path, -1, ":")) != NULL)
					{
						str_demark++;
						str_demark = g_strstr_len(str_demark, -1, ":");
						if(str_demark)
						{
							str_demark++;
							sub_level = g_ascii_strtoull(str_demark, NULL, 10) + 1;
						}

						gtk_tree_model_get_iter_first(tree_store, &iter);
						gtk_tree_model_get(tree_store, &iter, 0, &str_category, -1);
						if(str_category)
						{
							if(0 != wni_strcmp0(str_category, STR_ANTONYM_HEADER_DIRECT)) str_path[0] = '1';
							g_free(str_category);
						}

						strip_invalid_edges(&str_path[2]);
						/* if its a word, parent 0 means Direct and 1 Indirect */
						highlight_senses_from_antonyms((0 == str_path[0] - '0') ? DIRECT_ANT : INDIRECT_ANT, 
										g_ascii_strtoull(&str_path[2], NULL, 10), 
										sub_level, 
										text_view);
					}
					else if(!advanced_mode)
						// if its simple mode, set category to 0
						highlight_senses_from_antonyms(0, g_ascii_strtoull(str_path, NULL, 10), sub_level, text_view);
					break;
				case TREE_DOMAIN:
					if((str_demark = g_strstr_len(str_path, -1, ":")) != NULL)
					{
						g_snprintf(str_root, 3, "%c", str_path[0]);
						gtk_tree_model_get_iter_from_string(tree_store, &iter, str_root);

						gtk_tree_model_get(tree_store, &iter, 0, &str_category, -1);
						for(sub_level = 0; (0 != wni_strcmp0(domain_types[sub_level], str_category)); sub_level++);
						g_free(str_category);

						str_demark++;
						highlight_senses_from_domain(sub_level, g_ascii_strtoull(str_demark, NULL, 10), text_view);
					}
					break;
				case TREE_PERTAINYMS:
				case TREE_HYPERNYMS:
				case TREE_HYPONYMS:
				case TREE_HOLONYMS:
				case TREE_MERONYMS:
					//highlight_senses_from_relative_trees(1 << gtk_notebook_get_current_page(notebook), g_ascii_strtoull(str_path, NULL, 10), text_view);
					break;
				default:
					highlight_senses_from_relative_lists(1 << gtk_notebook_get_current_page(notebook), g_ascii_strtoull(str_path, NULL, 10), text_view);
					break;
			}
		}

		g_free(term);
		g_free(str_path);
	}
}

static void relative_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	gchar *sel_term = NULL;
	GtkTreeModel *tree_store = NULL;
	GtkTreeIter iter = {0};
	GtkComboBoxEntry *combo_query = NULL;
	GtkEntry *txtQuery = NULL;
	GtkButton *button_search = NULL;
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);


	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(tree_view), &tree_store, &iter))
	{
		gtk_tree_model_get(tree_store, &iter, 0, &sel_term, -1);

		if(sel_term)
		{
			combo_query = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(gui_builder, COMBO_QUERY));
			txtQuery = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_query)));
			button_search = GTK_BUTTON(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));

			gtk_entry_set_text(txtQuery, sel_term);

			gtk_button_clicked(button_search);
			
			g_free(sel_term);
		}
	}
}

static void create_stores_renderers(GtkBuilder *gui_builder)
{
	guint8 i = 0;
	GtkComboBox *combo_query = NULL;
	GtkListStore *list_store_query = NULL;

	GtkTreeView *tree_view = NULL;
	GtkTreeStore *tree_store = NULL;
	GtkCellRenderer *tree_renderer = NULL;
	gchar *col_name = NULL;
	
	// combo box data store
	combo_query = GTK_COMBO_BOX(gtk_builder_get_object(gui_builder, COMBO_QUERY));
	list_store_query = gtk_list_store_new(1, G_TYPE_STRING);
	g_signal_connect(GTK_TREE_MODEL(list_store_query), "row-inserted", G_CALLBACK(query_list_updated), gui_builder);
	gtk_combo_box_set_model(combo_query, GTK_TREE_MODEL(list_store_query));
	gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(combo_query), 0);
	g_object_unref(list_store_query);
	
	for(i = 0; i < TOTAL_RELATIVES; i++)
	{
		tree_view = GTK_TREE_VIEW(gtk_builder_get_object(gui_builder, relative_tree[i]));
		tree_renderer = gtk_cell_renderer_text_new();
		col_name = relative_tree[i];
		col_name = col_name + 4;	// skip the first 4 chars "tree" in relative_tree string array
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, col_name, tree_renderer, "text", 0, "weight", 1, NULL);
		
		if(i == TREE_ANTONYMS)
		{
			gtk_tree_view_insert_column_with_attributes(tree_view, -1, STR_ANTONYM_HEADER_INDIRECT_VIA, tree_renderer, "text", 2, NULL);
			tree_store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);
		}
		else
			tree_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_UINT);

		gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(tree_store));
		g_object_unref(tree_store);
				
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);

		g_signal_connect(tree_view, "cursor-changed", G_CALLBACK(relative_selection_changed), gui_builder);
		g_signal_connect(tree_view, "row-activated", G_CALLBACK(relative_row_activated), gui_builder);
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
	
	save_preferences();
}

static void button_next_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkComboBox *combo_query = GTK_COMBO_BOX(gtk_builder_get_object(gui_builder, COMBO_QUERY));
	gint16 active_item = gtk_combo_box_get_active(combo_query);
	GtkTreeModel *tree_model = NULL;

	if(-1 == active_item)
	{
		tree_model = gtk_combo_box_get_model(combo_query);
		active_item = gtk_tree_model_iter_n_children(tree_model, NULL);
	}

	gtk_combo_box_set_active(combo_query, active_item - 1);
}

static void button_prev_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	GtkBuilder *gui_builder = GTK_BUILDER(user_data);
	GtkComboBox *combo_query = GTK_COMBO_BOX(gtk_builder_get_object(gui_builder, COMBO_QUERY));
	gint16 active_item = gtk_combo_box_get_active(combo_query);

	// in case if the search was successful and the index is 0 (current word)
	// then prev. should move it 1st element, so set active_item = 0, which + 1 will make index as 1
	// else if search had failed earlier, then prev. should take it to the 0th element
	// so leave it as -1; if it is something lesser than -1, make it -1
	if(-1 == active_item && last_search_successful)
	{
		active_item = 0;
	}
	else if(active_item < -1)
	{
		active_item = -1;
	}

	gtk_combo_box_set_active(combo_query, active_item + 1);
}

static gboolean combo_query_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
	gint current_index = 0;

	if((GDK_SCROLL_UP == event->direction) || (GDK_SCROLL_DOWN == event->direction))
	{
		current_index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

		if(-1 == current_index && history_count > 0) current_index = 0;

		if(GDK_SCROLL_DOWN == event->direction)
		{
			current_index++;
			if(history_count <= current_index) current_index = 0;
		}
		else
		{
			current_index--;
			if(current_index < 0) current_index = history_count - 1;
		}

		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), current_index);

		return TRUE;
	}

	return FALSE;
}

static void setup_toolbar(GtkBuilder *gui_builder, GtkWidget *hotkey_editor_dialog)
{
	GtkToolbar *toolbar = NULL;
	GtkToolItem *toolbar_item = NULL;


	// toolbar code starts here
	toolbar = GTK_TOOLBAR(gtk_builder_get_object(gui_builder, TOOLBAR));

	toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_item), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_item), STR_TOOLITEM_PREV);
	gtk_tool_item_set_tooltip_text(toolbar_item, TOOLITEM_TOOLTIP_PREV);
	gtk_widget_set_sensitive(GTK_WIDGET(toolbar_item), FALSE);
	g_signal_connect(toolbar_item, "clicked", G_CALLBACK(button_prev_clicked), gui_builder);
	gtk_toolbar_insert(toolbar, toolbar_item, -1);

	toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_item), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_item), STR_TOOLITEM_NEXT);
	gtk_tool_item_set_tooltip_text(toolbar_item, TOOLITEM_TOOLTIP_NEXT);
	gtk_widget_set_sensitive(GTK_WIDGET(toolbar_item), FALSE);
	g_signal_connect(toolbar_item, "clicked", G_CALLBACK(button_next_clicked), gui_builder);
	gtk_toolbar_insert(toolbar, toolbar_item, -1);

	toolbar_item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(toolbar, toolbar_item, -1);

	toolbar_item = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_DIALOG_INFO);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_item), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_item), STR_TOOLITEM_MODE);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolbar_item), advanced_mode);
	gtk_tool_item_set_tooltip_text(toolbar_item, TOOLITEM_TOOLTIP_MODE);
	g_signal_connect(toolbar_item, "toggled", G_CALLBACK(mode_toggled), gui_builder);
	gtk_toolbar_insert(toolbar, toolbar_item, -1);
	
	toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_PREFERENCES);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_item), STR_TOOLITEM_HOTKEY);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_item), TRUE);
	gtk_tool_item_set_tooltip_text(toolbar_item, TOOLITEM_TOOLTIP_HOTKEY);
	g_signal_connect(toolbar_item, "clicked", G_CALLBACK(show_hotkey_editor), hotkey_editor_dialog);
	gtk_toolbar_insert(toolbar, toolbar_item, -1);

	/* if mod notify is present */
	if(notifier)
	{
		toolbar_item = gtk_toggle_tool_button_new_from_stock(notifier_enabled ? GTK_STOCK_YES : GTK_STOCK_NO);
		gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_item), TRUE);
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_item), STR_TOOLITEM_NOTIFY);
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolbar_item), notifier_enabled);
		gtk_tool_item_set_tooltip_text(toolbar_item, TOOLITEM_TOOLTIP_NOTIFY);
		g_signal_connect(toolbar_item, "toggled", G_CALLBACK(notification_toggled), gui_builder);
		gtk_toolbar_insert(toolbar, toolbar_item, -1);

		notify_toolbar_index = gtk_toolbar_get_item_index(toolbar, toolbar_item);
	}

	toolbar_item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(toolbar, toolbar_item, -1);

	toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_ABOUT);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_item), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_item), STR_TOOLITEM_ABOUT);
	gtk_tool_item_set_tooltip_text(toolbar_item, TOOLITEM_TOOLTIP_ABOUT);
	g_signal_connect(toolbar_item, "clicked", G_CALLBACK(about_activate), NULL);
	gtk_toolbar_insert(toolbar, toolbar_item, -1);

	toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(toolbar_item), TRUE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_item), STR_TOOLITEM_QUIT);
	gtk_tool_item_set_tooltip_text(toolbar_item, TOOLITEM_TOOLTIP_QUIT);
	g_signal_connect(toolbar_item, "clicked", G_CALLBACK(quit_activate), NULL);
	gtk_toolbar_insert(toolbar, toolbar_item, -1);

	gtk_widget_show_all(GTK_WIDGET(toolbar));
}

static GtkMenu *create_popup_menu(GtkBuilder *gui_builder)
{
	GtkMenu *menu = NULL;
	GtkSeparatorMenuItem *menu_separator = NULL;
	GtkImageMenuItem *menu_quit = NULL;
	
	// Initialize popup menu/sub menus
	menu = GTK_MENU(gtk_menu_new());

	/* if mod notify is present, setup a notifications menu */
	if(notifier)
	{
		menu_notify = GTK_CHECK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(STR_TOOLITEM_NOTIFY));
		// load the settings value
		gtk_check_menu_item_set_active(menu_notify, notifier_enabled);

		menu_separator = GTK_SEPARATOR_MENU_ITEM(gtk_separator_menu_item_new());
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menu_notify));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menu_separator));
		g_signal_connect(menu_notify, "toggled", G_CALLBACK(notification_toggled), gui_builder);
	}

	menu_quit = GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menu_quit));

	g_signal_connect(menu_quit, "activate", G_CALLBACK(quit_activate), NULL);

	gtk_widget_show_all(GTK_WIDGET(menu));

	return menu;
}

static void create_text_view_tags(GtkBuilder *gui_builder)
{
	guint8 i = 0;
	GtkTextView *text_view = NULL;
	GtkTextBuffer *buffer = NULL;

	// create text view tags
	text_view = GTK_TEXT_VIEW(gtk_builder_get_object(gui_builder, TEXT_VIEW_DEFINITIONS));
	buffer = gtk_text_view_get_buffer(text_view);

	gtk_text_buffer_create_tag(buffer, TAG_LEMMA, "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(buffer, TAG_POS, "foreground", "red", "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_buffer_create_tag(buffer, TAG_COUNTER, "left_margin", 15, "weight", PANGO_WEIGHT_BOLD, "foreground", "gray", NULL);
	gtk_text_buffer_create_tag(buffer, TAG_EXAMPLE, "foreground", "blue", "font", "Serif Italic 10", "left_margin", 45, NULL);
	gtk_text_buffer_create_tag(buffer, TAG_MATCH, "foreground", "DarkGreen", "weight", PANGO_WEIGHT_SEMIBOLD, NULL);
	gtk_text_buffer_create_tag(buffer, TAG_SUGGESTION, "foreground", "blue" , "left_margin", 25, NULL);
	gtk_text_buffer_create_tag(buffer, TAG_HIGHLIGHT, "background", "black", "foreground", "white", NULL);
	
	for(i = 0; i < FAMILIARITY_COUNT; i++)
	{
		gtk_text_buffer_create_tag(buffer, familiarity[i], "background", freq_colors[i], 
		"foreground", (i < 4)? "white" : "black", "font", "Monospace 9", NULL);
	}
	
	g_signal_connect(text_view, "button-press-event", G_CALLBACK(text_view_button_pressed), gui_builder);					
	g_signal_connect(text_view, "button-release-event", G_CALLBACK(text_view_button_released), gui_builder);
	g_signal_connect(text_view, "selection-received", G_CALLBACK(text_view_selection_made), gui_builder);
}

static gboolean load_preferences(GtkWindow *parent)
{
	gchar *conf_file_path = NULL;
	GKeyFile *key_file = NULL;
	gboolean first_run = FALSE;

	gchar *version = NULL;
	gchar **version_numbers = NULL;
	gchar **current_version_numbers = NULL;
	guint8 i = 0;
	GError *err = NULL;

	conf_file_path = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S, PACKAGE_TARNAME, CONF_FILE_EXT, NULL);
	key_file = g_key_file_new();

	if(g_key_file_load_from_file(key_file, conf_file_path, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		advanced_mode = g_key_file_get_boolean(key_file, GROUP_SETTINGS, KEY_MODE, NULL);
		notifier_enabled = g_key_file_get_boolean(key_file, GROUP_SETTINGS, KEY_NOTIFICATIONS, NULL);
		version = g_key_file_get_string(key_file, GROUP_SETTINGS, KEY_VERSION, NULL);

		if(version)
		{
			if(0 != wni_strcmp0(version, PACKAGE_VERSION))
			{
				/* version number is split here. Major, Minor, Micro */
				version_numbers = g_strsplit(version, ".", 3);
				current_version_numbers = g_strsplit(PACKAGE_VERSION, ".", 3);
				
				for(i = 0; (NULL != version_numbers[i] && NULL != current_version_numbers[i]); i++)
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

		/* zero can be returned in three cases:
			i.   Older version - no key set in conf (err will be set)
			ii.  New version with no key in conf file set (err will be set)
			iii. New version & entries present (err will not be set) */

		app_hotkey.accel_key = g_key_file_get_integer(key_file, GROUP_SETTINGS, KEY_ACCEL_KEY, &err);
		app_hotkey.accel_mods = g_key_file_get_integer(key_file, GROUP_SETTINGS, KEY_ACCEL_MODS, NULL);
		app_hotkey.accel_flags = g_key_file_get_integer(key_file, GROUP_SETTINGS, KEY_ACCEL_FLAGS, NULL);

		g_key_file_free(key_file);
		g_free(conf_file_path);
	}
	else
		first_run = TRUE;

	/* The user might never close the app. and just shut down the system.
	   Conf file won't be written to disk in that case. Hence save the 
	   conf file when its a first run with the defaults. This is done 
	   by the main func. */
	return first_run;
}

static void save_preferences()
{
	gchar *conf_file_path = NULL;
	GKeyFile *key_file = NULL;
	gchar *file_contents = NULL;
	gsize file_len = 0;
	GError *err = NULL;

	conf_file_path = g_strconcat(g_get_user_config_dir(), G_DIR_SEPARATOR_S, PACKAGE_TARNAME, CONF_FILE_EXT, NULL);
	key_file = g_key_file_new();
	
	g_key_file_set_comment(key_file, NULL, NULL, SETTINGS_COMMENT, NULL);

	g_key_file_set_string(key_file, GROUP_SETTINGS, KEY_VERSION, PACKAGE_VERSION);
	g_key_file_set_integer(key_file, GROUP_SETTINGS, KEY_ACCEL_KEY, app_hotkey.accel_key);
	g_key_file_set_integer(key_file, GROUP_SETTINGS, KEY_ACCEL_MODS, app_hotkey.accel_mods);
	g_key_file_set_integer(key_file, GROUP_SETTINGS, KEY_ACCEL_FLAGS, app_hotkey.accel_flags);
	g_key_file_set_boolean(key_file, GROUP_SETTINGS, KEY_MODE, advanced_mode);
	g_key_file_set_boolean(key_file, GROUP_SETTINGS, KEY_NOTIFICATIONS, notifier_enabled);

	file_contents = g_key_file_to_data(key_file, &file_len, NULL);
	
	if(!g_file_set_contents(conf_file_path, file_contents, file_len, &err))
	{
		g_warning("Failed to save preferences to %s\nError: %s\n", conf_file_path, err->message);
		g_error_free(err);
	}
	
	g_free(file_contents);

	g_key_file_free(key_file);
	g_free(conf_file_path);
}

static void about_email_hook(GtkAboutDialog *about_dialog, const gchar *link, gpointer user_data)
{
	GError *err = NULL;
	gchar *final_uri = NULL;

	final_uri = g_strconcat(MAILTO_PREFIX, link, NULL);

	if(!fp_show_uri(NULL, final_uri, GDK_CURRENT_TIME, &err))
	{
		g_warning("Email gtk_show_uri error: %s\n", err->message);
		g_error_free(err);
	}

	g_free(final_uri);
}

static void about_url_hook(GtkAboutDialog *about_dialog, const gchar *link, gpointer user_data)
{
	GError *err = NULL;

	if(!fp_show_uri(NULL, link, GDK_CURRENT_TIME, &err))
	{
		g_warning("URL gtk_show_uri error: %s\n", err->message);
		g_error_free(err);
	}
}

static gboolean wordnet_terms_load(GtkBuilder *gui_builder)
{
	gchar *index_file_path = NULL;
	gchar *contents = NULL;
	GString *word, *prev_word = NULL;
	gchar ch = 0;
	guint32 i = 0;
	guint32 count = 0;
	gboolean ret_val = FALSE;
	GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
	GtkStatusbar *status_bar = GTK_STATUSBAR(gtk_builder_get_object(gui_builder, STATUSBAR));
	gchar status_msg[MAX_STATUS_MSG] = "";

	if(SetSearchdir())
	{
		index_file_path = g_strdup_printf(SENSEIDXFILE, SetSearchdir());
		
		if(g_file_get_contents(index_file_path, &contents, NULL, NULL))
		{
			word = g_string_new("");
			prev_word = g_string_new("");
			wordnet_terms = g_string_new("");

			// set the loading message on status bar
			gtk_statusbar_pop(status_bar, status_msg_context_id);
			g_snprintf(status_msg, MAX_STATUS_MSG, STR_STATUS_INDEXING);
			status_msg_context_id = gtk_statusbar_get_context_id(status_bar, STATUS_DESC_LOADING_INDEX);
			gtk_statusbar_push(status_bar, status_msg_context_id, status_msg);

			// if visible, repaint the statusbar after setting the status message, for it to get reflected
			if(GTK_WIDGET_VISIBLE(window))
				gdk_window_process_updates(((GtkWidget*)status_bar)->window, FALSE);

			do
			{
				ch = contents[i++];
				
				if(ch == '_') ch = ' ';

				if(ch == '%')
				{
					for(; ((contents[i] != '\n') && (contents[i] != '\0')); i++);

					ch = contents[i++];
					word = g_string_append_c(word, ch);

					if(!g_string_equal(prev_word, word))
					{
						wordnet_terms = g_string_append(wordnet_terms, word->str);
						g_string_erase(prev_word, 0, prev_word->len);
						g_string_append(prev_word, word->str);

						count++;
					}
					word = g_string_erase(word, 0, word->len);
				}
				else
				{
					word = g_string_append_c(word, ch);
				}

			} while(ch != '\0');

			// put up the EOF char in place of the lastly appended '\n'
			// Note: since it's a GString, after this '\n' there will be
			// a default '\0' managed by GLib; GString->len gives you
			// the length without counting this char.
			wordnet_terms->str[wordnet_terms->len - 1] = ch;
			
			G_DEBUG("Total Dict. Terms Loaded: %d\n", ++count);

			g_free(contents);
			g_string_free(word, TRUE);
			g_string_free(prev_word, TRUE);
			
			ret_val = TRUE;

			// clear the loading message from status bar
			gtk_statusbar_pop(status_bar, status_msg_context_id);
		}
		
		g_free(index_file_path);
	}
	
	return ret_val;
}

static void lookup_ignorable_modifiers ()
{
	GdkKeymap *keymap = gdk_keymap_get_default();

	/* caps_lock */
	egg_keymap_resolve_virtual_modifiers (keymap, EGG_VIRTUAL_LOCK_MASK, &caps_lock_mask);
	/* num_lock */
	egg_keymap_resolve_virtual_modifiers (keymap, EGG_VIRTUAL_NUM_LOCK_MASK, &num_lock_mask);
	/* scroll_lock */
	egg_keymap_resolve_virtual_modifiers (keymap, EGG_VIRTUAL_SCROLL_LOCK_MASK, &scroll_lock_mask);
}

gboolean grab_ungrab_with_ignorable_modifiers (GtkAccelKey *binding, gboolean grab)
{
	guint i = 0, actual_mods = 0;;
	Window rootwin = XDefaultRootWindow(dpy);
	guint mod_masks [] =
	{
		0, /* modifier only */
		num_lock_mask,
		caps_lock_mask,
		scroll_lock_mask,
		num_lock_mask  | caps_lock_mask,
		num_lock_mask  | scroll_lock_mask,
		caps_lock_mask | scroll_lock_mask,
		num_lock_mask  | caps_lock_mask | scroll_lock_mask,
	};

	egg_keymap_resolve_virtual_modifiers (gdk_keymap_get_default(), binding->accel_mods, &actual_mods);

	for (i = 0; (i < G_N_ELEMENTS (mod_masks) && (False == x_error)); i++) 
	{
		if (grab)
		{
			XGrabKey (dpy, 
				  XKeysymToKeycode(dpy, binding->accel_key), 
				  actual_mods | mod_masks [i], 
				  rootwin, 
				  False, 
				  GrabModeAsync,
				  GrabModeAsync);
		}
		else
		{
			XUngrabKey (dpy,
				    XKeysymToKeycode(dpy, binding->accel_key),
				    actual_mods | mod_masks [i], 
				    rootwin);
		}
	}

	if (x_error)
	{
		x_error = False;
		return FALSE;
	}

	return TRUE;
}

static gboolean register_unregister_hotkey(gboolean first_run, gboolean setup_hotkey)
{
	guint i = 0;

	/* if it's first run and hotkey needs to be setup; try with the preset hotkey trials
	   else just grab or ungrab as per the last param passed */
	if(first_run && setup_hotkey)
	{
		app_hotkey.accel_mods = GDK_CONTROL_MASK | GDK_MOD1_MASK;

		for(i = 0; i < G_N_ELEMENTS(hotkey_trials); ++i)
		{
			app_hotkey.accel_key = hotkey_trials[i];

			if(grab_ungrab_with_ignorable_modifiers(&app_hotkey, TRUE))
				return TRUE;
		}

		/* when control reaches here, it means hotkey couldn't be set; but since
		it's first run, save_pref will be called and whatever value is  present
		in app_hotkey (last key in hotkey_trials) will be saved; so 0 it */
		app_hotkey.accel_key = app_hotkey.accel_mods = app_hotkey.accel_flags = 0;
		return FALSE;
	}

	return(grab_ungrab_with_ignorable_modifiers(&app_hotkey, setup_hotkey));
}

static void setup_hotkey_editor(GtkBuilder *gui_builder, GtkWidget **hotkey_editor_dialog)
{
	*hotkey_editor_dialog = GTK_WIDGET(gtk_builder_get_object(gui_builder, DIALOG_HOTKEY));
	GtkLabel *hotkey_label = GTK_LABEL(gtk_builder_get_object(gui_builder, DIALOG_HOTKEY_LABEL));
	GtkContainer *hbox = GTK_CONTAINER(gtk_builder_get_object(gui_builder, DIALOG_HOTKEY_HBOX));	

	GtkWidget *hotkey_accel_cell = create_hotkey_editor();
	gtk_container_add(hbox, hotkey_accel_cell);

	gtk_label_set_mnemonic_widget(hotkey_label, hotkey_accel_cell);

	g_signal_connect(GTK_WIDGET(*hotkey_editor_dialog), "response", G_CALLBACK(gtk_widget_hide), NULL);
}

static void show_hotkey_editor(GtkToolButton *toolbutton, gpointer user_data)
{
	GtkAccelKey hotkey_backup = app_hotkey;
	gint hotkey_dialog_response = gtk_dialog_run(GTK_DIALOG(user_data));

	/* if prev. hotkey and app_hotkey are not the same and 'Apply' wasn't clicked
	   then revert hotkey before saving the preferences */
	if(hotkey_backup.accel_key != app_hotkey.accel_key || 
	   hotkey_backup.accel_mods != app_hotkey.accel_mods || 
	   hotkey_backup.accel_flags != app_hotkey.accel_flags)
	{
		if(GTK_RESPONSE_APPLY != hotkey_dialog_response)
		{
			grab_ungrab_with_ignorable_modifiers(&app_hotkey, FALSE);
			app_hotkey = hotkey_backup;
			grab_ungrab_with_ignorable_modifiers(&app_hotkey, TRUE);
		}

		/* set hotkey flag */
		hotkey_set = (gboolean) app_hotkey.accel_key;

		save_preferences();
	}
}

static void destructor(GtkBuilder *gui_builder)
{
	if(wordnet_terms)
	{
	        g_string_free(wordnet_terms, TRUE);
	        wordnet_terms = NULL;
	}

	if(last_search)
		g_free(last_search);

	/* be responsible, give back the OS what you got from it :) */
	if(results)
	{
		wni_free(&results);
		results = NULL;
	}

	/* if hotkey is registered, unregister it */
	if(hotkey_set)
	{
		// remove the preset event filter
		gdk_window_remove_filter(gdk_get_default_root_window(), hotkey_pressed, gui_builder);

		register_unregister_hotkey(FALSE, FALSE);

		hotkey_set = FALSE;
	}

	if(mod_suggest)
	{
		suggestions_uninit();
		mod_suggest = FALSE;
	}

	if(notifier)
	{
		mod_notify_uninit();
	}
}

static void show_message_dlg(GtkWidget *parent_window, MessageResposeCode msg_code)
{
	GtkWidget *msg_dialog = NULL;
	GtkMessageType msg_type = GTK_MESSAGE_INFO;
	gchar *str_body = NULL;
	gchar *str_temp1 = NULL, *str_temp2 = NULL;

	if(MSG_DB_LOAD_ERROR == msg_code)
	{
		str_body = STR_MSG_WN_ERROR;
		msg_type = GTK_MESSAGE_ERROR;
	}
	else if(MSG_HOTKEY_SUCCEEDED_FIRST_RUN == msg_code)
	{
		str_temp1 = gtk_accelerator_get_label(app_hotkey.accel_key, app_hotkey.accel_mods);
		str_temp2 = g_strdup_printf(STR_MSG_WELCOME_ARBITRARY_SUCCEEDED, str_temp1);
		g_free(str_temp1); str_temp1 = NULL;

		str_body = str_temp1 = g_strdup_printf("%s\n\n%s %s", 
					STR_MSG_WELCOME_HOTKEY_HEADER, 
					str_temp2, 
					STR_MSG_WELCOME_HOTKEY_FOOTER);
	}
	else if(MSG_HOTKEY_FAILED_FIRST_RUN == msg_code)
	{
		str_body = str_temp1 = g_strdup_printf("%s\n%s", 
					STR_MSG_WELCOME_HOTKEY_HEADER,  
					STR_MSG_WELCOME_HOTKEY_FOOTER);
	}
	else if(MSG_HOTKEY_FAILED == msg_code)
	{
		str_temp1 = gtk_accelerator_get_label(app_hotkey.accel_key, app_hotkey.accel_mods);
		str_temp2 = g_strdup_printf(STR_MSG_WELCOME_HOTKEY_FAILED, str_temp1);
		g_free(str_temp1); str_temp1 = NULL;

		str_body = str_temp1 = g_strdup_printf("%s %s", str_temp2, STR_MSG_WELCOME_HOTKEY_FOOTER);
		msg_type = GTK_MESSAGE_WARNING;
	}
	else if(MSG_HOTKEY_NOTSET == msg_code)
	{
		str_body = STR_MSG_HOTKEY_NOTSET;
		msg_type = GTK_MESSAGE_WARNING;
	}

	msg_dialog = gtk_message_dialog_new_with_markup(parent_window ? GTK_WINDOW(parent_window) : NULL, 
							GTK_DIALOG_MODAL, 
							msg_type, 
							GTK_BUTTONS_OK,
							str_body, NULL);

	g_object_set(msg_dialog, "title", PACKAGE_NAME, NULL);
	gtk_dialog_run(GTK_DIALOG(msg_dialog));
	gtk_widget_destroy(msg_dialog);

	if(str_temp1)
	{
		g_free(str_temp1);
		str_temp1 = NULL;
	}

	if(str_temp2)
	{
		g_free(str_temp2);
		str_temp2 = NULL;
	}
}

int main(int argc, char *argv[])
{
	GtkBuilder *gui_builder = NULL;
	GtkWidget *window = NULL, *button_search = NULL, *combo_query = NULL, *combo_entry = NULL, *hotkey_editor_dialog = NULL;
	GtkStatusIcon *status_icon = NULL;
	GtkMenu *popup_menu = NULL;
	GtkExpander *expander = NULL;
	GdkPixbuf *app_icon = NULL;
	GModule *app_mod = NULL;
	GError *err = NULL;
	gboolean first_run = FALSE;
	gchar *ui_file_path = NULL, *icon_file_path = NULL;

	g_set_application_name(PACKAGE_NAME);

	if(gtk_init_check(&argc, &argv))
	{
		/* if we're not the first instance of artha, then quit */
		if(FALSE == instance_handler_am_i_unique())
		{
			/* signal the primary instance of this invocation */
			instance_handler_send_signal();

			/* notify the desktop env. that the start up is complete */
			gdk_notify_startup_complete();

			return 0;
		}

		gui_builder = gtk_builder_new();
		if(gui_builder)
		{
			ui_file_path = g_build_filename(APP_DIR, UI_FILE, NULL);
			if(gtk_builder_add_from_file(gui_builder, ui_file_path, &err))
			{
				/* if WordNet's database files are not opened, open it */
				if((OpenDB != 1) && (wninit() != 0))
						show_message_dlg(NULL, MSG_DB_LOAD_ERROR);

				window = GTK_WIDGET(gtk_builder_get_object(gui_builder, WINDOW_MAIN));
				if(window)
				{
					/* if the control's here, then it's sure that we're the unique instance;
					register for dbus signals from possible duplicate instances */
					if(!instance_handler_register_signal(GTK_WINDOW(window)))
					{
						g_error("Unable to register for duplicate instance signals!\n");
					}

					/* try to load preferences and see if this is the first run */
					first_run = load_preferences(GTK_WINDOW(window));

					/* Most Important: do not use the XServer's XGetDisplay, you will have to do XNextEvent (blocking) to 
					   get the event call, so get GDK's display; its X11 display equivalent */
					dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
					if (NULL == dpy)
					{
						g_error("Can't open Display %s!\n", gdk_display_get_name(gdk_display_get_default()));
						return -1;
					}

					G_DEBUG("XDisplay Opened!\n");

					XSynchronize(dpy, True);		/* Without calling this you get the error call back dealyed, much delayed! */
					XSetErrorHandler(x_error_handler);	/* Set the error handler for setting the flag */

					lookup_ignorable_modifiers();

					if(app_hotkey.accel_key || first_run)
					{
						if(register_unregister_hotkey(first_run, TRUE))
						{
							hotkey_set = TRUE;

							if(first_run)
							{
								show_message_dlg(window, MSG_HOTKEY_SUCCEEDED_FIRST_RUN);
							}

							/* Add a filter function to handle low level events, like X events.
							   For Param1 use gdk_get_default_root_window() instead of NULL or window, so that
							   only when hotkey combo is pressed will the filter func. be called, unlike
							   others, where it will be called for all events beforehand GTK handles */
							gdk_window_add_filter(gdk_get_default_root_window(), hotkey_pressed, gui_builder);
						}
						else
						{
							if(first_run)
								show_message_dlg(window, MSG_HOTKEY_FAILED_FIRST_RUN);
							else
							{
								show_message_dlg(window, MSG_HOTKEY_FAILED);
								/* since the previously set hotkey is now not registerable, memzero
								   (i.e. disable) app_hotkey and save prefs */
								app_hotkey.accel_key = app_hotkey.accel_mods = app_hotkey.accel_flags = 0;
							}
						}
					}

					/* save preferences here - after all setting based loads are done */
					save_preferences();
					
					setup_hotkey_editor(gui_builder, &hotkey_editor_dialog);

					icon_file_path = g_build_filename(ICON_DIR, ICON_FILE, NULL);
					if(g_file_test(icon_file_path, G_FILE_TEST_IS_REGULAR))
					{
						status_icon = gtk_status_icon_new_from_file(icon_file_path);
						gtk_status_icon_set_tooltip(status_icon, STR_APP_TITLE);
						gtk_status_icon_set_visible(status_icon, TRUE);
					}
					else
					{
						g_warning("Error loading icon file!\n%s not found!\n", icon_file_path);
					}
					g_free(icon_file_path);
					icon_file_path = NULL;

					if(status_icon)
						mod_notify_init(status_icon);

					/* pop-up menu creation should be after assessing the availability of notifications
					   since if it is not available, the Notify menu option can be stripped
					   create pop-up menu */
					popup_menu = create_popup_menu(gui_builder);

					g_signal_connect(status_icon, "activate", G_CALLBACK(status_icon_activate), gui_builder);
					g_signal_connect(status_icon, "popup-menu", G_CALLBACK(status_icon_popup), GTK_WIDGET(popup_menu));


					// using the status icon, app. icon is also set
					g_object_get(status_icon, "pixbuf", &app_icon, NULL);
					if(app_icon)
					{
						gtk_window_set_default_icon(app_icon);
						g_object_unref(app_icon);
					}


					button_search = GTK_WIDGET(gtk_builder_get_object(gui_builder, BUTTON_SEARCH));
					g_signal_connect(button_search, "clicked", G_CALLBACK(button_search_click), gui_builder);

					combo_query = GTK_WIDGET(gtk_builder_get_object(gui_builder, COMBO_QUERY));
					g_signal_connect(combo_query, "changed", G_CALLBACK(combo_query_changed), gui_builder);

					/* get the GtkEntry in GtkComboBoxEntry and set activates-default to TRUE; so that
					   it pass the ENTER key signal to query button */
					combo_entry = gtk_bin_get_child(GTK_BIN(combo_query));
					g_object_set(combo_entry, "activates-default", TRUE, NULL);


					create_text_view_tags(gui_builder);

					setup_toolbar(gui_builder, hotkey_editor_dialog);

					/* do main window specific connects */
					g_signal_connect_swapped(window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), window);
					
					// connect to Escape Key
					g_signal_connect(window, "key-press-event", G_CALLBACK(window_main_key_press), combo_query);
					g_signal_connect(combo_query, "scroll-event", G_CALLBACK(combo_query_scroll), button_search);
					
					expander = GTK_EXPANDER(gtk_builder_get_object(gui_builder, EXPANDER));
					g_signal_connect(expander, "activate", G_CALLBACK(expander_clicked), gui_builder);

					gtk_widget_grab_focus(GTK_WIDGET(combo_query));

					G_DEBUG("GUI loaded successfully!\n");
					
					create_stores_renderers(gui_builder);

					app_mod = g_module_open(NULL, G_MODULE_BIND_LAZY);
					if(app_mod)
					{
						if(g_module_symbol(app_mod, "gtk_show_uri", (gpointer*) &fp_show_uri))
						{
							gtk_about_dialog_set_email_hook(about_email_hook, (gpointer*) fp_show_uri, NULL);
							gtk_about_dialog_set_url_hook(about_url_hook, (gpointer*) fp_show_uri, NULL);
						}
						else
							G_DEBUG("Cannot find gtk_show_uri function symbol!\n");

						g_module_close(app_mod);
					}
					else
						g_warning("Cannot open module!\n");
					
					mod_suggest = suggestions_init();

					// show the window if it's a first run or a hotkey couldn't be set
					// if the window is not shown, set notify the startup is complete
					if(first_run || x_error)
						gtk_widget_show_all(window);
					else
						gdk_notify_startup_complete();

					// index all wordnet terms from the index.sense onto memory
					wordnet_terms_load(gui_builder);

					gtk_main();

					// since every setting change will call save prefs.
					// saving the pref. on exit isn't required
					//save_preferences(selected_key + 1);

					/* GtkBuilder drops references to any held, except toplevel widgets */
					gtk_widget_destroy(hotkey_editor_dialog);	
					gtk_widget_destroy(window);
					
					
					destructor(gui_builder);
				}
				else
				{
					g_error("Error loading GUI!\n Corrupted data in UI file!\n");
				}
			}
			else
			{
				g_error("Error loading GUI from %s!\nError Message: %s\n", ui_file_path, err->message);
				g_error_free(err);
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

