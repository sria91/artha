/* instance_handler.c
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
 * Code to handle multiple instances using dbus signals
 */



#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifdef DBUS_AVAILABLE

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gtk/gtk.h>

#include "tomboyutil.h"

#define STR_UNIQUE_BUS_NAME		"org.artha.unique"
#define STR_DUPLICATE_OBJECT_PATH	"/org/artha/duplicate"
#define STR_DUPLICATE_INTERFACE_NAME	"org.artha.duplicate"
#define STR_SIGNAL_NAME			"duplicate_instance_invoked"
#define STR_SIGNAL_MATCH_RULE		"type='signal',interface='org.artha.duplicate',path='/org/artha/duplicate'"

DBusConnection *bus = NULL;

static DBusHandlerResult signal_receiver(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	if(dbus_message_is_signal(message, STR_DUPLICATE_INTERFACE_NAME, STR_SIGNAL_NAME))
	{
		/* present the unqiue instance window to the user 
		when a duplicate instance was invoked */
		tomboy_window_present_hardcore(GTK_WINDOW(user_data));

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void instance_handler_send_signal()
{
	DBusMessage *mesg = NULL;

	g_assert(bus);

	mesg = dbus_message_new_signal(STR_DUPLICATE_OBJECT_PATH, STR_DUPLICATE_INTERFACE_NAME, STR_SIGNAL_NAME);
	dbus_connection_send(bus, mesg, NULL);

	dbus_message_unref(mesg);
	mesg = NULL;
}

gboolean instance_handler_am_i_unique()
{
	DBusError err = {0};
	int ret_code = 0;

	bus = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err))
		goto free_error_and_quit;

	dbus_connection_setup_with_g_main(bus, NULL);

	ret_code = dbus_bus_request_name(bus, STR_UNIQUE_BUS_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
	if(dbus_error_is_set(&err))
		goto free_error_and_quit;

	return (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER == ret_code);

free_error_and_quit:
	g_error("D-Bus Error: %s\n", err.message);
	dbus_error_free(&err);

	return FALSE;
}

gboolean instance_handler_register_signal(GtkWindow *window)
{
	DBusError err = {0};

	dbus_bus_add_match(bus, STR_SIGNAL_MATCH_RULE, &err);
	if(dbus_error_is_set(&err))
	{
		g_error("Error registering signal match rule: %s\n", err.message);
		dbus_error_free(&err);
		
		return FALSE;
	}

	return dbus_connection_add_filter(bus, signal_receiver, (void*) window, NULL);
}

#endif		/* DBUS_AVAILABLE */
