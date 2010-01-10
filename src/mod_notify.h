/* mod_notify.h
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
 * Dynamic linking of libnotify for passive desktop notification using 
 * GModule. Prototypes must match with notify.h & notification.h
 */


#ifndef __MOD_NOTIFY_H__
#define __MOD_NOTIFY_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _NotifyNotification        	NotifyNotification;
typedef struct _NotifyNotificationPrivate 	NotifyNotificationPrivate;

struct _NotifyNotification
{
	GObject parent_object;
	NotifyNotificationPrivate *priv;
};

gboolean	(*notify_init)		(const char *app_name);
void		(*notify_uninit)	(void);

NotifyNotification* (*notify_notification_new_with_status_icon) (
	const gchar *summary, const gchar *body,
	const gchar *icon, GtkStatusIcon *status_icon);

gboolean (*notify_notification_update)	(NotifyNotification *notification, 
						const gchar *summary, 
						const gchar *body, 
						const gchar *icon);

gboolean (*notify_notification_show)	(NotifyNotification *notification, 
						GError **error);

gboolean (*notify_notification_close)	(NotifyNotification *notification, 
						GError **error);


G_END_DECLS

#endif		/* __MOD_NOTIFY_H__ */

