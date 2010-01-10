/* tomboyutil.h
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
 * gtk_window_present function doesn't always present the window to the 
 * user; hence libtomboy's tomboy_window_present_hardcore function is 
 * used with minor modifications.
 *
 * Credit for this code goes fully to Alex Graveley, alex@beatniksoftware.com
 */


#ifndef __TOMBOY_UTIL_H__
#define __TOMBOY_UTIL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

gint tomboy_window_get_workspace (GtkWindow *window);

void tomboy_window_move_to_current_workspace (GtkWindow *window);

void tomboy_window_present_hardcore (GtkWindow *window);

G_END_DECLS

#endif		/* __TOMBOY_UTIL_H__ */

