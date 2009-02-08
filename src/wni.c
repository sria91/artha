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
   wni.c: WordNet Interface module, which does the actual talking to 
   libwordnet. Sits b/w the application and the wordnet database files, 
   receives requests for a given search string, if found in WordNet, 
   populates the requested lists.

   TODO: When in simple mode, see pertainyms of 'verbally' and hypernyms of 'conquest', 
   should, duplicate check for them in these simple mode case, be done? Likewise, see 
   'rupee's Meronyms in Simple mode, it lists cent and paise around 5 times!
*/

#include "wni.h"


// WNI Global Variable
GSList *global_list = NULL;


// WNI Helper function prototypes
static int depth_check(guint8 depth, SynsetPtr synptr);
static gchar* strip_adj_marker(gchar *word);
static void populate_adj_antonyms(SynsetPtr synptr, WNIProperties **antonyms_ptr, guint8 id, guint8 sense, gboolean advanced_mode);
static gchar *indirect_via(SynsetPtr synptr, guint8 *sense);
static void populate_antonyms(SynsetPtr synptr, WNIProperties **antonyms_ptr, guint8 id, guint8 sense, gboolean advanced_mode);
static gchar* parse_definition(gchar *str);
static gint pos_list_compare(gconstpointer a, gconstpointer b, gpointer actual_search);
static GSList* check_term_in_list(gchar *term, GSList **list, WNIRequestFlags flag);
static gboolean is_synm_a_lemma(GSList **list, gchar *synm);
static gchar *getexample(gchar *offset, gchar *wd);
static GSList *findexample(SynsetPtr synptr);
static gpointer *get_from_global_list(WNIRequestFlags flag);
static void prune_results(void);
static WNIDefinitionItem *create_synonym_def(guint8 pos, IndexPtr index, WNIOverview **overview_ptr);
static void populate_synonyms(gchar **lemma_ptr, SynsetPtr cursyn, IndexPtr index, WNIDefinitionItem **def_item_ptr, WNIOverview **overview_ptr, guint8 sense, guint8 pos);
static gboolean add_synonym_def(WNIDefinitionItem **def_item_ptr, WNIOverview **overview_ptr, gchar *actual_search);
static void populate_ptr(gchar *lemma, SynsetPtr synptr, WNIProperties **data_ptr, guint8 ptr, guint8 id, guint8 sense, WNIRequestFlags flag);
static void grow_tree(gchar *lemma, SynsetPtr synptr, guint8 ptr, GNode **root, guint8 depth);
static void trace_inherit(gchar *lemma, SynsetPtr synptr, GNode **root, guint8 depth);
static void populate_ptr_tree(gchar *lemma, SynsetPtr synptr, WNIProperties **data_ptr, guint8 ptr, guint8 id, guint8 sense, gboolean advanced_mode);
static void populate(gchar *lemma, guint8 pos, WNIRequestFlags req_flag, gchar *actual_search, guint definitions_set, gboolean advanced_mode);
static void simple_list_free(GSList **list, guint8 mode);
static void synonym_list_free(GSList **list, gboolean single_element);
static void definitions_free(GSList **list);
static void definitions_list_free(GSList **list);
static void implications_free(GSList **list, guint8 mode);
static void antonyms_list_free(GSList **list);
static void properties_list_free(GSList **list, WNIRequestFlags id);
static void class_list_free(GSList **list);
static gboolean node_free(GNode *node, gpointer data);
static void tree_free(GNode *node, gpointer data);
static void properties_tree_free(GSList **list, WNIRequestFlags id);



/*
	This function is equivalent to g_strcmp0, which is a 
	wrapper over the standard C lib. function strcmp. The 
	reason for having it instead of using wni_strcmp0 is that 
	it was introduced only in glib 2.16. So as to reduce 
	the version dependency, this function is employed.
*/

int wni_strcmp0(const char *s1, const char *s2)
{
	if(s1 != NULL && s2 != NULL)
		return strcmp(s1, s2);
	else
	{
		if(s1 == s2)
			return 0;
		else if(s1)
			return 1;
		else
			return -1;
	}
}


/*
	Checks if depth hasn't exceeded the MAXDEPTH.
*/

static int depth_check(guint8 depth, SynsetPtr synptr)
{
    if(depth >= MAXDEPTH)
    {
	g_error("WordNet library error: Error Cycle detected\n   %s\n", synptr->words[0]);
	depth = -1;		/* reset to get one more trace then quit */
    }
    return(depth);
}


/*
	Adjectives has adjective marker at their end, this function is used to strip them.
	NOTE: This modifies the string in place, does not create a new one.
	E.g. beautiful(ip), beautiful(p), beautiful(a)
*/

static gchar* strip_adj_marker(gchar *word)
{
	gchar *y = NULL;

	y = word;
	while(*y)
	{
		if(*y == '(')
			*y = '\0';
		else
			y++;
	}

	return(word);
}


/*
	Function that populates the antonyms for every POS other than adj.
*/

static void populate_antonyms(SynsetPtr synptr, WNIProperties **antonyms_ptr, guint8 id, guint8 sense, gboolean advanced_mode)
{
	guint16 i = 0, j = 0;
	SynsetPtr cur_synset = NULL;
	WNIProperties *antonyms = *antonyms_ptr;
	WNIAntonymItem *antonym = NULL;
	WNIAntonymMapping *antonym_mapping = NULL;
	WNIImplication *implication = NULL;
	GSList *temp_list = NULL;
	guint8 count = 0;

	for(i = 0; i < synptr->ptrcount; i++)
	{
		if(ANTPTR == synptr->ptrtyp[i] && ((0 == synptr->pfrm[i]) || (synptr->pfrm[i] == synptr->whichword)) && (0 != synptr->pto[i]))
		{
			cur_synset = read_synset(synptr->ppos[i], synptr->ptroff[i], "");

			//strsubst(strip_adj_marker(cur_synset->words[synptr->pto[i] - 1]), '_', ' ');
			strsubst(cur_synset->words[synptr->pto[i] - 1], '_', ' ');
			
			// See if this antonym is already in our list
			temp_list = check_term_in_list(cur_synset->words[synptr->pto[i] - 1], &antonyms->properties_list, WORDNET_INTERFACE_ANTONYMS);

			if(temp_list)
				antonym = (WNIAntonymItem*) temp_list->data;		// if yes, then just get its pointer and add the mapping and implications
			else								// else create a new antonym item and store the details
			{
				//antonym = (WNIAntonymItem*) g_malloc0(sizeof(WNIAntonymItem));
				antonym = (WNIAntonymItem*) g_slice_alloc0(sizeof(WNIAntonymItem));
				antonym->term = g_strdup(cur_synset->words[synptr->pto[i] - 1]);
				antonym->sense = cur_synset->wnsns[synptr->pto[i] - 1] - 1;				// Second - 1 is to make it count from 0 and not 1
				antonym->relation = DIRECT_ANT;

				antonyms->properties_list = g_slist_append(antonyms->properties_list, antonym);
			}
			temp_list = NULL;

			//G_PRINTF("Antonym for Sense %d: %s#%d\n", sense, cur_synset->words[synptr->pto[i] - 1], cur_synset->wnsns[synptr->pto[i] - 1]);

			if(synptr->pto[i] != 0 && advanced_mode)
			{
				for(j = 0; j < cur_synset->wcount; j++)
				{
					//strsubst(strip_adj_marker(cur_synset->words[j]), '_', ' ');
					strsubst(cur_synset->words[j], '_', ' ');

					// check against temp_list too, not only implications, so that we don't miss duplicates in the same list
					if(wni_strcmp0(antonym->term, cur_synset->words[j]) && 
					(NULL == check_term_in_list(cur_synset->words[j], &antonym->implications, 0)) && 
					(NULL == check_term_in_list(cur_synset->words[j], &temp_list, 0)))
					{
						//implication = (WNIImplication*) g_malloc0(sizeof(WNIImplication));
						implication = (WNIImplication*) g_slice_alloc0(sizeof(WNIImplication));
						implication->term = g_strdup(cur_synset->words[j]);
						implication->sense = cur_synset->wnsns[j] - 1;		// Second - 1 is to make it count from 0 and not 1

						temp_list = g_slist_prepend(temp_list, implication);
						count++;
					}
					
					//G_PRINTF("%s#%d\n", cur_synset->words[j], cur_synset->wnsns[j]);
				}
				temp_list = g_slist_reverse(temp_list);
				antonym->implications = g_slist_concat(antonym->implications, temp_list);
			}

			//antonym_mapping = (WNIAntonymMapping*) g_malloc0(sizeof(WNIAntonymMapping));
			antonym_mapping = (WNIAntonymMapping*) g_slice_alloc0(sizeof(WNIAntonymMapping));
			antonym_mapping->id = id;
			antonym_mapping->sense = sense;
			antonym_mapping->related_word_count = count;
			
			antonym->mapping = g_slist_append(antonym->mapping, antonym_mapping);
			
			free_synset(cur_synset);
		}
	}
}


/*
	This function returns the INDIRECT VIA string for a adj. antonym.
	The second arg. is a out variable, which carries the sense of the 'via' lemma.
*/

static gchar *indirect_via(SynsetPtr synptr, guint8 *sense)
{
	SynsetPtr temp_synset = NULL;
	gchar *temp_str = NULL;
	guint16 i = 0, j = 0, wdoff = 0;

	for(i = 0; i < synptr->ptrcount; i++)
	{
		if (synptr->ptrtyp[i] == ANTPTR && 1 == synptr->pfrm[i])
		{
			temp_synset = read_synset(ADJ, synptr->ptroff[i], "");
			for(j = 0; j < temp_synset->ptrcount; j++)
			{
				if (ANTPTR == temp_synset->ptrtyp[j] && 
				temp_synset->pto[j] == 1 && 
				temp_synset->ptroff[j] == synptr->hereiam)
				{
					wdoff = (temp_synset->pfrm[j] ? (temp_synset->pfrm[j] - 1) : 0);
					temp_str = g_strdup(strsubst(strip_adj_marker(temp_synset->words[wdoff]), '_', ' '));
					*sense = temp_synset->wnsns[wdoff] - 1;			// - 1 is to make it count from 0 and not 1
					break;
				}
			}
			free_synset(temp_synset);
			if(temp_str) break;
		}
	}
	return temp_str;
}


/*
	Function that populates the antonyms for adj.
*/

static void populate_adj_antonyms(SynsetPtr synptr, WNIProperties **antonyms_ptr, guint8 id, guint8 sense, gboolean advanced_mode)
{
	guint8 ant_type = DIRECT_ANT;
	guint16 i = 0, j = 0, k = 0;
	guint8 count = 0;
	SynsetPtr cur_synset = NULL, sim_synset = NULL, ant_synset = NULL;
	gchar *temp_str = NULL;

	WNIProperties *antonyms = *antonyms_ptr;
	WNIAntonymItem *antonym = NULL;
	WNIAntonymMapping *antonym_mapping = NULL;
	WNIImplication *implication = NULL;
	GSList *temp_list = NULL;

	// This search is only applicable for ADJ synsets which have
	// either direct or indirect antonyms (not valid for pertainyms or other antonyms).

	if(DIRECT_ANT == synptr->sstype || (INDIRECT_ANT == synptr->sstype && advanced_mode))
	{
		if(INDIRECT_ANT == synptr->sstype)
		{
			ant_type = INDIRECT_ANT;
			for(i = 0; synptr->ptrtyp[i] != SIMPTR; i++);
			cur_synset = read_synset(ADJ, synptr->ptroff[i], "");
		}
		else
			cur_synset = synptr;

		// find antonyms - if direct, make sure that the antonym ptr we're looking at is from this word 
		for(i = 0; i < cur_synset->ptrcount; i++)
		{
			if (ANTPTR == cur_synset->ptrtyp[i] &&
			((DIRECT_ANT == ant_type &&
			cur_synset->pfrm[i] == cur_synset->whichword) ||
			(INDIRECT_ANT == ant_type)))
			{
				ant_synset = read_synset(ADJ, cur_synset->ptroff[i], "");
				if(DIRECT_ANT == ant_type)
				{
					strsubst(strip_adj_marker(ant_synset->words[0]), '_', ' ');
					temp_list = check_term_in_list(ant_synset->words[0], &antonyms->properties_list, WORDNET_INTERFACE_ANTONYMS);

					if(temp_list)
						antonym = (WNIAntonymItem*) temp_list->data;
					else
					{
						//antonym = (WNIAntonymItem*) g_malloc0(sizeof(WNIAntonymItem));
						antonym = (WNIAntonymItem*) g_slice_alloc0(sizeof(WNIAntonymItem));
						antonym->term = g_strdup(ant_synset->words[0]);
						antonym->sense = ant_synset->wnsns[synptr->pto[i] - 1] - 1;				// - 1 is to make it count from 0 and not 1
						antonym->relation = DIRECT_ANT;

						antonyms->properties_list = g_slist_append(antonyms->properties_list, antonym);
					}
					temp_list = NULL;
					count = 0;

					//G_PRINTF("%s\n", strip_adj_marker(ant_synset->words[0]));	// Except the first word, all other words are treated as implications

					for(j = 1; j < ant_synset->wcount && advanced_mode; j++)			// Coz of this, notice that the index starts at 1 and not 0, it also has a -> before it. e.g. tall
					{
						strsubst(strip_adj_marker(ant_synset->words[j]), '_', ' ');
						if(NULL == check_term_in_list(ant_synset->words[j], &antonym->implications, 0))
						{
							//implication = (WNIImplication*) g_malloc0(sizeof(WNIImplication));
							implication = (WNIImplication*) g_slice_alloc0(sizeof(WNIImplication));
							implication->term = g_strdup(ant_synset->words[j]);
							implication->sense = ant_synset->wnsns[j] - 1;
						
							temp_list = g_slist_prepend(temp_list, implication);
							count++;
						}
						//G_PRINTF("->%s\n", strip_adj_marker(ant_synset->words[j]));
					}
					for(j = 0; j < ant_synset->ptrcount && advanced_mode; j++)
					{
						if(ant_synset->ptrtyp[j] == SIMPTR)
						{
							sim_synset = read_synset(ADJ, ant_synset->ptroff[j], "");
							for(k = 0; k < sim_synset->wcount; k++)
							{
								strsubst(strip_adj_marker(sim_synset->words[k]), '_', ' ');

								// check against temp_list too, not only implications, so that we don't miss duplicates in the same list
								// e.g. 'impure' shows 'clean' as an antonym twice, try removing the 2nd check to verify it
								if((NULL == check_term_in_list(sim_synset->words[k], &antonym->implications, 0)) && 
								(NULL == check_term_in_list(sim_synset->words[k], &temp_list, 0)))
								{
									//implication = (WNIImplication*) g_malloc0(sizeof(WNIImplication));
									implication = (WNIImplication*) g_slice_alloc0(sizeof(WNIImplication));
									implication->term = g_strdup(sim_synset->words[k]);
									implication->sense = sim_synset->wnsns[k] - 1;
						
									temp_list = g_slist_prepend(temp_list, implication);
									count++;
								}
								//G_PRINTF("\t=> %s\n", strip_adj_marker(sim_synset->words[k]));
							}
							free_synset(sim_synset);
						}
				    	}
				    	temp_list = g_slist_reverse(temp_list);
				    	antonym->implications = g_slist_concat(antonym->implications, temp_list);

					//antonym_mapping = (WNIAntonymMapping*) g_malloc0(sizeof(WNIAntonymMapping));
					antonym_mapping = (WNIAntonymMapping*) g_slice_alloc0(sizeof(WNIAntonymMapping));
					antonym_mapping->id = id;
					antonym_mapping->sense = sense;
					antonym_mapping->related_word_count = count;	// - 1 is to make it count from 0 and not 1
			
					antonym->mapping = g_slist_append(antonym->mapping, antonym_mapping);
				}
				else
				{
					temp_list = NULL;
					temp_str = indirect_via(ant_synset, &count);	// get the sense of indirect term via count variable

					//G_PRINTF("Indirect VIA: %s\n", temp_str);

					if(temp_str)
					{
						temp_list = check_term_in_list(temp_str, &antonyms->properties_list, WORDNET_INTERFACE_ANTONYMS);
						
						if(temp_list)
						{
							antonym = (WNIAntonymItem*) temp_list->data;
							g_free(temp_str);
						}
						else
						{
							//antonym = (WNIAntonymItem*) g_malloc0(sizeof(WNIAntonymItem));
							antonym = (WNIAntonymItem*) g_slice_alloc0(sizeof(WNIAntonymItem));
							antonym->term = temp_str;
							antonym->sense = count;
							antonym->relation = INDIRECT_ANT;

							antonyms->properties_list = g_slist_append(antonyms->properties_list, antonym);	
						}
						temp_list = NULL; temp_str = NULL; count = 0;

						for(j = 0; j < ant_synset->wcount; j++)
						{
							strsubst(strip_adj_marker(ant_synset->words[j]), '_', ' ');
							
							// check against temp_list too, not only implications, so that we don't miss duplicates in the same list
							if((NULL == check_term_in_list(ant_synset->words[j], &antonym->implications, 0)) && 
							(NULL == check_term_in_list(ant_synset->words[j], &temp_list, 0)))
							{
								//implication = (WNIImplication*) g_malloc0(sizeof(WNIImplication));
								implication = (WNIImplication*) g_slice_alloc0(sizeof(WNIImplication));
								implication->term = g_strdup(ant_synset->words[j]);
								implication->sense = ant_synset->wnsns[j] - 1;
				
								temp_list = g_slist_prepend(temp_list, implication);
								count++;
							}
							//G_PRINTF("\t\t=> %s\n", ant_synset->words[j]);
						}

					    	temp_list = g_slist_reverse(temp_list);
					    	antonym->implications = g_slist_concat(antonym->implications, temp_list);

						//antonym_mapping = (WNIAntonymMapping*) g_malloc0(sizeof(WNIAntonymMapping));
						antonym_mapping = (WNIAntonymMapping*) g_slice_alloc0(sizeof(WNIAntonymMapping));
						antonym_mapping->id = id;
						antonym_mapping->sense = sense;
						antonym_mapping->related_word_count = count;	// - 1 is to make it count from 0 and not 1
			
						antonym->mapping = g_slist_append(antonym->mapping, antonym_mapping);
					}
				}
				free_synset(ant_synset);
			}
		}
		if (cur_synset != synptr) free_synset(cur_synset);
	}
}


/*
Based on the following defns from WN, the below function was formulated: (when a new format is encountered, please add it here)
	set = "(set down according to a plan:\"a carefully laid table with places set for four people\"; \"stones laid in a pattern\")"
	jolly = "(full of or showing high-spirited merriment; \"when hearts were young and gay\"; \"a poet could not but be gay, in such a jocund company\"- Wordsworth; \"the jolly crowd at the reunion\"; \"jolly old Saint Nick\"; \"a jovial old gentleman\"; \"have a merry Christmas\"; \"peals of merry laughter\"; \"a mirthful laugh\")"
	kiss = "(touch with the lips or press the lips (against someone's mouth or other body part) as an expression of love, greeting, etc.; \"The newly married couple kissed\"; \"She kissed her grandfather on the forehead when she entered the room; so gentle\")"
	proper = "(having all the qualities typical of the thing specified; \"wanted a proper dinner; not just a snack\"; \"he finally has a proper job\")"
	phallus = "(the male organ of copulation (`member' is a euphemism))"
	passion = "(any object of warm affection or devotion; \"the theater was her first love\"; \"he has a passion for cock fighting\";)"
	passion = "(an irrational but irresistible motive for a belief or action)"
	alright = "(in a satisfactory or adequate manner; \"she'll do okay on her own\"; \"held up all right under pressure\"; (`alright' is a nonstandard variant of `all right'))"
	
	// TODO: notice the ',' instead of ';' as the delimiter between the examples
	compound = "make more intense, stronger, or more marked; \"The efforts were intensified\", \"Her rudeness intensified his dislike for her\"; "

	Takes a synset defn and applies delimiter '|' between definition and meanings.
	Returns a new string, should be freed.
*/

static gchar* parse_definition(gchar *str)
{
	GString *formatted_str = NULL;
	guint16 i = 0, len = 0;
	guint8 brace_met = FALSE;
	guint8 double_quotes_met = 0;
	guint8 just_ended = FALSE;
	gchar ch = 0;
	gchar *temp_ch = NULL;

	len = g_utf8_strlen(str, -1) - 1;	// skip the last close brace (len - 1)
	formatted_str = g_string_new("");
	for(i = 1; i < len; i++)		// skip the first open brace (i = 1)
	{
		ch = str[i];
		if(str[i] == '"')
		{
			if(!(double_quotes_met & 1))	// If double quotes met is not even i.e. odd, then insert delimiter '|'
			{
				temp_ch = &formatted_str->str[formatted_str->len-1];
				if(*temp_ch != '|') *temp_ch = '|';
				temp_ch = NULL;
			}
			double_quotes_met++;
			just_ended = FALSE;
			ch = 0;
		}
		else if(str[i] == ' ' && just_ended)
			ch = 0;
		else if(str[i] == '(' && just_ended)	// open brace met immediately after a statement end
		{
			just_ended = FALSE;
			brace_met = 1;
		}
		else if(str[i] == ')' && brace_met)	// on close brace, end the string by a ") " and skip the char (ch = 0)
		{
			formatted_str = g_string_insert(formatted_str, brace_met - 1, ") ");
			brace_met = FALSE;
			ch = 0;
		}
		else if((str[i] == ';') && ((i + 1 == len) || ((!(double_quotes_met & 1)) && (str[i + 2] == '"')) || (str[i + 2] == '(')))
		//else if((str[i] == ';') && ((i+1 == len) || ((!(double_quotes_met & 1)) && (str[i+2] == '\"'))))
		{
			ch = '|';
			just_ended = TRUE;
		}
		if(ch)
		{
			if(0 == brace_met)
				formatted_str = g_string_append_c(formatted_str, ch);				// copy to end, by default
			else
				formatted_str = g_string_insert_c(formatted_str, -1 + brace_met++, ch);		// copy to beginning, if its a brace statement
		}
	}

	return g_string_free(formatted_str, FALSE);
}


/*
	Compares Definition Items, by checking their lemmas, if it matches the actual search string while the other doesn't
	then it is deemed as greater. E.g. 'peoples' return both peoples & people in its search, eventhough people has greater tag val, peoples is more apt 
	for the search. On this basis, peoples is given higher priority. If both the lemma's are same, then the	tag count of the first definition (as the first 
	one's tag count is the maximum of the whole def. list) in both are compared. If they too are equal, then their respective polysemy counts are compared.

	Reason for preferring tag count over polysemy: E.g. 'tree' Verb def_item is given higher priority when polysemy count is taken, which is incorrect.
*/

static gint pos_list_compare(gconstpointer a, gconstpointer b, gpointer actual_search)
{
	guint16 weight_a = 0, weight_b = 0;
	gboolean not_equal = FALSE;

	WNIDefinitionItem *defn_item_a = (WNIDefinitionItem*) a, *defn_item_b = (WNIDefinitionItem*) b;

	not_equal = wni_strcmp0(defn_item_a->lemma, defn_item_b->lemma);
	if(not_equal)
	{
		if(0 == wni_strcmp0((gchar*) actual_search, defn_item_a->lemma))		// Actual search str compared against item a and b's lemma
			return -1;
		else if (0 == wni_strcmp0((gchar*) actual_search, defn_item_b->lemma))
			return 1;
	}

	weight_a = ((WNIDefinition*)defn_item_a->definitions->data)->tag_count;
	weight_b = ((WNIDefinition*)defn_item_b->definitions->data)->tag_count;

	if(weight_a  > weight_b)
		return -1;
	else if(weight_b  > weight_a)
		return 1;
	else
	{
		weight_a = g_slist_length(defn_item_a->definitions);
		weight_b = g_slist_length(defn_item_b->definitions);

		if(weight_a  > weight_b)
			return -1;
		else
			return 1;
	}
}


/*
	In a given list, it searches for the a string: term.
	The list's format is taken via flag.
	Returns the list item which has the term or NULL, if not found.
*/

static GSList* check_term_in_list(gchar *term, GSList **list, WNIRequestFlags flag)
{
	GSList *temp = *list;
	gchar *temp_str = NULL;
	guint16 check_len = 0;
	
	check_len = g_utf8_strlen(term, -1) + 1;	// check till \0 so that "Kelly" & "Kelly Gene" don't match

	while(temp)
	{
		if(temp->data)
		{
			// removed since WNISynonym & WNIPropertyItem are the same
			/*if(WORDNET_INTERFACE_OVERVIEW & flag)
				temp_str = (gchar*) ((WNISynonym*)temp->data)->term;
			else */
			if(WORDNET_INTERFACE_ANTONYMS & flag)
				temp_str = (gchar*) ((WNIAntonymItem*)temp->data)->term;
			else if((WORDNET_INTERFACE_ATTRIBUTES & flag) || (WORDNET_INTERFACE_CAUSES & flag) || (WORDNET_INTERFACE_ENTAILS & flag) || 
			(WORDNET_INTERFACE_SIMILAR & flag) || (WORDNET_INTERFACE_DERIVATIONS & flag) || (WORDNET_INTERFACE_OVERVIEW & flag))
				temp_str = (gchar*) ((WNIPropertyItem*)temp->data)->term;
			else if(WORDNET_INTERFACE_CLASS & flag)
				temp_str = (gchar*) ((WNIClassItem*)temp->data)->term;
			else if(0 == flag)
				temp_str = (gchar*) ((WNIImplication*)temp->data)->term;	//If not flag is set, its to check the implications list

			if(!g_ascii_strncasecmp(term, temp_str, check_len))
				return temp;
		}
		temp = g_slist_next(temp);
	}
	return NULL;
}


/*
	Returns TRUE if synm is a lemma in one of the definition items present in definitions_list.
*/

static gboolean is_synm_a_lemma(GSList **list, gchar *synm)
{
	WNIDefinitionItem *temp_def_item = NULL;
	GSList *temp_list = *list;

	while(temp_list)
	{
		if(temp_list->data)
		{
			temp_def_item = (WNIDefinitionItem*) temp_list->data;

			if(0 == wni_strcmp0(temp_def_item->lemma, synm))
				return TRUE;
		}
		temp_list = g_slist_next(temp_list);
	}

	return FALSE;
}


/*
	Fetch the given example sentence from the example file.
*/

static gchar *getexample(gchar *offset, gchar *wd)
{
	gchar *line = NULL, *example = NULL;
	guint16 last_char_index = 0;

	if (vsentfilefp != NULL)
	{
		if ((line = bin_search(offset, vsentfilefp)))
		{
			while(*line != ' ')
				line++;

			last_char_index = g_utf8_strlen(line, -1) - 1;
			// The last char is mostly a new line char. from file hence truncate it. If not we are dealing with the last line in the examples file :)
			line[last_char_index] = (line[last_char_index]=='\n')? '\0' : line[last_char_index];
			example = g_strdup_printf(g_strstrip(line), wd);
		}
	}
	
	return example;
}


/*
	Find the example sentence references in the example sentence index file.
*/

static GSList *findexample(SynsetPtr synptr)
{
	gchar tbuf[256]="", *temp = NULL, *returned_example = NULL, **splits = NULL;
	guint16 wdnum = 0, i = 0;
	GSList *examples = NULL;

	if (vidxfilefp != NULL)
	{
		wdnum = synptr->whichword - 1;

		sprintf(tbuf,"%s%%%-1.1d:%-2.2d:%-2.2d::",
			synptr->words[wdnum],
			getpos(synptr->pos),
			synptr->fnum,
			synptr->lexid[wdnum]);

		if ((temp = bin_search(tbuf, vidxfilefp)) != NULL)
		{
			// skip over sense key and get sentence numbers

			temp += g_utf8_strlen(synptr->words[wdnum], -1) + 11;
			g_stpcpy(tbuf, temp);
			
			splits = g_strsplit_set(tbuf, " ,\n", 0);
			while(splits[i])
			{
				if(0 != wni_strcmp0(splits[i], ""))
					if((returned_example = getexample(splits[i], synptr->words[wdnum])))
						examples = g_slist_prepend(examples, returned_example);
				i++;
			}
			g_strfreev(splits);
		}
	}

	return examples;
}


/*
	This function looks for an item in the global list, say OVERVIEW.
	If found, it returns the data part of that item found, else it allocates
	the requested item and returns its data pointer.
*/

static gpointer *get_from_global_list(WNIRequestFlags flag)
{
	GSList *temp_global_list = global_list;
	WNIRequestFlags temp_id = 0;
	WNINym *temp_nym = NULL;

	while(temp_global_list)
	{
		temp_id = ((WNINym*) temp_global_list->data)->id;
		if(flag & temp_id)
			return ((WNINym*) temp_global_list->data)->data;

		temp_global_list = g_slist_next(temp_global_list);
	}

	// Either the whole global list is null or there is no item with such ID.
	//temp_nym = (WNINym*) g_malloc0(sizeof(WNINym));
	temp_nym = (WNINym*) g_slice_alloc(sizeof(WNINym));
	temp_nym->id = flag;

	if(WORDNET_INTERFACE_OVERVIEW & flag)
		temp_nym->data = g_slice_alloc0(sizeof(WNIOverview));
		//temp_nym->data = g_malloc0(sizeof(WNIOverview));
	else/* if((WORDNET_INTERFACE_ANTONYMS & flag) || (WORDNET_INTERFACE_ATTRIBUTES & flag) || (WORDNET_INTERFACE_CAUSES & flag) || (WORDNET_INTERFACE_ENTAILS & flag) || 
	(WORDNET_INTERFACE_SIMILAR & flag) || (WORDNET_INTERFACE_DERIVATIONS & flag) || (WORDNET_INTERFACE_PERTAINYMS & flag) || (WORDNET_INTERFACE_CLASS & flag) || 
	(WORDNET_INTERFACE_HYPERNYMS & flag) || (WORDNET_INTERFACE_HYPONYMS & flag) || (WORDNET_INTERFACE_MERONYMS & flag) || (WORDNET_INTERFACE_HOLONYMS & flag))*/
		temp_nym->data = g_slice_alloc0(sizeof(WNIProperties));
		//temp_nym->data = g_malloc0(sizeof(WNIProperties));

	// append it to the global_list and not temp_global_list, since it will be pointing to NULL by now
	global_list = g_slist_append(global_list, temp_nym);

	return temp_nym->data;
}


/*
	Removes nodes from the global list which didn't have any fruition.
	E.g. lists like antonyms, attributes, etc. will be empty for 'apothecary'
	Even when is_defined() says defined, the list might be empty, hence this is essential.
	E.g. is_defined() says "gypsy cab" has holonyms, where the actual list is empty.
	NOTE: It makes an assumption that OVERVIEW is always not NULL, and hence doesn't check it.
*/

static void prune_results(void)
{
	WNINym *temp_nym = NULL;
	GSList *temp_list = global_list, *temp_prev = NULL;
	gpointer data_part = NULL;
	
	while(temp_list)
	{
		if(temp_list->data)
		{
			temp_nym = (WNINym*) temp_list->data;

/*			if((WORDNET_INTERFACE_ANTONYMS == temp_nym->id) || (WORDNET_INTERFACE_ATTRIBUTES == temp_nym->id) || (WORDNET_INTERFACE_CAUSES == temp_nym->id) || 
			(WORDNET_INTERFACE_ENTAILS == temp_nym->id) || (WORDNET_INTERFACE_SIMILAR == temp_nym->id) || (WORDNET_INTERFACE_DERIVATIONS == temp_nym->id) || 
			(WORDNET_INTERFACE_PERTAINYMS == temp_nym->id) || (WORDNET_INTERFACE_CLASS == temp_nym->id) || (WORDNET_INTERFACE_HYPERNYMS == temp_nym->id) || 
			(WORDNET_INTERFACE_HYPONYMS == temp_nym->id) || (WORDNET_INTERFACE_MERONYMS == temp_nym->id) || (WORDNET_INTERFACE_HOLONYMS == temp_nym->id))*/
			if(temp_nym->id != WORDNET_INTERFACE_OVERVIEW)
				data_part = ((WNIProperties*) temp_nym->data)->properties_list;

			if(NULL == data_part && WORDNET_INTERFACE_OVERVIEW != temp_nym->id)
			{
				G_MESSAGE("Pruned: %d\n", temp_nym->id);
				//g_free(temp_nym->data);
				//g_free(temp_nym);
				g_slice_free1(sizeof(WNIProperties), temp_nym->data);
				g_slice_free1(sizeof(WNINym), temp_nym);
				temp_nym = NULL;
				global_list = g_slist_delete_link(global_list, temp_list);
				temp_list = temp_prev;		// Reach the prev. node and continue normally
			}
		}
		temp_prev = temp_list;				// This is a back up of the node, so that we can get the prev. node
		temp_list = g_slist_next(temp_list);
	}
}


/*
	Creates the definition item for storing the definition, examples, synonyms and mapping.
	In case if the term is already a synonym, remove that synonym alone from the synonyms list.
*/

static WNIDefinitionItem *create_synonym_def(guint8 pos, IndexPtr index, WNIOverview **overview_ptr)
{
	WNIDefinitionItem *def_item = NULL;
	GSList *temp_list = NULL;
	WNIOverview *overview = *overview_ptr;
	gchar *mutable_lemma = g_strdup(index->wd);	// do not change index->wd in place, it is referred to by WnsnsToStr, etc. which may lead to a crash e.g. for "work out"


	//strsubst(strip_adj_marker(mutable_lemma), '_', ' ');
	if(pos == ADJ)
		strip_adj_marker(mutable_lemma);
	strsubst(mutable_lemma, '_', ' ');

	// Create a definition item to store the definition, its POS. Note ID is the unique ID which the synonym's list maps to for cross ref.
	//def_item = (WNIDefinitionItem*) g_malloc0 (sizeof(WNIDefinitionItem));
	def_item = (WNIDefinitionItem*) g_slice_alloc0 (sizeof(WNIDefinitionItem));
	def_item->pos = pos;
	def_item->lemma = g_strdup(mutable_lemma);
	def_item->definitions = NULL;					// List to store the definitions & examples associated to it
	def_item->id = g_slist_length(overview->definitions_list);	// First time call will give 0, since it will be NULL; next time onwards it will be 1, 2, so on..

	g_free(mutable_lemma);
	// Check for already added synonyms, if it is the current lemma added. If found, remove them, since they will be definitions when we are done, and not synonyms
	// e.g. axes => axe should be upgraded from synonym to lemma state
	if(overview->synonyms_list)
	{
		do
		{
			temp_list = check_term_in_list(def_item->lemma, &overview->synonyms_list, WORDNET_INTERFACE_OVERVIEW);
			if(temp_list)
			{
				// Delete data of the node and the node from the list
				G_MESSAGE("Synonym upgraded to lemma: %s\n", ((WNIPropertyItem*)temp_list->data)->term);
				synonym_list_free(&temp_list, TRUE);	// 2nd argument is imp., since it says the function to remove this element alone, or else it will be a total wipe-off of the list
				overview->synonyms_list = g_slist_delete_link(overview->synonyms_list, temp_list);	// Delete the link once we are done freeing the data associated
			}
		} while(temp_list);
	}
	
	return def_item;
}

/*
	Populates the definition, examples and synonyms for WNI Overview.
*/

static void populate_synonyms(gchar **lemma_ptr, SynsetPtr cursyn, IndexPtr index, WNIDefinitionItem **def_item_ptr, WNIOverview **overview_ptr, guint8 sense, guint8 pos)
{
	WNIDefinition *defn = NULL;
	gchar *temp_str = NULL, **splits = NULL;
	GSList *temp_list = NULL;
	WNISynonymMapping *mapping = NULL;
	WNIPropertyItem *synm = NULL;
	WNIDefinitionItem *def_item = *def_item_ptr;
	WNIOverview *overview = *overview_ptr;
	guint16 i = 0, check_len = 0;
	gchar *lemma = *lemma_ptr;

	//defn = (WNIDefinition*) g_malloc0 (sizeof(WNIDefinition));
	defn = (WNIDefinition*) g_slice_alloc0 (sizeof(WNIDefinition));

	if (index->tagged_cnt != -1 && ((sense + 1) <= index->tagged_cnt))	// a tag_count is present, overwrite the default 0
		defn->tag_count = GetTagcnt(index, sense + 1);

	temp_str = parse_definition(cursyn->defn);	// Get it delimited
	splits = g_strsplit(temp_str, "|", 0);		// Split them into definition and examples

	defn->definition = g_strdup(splits[0]);

	for(i = 1; splits[i]; i++)
		if(wni_strcmp0(splits[i], ""))		// make sure examples are not null strings
			defn->examples = g_slist_prepend(defn->examples, g_strdup(splits[i]));

	defn->examples = g_slist_reverse(defn->examples);

	g_strfreev(splits); splits = NULL;
	g_free(temp_str); temp_str = NULL;

	// if pos is VERB and doesn't have any examples, try to get from WN's examples file
	if(VERB == pos && NULL == defn->examples)
	{
		defn->examples = findexample(cursyn);
	}

	def_item->definitions = g_slist_prepend(def_item->definitions, defn);	// with this, definitions_list section is over, remaining is synonyms_list appending/mapping
	def_item->count++;
	
	check_len = g_utf8_strlen(lemma, -1) + 1;

	for(i = 0; i < cursyn->wcount; i++)
	{
		//temp_str = strsubst(strip_adj_marker(cursyn->words[i]), '_', ' ');
		if(pos == ADJ)
			strip_adj_marker(cursyn->words[i]);
		temp_str = strsubst(cursyn->words[i], '_', ' ');

		// Check if the synonym is any of the lemma we have in definitions list.
		// If present, skip this synonym and go to the next one.
		if(is_synm_a_lemma(&overview->definitions_list, temp_str))
			continue;

		if(g_ascii_strncasecmp(lemma, temp_str, check_len))	// Additional check to see if the current lemma and synm aren't the same
		{
			//mapping = (WNISynonymMapping*) g_malloc0 (sizeof(WNISynonymMapping));
			mapping = (WNISynonymMapping*) g_slice_alloc0 (sizeof(WNISynonymMapping));

			mapping->id = def_item->id;
			mapping->sense = sense;

			if(overview->synonyms_list)
				temp_list = check_term_in_list(temp_str, &overview->synonyms_list, WORDNET_INTERFACE_OVERVIEW);

			// if found, append the mapping to mapping list, else create a synonym and then append it to synonyms list
			if(temp_list)
			{
				//temp_list = ((WNISynonym*)temp_list->data)->mapping;
				temp_list = ((WNIPropertyItem*)temp_list->data)->mapping;
				temp_list = g_slist_append(temp_list, mapping);
			}
			else
			{
				//synm = (WNISynonym*) g_malloc0 (sizeof(WNISynonym));
				//synm = (WNISynonym*) g_slice_alloc0 (sizeof(WNISynonym));
				synm = (WNIPropertyItem*) g_slice_alloc0 (sizeof(WNIPropertyItem));
				synm->term = g_strdup(temp_str);
				synm->mapping = g_slist_append(synm->mapping, mapping);
				overview->synonyms_list = g_slist_append(overview->synonyms_list, synm);
			}
		}
		else
		{
			// for cases where search is like 'wordsworth', substitute the proper case equivalent, provided if its the prime defn.
			// I.e. if its the first defn. of the whole group. Since every defn. is prepended this is found by checking the next ptr of the list.
			// If next is there, it means earlier there were defns before this one, else this is the first defn.
			if(0 != wni_strcmp0(lemma, temp_str) && NULL == g_slist_next(def_item->definitions))
			{
				g_free(lemma);
				*lemma_ptr = g_strdup(temp_str);
				lemma = *lemma_ptr;
				G_MESSAGE("Opted proper case synonym '%s' as lemma!", lemma);
			}
		}
	}
}


/*
	Once the definition item is created, this function appends it to the global definitions list.
	If no definitions are present, it will free the item.
*/

static gboolean add_synonym_def(WNIDefinitionItem **def_item_ptr, WNIOverview **overview_ptr, gchar *actual_search)
{
	WNIDefinitionItem *def_item = *def_item_ptr;
	WNIOverview *overview = *overview_ptr;

	if(def_item->definitions)
	{
		def_item->definitions = g_slist_reverse(def_item->definitions);
		overview->definitions_list = g_slist_insert_sorted_with_data(overview->definitions_list, def_item, &pos_list_compare, actual_search);
		
		return TRUE;
	}
	else	// if no definitions were added to the definitions list, no point in adding the Definition Item
	{
		g_free(def_item->lemma); def_item->lemma = NULL;
		//g_free(def_item); def_item = NULL;
		g_slice_free1(sizeof(WNIDefinitionItem), def_item); *def_item_ptr = NULL;
	}

	return FALSE;
}


/*
	This function populates the list for the given ptr. Usually for attributes, causes, entails, derivations, similar or class/classifications.
	NOTE: This function is non-recursive, unlike WN's traceptrs. For generating trees, grow_trees() is employed. These functions were decoupled 
	for simplicity & readability reasons.
*/

static void populate_ptr(gchar *lemma, SynsetPtr synptr, WNIProperties **data_ptr, guint8 ptr, guint8 id, guint8 sense, WNIRequestFlags flag)
{
	SynsetPtr cursyn = NULL;
	WNIProperties *properties = *data_ptr;
	WNIPropertyItem *item = NULL;
	WNIPropertyMapping *mapping = NULL;
	WNIClassItem *class_item = NULL;
	GSList *temp_list = NULL;
	guint16 i = 0, j = 0, check_len = 0;
	
	check_len = g_utf8_strlen(lemma, -1) + 1;

	for(i = 0; i < synptr->ptrcount; i++)
	{
		// Check if the given ptr and the current one are the same. In case of CLASS check for CLASS/CLASSIF_START and CLASS/CLASSIF_END
		// HYPERPTR is added only for the case of Similar for nouns and verbs, which don't have an actual SIMPTR, depth 0 hypernyms are taken as Similar
		if(((synptr->ptrtyp[i] == ptr) && ((0 == synptr->pfrm[i]) || (synptr->pfrm[i] == synptr->whichword))) || 
		(CLASS == ptr && (((synptr->ptrtyp[i] >= CLASSIF_START) && (synptr->ptrtyp[i] <= CLASSIF_END)) || 
			((synptr->ptrtyp[i] >= CLASS_START) &&(synptr->ptrtyp[i] <= CLASS_END)))) || 
		((HYPERPTR == ptr) && ((HYPERPTR == synptr->ptrtyp[i]) || (INSTANCE == synptr->ptrtyp[i]))))
		{
			cursyn = read_synset(synptr->ppos[i], synptr->ptroff[i], "");

			for(j = 0; j < cursyn->wcount; j++)
			{
				// sstype is the type of ADJ synset, if its unset, it isn't a ADJ
				if(cursyn->sstype != 0)
					strip_adj_marker(cursyn->words[j]);
				strsubst(cursyn->words[j], '_', ' ');

				// check if the lemma and the word aren't the same. E.g. move causes move is redundant. Check move's 'CAUSES' section
				if(0 != g_ascii_strncasecmp(lemma, cursyn->words[j], check_len))
				{
					temp_list = check_term_in_list(cursyn->words[j], &properties->properties_list, flag);
					if(CLASS != ptr)
					{
						if(temp_list)
							item = (WNIPropertyItem*) temp_list->data;
						else
						{
							//item = (WNIPropertyItem*) g_malloc0(sizeof(WNIPropertyItem));
							item = (WNIPropertyItem*) g_slice_alloc0(sizeof(WNIPropertyItem));
							item->term = g_strdup(cursyn->words[j]);
							properties->properties_list = g_slist_append(properties->properties_list, item);
						}
						temp_list = NULL;
						//mapping = (WNIPropertyMapping*) g_malloc0(sizeof(WNIPropertyMapping));
						mapping = (WNIPropertyMapping*) g_slice_alloc0(sizeof(WNIPropertyMapping));
						mapping->self_sense = cursyn->wnsns[j] - 1;
						mapping->id = id;
						mapping->sense = sense;

						item->mapping = g_slist_append(item->mapping, mapping);
					}
					else
					{
						if(NULL == temp_list)
						{
							//class_item = (WNIClassItem*) g_malloc0(sizeof(WNIClassItem));
							class_item = (WNIClassItem*) g_slice_alloc0(sizeof(WNIClassItem));
							class_item->term = g_strdup(cursyn->words[j]);
							class_item->self_sense = cursyn->wnsns[j] - 1;
							class_item->self_pos = synptr->ppos[i];
							class_item->id = id;
							class_item->sense = sense;
							class_item->type = synptr->ptrtyp[i];
						
							properties->properties_list = g_slist_append(properties->properties_list, class_item);
						}
					}
				}
				//G_PRINTF("ID: %d, Sense: %d, %s\n", id, sense, cursyn->words[j]);
			}
			free_synset(cursyn);
		}
	}
}


/*
	This function grows tree for the requested ptr.
	In case of pertainyms, multiple recursions are avoided. Only a single recursion is done with HYPERPTR.
*/

static void grow_tree(gchar *lemma, SynsetPtr synptr, guint8 ptr, GNode **root, guint8 depth)
{
	SynsetPtr cursyn = NULL;
	guint16 i = 0, j = 0;
	GSList *node_list = NULL;
	WNIImplication *implication = NULL;
	WNITreeList *tree_list = NULL;
	GNode *cur_tree = NULL;

	for(i = 0; i < synptr->ptrcount; i++)
	{
		if(((ptr == synptr->ptrtyp[i]) && ((synptr->pfrm[i] == 0) || (synptr->pfrm[i] == synptr->whichword)))  || 
		((HYPERPTR == ptr) && ((HYPERPTR == synptr->ptrtyp[i]) || (INSTANCE == synptr->ptrtyp[i]))) || 
		((HYPOPTR == ptr) && ((HYPOPTR == synptr->ptrtyp[i]) || (INSTANCES == synptr->ptrtyp[i]))))
		{
			node_list = NULL;
			cursyn = read_synset(synptr->ppos[i], synptr->ptroff[i], "");

			for(j = 0; j < cursyn->wcount; j++)
			{
				if(cursyn->sstype != 0)
					strip_adj_marker(cursyn->words[j]);
				strsubst(cursyn->words[j], '_', ' ');
				// what if no words are present except the actual lemma searched? so duplicate check is commented here e.g. hypernyms for 'register'
				//if(0 != g_ascii_strncasecmp(lemma, cursyn->words[j], g_utf8_strlen(cursyn->words[j], -1) + 1))
				//{
					//implication = (WNIImplication*) g_malloc0(sizeof(WNIImplication));
					implication = (WNIImplication*) g_slice_alloc0(sizeof(WNIImplication));
					implication->term = g_strdup(cursyn->words[j]);
					implication->sense = cursyn->wnsns[j] - 1;
					node_list = g_slist_prepend(node_list, implication);
				//}
			}
			if(node_list)
			{
				//tree_list = (WNITreeList*) g_malloc0(sizeof(WNITreeList));
				tree_list = (WNITreeList*) g_slice_alloc0(sizeof(WNITreeList));
				tree_list->word_list = g_slist_reverse(node_list);
				tree_list->type = synptr->ptrtyp[i];			// store the actual ptr's type here. E.g. INSTANCE, HASPARTPTR, etc.

				cur_tree = g_node_new(tree_list);
				g_node_append(*root, cur_tree);

				if(PERTPTR == ptr && depth)
				{
					depth = 0;
					grow_tree(lemma, cursyn, HYPERPTR, &cur_tree, depth);
				}
			}
			if(depth && cur_tree != NULL)
			{
				depth = depth_check(depth, cursyn);
				grow_tree(lemma, cursyn, ptr, &cur_tree, (depth + 1));
			}

			free_synset(cursyn);
		}
	}
}


/*
	This function parses thru all inherited meronyms by travelling Hypernym tree.
	At exit, it checks if the node created is actually having any of the meronym ptr
	like HASPARTPTR, etc. if not, it prunes it.
*/

static void trace_inherit(gchar *lemma, SynsetPtr synptr, GNode **root, guint8 depth)
{
	SynsetPtr cursyn = NULL;
	guint16 i = 0, j = 0;
	GSList *node_list = NULL;
	WNIImplication *implication = NULL;
	WNITreeList *tree_list = NULL;
	GNode *cur_tree = NULL;

	for(i = 0; i < synptr->ptrcount; i++)
	{
		if((synptr->ptrtyp[i] == HYPERPTR) && ((synptr->pfrm[i] == 0) || (synptr->pfrm[i] == synptr->whichword)))
		{
			cur_tree = NULL;
			node_list = NULL;
			cursyn = read_synset(synptr->ppos[i], synptr->ptroff[i], "");

			for(j = 0; j < cursyn->wcount; j++)
			{
				if(cursyn->sstype != 0)
					strip_adj_marker(cursyn->words[j]);
				strsubst(cursyn->words[j], '_', ' ');
				// what if no words are present except the actual lemma searched? so duplicate check is commented here
				//if(0 != g_ascii_strncasecmp(lemma, cursyn->words[j], g_utf8_strlen(cursyn->words[j], -1) + 1))
				//{
					//implication = (WNIImplication*) g_malloc0(sizeof(WNIImplication));
					implication = (WNIImplication*) g_slice_alloc0(sizeof(WNIImplication));
					implication->term = g_strdup(cursyn->words[j]);
					implication->sense = cursyn->wnsns[j] - 1;

					node_list = g_slist_prepend(node_list, implication);
				//}
			}

			if(node_list)
			{
				//tree_list = (WNITreeList*) g_malloc0(sizeof(WNITreeList));
				tree_list = (WNITreeList*) g_slice_alloc0(sizeof(WNITreeList));
				tree_list->word_list = g_slist_reverse(node_list);
				tree_list->type = 0;

				cur_tree = g_node_new(tree_list);

				grow_tree(lemma, cursyn, HASMEMBERPTR, &cur_tree, depth);
				grow_tree(lemma, cursyn, HASSTUFFPTR, &cur_tree, depth);
				grow_tree(lemma, cursyn, HASPARTPTR, &cur_tree, depth);
				
				g_node_append(*root, cur_tree);
			}

			if(depth && cur_tree != NULL)
			{
				depth = depth_check(depth, cursyn);
				trace_inherit(lemma, cursyn, &cur_tree, (depth + 1));
			}

			if(cur_tree)
			{
				if(G_NODE_IS_LEAF(cur_tree))
				{
					tree_list = cur_tree->data;
					if(0 == tree_list->type)	// if the leaf doesn't contain a meronym, prune it
					{
						implications_free(&tree_list->word_list, 0);
						g_slist_free(tree_list->word_list);
						tree_list->word_list = NULL;
						//g_free(tree_list);
						g_slice_free1(sizeof(WNITreeList), tree_list);
						tree_list = NULL;
						g_node_destroy(cur_tree);
						cur_tree = NULL;
					}
				}
			}
			free_synset(cursyn);
		}
	}
}


/*
	This function calls grow_tree for the given pointer to get the hierarchal elements of the reqd. ptr.
	In case of mernonyms, it also calls trace_inherit to get inherited meronyms, if present.
	'sonora' is a case where only Meronoyms are present I.e. no inherited meronyms.
	'water' is a case where only Holonyms are present I.e. no inherited holonyms. grow_tree() is called 
	with the exact ptr redq. (like HASPARTPTR, etc.) and not generalised HHOLONYM/HMERONYM.
*/
static void populate_ptr_tree(gchar *lemma, SynsetPtr synptr, WNIProperties **data_ptr, guint8 ptr, guint8 id, guint8 sense, gboolean advanced_mode)
{
	WNIProperties *tree_list = *data_ptr;
	WNISynonymMapping *tree_mapping = NULL;
	GNode *tree = NULL;
	guint8 depth = 0;

	if(HYPERPTR == ptr || HYPOPTR == ptr || HMERONYM == ptr || HHOLONYM == ptr || PERTPTR == ptr)
	{
		//tree_mapping = (WNISynonymMapping*) g_malloc0(sizeof(WNISynonymMapping));
		tree_mapping = (WNISynonymMapping*) g_slice_alloc0(sizeof(WNISynonymMapping));
		tree_mapping->id = id;
		tree_mapping->sense = sense;

		tree = g_node_new(tree_mapping);

		if(advanced_mode) depth = 1;

		// if tree is not defined, go for depth 0
		if(ptr == HMERONYM || ptr == HHOLONYM)
		{
			// CAUTION: is_defined() of wn.h actually changes case of the string in place, without 
			// making a copy. This is not documented. Hence make a copy before passing it.
			lemma = g_strdup(lemma);

			if(!(is_defined(lemma, NOUN) & bit(ptr)))
			{
				depth = 0;
			}

			if(ptr == HMERONYM)
			{
				grow_tree(lemma, synptr, HASMEMBERPTR, &tree, depth);
				grow_tree(lemma, synptr, HASSTUFFPTR, &tree, depth);
				grow_tree(lemma, synptr, HASPARTPTR, &tree, depth);
			}
			else if(ptr == HHOLONYM)
			{
				grow_tree(lemma, synptr, ISMEMBERPTR, &tree, depth);
				grow_tree(lemma, synptr, ISSTUFFPTR, &tree, depth);
				grow_tree(lemma, synptr, ISPARTPTR, &tree, depth);
			}
			
			// before calling trace_inheritance, for meronym, check if depth is 1 (meaning, it was 
			// defined and WNI also that WNI is in advanced mode)
			if(HMERONYM == ptr && 1 == depth)
			{
				trace_inherit(lemma, synptr, &tree, 1);
			}

			g_free(lemma);	// free the copy created for HOLO/MERO
		}
		else
		{
			grow_tree(lemma, synptr, ptr, &tree, depth);
		}

		// if a tree didn't grow, clear it, else append it.
		if(G_NODE_IS_LEAF(tree))
		{
			//g_free(tree_mapping);
			g_slice_free1(sizeof(WNISynonymMapping), tree_mapping);
			g_node_destroy(tree);
		}
		else
			tree_list->properties_list = g_slist_append(tree_list->properties_list, tree);
	}
}


/*
	Populates the requested flags for the given lemma in the given POS
*/

static void populate(gchar *lemma, guint8 pos, WNIRequestFlags flag, gchar *actual_search, guint definitions_set, gboolean advanced_mode)
{
	SynsetPtr cursyn = NULL;
	IndexPtr index = NULL;
	guint16 i = 0, offset_count = 0;
	guint8 sense = 0;
	unsigned long offsets[MAXSENSE] = {0};	//MAXSENSE is 75
	gboolean skip_it = FALSE;
	WNIDefinitionItem *def_item = NULL;
	WNIOverview *overview = NULL;
	WNIProperties *antonyms = NULL, *attributes = NULL, *causes = NULL, *entails = NULL, *similar = NULL, *derivations = NULL, *classes = NULL;
	WNIProperties *pertains = NULL, *hypertree = NULL, *hypotree = NULL, *merotree = NULL, *holotree = NULL;


	if(WORDNET_INTERFACE_OVERVIEW & flag)
		overview = (WNIOverview*) get_from_global_list(WORDNET_INTERFACE_OVERVIEW);

	// Create or get the required flag's data part in master list
	if((WORDNET_INTERFACE_ANTONYMS & flag) && (definitions_set & bit(ANTPTR)))
		antonyms = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_ANTONYMS);

	if((WORDNET_INTERFACE_ATTRIBUTES & flag) && (definitions_set & bit(ATTRIBUTE)))
		attributes = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_ATTRIBUTES);

	if((WORDNET_INTERFACE_CAUSES & flag) && (definitions_set & bit(CAUSETO)))
		causes = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_CAUSES);

	if((WORDNET_INTERFACE_ENTAILS & flag) && (definitions_set & bit(ENTAILPTR)))
		entails = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_ENTAILS);

	if((WORDNET_INTERFACE_SIMILAR & flag) && (definitions_set & bit(SIMPTR)))
		similar = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_SIMILAR);

	if((WORDNET_INTERFACE_DERIVATIONS & flag) && (definitions_set & bit(DERIVATION)))
		derivations = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_DERIVATIONS);

	if((WORDNET_INTERFACE_PERTAINYMS & flag) && (definitions_set & bit(PERTPTR)))
		pertains = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_PERTAINYMS);

	if((WORDNET_INTERFACE_CLASS & flag) && ((definitions_set & bit(CLASS)) || (definitions_set & bit(CLASSIFICATION))))
		classes = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_CLASS);

	if((WORDNET_INTERFACE_HYPERNYMS & flag) && (definitions_set & bit(HYPERPTR)))
		hypertree = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_HYPERNYMS);

	if((WORDNET_INTERFACE_HYPONYMS & flag) && (definitions_set & bit(HYPOPTR)))
		hypotree = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_HYPONYMS);

	if((WORDNET_INTERFACE_MERONYMS & flag) && ((definitions_set & bit(HMERONYM)) || (definitions_set & bit(MERONYM))))
		merotree = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_MERONYMS);

	if((WORDNET_INTERFACE_HOLONYMS & flag) && ((definitions_set & bit(HHOLONYM)) || (definitions_set & bit(HOLONYM))))
		holotree = (WNIProperties*) get_from_global_list(WORDNET_INTERFACE_HOLONYMS);


	while((index = getindex(lemma, pos)))
	{
		lemma = NULL;		// Make lemma NULL, so that the next call returns further indices for the first search

		// create a synonym defn. item
		if(WORDNET_INTERFACE_OVERVIEW & flag)
			def_item = create_synonym_def(pos, index, &overview);

		// def_item is checked against NULL to make sure we can place ID's for the generated properties
		for(sense = 0; (def_item != NULL) && (sense < index->off_cnt); sense++)
		{
			for(i = 0, skip_it = FALSE; i < offset_count && !skip_it; i++)
				if(offsets[i] == index->offset[sense]) skip_it = TRUE;
			if(!skip_it)
			{
				offsets[offset_count++] = index->offset[sense];
				cursyn = read_synset(pos, index->offset[sense], index->wd);

				if(WORDNET_INTERFACE_OVERVIEW & flag)
					populate_synonyms(&def_item->lemma, cursyn, index, &def_item, &overview, sense, def_item->pos);

				if((WORDNET_INTERFACE_ANTONYMS & flag) && antonyms)
				{
					if(ADJ == pos)
						populate_adj_antonyms(cursyn, &antonyms, def_item->id, sense, advanced_mode);
					else
						populate_antonyms(cursyn, &antonyms, def_item->id, sense, advanced_mode);
				}

				if((WORDNET_INTERFACE_ATTRIBUTES & flag) && attributes) //(NOUN == pos || ADJ == pos))
					populate_ptr(def_item->lemma, cursyn, &attributes, ATTRIBUTE, def_item->id, sense, WORDNET_INTERFACE_ATTRIBUTES);

				if((WORDNET_INTERFACE_CAUSES & flag) && causes) //(VERB == pos))
					populate_ptr(def_item->lemma, cursyn, &causes, CAUSETO, def_item->id, sense, WORDNET_INTERFACE_CAUSES);

				if((WORDNET_INTERFACE_ENTAILS & flag) && entails) //(VERB == pos))
					populate_ptr(def_item->lemma, cursyn, &entails, ENTAILPTR, def_item->id, sense, WORDNET_INTERFACE_ENTAILS);

				if((WORDNET_INTERFACE_SIMILAR & flag) && similar) //(ADJ == pos))
				{
					// nouns & verbs don't have a SIMPTR defined explicitly, HYPERPTR is used instead, with 0 depth
					if(NOUN == pos || VERB == pos)
						populate_ptr(def_item->lemma, cursyn, &similar, HYPERPTR, def_item->id, sense, WORDNET_INTERFACE_SIMILAR);
					else
						populate_ptr(def_item->lemma, cursyn, &similar, SIMPTR, def_item->id, sense, WORDNET_INTERFACE_SIMILAR);
				}

				if((WORDNET_INTERFACE_DERIVATIONS & flag) && derivations)// (ADV != pos))
					populate_ptr(def_item->lemma, cursyn, &derivations, DERIVATION, def_item->id, sense, WORDNET_INTERFACE_DERIVATIONS);

				if((WORDNET_INTERFACE_PERTAINYMS & flag) && pertains)// (ADJ == pos || ADV == pos))
					populate_ptr_tree(def_item->lemma, cursyn, &pertains, PERTPTR, def_item->id, sense, advanced_mode);

				if((WORDNET_INTERFACE_CLASS & flag) && classes)
					populate_ptr(def_item->lemma, cursyn, &classes, CLASS, def_item->id, sense, WORDNET_INTERFACE_CLASS);

				if((WORDNET_INTERFACE_HYPERNYMS & flag) && hypertree)// (NOUN == pos || VERB == pos))
					populate_ptr_tree(def_item->lemma, cursyn, &hypertree, HYPERPTR, def_item->id, sense, advanced_mode);

				if((WORDNET_INTERFACE_HYPONYMS & flag) && hypotree)// (NOUN == pos || VERB == pos))
					populate_ptr_tree(def_item->lemma, cursyn, &hypotree, HYPOPTR, def_item->id, sense, advanced_mode);

				if((WORDNET_INTERFACE_MERONYMS & flag) && merotree)
					populate_ptr_tree(def_item->lemma, cursyn, &merotree, HMERONYM, def_item->id, sense, advanced_mode);

				if((WORDNET_INTERFACE_HOLONYMS & flag) && holotree)
					populate_ptr_tree(def_item->lemma, cursyn, &holotree, HHOLONYM, def_item->id, sense, advanced_mode);

				free_synset(cursyn);
			}
		}

		// add the created synonym
		if(WORDNET_INTERFACE_OVERVIEW & flag)
			add_synonym_def(&def_item, &overview, actual_search);

		free_index(index);
	}
}


/*
	Exposed function to which external quries reach for populating the requested info in response_list
*/

void wni_request_nyms(gchar *search_str, GSList **response_list, WNIRequestFlags additional_request_flags, gboolean advanced_mode)
{
	guint definitions_set = 0, i = 0;
	gchar *morphword = NULL;
	gboolean morphword_in_file = TRUE;			// This bit denotes if the lemma is in exceptions file, then no need to do prediction tech. and vice-versa

	if(NULL == search_str)
		return;

	// if WordNet's database files are not opened, open it
	if(OpenDB != 1) wninit();

	if(1 == OpenDB)
	{
		if(*response_list)			// if the list passed isn't empty, free it
			wni_free(response_list);

		search_str = g_strdup(search_str);
		// WordNet has index only in lowercase and spaces are in underscore form.
		// Note that strtolower not only changes case, it also ends the string at the point where a '(' occurs.
		// See WordNet strtolower's doc.
		strtolower(strsubst(g_strstrip(search_str), ' ', '_'));
		if(g_utf8_strlen(search_str, -1) > 0)
		{
			for(i = 1; i <= NUMPARTS; i++)
			{
				if((definitions_set = is_defined(search_str, i)) != 0)	// search for the unmorphed occurance of the search string
					populate(search_str, i, WORDNET_INTERFACE_OVERVIEW | additional_request_flags, search_str, definitions_set, advanced_mode);

				if (morphword_in_file && (morphword = morphstr(search_str, i - 1)) != NULL)	// i -1 is because of the issue Wordnet has in Ubuntu which it doesn't have in Windows
				{									// Searching for 'automata' returns NULL in Ubuntu and "automaton" in Windows.
					do
					{
						//G_PRINTF("i - 1: %d, %s\n", i, morphword);

						if ((definitions_set = is_defined(morphword, i)) != 0)
							populate(morphword, i, WORDNET_INTERFACE_OVERVIEW | additional_request_flags, search_str, definitions_set, advanced_mode);
					} while((morphword = morphstr(NULL, i - 1)) != NULL );
				}
				else if((morphword = morphstr(search_str, i)) != NULL)			// This is the actual search mech. which should work fine
				{
					morphword_in_file = FALSE;					// In the first go if it's not present in the excp. file, then its not there at all
					do
					{
						//G_PRINTF("i: %d, %s\n", i, morphword);
						if ((definitions_set = is_defined(morphword, i)) != 0)
							populate(morphword, i, WORDNET_INTERFACE_OVERVIEW | additional_request_flags, search_str, definitions_set, advanced_mode);
					} while((morphword = morphstr(NULL, i)) != NULL );
				}
			}
		}

		g_free(search_str);
		// once all the requests are generated, prune the results
		prune_results();

		// point the requested list ptr to the generated global list
		if(global_list)
			*response_list = global_list;
		global_list = NULL;
	}
}


/*
	A simple list whose data part doesn't have any further links and is terminal, can be freed 
	using this function. For debugging purposes, the second argument (mode) was introduced. It sets the 
	mode to prints a synonym mapping, antonym mapping or a string. It is also used for freeing the memory back to slice alloc.
*/

static void simple_list_free(GSList **list, guint8 mode)
{
	GSList *temp_list = *list;
	WNISynonymMapping *synonym_mapping = NULL;
	WNIAntonymMapping *antonym_mapping = NULL;
	WNIPropertyMapping *property_mapping = NULL;

	while(temp_list)
	{
		if(temp_list->data)
		{
			if(SYNONYM_MAPPING == mode)
			{
				synonym_mapping = (WNISynonymMapping*) temp_list->data;
				G_PRINTF("(%d, %d)\n", synonym_mapping->id, synonym_mapping->sense);
				g_slice_free1(sizeof(WNISynonymMapping), temp_list->data);
			}
			else if(ANTONYM_MAPPING == mode)
			{
				antonym_mapping = (WNIAntonymMapping*) temp_list->data;
				G_PRINTF("ID: %d, Sense: %d, Related Word Count: %d\n", antonym_mapping->id, antonym_mapping->sense, antonym_mapping->related_word_count);
				g_slice_free1(sizeof(WNIAntonymMapping), temp_list->data);
			}
			else if(PROPERTY_MAPPING == mode)
			{
				property_mapping = (WNIPropertyMapping*) temp_list->data;
				G_PRINTF("ID: %d, Sense: %d, Self Sense: %d\n", property_mapping->id, property_mapping->sense, property_mapping->self_sense);
				g_slice_free1(sizeof(WNIPropertyMapping), temp_list->data);
			}
			else
			{
				G_PRINTF("%s\n", (gchar*) temp_list->data);
				g_free(temp_list->data);			// it should be a string created with g_strdup()
			}
		}

		//g_free(temp_list->data);
		temp_list->data = NULL;
		temp_list = g_slist_next(temp_list);
	}
}


/*
	A function to free the synonyms list. When a lemma is found as synonym, 
	the single_element is set, so as to only remove that particular element.
	If done, the node should be freed. This mechanism is employed only in 
	this place, elsewhere its not and shouldn't be set.
*/

static void synonym_list_free(GSList **list, gboolean single_element)
{
	//WNISynonym *temp_synonym = NULL;
	WNIPropertyItem *temp_synonym = NULL;
	GSList *temp_list = *list;

	while(temp_list)
	{
		if(temp_list->data)
		{
			//temp_synonym = (WNISynonym*) temp_list->data;
			temp_synonym = (WNIPropertyItem*) temp_list->data;
			if(temp_synonym->mapping)
			{
				simple_list_free(&temp_synonym->mapping, SYNONYM_MAPPING);
				g_slist_free(temp_synonym->mapping);
			}

			G_PRINTF("Synonym: %s\n", temp_synonym->term);

			g_free(temp_synonym->term);
			//g_free(temp_synonym);
			//g_slice_free1(sizeof(WNISynonym), temp_synonym);
			g_slice_free1(sizeof(WNIPropertyItem), temp_synonym);
			temp_synonym = temp_list->data = NULL;
		}
		temp_list = single_element?NULL:g_slist_next(temp_list);	// if single_element set, stop traversal.
	}
}


/*
	A function to free the definitions.
*/

static void definitions_free(GSList **list)
{
	WNIDefinition *temp_def = NULL;
	GSList *temp_list = *list;

	while(temp_list)
	{
		if(temp_list->data);
		{
			temp_def = (WNIDefinition*) temp_list->data;
			G_PRINTF("(%d) %s\n", temp_def->tag_count, temp_def->definition);

			if(temp_def->examples)
			{
				simple_list_free(&temp_def->examples, 0);
				g_slist_free(temp_def->examples);
			}
			g_free(temp_def->definition);
			//g_free(temp_def);
			g_slice_free1(sizeof(WNIDefinition), temp_def);
			temp_def = temp_list->data = NULL;
		}
		temp_list = g_slist_next(temp_list);
	}
}


/*
	A function to free the definitions list.
*/

static void definitions_list_free(GSList **list)
{
	WNIDefinitionItem *tmp_def_item = NULL;
	GSList *temp_list = *list;
	
	while(temp_list)
	{
		if(temp_list->data)
		{
			tmp_def_item = (WNIDefinitionItem*) temp_list->data;
			if(tmp_def_item->definitions)
			{
				definitions_free(&tmp_def_item->definitions);
				g_slist_free(tmp_def_item->definitions);
			}

			G_PRINTF("%s -> ID: %d - POS: %s\n", tmp_def_item->lemma, tmp_def_item->id, partnames[tmp_def_item->pos]);

			g_free(tmp_def_item->lemma);
			//g_free(tmp_def_item);
			g_slice_free1(sizeof(WNIDefinitionItem), tmp_def_item);
			tmp_def_item = temp_list->data = NULL;
		}
		temp_list = g_slist_next(temp_list);
	}
}


/*
	A function to free the implications list.
*/

static void implications_free(GSList **list, guint8 mode)
{
	WNIImplication *temp_implication = NULL;
	GSList *temp_list = *list;

	while(temp_list)
	{
		if(temp_list->data)
		{
			temp_implication = (WNIImplication*) temp_list->data;

			if(mode)
				G_PRINTF("=>%s #%d\n", temp_implication->term, temp_implication->sense);

			g_free(temp_implication->term);
			//g_free(temp_implication);
			g_slice_free1(sizeof(WNIImplication), temp_implication);
			temp_implication = temp_list->data = NULL;
		}
		temp_list = g_slist_next(temp_list);
	}
}


/*
	A function to free the antonyms list.
*/

static void antonyms_list_free(GSList **list)
{
	WNIAntonymItem *temp_antonym = NULL;
	GSList *temp_list = *list;

	while(temp_list)
	{
		if(temp_list->data)
		{
			temp_antonym = (WNIAntonymItem*) temp_list->data;
			simple_list_free(&temp_antonym->mapping, ANTONYM_MAPPING);

			G_PRINTF("%s #%d - Relation: %s\n", temp_antonym->term, temp_antonym->sense, (DIRECT_ANT == temp_antonym->relation)?"Direct":"Indirect");

			implications_free(&temp_antonym->implications, 1);
			g_slist_free(temp_antonym->mapping);
			g_slist_free(temp_antonym->implications);
			g_free(temp_antonym->term);
			//g_free(temp_antonym);
			g_slice_free1(sizeof(WNIAntonymItem), temp_antonym);
			temp_antonym = temp_list->data = NULL;
		}
		temp_list = g_slist_next(temp_list);
	}
}


/*
	A function to free the properties list.
*/

static void properties_list_free(GSList **list, WNIRequestFlags id)
{
	WNIPropertyItem *temp_property = NULL;
	GSList *temp_list = *list;
	
	while(temp_list)
	{
		if(temp_list->data)
		{
			temp_property = (WNIPropertyItem*) temp_list->data;
			simple_list_free(&temp_property->mapping, PROPERTY_MAPPING);

			if(WORDNET_INTERFACE_ATTRIBUTES & id)
				G_PRINTF("Attribute: %s\n", temp_property->term);
			else if(WORDNET_INTERFACE_CAUSES & id)
				G_PRINTF("Causes: %s\n", temp_property->term);
			else if(WORDNET_INTERFACE_ENTAILS & id)
				G_PRINTF("Entails: %s\n", temp_property->term);
			else if(WORDNET_INTERFACE_SIMILAR & id)
				G_PRINTF("Similar: %s\n", temp_property->term);
			else if(WORDNET_INTERFACE_DERIVATIONS & id)
				G_PRINTF("Derivations: %s\n", temp_property->term);

			g_slist_free(temp_property->mapping);
			g_free(temp_property->term);
			//g_free(temp_property);
			g_slice_free1(sizeof(WNIPropertyItem), temp_property);
			temp_property = temp_list->data = NULL;
		}
		temp_list = g_slist_next(temp_list);
	}
}


/*
	A function to free the classifications list.
*/

static void class_list_free(GSList **list)
{
	GSList *temp_list = *list;
	WNIClassItem *temp_class_item = NULL;
	
	while(temp_list)
	{
		if(temp_list->data)
		{
			temp_class_item = (WNIClassItem*) temp_list->data;

			switch(temp_class_item->type)
			{
				case CLASSIF_CATEGORY:
					G_PRINTF("Topic: ");
					break;
				case CLASSIF_USAGE:
					G_PRINTF("Usage: ");
					break;
				case CLASSIF_REGIONAL:
					G_PRINTF("Region: ");
					break;
				case CLASS_CATEGORY:
					G_PRINTF("Topic Term: ");
					break;
				case CLASS_USAGE:
					G_PRINTF("Usage Term: ");
					break;
				case CLASS_REGIONAL:
					G_PRINTF("Regional Term: ");
					break;
			}
			G_PRINTF("(%c) %s#%d -> ID: %d, Sense: %d\n", partchars[temp_class_item->self_pos], 
			temp_class_item->term, temp_class_item->self_sense, temp_class_item->id, temp_class_item->sense);

			g_free(temp_class_item->term);
			//g_free(temp_class_item);
			g_slice_free1(sizeof(WNIClassItem), temp_class_item);
			temp_class_item = temp_list->data = NULL;
		}
		temp_list = g_slist_next(temp_list);
	}
}


/*
	This function clears a given node's data part. It assumes 
	that the passed node isn't the actual root of the tree.
	I.e. the node contains a list and a list type in its data.
*/

static gboolean node_free(GNode *node, gpointer data)
{
	WNITreeList *temp_tree_list = node->data;
	WNIImplication *implication = NULL;
	GSList *temp_list = NULL;

	if(temp_tree_list)
	{

		G_PRINTF("Type: %d\n", temp_tree_list->type);	// print the type of list - has stuff, is member, instance of, etc. default is 0 (no type)

		temp_list = temp_tree_list->word_list;
		while(temp_list)
		{
			if(temp_list->data)
			{
				implication = (WNIImplication*) temp_list->data;

				G_PRINTF("\t%s #%d\n", implication->term, implication->sense);

				g_free(implication->term);
				//g_free(temp_list->data);
				g_slice_free1(sizeof(WNIImplication), temp_list->data);
				implication = temp_list->data = NULL;
			}
			temp_list = g_slist_next(temp_list);
		}
		g_slist_free(temp_tree_list->word_list);
		//g_free(node->data);
		g_slice_free1(sizeof(WNITreeList), node->data);
		temp_tree_list = node->data = NULL;
	}
	
	return FALSE;
}


/*
	This function is called for every child of actual root, which in turn
	clears the child node and its subtree's data part.
*/

static void tree_free(GNode *node, gpointer data)
{
	g_node_traverse(node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, node_free, NULL);
}


/*
	A function to free the properties tree. Property Trees' root node always 
	have a synonym mapping as its data. Further below are the actual trees.
*/

static void properties_tree_free(GSList **list, WNIRequestFlags id)
{
	GNode *tree_root = NULL;
	GSList *temp_list = *list;
	WNISynonymMapping *mapping  = NULL;
	
	while(temp_list)
	{
		if(temp_list->data)
		{
			tree_root = (GNode*) temp_list->data;
			mapping = (WNISynonymMapping*) tree_root->data;

			if(WORDNET_INTERFACE_HYPERNYMS & id)
				G_PRINTF("Hypernyms: %d, %d\n", mapping->id, mapping->sense);
			else if (WORDNET_INTERFACE_HYPONYMS & id)
				G_PRINTF("Hyponyms: %d, %d\n", mapping->id, mapping->sense);
			else if (WORDNET_INTERFACE_MERONYMS & id)
				G_PRINTF("Meronyms: %d, %d\n", mapping->id, mapping->sense);
			else if (WORDNET_INTERFACE_HOLONYMS & id)
				G_PRINTF("Holonyms: %d, %d\n", mapping->id, mapping->sense);
			else if (WORDNET_INTERFACE_PERTAINYMS & id)
				G_PRINTF("Pertains: %d, %d\n", mapping->id, mapping->sense);

			//g_free(tree_root->data);
			g_slice_free1(sizeof(WNISynonymMapping), tree_root->data);
			mapping = tree_root->data = NULL;
			g_node_children_foreach(tree_root, G_TRAVERSE_ALL, tree_free, NULL);
			g_node_destroy(tree_root);
		}
		temp_list = g_slist_next(temp_list);
	}
}


/*
	A function designed to free all the data allocated by WNI.
*/

void wni_free(GSList **response_list)
{
	WNINym *temp_nym = NULL;
	WNIOverview *temp_overview = NULL;
	WNIProperties *temp_antonyms = NULL;
	WNIProperties *temp_properties = NULL;

	global_list = *response_list;
	while(global_list)
	{
		if(global_list->data)
		{
			temp_nym = (WNINym*) global_list->data;
			switch(temp_nym->id)
			{
				case WORDNET_INTERFACE_OVERVIEW:
				{
					if(temp_nym->data)
					{
						temp_overview = (WNIOverview*) temp_nym->data;
						if(temp_overview->definitions_list)
						{
							definitions_list_free(&temp_overview->definitions_list);
							g_slist_free(temp_overview->definitions_list);
						}
						temp_overview->definitions_list = NULL;

						if(temp_overview->synonyms_list)
						{
							synonym_list_free(&temp_overview->synonyms_list, FALSE);
							g_slist_free(temp_overview->synonyms_list);
						}
						temp_overview->synonyms_list = NULL;

						//g_free(temp_nym->data); temp_nym->data = NULL;
						g_slice_free1(sizeof(WNIOverview), temp_nym->data);
						temp_overview = temp_nym->data = NULL;
					}
					break;
				}
				case WORDNET_INTERFACE_ANTONYMS:
				{
					if(temp_nym->data)
					{
						temp_antonyms = (WNIProperties*) temp_nym->data;
						if(temp_antonyms->properties_list)
						{
							antonyms_list_free(&temp_antonyms->properties_list);
							g_slist_free(temp_antonyms->properties_list);
						}
						temp_antonyms->properties_list = NULL;
						
						//g_free(temp_nym->data); temp_nym->data = NULL;
						g_slice_free1(sizeof(WNIProperties), temp_nym->data);
						temp_antonyms = temp_nym->data = NULL;
					}
					break;
				}
				case WORDNET_INTERFACE_ATTRIBUTES:
				case WORDNET_INTERFACE_CAUSES:
				case WORDNET_INTERFACE_ENTAILS:
				case WORDNET_INTERFACE_SIMILAR:
				case WORDNET_INTERFACE_DERIVATIONS:
				case WORDNET_INTERFACE_CLASS:
				{
					if(temp_nym->data)
					{
						temp_properties = (WNIProperties*) temp_nym->data;
						if(temp_properties->properties_list)
						{
							if(temp_nym->id != WORDNET_INTERFACE_CLASS)
								properties_list_free(&temp_properties->properties_list, temp_nym->id);
							else
								class_list_free(&temp_properties->properties_list);
							g_slist_free(temp_properties->properties_list);
						}
						temp_properties->properties_list = NULL;

						//g_free(temp_nym->data); temp_nym->data = NULL;
						g_slice_free1(sizeof(WNIProperties), temp_nym->data);
						temp_properties = temp_nym->data = NULL;
					}
					break;
				}
				case WORDNET_INTERFACE_HYPERNYMS:
				case WORDNET_INTERFACE_HYPONYMS:
				case WORDNET_INTERFACE_MERONYMS:
				case WORDNET_INTERFACE_HOLONYMS:
				case WORDNET_INTERFACE_PERTAINYMS:
				{
					if(temp_nym->data)
					{
						temp_properties = (WNIProperties*) temp_nym->data;
						if(temp_properties->properties_list)
						{
							properties_tree_free(&temp_properties->properties_list, temp_nym->id);
							g_slist_free(temp_properties->properties_list);
						}
						temp_properties->properties_list = NULL;
						
						//g_free(temp_nym->data); temp_nym->data = NULL;
						g_slice_free1(sizeof(WNIProperties), temp_nym->data);
						temp_properties = temp_nym->data = NULL;
					}
					break;
				}
				default:
					//g_free(temp_nym->data); temp_nym->data = NULL;	// should never reach here, since all the cases are handled above specifically
					g_slice_free1(sizeof(WNIProperties), temp_nym->data); temp_nym->data = NULL;
					break;
			}
			//g_free(global_list->data); global_list->data = NULL;
			g_slice_free1(sizeof(WNINym), global_list->data);
			temp_nym = global_list->data = NULL;
		}
		global_list = g_slist_next(global_list);
	}
	g_slist_free(*response_list);
	*response_list = global_list = NULL;
}

