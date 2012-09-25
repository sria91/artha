/* suggestions.c
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
 * Dynamic loading of libenchant's functions for spell checks.
 */


#include <string.h>
#include <sys/types.h>
#include <gmodule.h>
#include "suggestions.h"
#include "wni.h"

#ifdef _WIN32
#	define ENCHANT_FILE		"libenchant.dll"
#else
#	define ENCHANT_FILE		"libenchant.so.1"
#endif
#define	DICT_TAG_MAX_LENGTH	7

// Global variables

GModule *mod_enchant = NULL;
EnchantBroker *enchant_broker = NULL;
EnchantDict *enchant_dict = NULL;

const gchar* dict_lang_tag = "en";

GSList* suggestions_get(const gchar *lemma)
{
	GSList *suggestions_list = NULL;

	// check if Enchant broker and English dictionary are initialized
	if(enchant_broker && enchant_dict)
	{
		if(enchant_dict_check(enchant_dict, lemma, -1))
		{
			size_t suggestions_count = 0;
			gchar **suggestions = enchant_dict_suggest(enchant_dict, lemma, -1, &suggestions_count);

			for(size_t i = 0; i < suggestions_count; i++)
			{
				if(wni_request_nyms(suggestions[i], NULL, (WNIRequestFlags) 0, FALSE))
				{
					suggestions_list = g_slist_append(suggestions_list, g_strdup(suggestions[i]));
				}
			}
			
			if(suggestions)
				enchant_dict_free_string_list(enchant_dict, suggestions);
		}
	}
	
	return suggestions_list;
}

/* 
   checks if the passed LANG (code) is a form of the language we're interested in; this
   is to know if the lang is the same, omitting the locale e.g. en_IN is suitable for 'en'
 */
static gboolean is_lang_suitable(const gchar *lang_code)
{
	if(strlen(lang_code) >= strlen(dict_lang_tag))
	{
		if(lang_code[0] == dict_lang_tag[0] && lang_code[1] == dict_lang_tag[1] && 
		('_' == lang_code[2] || '\0' == lang_code[2] || '-' == lang_code[2]))
			return TRUE;
	}
	
	return FALSE;
}

static void find_dictionary(const char * const lang_tag, const char * const provider_name,
						  const char * const provider_desc,
						  const char * const provider_file,
						  void * user_data)
{
	gchar *str_dict_tag = user_data;

	if(str_dict_tag[0] == '\0')
	{
		if(is_lang_suitable(lang_tag))
		{
			g_snprintf(str_dict_tag, DICT_TAG_MAX_LENGTH, "%s", lang_tag);
			G_MESSAGE("Alternative dict '%s' selected!\n", lang_tag);
		}
	}
}

static gboolean try_sys_lang(gchar *copy_dict_tag, guint8 max_length)
{
	const gchar *sys_lang = g_getenv("LANG");
	gchar *temp_str = NULL;

	if(sys_lang)
	{
		if(is_lang_suitable(sys_lang))
		{
			g_strlcpy(copy_dict_tag, sys_lang, max_length);

			/* some machines have not only the lang but also the char encoding
			   e.g. "en_IN.UTF-8"; keep only the language "en_IN" and truncate 
			   the encoding "UTF-8", if present */
			temp_str = g_strstr_len(copy_dict_tag, -1, ".");
			if(temp_str) *temp_str = '\0';

			return TRUE;
		}
	}
	
	return FALSE;
}

gboolean suggestions_init()
{
	gchar dict_tag[DICT_TAG_MAX_LENGTH] = "";

	if(g_module_supported() && (mod_enchant = g_module_open(ENCHANT_FILE, G_MODULE_BIND_LAZY)))
	{
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_broker_init), (gpointer *) &enchant_broker_init);
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_broker_free), (gpointer *) &enchant_broker_free);
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_broker_list_dicts), (gpointer *) &enchant_broker_list_dicts);
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_broker_dict_exists), (gpointer *) &enchant_broker_dict_exists);
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_broker_request_dict), (gpointer *) &enchant_broker_request_dict);
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_broker_free_dict), (gpointer *) &enchant_broker_free_dict);
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_dict_check), (gpointer *) &enchant_dict_check);
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_dict_suggest), (gpointer *) &enchant_dict_suggest);
		g_module_symbol(mod_enchant, G_STRINGIFY(enchant_dict_free_string_list), (gpointer *) &enchant_dict_free_string_list);

		// in older version of Enchant, enchant_dict_free_string_list might be absent, they will have the
		// now deprecated function enchant_dict_free_suggestions
		if(NULL == enchant_dict_free_string_list)
			g_module_symbol(mod_enchant, G_STRINGIFY(enchant_dict_free_string_list), (gpointer *) &enchant_dict_free_string_list);

		// check if we have obtained the essential function pointers
		if(NULL != enchant_broker_init && NULL != enchant_broker_free && NULL != enchant_broker_dict_exists &&
		NULL != enchant_broker_request_dict && NULL != enchant_broker_free_dict && NULL != enchant_dict_check && 
		NULL != enchant_dict_suggest && NULL != enchant_dict_free_string_list)
		{
			enchant_broker = enchant_broker_init();
			if(enchant_broker)
			{
				/* if the sys. lang is suitable, copy that
				   else copy the default lang tag */
				if(try_sys_lang(dict_tag, DICT_TAG_MAX_LENGTH))
				{
					if(enchant_broker_dict_exists(enchant_broker, dict_tag))
					{
						enchant_dict = enchant_broker_request_dict(enchant_broker, dict_tag);
						return TRUE;
					}
					
					G_MESSAGE("Suggestions: Couldn't get '%s' dict. Looking for alternatives...\n", dict_lang_tag);
				}

				g_strlcpy(dict_tag, dict_lang_tag, DICT_TAG_MAX_LENGTH);

				/* if the req. dict. doesn't exist and if list dict func. exists then try to enumerate 
				   the dictionaries and see if a compatible one can be found */
				if(!enchant_broker_dict_exists(enchant_broker, dict_tag) && enchant_broker_list_dicts)
				{
					G_MESSAGE("Suggestions: Couldn't get '%s' dict. Looking for alternatives...\n", dict_lang_tag);

					dict_tag[0] = '\0';
					enchant_broker_list_dicts(enchant_broker, find_dictionary, dict_tag);
				}
				
				if(dict_tag[0] != '\0')
				{
					enchant_dict = enchant_broker_request_dict(enchant_broker, dict_tag);
					G_MESSAGE("Suggestions module successfully loaded");
					return TRUE;
				}
			}
		}
	}

	if(enchant_broker)
	{
		enchant_broker_free(enchant_broker);
		enchant_broker = NULL;
	}

	if(mod_enchant)
	{
		g_module_close(mod_enchant);
		mod_enchant = NULL;
	}
	
	G_MESSAGE("Failed to load suggestions module");

	return FALSE;
}

gboolean suggestions_uninit()
{
	gboolean free_success = FALSE;

	if(enchant_dict)
	{
		enchant_broker_free_dict(enchant_broker, enchant_dict);
		enchant_dict = NULL;
	}

	if(enchant_broker)
	{
		enchant_broker_free(enchant_broker);
		enchant_broker = NULL;
	}

	if(mod_enchant)
	{
		free_success = g_module_close(mod_enchant);
		mod_enchant = NULL;
	}
	
	return free_success;
}

