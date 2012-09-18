/* wni.h
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
 * WordNet Interface Header
 * This header exposes the functions required to query data from wordnet
 */

 
#ifndef __WNI_H__
#define __WNI_H__

#include <glib.h>

G_BEGIN_DECLS

#ifdef HAVE_CONFIG_H

#	if DEBUG_LEVEL >= 1
#		define G_DEBUG(...) g_debug(__VA_ARGS__)
#	endif		/* DEBUG_LEVEL >= 1 */

#	if DEBUG_LEVEL >= 2
#		define G_MESSAGE(...) g_message(__VA_ARGS__)
#	endif		/* DEBUG_LEVEL >= 2 */

#	if DEBUG_LEVEL >= 3
#		include <glib/gprintf.h>
#		define G_PRINTF(...) g_printf(__VA_ARGS__)
#	endif		/* DEBUG_LEVEL >= 3 */

#endif		/* HAVE_CONFIG_H */

#ifndef __noop
#define __noop(...) ((void) 0)
#endif

#ifndef G_DEBUG
#define G_DEBUG __noop
#endif

#ifndef G_MESSAGE
#define G_MESSAGE __noop
#endif

#ifndef G_PRINTF
#define G_PRINTF __noop
#endif


/* For adjectives, indicates synset type */
#define DONT_KNOW	0
#define DIRECT_ANT	1	/* direct antonyms (cluster head) */
#define INDIRECT_ANT	2	/* indrect antonyms (similar) */
#define PERTAINYM	3	/* no antonyms or similars (pertainyms) */

#define MAX_BUFFER 	256

#define SYNONYM_MAPPING		1
#define ANTONYM_MAPPING		2
#define PROPERTY_MAPPING	3

// WNI Data Structures Used
typedef enum
{
	WORDNET_INTERFACE_NONE		= 0,
	WORDNET_INTERFACE_OVERVIEW	= 1 << 0,
	WORDNET_INTERFACE_ANTONYMS	= 1 << 1,
	WORDNET_INTERFACE_DERIVATIONS	= 1 << 2,
	WORDNET_INTERFACE_PERTAINYMS	= 1 << 3,
	WORDNET_INTERFACE_ATTRIBUTES	= 1 << 4,
	WORDNET_INTERFACE_SIMILAR	= 1 << 5,
	WORDNET_INTERFACE_CLASS		= 1 << 6,
	WORDNET_INTERFACE_CAUSES	= 1 << 7,
	WORDNET_INTERFACE_ENTAILS	= 1 << 8,
	WORDNET_INTERFACE_HYPERNYMS	= 1 << 9,
	WORDNET_INTERFACE_HYPONYMS	= 1 << 10,
	WORDNET_INTERFACE_HOLONYMS	= 1 << 11,
	WORDNET_INTERFACE_MERONYMS	= 1 << 12,
	WORDNET_INTERFACE_ALL		=(WORDNET_INTERFACE_OVERVIEW | WORDNET_INTERFACE_ANTONYMS | WORDNET_INTERFACE_DERIVATIONS |
					  WORDNET_INTERFACE_PERTAINYMS | WORDNET_INTERFACE_ATTRIBUTES | WORDNET_INTERFACE_SIMILAR | 
					  WORDNET_INTERFACE_CLASS | WORDNET_INTERFACE_CAUSES | WORDNET_INTERFACE_ENTAILS | 
					  WORDNET_INTERFACE_HYPERNYMS | WORDNET_INTERFACE_HYPONYMS | WORDNET_INTERFACE_HOLONYMS | WORDNET_INTERFACE_MERONYMS)
}	WNIRequestFlags;


struct _nym
{
	WNIRequestFlags id;			// type of the node
	gpointer data;				// data based on the type
};
typedef struct _nym WNINym;


struct _overview
{
	GSList *definitions_list;	// list of WNIDefinitionItems
	GSList *synonyms_list;		// list of WNIPropertyItems
};
typedef struct _overview WNIOverview;


struct _definition_item
{
	gchar *lemma;				// searched term to which these definitions correspond to
	guint8 id;					// unique ID for this node within definitions_list of it's parent
	guint8 pos;					// part of speech
	guint8 count;				// count of WNIDefinitions present
	GSList *definitions;		// list of WNIDefinitions
};
typedef struct _definition_item WNIDefinitionItem;


struct _definition
{
	gchar *definition;			// definition string
	guint16 tag_count;			// number of semantic tags to sense; refer WN
	GSList *examples;			// list of example strings
};
typedef struct _definition WNIDefinition;


// each synonym belongs to a POS and inside a POS it maps to a particular sense
// sense numbers starts from 0, for each POS category
struct _synonym_mapping
{
	guint8 id;					// POS it belongs to can be found by comparing against WNIDefinitionItem.id
	guint16 sense;				// nth sense inside a particular POS
};
typedef struct _synonym_mapping WNISynonymMapping;


struct _antonym_item
{
	gchar *term;				// antonym term
	guint16 sense;				// WordNet sense number - 1
	guint8 relation;			// direct of indirect antonym flag DIRECT_ANT/INDIRECT_ANT
	GSList *mapping;			// list of WNIAntonymMappings
	GSList *implications;		// list of WNIImplications
};
typedef struct _antonym_item WNIAntonymItem;


struct _implication
{
	gchar *term;				// relative term
	guint16 sense;				// WordNet sense number - 1
};
typedef struct _implication WNIImplication;


struct _antonym_mapping
{
	guint8 id;					// POS it belongs to can be found by comparing against WNIDefinitionItem.id
	guint16 sense;				// nth sense inside a particular POS
	guint8 related_word_count;	// number of related words
};
typedef struct _antonym_mapping WNIAntonymMapping;


struct _properties_list
{
	GSList *properties_list;	// list of WNIPropertyItems
};
typedef struct _properties_list WNIProperties;


struct _property_item
{
	gchar *term;				// relative term
	GSList *mapping;			// list of WNIPropertyMappings or WNISynonymMappings
};
typedef struct _property_item WNIPropertyItem;


struct _property_mapping
{
	guint16 self_sense;			// WordNet sense id - 1
	guint8 id;					// POS it belongs to can be found by comparing against WNIDefinitionItem.id
	guint16 sense;              // nth sense inside a particular POS
};
typedef struct _property_mapping WNIPropertyMapping;


struct _class_item
{
	gchar *term;				// relative term based on classification
	guint8 self_pos;			// POS of this term
	guint16 self_sense;			// WordNet sense id - 1
	guint8 id;					// mapping to which WNIDefinitionItem it belongs to
	guint16 sense;				// nth sense inside a particular POS
	guint8 type;				// type of the classification -> [ISMEMBERPTR ... HASPARTPTR] (refer wn.h)
};
typedef struct _class_item WNIClassItem;


struct _tree_list
{
	GSList *word_list;			// list of WNIImplications
	guint8 type;				// type of the implications -> [1 ... MAXPTR] (refer wn.h)
};
typedef struct _tree_list WNITreeList;


// WNI ~ Exposed functions
gboolean wni_request_nyms(gchar *search_str, GSList **response_list, WNIRequestFlags additional_request_flags, gboolean advanced_mode);
void wni_free(GSList **response_list);
int wni_strcmp0(const char *s1, const char *s2);

G_END_DECLS

#endif		/* __WNI_H__ */

