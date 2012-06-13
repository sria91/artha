/* libnotify.h
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
 * libnotify (win32) export/import header
 */

 
 /*
	The following ifdef block is the standard way of creating macros which make exporting 
	from a DLL simpler. All files within this DLL are compiled with the LIBNOTIFY_EXPORTS
	symbol defined on the command line. This symbol should not be defined on any project
	that uses this DLL. This way any other project whose source files include this file see 
	LIBNOTIFY_API functions as being imported from a DLL, whereas this DLL sees symbols
	defined with this macro as being exported.
*/
#ifndef __LIBNOTIFY_H__
#define __LIBNOTIFY_H__

#ifdef DLL_EXPORT
#define LIBNOTIFY_API __declspec(dllexport)
#else
#define LIBNOTIFY_API extern __declspec(dllimport)
#endif

#include <glib.h>

typedef struct _NotifyNotification        	NotifyNotification;

LIBNOTIFY_API gboolean notify_init(const char* app_name);
LIBNOTIFY_API void notify_uninit(void);

LIBNOTIFY_API NotifyNotification* notify_notification_new(
	const gchar *summary, const gchar *body,
	const gchar *icon);

LIBNOTIFY_API gboolean notify_notification_update
(NotifyNotification *notification,
        const gchar *summary,
        const gchar *body,
        const gchar *icon);

LIBNOTIFY_API gboolean notify_notification_show(
NotifyNotification *notification,
			GError **error);

LIBNOTIFY_API gboolean notify_notification_close(
NotifyNotification *notification,
			GError **error);

#endif		/* __LIBNOTIFY_H__ */
