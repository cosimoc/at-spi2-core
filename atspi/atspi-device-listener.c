/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2002 Ximian Inc.
 * Copyright 2002 Sun Microsystems, Inc.
 * Copyright 2010, 2011 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "atspi-private.h"

#include "xml/a11y-atspi-device-event-listener.h"
#include <stdio.h>

typedef struct
{
  AtspiDeviceListenerCB    callback;
  gpointer user_data;
  GDestroyNotify callback_destroyed;
} DeviceEventHandler;

typedef struct {
  A11yAtspiDeviceEventListener *listener_skeleton;
} AtspiDeviceListenerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (AtspiDeviceListener, atspi_device_listener,
                            G_TYPE_OBJECT)

/*
 * Misc. helpers.
 */

static DeviceEventHandler *
device_event_handler_new (AtspiDeviceListenerCB callback,
                          GDestroyNotify callback_destroyed,
                          gpointer user_data)
{
  DeviceEventHandler *eh = g_new0 (DeviceEventHandler, 1);

  eh->callback = callback;
  eh->callback_destroyed = callback_destroyed;
  eh->user_data = user_data;

  return eh;
}

static gboolean
device_remove_datum (const AtspiDeviceEvent *event, void *user_data)
{
  AtspiDeviceListenerSimpleCB cb = user_data;
  return cb (event);
}
  
static void
device_event_handler_free (DeviceEventHandler *eh)
{
#if 0
  /* TODO; Test this; it will probably crash with pyatspi for unknown reasons */
  if (eh->callback_destroyed)
  {
    gpointer rea_callback = (eh->callback == device_remove_datum ?
                            eh->user_data : eh->callback);
    (*eh->callback_destroyed) (real_callback);
  }
#endif
  g_free (eh);
}

static GList *
event_list_remove_by_cb (GList *list, AtspiDeviceListenerCB callback)
{
  GList *l, *next;
	
  for (l = list; l; l = next)
    {
      DeviceEventHandler *eh = l->data;
      next = l->next;

      if (eh->callback == callback)
      {
        list = g_list_delete_link (list, l);
        device_event_handler_free (eh);
      }
    }

  return list;
}

/*
 * Standard event dispatcher
 */

static guint listener_id = 0;
static GList *device_listeners = NULL;

static gboolean
id_is_free (guint id)
{
  GList *l;

  for (l = device_listeners; l; l = g_list_next (l))
  {
    AtspiDeviceListener *listener = l->data;
    if (listener->id == id) return FALSE;
  }
  return TRUE;
}

static AtspiDeviceEvent *
atspi_device_event_copy (const AtspiDeviceEvent *src)
{
  AtspiDeviceEvent *dst = g_new0 (AtspiDeviceEvent, 1);
  dst->type = src->type;
  dst->id = src->id;
  dst->hw_code = src->hw_code;
  dst->modifiers = src->modifiers;
  dst->timestamp = src->timestamp;
  if (src->event_string)
    dst->event_string = g_strdup (src->event_string);
  dst->is_text = src->is_text;
  return dst;
}

void
atspi_device_event_free (AtspiDeviceEvent *event)
{
  if (event->event_string)
    g_free (event->event_string);
  g_free (event);
}

/* 
 * Device event handler
 */
static gboolean
atspi_device_event_dispatch (AtspiDeviceListener               *listener,
		   const AtspiDeviceEvent *event)
{
  GList *l;
  gboolean handled = FALSE;

  /* FIXME: re-enterancy hazard on this list */
  for (l = listener->callbacks; l; l = l->next)
    {
      DeviceEventHandler *eh = l->data;

      if ((handled = eh->callback (atspi_device_event_copy (event), eh->user_data)))
        {
	  break;
	}
    }

  return handled;
}

static gboolean
handle_notify_event (A11yAtspiDeviceEventListener *skeleton,
                     GDBusMethodInvocation *invocation,
                     GVariant *variant,
                     gpointer user_data)
{
  AtspiDeviceListener *listener = user_data;
  AtspiDeviceListenerClass *klass;
  AtspiDeviceEvent event;
  gboolean retval = FALSE;

  g_variant_get (variant, "(uiuuisb)",
    &event.type,
    &event.id,
    &event.hw_code,
    &event.modifiers,
    &event.timestamp,
    &event.event_string,
    &event.is_text);

  klass = ATSPI_DEVICE_LISTENER_GET_CLASS (listener);
  if (klass->device_event)
    retval = (*klass->device_event) (listener, &event);

  a11y_atspi_device_event_listener_complete_notify_event (skeleton, invocation, retval);
  return TRUE;
}

static void
atspi_device_listener_init (AtspiDeviceListener *listener)
{
  AtspiDeviceListenerPrivate *priv = atspi_device_listener_get_instance_private (listener);
  char *path;

  do
  {
    listener->id = listener_id++;
  } while (!id_is_free (listener->id));
  device_listeners = g_list_append (device_listeners, listener);

  path = _atspi_device_listener_get_path (listener);
  priv->listener_skeleton = a11y_atspi_device_event_listener_skeleton_new ();
  g_signal_connect (priv->listener_skeleton, "handle-notify-event",
                    G_CALLBACK (handle_notify_event), listener);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->listener_skeleton),
                                    _atspi_bus (), path, NULL);
  g_free (path);
}

static void
atspi_device_listener_finalize (GObject *object)
{
  AtspiDeviceListener *listener = (AtspiDeviceListener *) object;
  GList *l;
  
  for (l = listener->callbacks; l; l = l->next)
    {
      device_event_handler_free (l->data);
    }
  
  g_list_free (listener->callbacks);

  G_OBJECT_CLASS (atspi_device_listener_parent_class)->finalize (object);
}

static void
atspi_device_listener_class_init (AtspiDeviceListenerClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->finalize = atspi_device_listener_finalize;

  klass->device_event = atspi_device_event_dispatch;
}

/**
 * atspi_device_listener_new:
 * @callback: (scope notified): an #AtspiDeviceListenerCB callback function,
 *            or NULL.
 * @user_data: (closure): a pointer to data which will be passed to the
 * callback when invoked.
 * @callback_destroyed: A #GDestroyNotify called when the listener is freed
 * and data associated with the callback should be freed. It can be NULL.
 *
 * Creates a new #AtspiDeviceListener with a specified callback function.
 *
 * Returns: (transfer full): a pointer to a newly-created #AtspiDeviceListener.
 *
 **/
AtspiDeviceListener *
atspi_device_listener_new (AtspiDeviceListenerCB callback,
                           void *user_data,
                           GDestroyNotify callback_destroyed)
{
  AtspiDeviceListener *listener = g_object_new (atspi_device_listener_get_type (), NULL);

  if (callback)
    atspi_device_listener_add_callback (listener, callback, callback_destroyed,
                                       user_data);
  return listener;
}

/**
 * atspi_device_listener_new_simple: (skip)
 * @callback: (scope notified): an #AtspiDeviceListenerCB callback function,
 *            or NULL.
 * @callback_destroyed: A #GDestroyNotify called when the listener is freed
 * and data associated with the callback should be freed.  It an be NULL.
 *
 * Creates a new #AtspiDeviceListener with a specified callback function.
 * This method is similar to #atspi_device_listener_new, but callback
 * takes no user data.
 *
 * Returns: a pointer to a newly-created #AtspiDeviceListener.
 *
 **/
AtspiDeviceListener *
atspi_device_listener_new_simple (AtspiDeviceListenerSimpleCB callback,
                           GDestroyNotify callback_destroyed)
{
  return atspi_device_listener_new (device_remove_datum, callback, callback_destroyed);
}

/**
 * atspi_device_listener_add_callback:
 * @listener: the #AtspiDeviceListener instance to modify.
 * @callback: (scope notified): an #AtspiDeviceListenerCB function pointer.
 * @callback_destroyed: A #GDestroyNotify called when the listener is freed
 * and data associated with the callback should be freed. It can be NULL.
 * @user_data: (closure): a pointer to data which will be passed to the
 *             callback when invoked. 
 *
 * Adds an in-process callback function to an existing #AtspiDeviceListener.
 *
 **/
void
atspi_device_listener_add_callback (AtspiDeviceListener  *listener,
			     AtspiDeviceListenerCB callback,
			     GDestroyNotify callback_destroyed,
			     void                      *user_data)
{
  g_return_if_fail (ATSPI_IS_DEVICE_LISTENER (listener));
  DeviceEventHandler *new_handler;

  new_handler = device_event_handler_new (callback,
                                          callback_destroyed, user_data);

  listener->callbacks = g_list_prepend (listener->callbacks, new_handler);
}

/**
 * atspi_device_listener_remove_callback:
 * @listener: the #AtspiDeviceListener instance to modify.
 * @callback: (scope call): an #AtspiDeviceListenerCB function pointer.
 *
 * Removes an in-process callback function from an existing 
 * #AtspiDeviceListener.
 *
 **/
void
atspi_device_listener_remove_callback (AtspiDeviceListener  *listener,
				AtspiDeviceListenerCB callback)
{
  g_return_if_fail (ATSPI_IS_DEVICE_LISTENER (listener));

  listener->callbacks = event_list_remove_by_cb (listener->callbacks, (void *) callback);
}

gchar *
_atspi_device_listener_get_path (AtspiDeviceListener *listener)
{  return g_strdup_printf ("/org/a11y/atspi/listeners/%d", listener->id);
}

G_DEFINE_BOXED_TYPE (AtspiDeviceEvent,
                     atspi_device_event,
                     atspi_device_event_copy,
                     atspi_device_event_free)
