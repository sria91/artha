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


#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "mod_notify.h"
#include "wni.h"
#include <gmodule.h>
#include <stdlib.h>

#ifdef G_OS_WIN32
#	define NOTIFY_FILE		"libnotify-1.dll"
#else
#	define NOTIFY_FILE		"libnotify.so.4"
#endif

GModule *mod_notify = NULL;
gboolean notify_inited = FALSE;
extern NotifyNotification *notifier;

static GModule* lookup_dynamic_library()
{
	GModule *mod = g_module_open(NOTIFY_FILE, G_MODULE_BIND_LAZY);
#ifndef G_OS_WIN32
	// for non Win32 machines try to lookup older versions of libnotify
	if(!mod)
	{
		gchar notify_file_str[] = NOTIFY_FILE;
		const size_t version_index = G_N_ELEMENTS(NOTIFY_FILE) - 2;
		for(gchar i = '3'; ((!mod) && (i > '0')); --i)
		{
			notify_file_str[version_index] = i;
			mod = g_module_open(notify_file_str, G_MODULE_BIND_LAZY);
		}
	}
#endif
	return mod;
}

gboolean mod_notify_init()
{
    gboolean retval = FALSE;

	if(g_module_supported() && (mod_notify = lookup_dynamic_library()))
	{
		g_module_symbol(mod_notify, G_STRINGIFY(notify_init), (gpointer *) &notify_init);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_uninit), (gpointer *) &notify_uninit);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_notification_new), (gpointer *) &notify_notification_new);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_notification_update), (gpointer *) &notify_notification_update);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_notification_show), (gpointer *) &notify_notification_show);
		g_module_symbol(mod_notify, G_STRINGIFY(notify_notification_close), (gpointer *) &notify_notification_close);

		if(NULL != notify_init && NULL != notify_uninit && NULL != notify_notification_new &&
		NULL != notify_notification_update && NULL != notify_notification_show && NULL != notify_notification_close)
		{
			if(notify_init(PACKAGE_NAME))
			{
                notify_inited = TRUE;

				/* initialize summary as Artha (Package Name)
				   this will, however, be modified to the looked up word before display */
				notifier = notify_notification_new(PACKAGE_NAME, NULL, "gtk-dialog-info");
                if(notifier)
                {
                    G_MESSAGE("Notification module successfully loaded");
                    retval = TRUE;
                }
			}
		}

		if(!notifier)
		{
            if(notify_inited)
            {
                notify_uninit();
                notify_inited = FALSE;
            }
		    g_module_close(mod_notify);
		    mod_notify = NULL;
		}
	}

	G_MESSAGE("Failed to load notifications module");
	return retval;
}

gboolean mod_notify_uninit()
{
	if(notifier)
	{
		/* close notifications, if open */
		notify_notification_close(notifier, NULL);

        /*
           reason it's not required in Win32 is this
           module is custom written and doesn't use 
           the GObject framework, while in *nix 
           notify_notification_new internally does a
           g_object_new and returns GObject* as
           NotifyNotification*
        */
#ifdef G_OS_WIN32
		free(notifier);
		notifier = NULL;
#else
		g_object_unref(G_OBJECT(notifier));
#endif
		notifier = NULL;
	}
    
    if(notify_inited)
    {
        notify_uninit();
        notify_inited = FALSE;
    }

    gboolean mod_uninit = TRUE;
    if(mod_notify)
    {
		mod_uninit = g_module_close(mod_notify);
        mod_notify = NULL;
	}

	return mod_uninit;
}
