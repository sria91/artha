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


/* gui.h: GUI Header */


#include "wni.h"
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include "addons.h"

#ifdef HAVE_CONFIG_H

#include <config.h>

//#ifdef G_OS_UNIX
#ifdef NOTIFIER_SUPPORT
	#define NOTIFY
#endif
//#endif

#define MAILTO_PREFIX "mailto:"

#if DEBUG_LEVEL >= 1
	#define G_DEBUG(format, args...) g_debug(format, ##args)
#else
	#define G_DEBUG(format, ...) 
#endif

#endif

#ifdef NOTIFY
#include <libnotify/notify.h>
#include <dbus/dbus-glib.h>
#endif

#define NEW_LINE	"\r\n"
#define ICON_FILE	"/artha.png"
#define UI_FILE		"/gui.ui"

#define ARTHA_RESPONSE_REPORT_BUG	1
#define BUTTON_TEXT_BUG			"Report a _Bug"

#define SETTINGS_COMMENT	"Artha Preferences File"
#define GROUP_SETTINGS		"Settings"
#define KEY_HOTKEY_INDEX	"Hotkey"
#define KEY_VERSION		"Version"
#define KEY_MODE		"DetailedMode"
#ifdef NOTIFY
	#define KEY_NOTIFICATIONS "Notifications"
#endif

#define WINDOW_MAIN			"wndMain"
#define BUTTON_SEARCH			"btnSearch"
#define TEXT_VIEW_DEFINITIONS		"txtDefinitions"
#define COMBO_QUERY			"cboQuery"
#define TOOLBAR				"toolbar"
#define NOTEBOOK			"notebook"
#define EXPANDER			"expander"
#define STATUSBAR			"statusbar"
#define LABEL_ATTRIBUTES		"lblAttributes"
#define LABEL_TEXT_ATTRIBUTES		"Attributes"
#define LABEL_TEXT_ATTRIBUTE_OF		"Attribute of"

#define MAX_CONCAT_STR		500
#define MAX_STATUS_MSG		75

#define TREE_SYNONYMS		0
#define TREE_ANTONYMS		1
#define TREE_DERIVATIVES	2
#define TREE_PERTAINYMS		3
#define TREE_ATTRIBUTES		4
#define TREE_SIMILAR		5
#define TREE_DOMAIN		6
#define TREE_CAUSES		7
#define TREE_ENTAILS		8
#define TREE_HYPERNYMS		9
#define TREE_HYPONYMS		10
#define TREE_HOLONYMS		11
#define TREE_MERONYMS		12

#define TOTAL_RELATIVES		TREE_MERONYMS + 1

// Artha Global variables

// Names of relative tree tab widgets from UI file
// Note that the 'tree" prefix will be stripped and will be used within code
gchar *relative_tree[] = {"treeSynonyms", "treeAntonyms", "treeDerivatives", "treePertainyms", "treeAttributes", "treeSimilar", 
"treeDomain", "treeCauses", "treeEntails", "treeHypernyms", "treeHyponyms", "treeHolonyms", "treeMeronyms"};

#define DOMAINS_COUNT (CLASS_END - CLASSIF_START + 1)
gchar *domain_types[] = {"Topic", "Usage", "Region", "Topic Terms", "Usage Terms", "Regional Terms"};

//gchar *list_type[] = {"MEMBER OF", "SUBSTANCE OF", "PART OF", "MEMBERS", "HAS SUBSTANCE", "PARTS"};
//gchar *hypernym_type[] = {"INSTANCE OF", "INSTANCES"};

#define FAMILIARITY_COUNT	8
gchar *familiarity[] = {"extremely rare","very rare","rare","uncommon","common", "familiar","very familiar","extremely familiar"};
gchar *freq_colors[] = {"Black", "SaddleBrown", "FireBrick", "SeaGreen", "DarkOrange", "gold", "PaleGoldenrod", "PeachPuff1"};
//none, scroll (v), scroll (n), alright, sequence, set (n), set (v), give

#define	TAG_LEMMA	"tag_lemma"
#define TAG_POS		"tag_pos"
#define TAG_COUNTER	"tag_counter"
#define TAG_EXAMPLE	"tag_example"
#define TAG_HIGHLIGHT	"tag_highlight"
#define TAG_SUGGEST	"tag_suggest"
#define TAG_SUGGESTION	"tag_suggestion"

#define STR_SUGGEST_MATCHES		"Near matches found:"

#define DIRECT_ANTONYM_HEADER		"Direct Antonyms"
#define	INDIRECT_ANTONYM_HEADER		"Indirect Antonyms"
#define	INDIRECT_ANTONYM_COLUMN_HEADER	"Indirect via Similar Term"

#define	MAX_SENSE_DIGITS	5
Bool 		x_error = False;
GSList 		*results = NULL;
gchar 		*last_search = NULL;
gboolean 	was_double_click = FALSE, last_search_successful = FALSE, advanced_mode = FALSE, mod_suggest = FALSE;
gint8		hotkey_index;
guint 		hot_key_vals[] = {GDK_w, GDK_a, GDK_t, GDK_q};
gint		history_count = 0;

#ifdef NOTIFY
gboolean 		notifier_enabled = FALSE;
NotifyNotification	*notifier = NULL;
GtkCheckMenuItem	*menu_notify = NULL;
GtkToolItem		*toolbar_notify = NULL;
#endif

// Artha App. Strings

#define STR_TOOLITEM_QUIT	"Q_uit"
#define STR_TOOLITEM_ABOUT	"_About"
#define STR_TOOLITEM_NOTIFY	"N_otify"
#define STR_TOOLITEM_MODE	"_Detailed"
#define STR_TOOLITEM_NEXT	"_Next"
#define STR_TOOLITEM_PREV	"_Previous"

#define ABOUT_HOTKEY_SET	"The hot key set to summon Artha is Ctrl + Alt + "

#define QUIT_TOOLITEM_TOOLTIP	"Exit altogether. To minimize to system tray, click the Close Window (X) button on the title bar or the system try icon or press Esc"
#define ABOUT_TOOLITEM_TOOLTIP	"About Artha -> Copyright, Credits, Licence, etc."
#define PREV_TOOLITEM_TOOLTIP	"Go to the previous search term"
#define NEXT_TOOLITEM_TOOLTIP	"Go to the next search term"
#define MODE_TOOLITEM_TOOLTIP	"Toggle between simple/advanced modes"

#define STR_STATUS_QUERY_SUCCESS "Search complete. Results returned: %d sense(s) in %d POS(s)!"
#define MSG_WN_ERROR		"Failed to open WordNet database files!\n\
Make sure WordNet's database files are present at\n\n%s.\n\nIf present elsewhere, set the environment variable WNHOME to point to it."
#define STR_QUERY_FAILED	"Queried string not found in thesaurus!"
#define STR_STATUS_QUERY_FAILED	"Oops! Search string not found!"

#ifdef NOTIFY
#define NOTIFY_TOOLITEM_TOOLTIP	"Notify: When in the system tray, if called by the hot key, instead of popping up, Artha will show a notification of the selected term's definition"
#define NOTIFY_QUERY_FAIL_TITLE	"Oops!"
#define NOTIFY_QUERY_FAIL_BODY	"Queried term not found!"
#endif

#define WELCOME_TITLE		"Welcome to Artha!"
#define WELCOME_UPGRADED	"Thank you for updating Artha to a newer version!"
#define WELCOME_HOTKEY_NORMAL	"The hot key set for Artha is <b>Ctrl + Alt + %c</b>."

#define WELCOME_HOTKEY_INFO	" Press this key combination to call Artha from the system tray. Selecting text \
in any window and calling Artha will pop it up with the selected text's definitions."
#define WELCOME_MANUAL		"\n\nRefer manual ('man artha' in terminal) for detailed info/help."

#ifdef NOTIFY
#define WELCOME_NOTIFY		"\n\nIf notifications are enabled, instead of popping up, Artha will \
notify the first (prime) definition of the selection. Notifications can be enabled/disabled by \
right-clicking on Artha's status icon on the system tray and selecting the required option."
#endif

#define WELCOME_NOHOTKEY	"Artha tried to set one of the hot key combos \
<b>Ctrl + Alt + [W|A|T|Q]</b> and found all of them to be already occupied by some other \
application. Release atleast one of them and restart Artha to use the hot key feature.\n\nIf this \
feature is enabled Artha can be called from any window, selecting some text, it will pop up with \
the definitions of the selected text. This feature is also required to enable Notifications."

#define WELCOME_NOTE_HOTKEY_CHANGED	"Artha's hot key is now changed to <b>Ctrl + Alt + %c</b>."


#define STRING_COPYRIGHT	"Copyright © 2009  Sundaram Ramaswamy. All Rights Reserved.\n\nWordNet 3.0 \
Copyright 2006 by Princeton University.  All rights reserved."

#define STRING_WEBSITE		"http://artha.sourceforge.net/"

#define STRING_BUG_WEBPAGE	"http://launchpad.net/artha/+filebug"

#define STRING_WEBSITE_LABEL	"Artha Homepage"

#define STRING_ABOUT		"A handy off-line thesaurus based on WordNet"

#define STRING_LICENCE "Artha is free software; you can redistribute it and/or modify it under the terms of \
the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, \
or (at your option) any later version.\n\nArtha is distributed in the hope that it will be useful, but WITHOUT \
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See \
the GNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public \
License along with Artha; if not, see <www.gnu.org/licenses>.\n\nWordNet 3.0 Disclaimer\nTHIS SOFTWARE AND \
DATABASE IS PROVIDED \"AS IS\" AND PRINCETON UNIVERSITY MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR \
IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, PRINCETON UNIVERSITY MAKES NO REPRESENTATIONS OR WARRANTIES \
OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF THE LICENSED SOFTWARE, DATABASE OR \
DOCUMENTATION WILL NOT INFRINGE ANY THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS."

gchar *strv_authors[] = {"Sundaram Ramaswamy <legends2k@yahoo.com>"};


// Dynamically loaded gtk_show_uri function's prototype
typedef gboolean (*ShowURIFunc) (GdkScreen *screen, const gchar *uri, guint32 timestamp, GError **error);
ShowURIFunc fp_show_uri = NULL;


