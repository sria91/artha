/* mod_notify.c
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
 * Dynamic linking of libnotify's functions for passive desktop 
 * notifications.
 */


#include "mod_notify.h"
#include "wni.h"
#include <gmodule.h>

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifdef G_OS_WIN32
#	define NOTIFY_FILE		"libnotify-1.dll"
#else
#	define NOTIFY_FILE		"libnotify.so.1"
#endif

GModule *mod_notify = NULL;
extern NotifyNotification *notifier;

gboolean mod_notify_init(GtkStatusIcon *status_icon)
{
	g_assert(status_icon);

	if(g_module_supported() && (mod_notify = g_module_open(NOTIFY_FILE, G_MODULE_BIND_LAZY)))
	{
		g_module_symbol(mod_notify, G_STRINGIFY(notify_init), (gpointer *) &notify_init);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_uninit), (gpointer *) &notify_uninit);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_notification_new_with_status_icon), (gpointer *) &notify_notification_new_with_status_icon);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_notification_update), (gpointer *) &notify_notification_update);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_notification_show), (gpointer *) &notify_notification_show);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_notification_close), (gpointer *) &notify_notification_close);
		
		if(NULL != notify_init && NULL != notify_uninit && NULL != notify_notification_new_with_status_icon &&
		NULL != notify_notification_update && NULL != notify_notification_show && NULL != notify_notification_close)
		{
			if(notify_init(PACKAGE_NAME))
			{
				/* initialize summary as Artha (Package Name)
				   this will, however, be modified to the looked up word before display */
				notifier = notify_notification_new_with_status_icon(PACKAGE_NAME, NULL, "gtk-dialog-info", status_icon);
				G_MESSAGE("Notification module successfully loaded", NULL);

				return TRUE;
			}
		}
	}

	G_MESSAGE("Failed to load notifications module", NULL);
	return FALSE;
}

gboolean mod_notify_uninit(void)
{
	if(mod_notify && notifier)
	{
		/* close notifications, if open */
		notify_notification_close(notifier, NULL);

#ifndef G_OS_WIN32
		g_object_unref(G_OBJECT(notifier));
#endif
		notifier = NULL;

		notify_uninit();
		
		g_module_close(mod_notify);

		return TRUE;
	}
	
	return FALSE;
}

