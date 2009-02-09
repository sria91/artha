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
 	Header: addons.h
 	
 	Modules that are looked up at run time for additional features 
 	by Artha (plugins) will have function prototypes here.
 */
 
#ifndef ADDONS_H
#define ADDONS_H

#include <glib.h>

G_BEGIN_DECLS

// Exposed functions of the 'Suggestions' module

extern gboolean suggestions_init();
extern gchar** suggestions_get(gchar* lemma);
extern gboolean suggestions_uninit();

G_END_DECLS

#endif

