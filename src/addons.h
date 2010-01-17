/* addons.h
 * Artha - Free cross-platform open thesaurus
 * Copyright (C) 2009, 2010 Sundaram Ramaswamy, legends2k@yahoo.com
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
 * Modules that are looked up at run time for additional features
 * by Artha (plugins) will have function prototypes here.
 */

 
#ifndef __ADDONS_H__
#define __ADDONS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Exposed functions of the 'Suggestions' module */

gboolean 	suggestions_init();
gchar** 	suggestions_get(const gchar* lemma);
gboolean 	suggestions_uninit();


/* Exposed functions of 'Notify' module */
gboolean	mod_notify_init(GtkStatusIcon *status_icon);
gboolean	mod_notify_uninit(void);

G_END_DECLS

#endif		/* __ADDONS_H__ */

