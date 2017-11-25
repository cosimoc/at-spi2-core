
/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2001, 2002 Sun Microsystems Inc.,
 * Copyright 2001, 2002 Ximian, Inc.
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

/* atspi_registry.c: Global functions wrapping the registry */

#include "atspi-private.h"

#include "xml/a11y-atspi-device-event-controller.h"

typedef struct
{
  AtspiDeviceListener *listener;
  GArray             *key_set;
  AtspiKeyMaskType         modmask;
  AtspiKeyEventMask        event_types;
  gint sync_type;
} DeviceListenerEntry;

static GList *device_listeners;
static A11yAtspiDeviceEventController *dec_proxy = NULL;

static A11yAtspiDeviceEventController *
get_dec_proxy (void)
{
  if (!dec_proxy)
    dec_proxy = a11y_atspi_device_event_controller_proxy_new_sync (_atspi_bus(),
                                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                                   atspi_bus_registry,
                                                                   atspi_path_dec,
                                                                   NULL, NULL);
  return dec_proxy;
}

/**
 * atspi_get_desktop_count:
 *
 * Gets the number of virtual desktops.
 * NOTE: multiple virtual desktops are not implemented yet; as a 
 * consequence, this function always returns 1.
 *
 * Returns: a #gint indicating the number of active virtual desktops.
 **/
gint
atspi_get_desktop_count ()
{
  return 1;
}

/**
 * atspi_get_desktop:
 * @i: a #gint indicating which of the accessible desktops is to be returned.
 *
 * Gets the virtual desktop indicated by index @i.
 * NOTE: currently multiple virtual desktops are not implemented;
 * as a consequence, any @i value different from 0 will not return a
 * virtual desktop - instead it will return NULL.
 *
 * Returns: (transfer full): a pointer to the @i-th virtual desktop's
 * #AtspiAccessible representation.
 **/
AtspiAccessible*
atspi_get_desktop (gint i)
{
  if (i != 0) return NULL;
  return _atspi_ref_accessible (atspi_bus_registry, atspi_path_root);
}

/**
 * atspi_get_desktop_list:
 *
 * Gets the list of virtual desktops.  On return, @list will point
 *     to a newly-created, NULL terminated array of virtual desktop
 *     pointers.
 *     It is the responsibility of the caller to free this array when
 *     it is no longer needed.
 * NOTE: currently multiple virtual desktops are not implemented;
 * this implementation always returns a #Garray with a single
 * #AtspiAccessible desktop.
 *
 * Returns: (element-type AtspiAccessible*) (transfer full): a #GArray of
 * desktops.
 **/
GArray *
atspi_get_desktop_list ()
{
  GArray *array = g_array_new (TRUE, TRUE, sizeof (AtspiAccessible *));
  AtspiAccessible *desktop;

  desktop = _atspi_ref_accessible (atspi_bus_registry, atspi_path_root);
  if (array)
    g_array_index (array, AtspiAccessible *, 0) = desktop;
  return array;
}

static GVariant *
key_set_to_variant (GArray *key_set)
{
  gint i;
  GVariantBuilder builder;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(iisi)"));
  for (i = 0; i < key_set->len; ++i)
    {
      AtspiKeyDefinition *kd =  ((AtspiKeyDefinition *) key_set->data) + i;

      g_variant_builder_open (&builder, G_VARIANT_TYPE ("(iisi)"));
      g_variant_builder_add (&builder, "(iisi)",
                             kd->keycode,
                             kd->keysym,
                             kd->keystring ? kd->keystring : "",
                             0);
      g_variant_builder_close (&builder);
    }

  return g_variant_builder_end (&builder);
}

static gboolean
notify_keystroke_listener (DeviceListenerEntry *e)
{
  gchar *path = _atspi_device_listener_get_path (e->listener);
  gboolean retval = FALSE;
  GError *error = NULL;
  GVariant *key_set_variant, *mode_variant;

  key_set_variant = key_set_to_variant (e->key_set);
  mode_variant = g_variant_new ("(bbb)",
                                ((e->sync_type & ATSPI_KEYLISTENER_SYNCHRONOUS)!=0),
                                ((e->sync_type & ATSPI_KEYLISTENER_CANCONSUME)!=0),
                                ((e->sync_type & ATSPI_KEYLISTENER_ALL_WINDOWS)!=0));

  a11y_atspi_device_event_controller_call_register_keystroke_listener_sync
    (get_dec_proxy (),
     path, key_set_variant, e->modmask, e->event_types, mode_variant,
     &retval, NULL, &error);

  if (error)
    {
      g_warning ("RegisterKeystrokeListener failed: %s", error->message);
      g_error_free (error);
    }

  g_free (path);
  return retval;
}

static void
device_listener_entry_free (DeviceListenerEntry *e)
{
  g_array_free (e->key_set, TRUE);
  g_free (e);
}

static void
unregister_listener (gpointer data, GObject *object)
{
  GList *l;
  AtspiDeviceListener *listener = ATSPI_DEVICE_LISTENER (object);

  for (l = device_listeners; l;)
    {
      DeviceListenerEntry *e = l->data;
      if (e->listener == listener)
        {
          GList *next = l->next;
          device_listener_entry_free (e);
          device_listeners = g_list_delete_link (device_listeners, l);
          l = next;
        }
      else
        l = l->next;
    }
}

/**
 * atspi_register_keystroke_listener:
 * @listener:  a pointer to the #AtspiDeviceListener for which
 *             keystroke events are requested.
 * @key_set: (element-type AtspiKeyDefinition) (allow-none): a pointer to the
 *        #AtspiKeyDefinition array indicating which keystroke events are
 *        requested, or NULL
 *        to indicate that all keycodes and keyvals for the specified
 *        modifier set are to be included.
 * @modmask:   an #AtspiKeyMaskType mask indicating which
 *             key event modifiers must be set in combination with @keys,
 *             events will only be reported for key events for which all
 *             modifiers in @modmask are set.  If you wish to listen for
 *             events with multiple modifier combinations, you must call
 *             #atspi_register_keystroke_listener once for each
 *             combination.
 * @event_types: an #AtspiKeyMaskType mask indicating which
 *             types of key events are requested (%ATSPI_KEY_PRESSED etc.).
 * @sync_type: an #AtspiKeyListenerSyncType parameter indicating
 *             the behavior of the notification/listener transaction.
 * @error: (allow-none): a pointer to a %NULL #GError pointer, or %NULL
 *             
 * Registers a listener for keystroke events, either pre-emptively for
 *             all windows (%ATSPI_KEYLISTENER_ALL_WINDOWS),
 *             non-preemptively (%ATSPI_KEYLISTENER_NOSYNC), or
 *             pre-emptively at the toolkit level (%ATSPI_KEYLISTENER_CANCONSUME).
 *             If ALL_WINDOWS or CANCONSUME are used, the event is consumed
 *             upon receipt if one of @listener's callbacks returns %TRUE 
 *             (other sync_type values may be available in the future).
 *
 * Returns: %TRUE if successful, otherwise %FALSE.
 **/
gboolean
atspi_register_keystroke_listener (AtspiDeviceListener  *listener,
					 GArray             *key_set,
					 AtspiKeyMaskType         modmask,
					 AtspiKeyEventMask        event_types,
					 AtspiKeyListenerSyncType sync_type,
                                         GError **error)
{
  DeviceListenerEntry *e;

  g_return_val_if_fail (listener != NULL, FALSE);

  e = g_new0 (DeviceListenerEntry, 1);
  e->listener = listener;
  e->modmask = modmask;
  e->event_types = event_types;
  e->sync_type = sync_type;
  if (key_set)
    {
      gint i;
      e->key_set = g_array_sized_new (FALSE, TRUE, sizeof (AtspiKeyDefinition),
                                      key_set->len);
      e->key_set->len = key_set->len;
      for (i = 0; i < key_set->len; i++)
        {
	  AtspiKeyDefinition *kd =  ((AtspiKeyDefinition *) key_set->data) + i;
	  AtspiKeyDefinition *d_kd =  ((AtspiKeyDefinition *) e->key_set->data) + i;
          d_kd->keycode = kd->keycode;
	  d_kd->keysym = kd->keysym;
	  if (kd->keystring)
	    {
	      d_kd->keystring = kd->keystring;
	    } 
          else 
            {
	      d_kd->keystring = "";
	    }
        }
    }
  else
    {
      e->key_set = g_array_sized_new (FALSE, TRUE, sizeof (AtspiKeyDefinition), 0);
    }

  g_object_weak_ref (G_OBJECT (listener), unregister_listener, NULL);
  device_listeners = g_list_prepend (device_listeners, e);
  return notify_keystroke_listener (e);
}

/**
 * atspi_deregister_keystroke_listener:
 * @listener: a pointer to the #AtspiDeviceListener for which
 *            keystroke events are requested.
 * @key_set: (element-type AtspiKeyDefinition) (allow-none): a pointer to the
 *        #AtspiKeyDefinition array indicating which keystroke events are
 *        requested, or %NULL
 *        to indicate that all keycodes and keyvals for the specified
 *        modifier set are to be included.
 * @modmask:  the key modifier mask for which this listener is to be
 *            'deregistered' (of type #AtspiKeyMaskType).
 * @event_types: an #AtspiKeyMaskType mask indicating which
 *             types of key events were requested (%ATSPI_KEY_PRESSED, etc.).
 * @error: (allow-none): a pointer to a %NULL #GError pointer, or %NULL
 *
 * Removes a keystroke event listener from the registry's listener queue,
 *            ceasing notification of events with modifiers matching @modmask.
 *
 * Returns: %TRUE if successful, otherwise %FALSE.
 **/
gboolean
atspi_deregister_keystroke_listener (AtspiDeviceListener *listener,
                                     GArray              *key_set,
                                     AtspiKeyMaskType     modmask,
                                     AtspiKeyEventMask    event_types,
                                     GError             **error)
{
  GVariant *variant;
  gchar *path;
  GList *l;

  g_return_val_if_fail (listener != NULL, FALSE);

  path = _atspi_device_listener_get_path (listener);
  variant = key_set_to_variant (key_set);

  a11y_atspi_device_event_controller_call_deregister_keystroke_listener_sync
    (get_dec_proxy (), path, variant, modmask, event_types,
     NULL, error);

  unregister_listener (listener, NULL);
  for (l = device_listeners; l;)
    {
      /* TODO: This code is all wrong / doesn't match what is in
 *       deviceeventcontroller.c. It would be nice to deprecate these methods
 *       in favor of methods that return an ID for the registration that can
 *       be passed to a deregister function, for instance. */
      DeviceListenerEntry *e = l->data;
      if (e->modmask == modmask && e->event_types == event_types)
        {
          GList *next = l->next;
          device_listener_entry_free (e);
          device_listeners = g_list_delete_link (device_listeners, l);
          l = next;
        }
      else
        l = l->next;
    }

  g_free (path);
  return TRUE;
}

/**
 * atspi_register_device_event_listener:
 * @listener:  a pointer to the #AtspiDeviceListener which requests
 *             the events.
 * @event_types: an #AtspiDeviceEventMask mask indicating which
 *             types of key events are requested (%ATSPI_KEY_PRESSED, etc.).
 * @filter: (allow-none): Unused parameter.
 * @error: (allow-none): a pointer to a %NULL #GError pointer, or %NULL
 *             
 * Registers a listener for device events, for instance button events.
 *
 * Returns: %TRUE if successful, otherwise %FALSE.
 **/
gboolean
atspi_register_device_event_listener (AtspiDeviceListener  *listener,
				 AtspiDeviceEventMask  event_types,
				 void                      *filter, GError **error)
{
  A11yAtspiDeviceEventController *dec_proxy = get_dec_proxy ();
  gboolean retval = FALSE;
  gchar *path;

  g_return_val_if_fail (listener != NULL, FALSE);

  path = _atspi_device_listener_get_path (listener);
  retval = a11y_atspi_device_event_controller_call_register_device_event_listener_sync
    (dec_proxy, path, event_types, &retval, NULL, error);
  g_free (path);

  return retval;
}

/**
 * atspi_deregister_device_event_listener:
 * @listener: a pointer to the #AtspiDeviceListener for which
 *            device events are requested.
 * @filter: (allow-none): Unused parameter.
 * @error: (allow-none): a pointer to a %NULL #GError pointer, or %NULL
 *
 * Removes a device event listener from the registry's listener queue,
 *            ceasing notification of events of the specified type.
 *
 * Returns: %TRUE if successful, otherwise %FALSE.
 **/
gboolean
atspi_deregister_device_event_listener (AtspiDeviceListener *listener,
				   void                     *filter, GError **error)
{
  A11yAtspiDeviceEventController *dec_proxy = get_dec_proxy ();
  guint32 event_types = 0;
  gchar *path;
  gboolean retval = FALSE;

  g_return_val_if_fail (listener != NULL, FALSE);

  path = _atspi_device_listener_get_path (listener);
  event_types |= (1 << ATSPI_BUTTON_PRESSED_EVENT);
  event_types |= (1 << ATSPI_BUTTON_RELEASED_EVENT);

  retval = a11y_atspi_device_event_controller_call_deregister_device_event_listener_sync
    (dec_proxy, path, event_types, NULL, error);
  g_free (path);

  return retval;
}

/**
 * atspi_generate_keyboard_event:
 * @keyval: a #gint indicating the keycode or keysym of the key event
 *           being synthesized.
 * @keystring: (allow-none): an (optional) UTF-8 string which, if
 *           @synth_type is %ATSPI_KEY_STRING, indicates a 'composed'
 *           keyboard input string being synthesized; this type of
 *           keyboard event synthesis does not emulate hardware
 *           keypresses but injects the string as though a composing
 *           input method (such as XIM) were used.
 * @synth_type: an #AtspiKeySynthType flag indicating whether @keyval
 *           is to be interpreted as a keysym rather than a keycode
 *           (%ATSPI_KEY_SYM) or a string (%ATSPI_KEY_STRING), or
 *           whether to synthesize %ATSPI_KEY_PRESS,
 *           %ATSPI_KEY_RELEASE, or both (%ATSPI_KEY_PRESSRELEASE).
 * @error: (allow-none): a pointer to a %NULL #GError pointer, or %NULL
 *
 * Synthesizes a keyboard event (as if a hardware keyboard event occurred in the
 * current UI context).
 *
 * Returns: %TRUE if successful, otherwise %FALSE.
 **/
gboolean
atspi_generate_keyboard_event (glong keyval,
			   const gchar *keystring,
			   AtspiKeySynthType synth_type, GError **error)
{
  A11yAtspiDeviceEventController *dec_proxy = get_dec_proxy ();
  return a11y_atspi_device_event_controller_call_generate_keyboard_event_sync
    (dec_proxy, keyval, keystring, synth_type, NULL, error);
}

/**
 * atspi_generate_mouse_event:
 * @x: a #glong indicating the screen x coordinate of the mouse event.
 * @y: a #glong indicating the screen y coordinate of the mouse event.
 * @name: a string indicating which mouse event to be synthesized
 *        (e.g. "b1p", "b1c", "b2r", "rel", "abs").
 * @error: (allow-none): a pointer to a %NULL #GError pointer, or %NULL
 *
 * Synthesizes a mouse event at a specific screen coordinate.
 * Most AT clients should use the #AccessibleAction interface when
 * tempted to generate mouse events, rather than this method.
 * Event names: b1p = button 1 press; b2r = button 2 release;
 *              b3c = button 3 click; b2d = button 2 double-click;
 *              abs = absolute motion; rel = relative motion.
 *
 * Returns: %TRUE if successful, otherwise %FALSE.
 **/
gboolean
atspi_generate_mouse_event (glong x, glong y, const gchar *name, GError **error)
{
  A11yAtspiDeviceEventController *dec_proxy = get_dec_proxy ();
  return a11y_atspi_device_event_controller_call_generate_mouse_event_sync
    (dec_proxy, x, y, name, NULL, error);
}

AtspiKeyDefinition *
atspi_key_definition_copy (AtspiKeyDefinition *src)
{
  AtspiKeyDefinition *dst;

  dst = g_new0 (AtspiKeyDefinition, 1);
  dst->keycode = src->keycode;
  dst->keysym = src->keysym;
  if (src->keystring)
    dst->keystring = g_strdup (src->keystring);
  dst->unused = src->unused;
  return dst;
}

void
atspi_key_definition_free (AtspiKeyDefinition *kd)
{
  if (kd->keystring)
    g_free (kd->keystring);
  g_free (kd);
}

void
_atspi_reregister_device_listeners ()
{
  GList *l;
  DeviceListenerEntry *e;

  for (l = device_listeners; l; l = l->next)
    {
      e = l->data;
      notify_keystroke_listener (e);
    }
}
G_DEFINE_BOXED_TYPE (AtspiKeyDefinition, atspi_key_definition, atspi_key_definition_copy, atspi_key_definition_free)
