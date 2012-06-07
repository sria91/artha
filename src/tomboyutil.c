/* tomboyutil.c
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


#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifdef X11_AVAILABLE

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>

#  if DEBUG_LEVEL >= 2
#     define TRACE(x) x
#  endif
#endif

#ifndef TRACE
#  define TRACE(x) do {} while (FALSE);
#endif

extern gboolean	hotkey_processing;
extern guint32	last_hotkey_time;

static guint32
tomboy_keybinder_get_current_event_time (void)
{
	if (hotkey_processing) 
		return last_hotkey_time;
	else
		return GDK_CURRENT_TIME;
}

static void
tomboy_window_move_to_current_workspace (GtkWindow *window)
{
	GdkWindow *gdkwin = GTK_WIDGET (window)->window;
	GdkWindow *rootwin = 
		gdk_screen_get_root_window (gdk_drawable_get_screen (gdkwin));

	GdkAtom current_desktop = 
		gdk_atom_intern ("_NET_CURRENT_DESKTOP", FALSE);
	GdkAtom wm_desktop = gdk_atom_intern ("_NET_WM_DESKTOP", FALSE);
	GdkAtom out_type;
	gint out_format, out_length;
	gulong *out_val;
	int workspace;
	XEvent xev;

	if (!gdk_property_get (rootwin,
			       current_desktop,
			       _GDK_MAKE_ATOM (XA_CARDINAL),
			       0, G_MAXLONG,
			       FALSE,
			       &out_type,
			       &out_format,
			       &out_length,
			       (guchar **) &out_val))
		return;

	workspace = *out_val;
	g_free (out_val);

	TRACE (g_print ("Setting _NET_WM_DESKTOP to: %d\n", workspace));

	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.display = GDK_WINDOW_XDISPLAY (gdkwin);
	xev.xclient.window = GDK_WINDOW_XWINDOW (gdkwin);
	xev.xclient.message_type = 
		gdk_x11_atom_to_xatom_for_display(
			gdk_drawable_get_display (gdkwin),
			wm_desktop);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = workspace;
	xev.xclient.data.l[1] = 0;
	xev.xclient.data.l[2] = 0;

	XSendEvent (GDK_WINDOW_XDISPLAY (rootwin),
		    GDK_WINDOW_XWINDOW (rootwin),
		    False,
		    SubstructureRedirectMask | SubstructureNotifyMask,
		    &xev);
}

static void
tomboy_window_override_user_time (GtkWindow *window)
{
	guint32 ev_time = gtk_get_current_event_time();

	if (ev_time == 0) {
		/* 
		 * FIXME: Global keypresses use an event filter on the root
		 * window, which processes events before GDK sees them.
		 */
		ev_time = tomboy_keybinder_get_current_event_time ();
	}
	if (ev_time == 0) {
		gint ev_mask = gtk_widget_get_events (GTK_WIDGET(window));
		if (!(ev_mask & GDK_PROPERTY_CHANGE_MASK)) {
			gtk_widget_add_events (GTK_WIDGET (window),
					       GDK_PROPERTY_CHANGE_MASK);
		}

		/* 
		 * NOTE: Last resort for D-BUS or other non-interactive
		 *       openings.  Causes roundtrip to server.  Lame. 
		 */
		ev_time = gdk_x11_get_server_time (GTK_WIDGET(window)->window);
	}

	TRACE (g_print("Setting _NET_WM_USER_TIME to: %d\n", ev_time));
	gdk_x11_window_set_user_time (GTK_WIDGET(window)->window, ev_time);
}

void
tomboy_window_present_hardcore (GtkWindow *window)
{
	if (!GTK_WIDGET_REALIZED (window))
		gtk_widget_realize (GTK_WIDGET (window));
	else if (GTK_WIDGET_VISIBLE (window))
		tomboy_window_move_to_current_workspace (window);

	tomboy_window_override_user_time (window);

	gtk_window_present (window);
}

#endif		/* X11_AVAILABLE */
