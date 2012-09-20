/* gui.h
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
 * GUI Header declarations
 */


#ifndef __GUI_H__
#define __GUI_H__


#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <wn.h>
#include <string.h>

#ifdef X11_AVAILABLE
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#endif

/* custom headers */
#include "wni.h"
#include "instance_handler.h"
#include "eggaccelerators.h"
#include "hotkey_editor.h"
#include "mod_notify.h"
#include "tomboyutil.h"

/* pluggable module headers */
#include "addons.h"

#ifdef G_OS_WIN32
#	include "gdk/gdkwin32.h"
#	include "windows.h"
#	include "shellapi.h"
#	define KEY_COUNT 4
#	define WM_ARTHA_RELAUNCH	(WM_USER + 100)
#endif	// G_OS_WIN32

/* general constants */
#define NEW_LINE		"\r\n"
#define ICON_FILE		"artha.png"
#define UI_FILE			"gui.glade"
#define MAX_CONCAT_STR		500
#define MAX_STATUS_MSG		100
#define	MAX_SENSE_DIGITS	5

/* about box constants */
#define MAILTO_PREFIX "mailto:"
#define ARTHA_RESPONSE_REPORT_BUG	1
#define STR_REPORT_BUG		"Report a _Bug"

/* conf file constants */
#define CONF_FILE_EXT	".conf"
#define SETTINGS_COMMENT	" Artha Preferences File"
#define GROUP_SETTINGS		"Settings"
#define KEY_ACCEL_KEY		"Accel_Key"
#define KEY_ACCEL_MODS		"Accel_Mods"
#define KEY_ACCEL_FLAGS		"Accel_Flags"
#define KEY_VERSION		"Version"
#define KEY_MODE		"DetailedMode"
#define KEY_NOTIFICATIONS	"Notifications"
#define KEY_POLYSEMY		"Polysemy"
#define KEY_TRAYICON		"TrayIcon"

/* UI element names - refer gui.glade */
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
#define	VPANE				"vpanedMain"
#define DIALOG_HOTKEY			"dlgHotkeyChooser"
#define DIALOG_HOTKEY_LABEL		"lblHotkey"
#define DIALOG_HOTKEY_HBOX		"hboxHotkey"
#define DIALOG_POLYSEMY_CHKBTN		"chkBtnPolysemy"

/* Relative IDs */

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

/* GtkTextView tag constants */
#define	TAG_LEMMA	"tag_lemma"
#define TAG_POS		"tag_pos"
#define TAG_COUNTER	"tag_counter"
#define TAG_EXAMPLE	"tag_example"
#define TAG_HIGHLIGHT	"tag_highlight"
#define TAG_MATCH	"tag_match"
#define TAG_SUGGESTION	"tag_suggestion"

/* App. Strings */
#define STR_SUGGEST_MATCHES			"Matches found:"
#define STR_ANTONYM_HEADER_DIRECT		"Direct Antonyms"
#define	STR_ANTONYM_HEADER_INDIRECT		"Indirect Antonyms"
#define	STR_ANTONYM_HEADER_INDIRECT_VIA		"Indirect via Similar Term"

#define STR_TOOLITEM_QUIT	"Q_uit"
#define STR_TOOLITEM_ABOUT	"_About"
#define STR_TOOLITEM_TRAYICON "_Tray Icon"
#define STR_TOOLITEM_NOTIFY	"N_otify"
#define STR_TOOLITEM_OPTIONS	"_Options"
#define STR_TOOLITEM_MODE	"_Detailed"
#define STR_TOOLITEM_NEXT	"_Next"
#define STR_TOOLITEM_PREV	"_Previous"

#define TOOLITEM_TOOLTIP_QUIT	"Exit altogether. To minimize to system tray, press Esc or click the Close Window (X) button or the system tray icon"
#define TOOLITEM_TOOLTIP_ABOUT	"About Artha -> Copyright, Credits, Licence, etc."
#define TOOLITEM_TOOLTIP_PREV	"Go to the previous search term"
#define TOOLITEM_TOOLTIP_NEXT	"Go to the next search term"
#define TOOLITEM_TOOLTIP_MODE	"Toggle between simple/advanced modes"
#define TOOLITEM_TOOLTIP_OPTIONS "Select preferences like showing polysemy classification and the hotkey to summon Artha from inside a window, after selecting some text in it"
#define TOOLITEM_TOOLTIP_NOTIFY	"Notify: When on the system tray, if called by the hotkey, instead of popping up, Artha will show a notification of the selected term's definition"
#define TOOLITEM_TOOLTIP_TRAYICON "Toggle tray icon visibility. Even when invisible, Artha can be summoned using the set hotkey; notifications will continue to work as usual."

#define STR_STATUS_QUERY_SUCCESS "Results returned: %d sense(s) in %d POS(s)!"
#define STR_MSG_WN_ERROR	"Failed to open WordNet database files!\n\
Make sure WordNet's database files are present at\n\n%s.\n\nIf present elsewhere, set the environment variable WNHOME to point to it."
#define STR_QUERY_FAILED	"Queried string not found in thesaurus!"
#define STR_REGEX_DETECTED	"Regular expression pattern detected"
#define STR_REGEX_FAILED	"No matches found! Please check your expression and try again."
#define STR_REGEX_FILE_MISSING	"File index.sense not found at %s\nPlease install it and restart Artha to do a regular expression based search."
#define STR_STATUS_INDEXING	"Indexing... please wait"
#define STR_STATUS_QUERY_FAILED	"Queried term not found!"
#define STR_STATUS_LOOKUP_HINT	"For compound words hold ctrl & drag-sel. whole term to look it up."
#define STR_STATUS_REGEX	"%d match(es) found! %s"
#define STR_STATUS_SEARCHING	"Searching... please wait"
#define STR_STATUS_REGEX_FILE_MISSING	"Error: index.sense not found!"

#define STATUS_DESC_LOADING_INDEX	"loading_index"
#define STATUS_DESC_SEARCH_SUCCESS	"search_successful"
#define STATUS_DESC_SEARCH_FAILURE	"search_failed"
#define STATUS_DESC_REGEX_RESULTS	"regex_mode_results"
#define STATUS_DESC_REGEX_SEARCHING	"regex_mode_searching"
#define STATUS_DESC_REGEX_ERROR		"regex_mode_error"

#define STR_MSG_WELCOME_TITLE		"Welcome to Artha!"
#define STR_MSG_WELCOME_HOTKEY_HEADER	"Artha can be summoned with a hotkey when required. Selecting text in any window and pressing that (hotkey) combination \
will pop up Artha with the selected text looked up."
#define STR_MSG_WELCOME_ARBITRARY_SUCCEEDED	"Since this is the first launch an arbitrary hotkey <b>%s</b> is set."
#ifdef G_OS_WIN32
#define STR_MSG_WELCOME_HOTKEY_FOOTER	"This can be changed via the hotkey button in the toolbar."
#else
#define STR_MSG_WELCOME_HOTKEY_FOOTER	"This can be changed via the hotkey button in the toolbar.\n\nRefer manual ('man artha' in terminal) for detailed help."
#endif
#define STR_MSG_WELCOME_HOTKEY_FAILED	"Unable to set the previously chosen hotkey <b>%s</b> for Artha. This could be because of some other application \
registered with the same key combination."
#define STR_MSG_HOTKEY_NOTSET		"You have enabled notifications; but notifications can only be shown upon pressing a hotkey combination which is \
currently not set. It can be set via the hotkey button in the toolbar."

#define STR_APP_TITLE		"Artha ~ The Open Thesaurus"

#define STR_COPYRIGHT		"Copyright © 2009, 2010  Sundaram Ramaswamy\n\nWordNet 3.0 \
Copyright 2006 - 2010 by Princeton University.  All rights reserved."

#define STR_BUG_WEBPAGE		"http://launchpad.net/artha/+filebug"

#define STR_WEBSITE_LABEL	"Artha Homepage"

#define STR_ABOUT		"A handy off-line thesaurus based on WordNet"

#define STR_LICENCE "Artha is free software; you can redistribute it and/or modify it under the terms of \
the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, \
or (at your option) any later version.\n\nArtha is distributed in the hope that it will be useful, but WITHOUT \
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See \
the GNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public \
License along with Artha; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth \
Floor, Boston, MA  02110-1301  USA.\n\nWordNet 3.0 Disclaimer\nTHIS SOFTWARE AND DATABASE IS PROVIDED \"AS IS\" AND \
PRINCETON UNIVERSITY MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.  BY WAY OF EXAMPLE, BUT NOT \
LIMITATION, PRINCETON UNIVERSITY MAKES NO REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY \
PARTICULAR PURPOSE OR THAT THE USE OF THE LICENSED SOFTWARE, DATABASE OR DOCUMENTATION WILL NOT INFRINGE ANY \
THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS."

typedef enum
{
	MSG_DB_LOAD_ERROR,
	MSG_HOTKEY_FAILED,
	MSG_HOTKEY_SUCCEEDED_FIRST_RUN,
	MSG_HOTKEY_FAILED_FIRST_RUN,
	MSG_HOTKEY_NOTSET
} MessageResposeCode;


/* Global variables */

const gchar *strv_authors[] = {"Sundaram Ramaswamy <legends2k@yahoo.com>", NULL};

/* Names of relative tree tab widgets from UI file
   Note that the 'tree" prefix will be stripped and will be used within code */
const gchar *relative_tree[] = {"treeSynonyms", "treeAntonyms", "treeDerivatives", "treePertainyms", "treeAttributes", "treeSimilar", 
"treeDomain", "treeCauses", "treeEntails", "treeHypernyms", "treeHyponyms", "treeHolonyms", "treeMeronyms"};

#define DOMAINS_COUNT (CLASS_END - CLASSIF_START + 1)
const gchar *domain_types[] = {"Topic", "Usage", "Region", "Topic Terms", "Usage Terms", "Regional Terms"};

#define HOLO_MERO_COUNT		3
const gchar *holo_mero_types[][3] = {{"Member Of", "Substance Of", "Part Of"}, {"Has Members", "Has Substances", "Has Parts"}};

#define FAMILIARITY_COUNT	8
const gchar *familiarity[] = {"extremely rare","very rare","rare","uncommon","common", "familiar","very familiar","extremely familiar"};
const gchar *freq_colors[] = {"Black", "SaddleBrown", "FireBrick", "SeaGreen", "DarkOrange", "gold", "PaleGoldenrod", "PeachPuff1"};
/* words for checking familiarity types - none, scroll (v), scroll (n), alright, sequence, set (n), set (v), give */

/* notifier_enabled is for the setting "Notify" and *notifier is for the module availability */
GSList 			*results = NULL;
gchar 			*last_search = NULL;
gboolean 		was_double_click = FALSE, last_search_successful = FALSE, advanced_mode = FALSE, auto_contract = FALSE, show_trayicon = TRUE;
gboolean		hotkey_set = FALSE, hotkey_processing = FALSE, notifier_enabled = FALSE, mod_suggest = FALSE, show_polysemy = FALSE;
#ifdef X11_AVAILABLE
Display			*dpy = NULL;
guint32			last_hotkey_time = 0;
guint			num_lock_mask = 0, caps_lock_mask = 0, scroll_lock_mask = 0;
#endif
guint 			hotkey_trials[] = {GDK_w, GDK_a, GDK_t, GDK_q};
GtkAccelKey		app_hotkey = {0};
gint			history_count = 0, notify_toolbar_index = -1;
guint			status_msg_context_id = 0;
GString			*wordnet_terms = NULL;
NotifyNotification	*notifier = NULL;
GtkCheckMenuItem	*menu_notify = NULL;

#ifdef G_OS_WIN32
HWND			hMainWindow = NULL;
#endif

#endif		/* __GUI_H__ */

