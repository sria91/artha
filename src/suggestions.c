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


/*
	Code suggestions.c

	Dynamic loading of libenchant for spell checks using 
	GLib's GModule. Function prototypes are taken from 
	suggestions.h
*/


#include "suggestions.h"


gchar** suggestions_get(gchar *lemma)
{
	gchar **suggestions = NULL;
	gchar **valid_suggestions = NULL;
	size_t suggestions_count = 0, i = 0, valid_count = 0;
	GSList *str_list = NULL, *temp_list = NULL;

	// check if Enchant broker and English dictionary are initialized
	// also check if WordNet database files are opened
	if(enchant_broker && enchant_dict && (1 == OpenDB))
	{
		if(enchant_dict_check(enchant_dict, lemma, -1))
		{
			suggestions = enchant_dict_suggest(enchant_dict, lemma, -1, &suggestions_count);
			for(i = 0; i < suggestions_count; i++)
			{
				if(wni_request_nyms(suggestions[i], NULL, 0, FALSE))
				{
					str_list = g_slist_prepend(str_list, g_strdup(suggestions[i]));
					++valid_count;
				}
			}
			
			if(valid_count)
			{
				valid_suggestions = g_new0(gchar*, valid_count + 1);

				for(temp_list = str_list; temp_list; temp_list = g_slist_next(temp_list))
					valid_suggestions[--valid_count] = temp_list->data;

				g_slist_free(str_list);
			}
			
			if(suggestions)
				enchant_dict_free_string_list(enchant_dict, suggestions);
		}
	}
	
	return valid_suggestions;
}

void find_dictionary(const char * const lang_tag, const char * const provider_name,
						  const char * const provider_desc,
						  const char * const provider_file,
						  void * user_data)
{
	gchar *str_dict_tag = user_data;

	if(str_dict_tag[0] == '\0')
		if(g_str_has_prefix(lang_tag, DICT_ENGLISH_PREFIX))
		{
			g_snprintf(str_dict_tag, DICT_TAG_MAX_LENGTH, "%s", lang_tag);
			G_PRINTF("Alternative dict '%s' selected!\n", lang_tag);
		}
}

gboolean suggestions_init()
{
	gboolean dict_initialized = FALSE;
	gchar dict_tag[DICT_TAG_MAX_LENGTH] = DICT_TAG_ENGLISH;

	if(g_module_supported() && (mod_enchant = g_module_open(ENCHANT_FILE, G_MODULE_BIND_LAZY)))
	{
		g_module_symbol(mod_enchant, FUNC_BROKER_INIT, (gpointer *) &enchant_broker_init);
		g_module_symbol(mod_enchant, FUNC_BROKER_FREE, (gpointer *) &enchant_broker_free);
		g_module_symbol(mod_enchant, FUNC_BROKER_LIST_DICTS, (gpointer *) &enchant_broker_list_dicts);
		g_module_symbol(mod_enchant, FUNC_BROKER_DICT_EXISTS, (gpointer *) &enchant_broker_dict_exists);
		g_module_symbol(mod_enchant, FUNC_BROKER_REQ_DICT, (gpointer *) &enchant_broker_request_dict);
		g_module_symbol(mod_enchant, FUNC_BROKER_FREE_DICT, (gpointer *) &enchant_broker_free_dict);
		g_module_symbol(mod_enchant, FUNC_DICT_CHECK, (gpointer *) &enchant_dict_check);
		g_module_symbol(mod_enchant, FUNC_DICT_SUGGEST, (gpointer *) &enchant_dict_suggest);
		g_module_symbol(mod_enchant, FUNC_DICT_FREE_STRINGS, (gpointer *) &enchant_dict_free_string_list);

		// in older version of Enchant, enchant_dict_free_string_list might be absent, they will have the
		// now deprecated function enchant_dict_free_suggestions
		if(NULL == enchant_dict_free_string_list)
			g_module_symbol(mod_enchant, FUNC_DICT_FREE_SUGGESTS, (gpointer *) &enchant_dict_free_string_list);

		// check if we have obtained the essential function pointers
		if(NULL != enchant_broker_init && NULL != enchant_broker_free && NULL != enchant_broker_dict_exists &&
		NULL != enchant_broker_request_dict && NULL != enchant_broker_free_dict && NULL != enchant_dict_check && 
		NULL != enchant_dict_suggest && NULL != enchant_dict_free_string_list)
		{
			enchant_broker = enchant_broker_init();
			if(enchant_broker)
			{
				if(!enchant_broker_dict_exists(enchant_broker, dict_tag) && enchant_broker_list_dicts)
				{
					dict_tag[0] = '\0';

					G_PRINTF("Suggestions: Couldn't find dict 'en'. Looking for alternatives...\n");
					enchant_broker_list_dicts(enchant_broker, find_dictionary, dict_tag);
				}

				if(dict_tag[0] != '\0')
				{
					enchant_dict = enchant_broker_request_dict(enchant_broker, dict_tag);
					if(enchant_dict) dict_initialized = TRUE;
				}

				if(!dict_initialized)
				{
					enchant_broker_free(enchant_broker);
					enchant_broker = NULL;

					g_module_close(mod_enchant);
					mod_enchant = NULL;
				}
			}
		}
	}
	
	return dict_initialized;
}

gboolean suggestions_uninit()
{
	gboolean free_success = TRUE;

	if(enchant_dict)
		enchant_broker_free_dict(enchant_broker, enchant_dict);
	if(enchant_broker)
		enchant_broker_free(enchant_broker);

	enchant_dict = NULL;
	enchant_broker = NULL;

	if(mod_enchant)	
		free_success = g_module_close(mod_enchant);

	mod_enchant = NULL;
	
	return free_success;
}

