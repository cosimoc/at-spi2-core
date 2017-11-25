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
#include "atspi-accessible-private.h"

#include "xml/a11y-atspi-registry.h"

#include <string.h>
#include <ctype.h>

typedef struct
{
  AtspiEventListenerCB callback;
  void *user_data;
  GDestroyNotify callback_destroyed;
  char *event_type;
  char *category;
  char *name;
  char *detail;
  GArray *properties;

  guint signal_subscription;
  guint arg0path_signal_subscription;
} EventListenerEntry;

G_DEFINE_TYPE (AtspiEventListener, atspi_event_listener, G_TYPE_OBJECT)

void
atspi_event_listener_init (AtspiEventListener *listener)
{
}

void
atspi_event_listener_class_init (AtspiEventListenerClass *klass)
{
}

static void
remove_datum (AtspiEvent *event, void *user_data)
{
  AtspiEventListenerSimpleCB cb = user_data;
  cb (event);
}

typedef struct
{
  gpointer callback;
  GDestroyNotify callback_destroyed;
  gint ref_count;
} CallbackInfo;
static GHashTable *callbacks;

void
callback_ref (void *callback, GDestroyNotify callback_destroyed)
{
  CallbackInfo *info;

  if (!callbacks)
  {
    callbacks = g_hash_table_new (g_direct_hash, g_direct_equal);
    if (!callbacks)
      return;
  }

  info = g_hash_table_lookup (callbacks, callback);
  if (!info)
  {
    info = g_new (CallbackInfo, 1);
    info->callback = callback;
    info->callback_destroyed = callback_destroyed;
    info->ref_count = 1;
    g_hash_table_insert (callbacks, callback, info);
  }
  else
    info->ref_count++;
}

void
callback_unref (gpointer callback)
{
  CallbackInfo *info;

  if (!callbacks)
    return;
  info = g_hash_table_lookup (callbacks, callback);
  if (!info)
  {
    g_warning ("Atspi: Dereferencing invalid callback %p\n", callback);
    return;
  }
  info->ref_count--;
  if (info->ref_count == 0)
  {
#if 0
    /* TODO: Figure out why this seg faults from Python */
    if (info->callback_destroyed)
      (*info->callback_destroyed) (info->callback);
#endif
    g_free (info);
    g_hash_table_remove (callbacks, callback);
  }
}

/**
 * atspi_event_listener_new:
 * @callback: (scope notified): An #AtspiEventListenerCB to be called
 * when an event is fired.
 * @user_data: (closure): data to pass to the callback.
 * @callback_destroyed: A #GDestroyNotify called when the listener is freed
 * and data associated with the callback should be freed.  Can be NULL.
 *
 * Creates a new #AtspiEventListener associated with a specified @callback.
 *
 * Returns: (transfer full): A new #AtspiEventListener.
 */
AtspiEventListener *
atspi_event_listener_new (AtspiEventListenerCB callback,
                                 gpointer user_data,
                                 GDestroyNotify callback_destroyed)
{
  AtspiEventListener *listener = g_object_new (ATSPI_TYPE_EVENT_LISTENER, NULL);
  listener->callback = callback;
  callback_ref (callback, callback_destroyed);
  listener->user_data = user_data;
  listener->cb_destroyed = callback_destroyed;
  return listener;
}

/**
 * atspi_event_listener_new_simple: (skip)
 * @callback: (scope notified): An #AtspiEventListenerSimpleCB to be called
 * when an event is fired.
 * @callback_destroyed: A #GDestroyNotify called when the listener is freed
 * and data associated with the callback should be freed.  Can be NULL.
 *
 * Creates a new #AtspiEventListener associated with a specified @callback.
 * Returns: (transfer full): A new #AtspiEventListener.
 **/
AtspiEventListener *
atspi_event_listener_new_simple (AtspiEventListenerSimpleCB callback,
                                 GDestroyNotify callback_destroyed)
{
  AtspiEventListener *listener = g_object_new (ATSPI_TYPE_EVENT_LISTENER, NULL);
  listener->callback = remove_datum;
  callback_ref (remove_datum, callback_destroyed);
  listener->user_data = callback;
  listener->cb_destroyed = callback_destroyed;
  return listener;
}

static GList *event_listeners = NULL;

static gchar *
convert_name_from_dbus (const char *name, gboolean path_hack)
{
  gchar *ret = g_malloc (g_utf8_strlen (name, -1) * 2 + 1);
  const char *p = name;
  gchar *q = ret;

  if (!ret)
    return NULL;

  while (*p)
  {
    if (isupper (*p))
    {
      if (q > ret)
        *q++ = '-';
      *q++ = tolower (*p++);
    }
    else if (path_hack && *p == '/')
    {
      *q++ = ':';
      p++;
    }
    else
      *q++ = *p++;
  }
  *q = '\0';
  return ret;
}

static void
cache_process_children_changed (AtspiEvent *event)
{
  AtspiAccessible *child;

  if (!G_VALUE_HOLDS (&event->any_data, ATSPI_TYPE_ACCESSIBLE) ||
      !(event->source->cached_properties & ATSPI_CACHE_CHILDREN) ||
      atspi_state_set_contains (event->source->states, ATSPI_STATE_MANAGES_DESCENDANTS))
    return;

  child = g_value_get_object (&event->any_data);
  if (child == NULL)
    return;

  if (!strncmp (event->type, "object:children-changed:add", 27))
  {
    g_ptr_array_remove (event->source->children, child); /* just to be safe */
    if (event->detail1 < 0 || event->detail1 > event->source->children->len)
    {
      event->source->cached_properties &= ~ATSPI_CACHE_CHILDREN;
      return;
    }
    /* Unfortunately, there's no g_ptr_array_insert or similar */
    g_ptr_array_add (event->source->children, NULL);
    memmove (event->source->children->pdata + event->detail1 + 1,
             event->source->children->pdata + event->detail1,
             (event->source->children->len - event->detail1 - 1) * sizeof (gpointer));
    g_ptr_array_index (event->source->children, event->detail1) = g_object_ref (child);
  }
  else
  {
    g_ptr_array_remove (event->source->children, child);
    if (child == child->parent.app->root)
      g_object_run_dispose (G_OBJECT (child->parent.app));
  }
}

static void
cache_process_property_change (AtspiEvent *event)
{
  if (!strcmp (event->type, "object:property-change:accessible-parent"))
  {
    if (event->source->accessible_parent)
      g_object_unref (event->source->accessible_parent);
    if (G_VALUE_HOLDS (&event->any_data, ATSPI_TYPE_ACCESSIBLE))
    {
      event->source->accessible_parent = g_value_dup_object (&event->any_data);
      _atspi_accessible_add_cache (event->source, ATSPI_CACHE_PARENT);
    }
    else
    {
      event->source->accessible_parent = NULL;
      event->source->cached_properties &= ~ATSPI_CACHE_PARENT;
    }
  }
  else if (!strcmp (event->type, "object:property-change:accessible-name"))
  {
    if (event->source->name)
      g_free (event->source->name);
    if (G_VALUE_HOLDS_STRING (&event->any_data))
    {
      event->source->name = g_value_dup_string (&event->any_data);
      _atspi_accessible_add_cache (event->source, ATSPI_CACHE_NAME);
    }
    else
    {
      event->source->name = NULL;
      event->source->cached_properties &= ~ATSPI_CACHE_NAME;
    }
  }
  else if (!strcmp (event->type, "object:property-change:accessible-description"))
  {
    if (event->source->description)
      g_free (event->source->description);
    if (G_VALUE_HOLDS_STRING (&event->any_data))
    {
      event->source->description = g_value_dup_string (&event->any_data);
      _atspi_accessible_add_cache (event->source, ATSPI_CACHE_DESCRIPTION);
    }
    else
    {
      event->source->description = NULL;
      event->source->cached_properties &= ~ATSPI_CACHE_DESCRIPTION;
    }
  }
  else if (!strcmp (event->type, "object:property-change:accessible-role"))
  {
    if (G_VALUE_HOLDS_INT (&event->any_data))
    {
      event->source->role = g_value_get_int (&event->any_data);
      _atspi_accessible_add_cache (event->source, ATSPI_CACHE_ROLE);
    }
    else
    {
      event->source->cached_properties &= ~ATSPI_CACHE_ROLE;
    }
  }
}

static void
cache_process_state_changed (AtspiEvent *event)
{
  if (event->source->states)
    atspi_state_set_set_by_name (event->source->states, event->type + 21,
                                 event->detail1);
}

static gchar *
strdup_and_adjust_for_dbus (const char *s)
{
  gchar *d = g_strdup (s);
  gchar *p;
  int parts = 0;

  if (!d)
    return NULL;

  for (p = d; *p; p++)
  {
    if (*p == '-')
    {
      memmove (p, p + 1, g_utf8_strlen (p, -1));
      *p = toupper (*p);
    }
    else if (*p == ':')
    {
      parts++;
      if (parts == 2)
        break;
      p [1] = toupper (p [1]);
    }
  }

  d [0] = toupper (d [0]);
  return d;
}

static gboolean
convert_event_type_to_dbus (const char *eventType, char **categoryp, char **namep, char **detailp)
{
  gchar *tmp = strdup_and_adjust_for_dbus (eventType);
  char *category = NULL, *name = NULL, *detail = NULL;
  char *saveptr = NULL;

  if (tmp == NULL) return FALSE;
  category = strtok_r (tmp, ":", &saveptr);
  if (category) category = g_strdup (category);
  name = strtok_r (NULL, ":", &saveptr);
  if (name)
  {
    name = g_strdup (name);
    detail = strtok_r (NULL, ":", &saveptr);
    if (detail) detail = g_strdup (detail);
  }
  if (categoryp) *categoryp = category;
  else g_free (category);
  if (namep) *namep = name;
  else if (name) g_free (name);
  if (detailp) *detailp = detail;
  else if (detail) g_free (detail);
  g_free (tmp);
  return TRUE;
}

static void
listener_entry_free (EventListenerEntry *e)
{
  gpointer callback = (e->callback == remove_datum ? (gpointer)e->user_data : (gpointer)e->callback);
  g_free (e->event_type);
  g_free (e->category);
  g_free (e->name);
  g_free (e->detail);
  callback_unref (callback);
  g_free (e);
}

/**
 * atspi_event_listener_register:
 * @listener: The #AtspiEventListener to register against an event type.
 * @event_type: a character string indicating the type of events for which
 *            notification is requested.  Format is
 *            EventClass:major_type:minor_type:detail
 *            where all subfields other than EventClass are optional.
 *            EventClasses include "object", "window", "mouse",
 *            and toolkit events (e.g. "Gtk", "AWT").
 *            Examples: "focus:", "Gtk:GtkWidget:button_press_event".
 *
 * Adds an in-process callback function to an existing #AtspiEventListener.
 *
 * Legal object event types:
 *
 *    (property change events)
 *
 *            object:property-change
 *            object:property-change:accessible-name
 *            object:property-change:accessible-description
 *            object:property-change:accessible-parent
 *            object:property-change:accessible-value
 *            object:property-change:accessible-role
 *            object:property-change:accessible-table-caption
 *            object:property-change:accessible-table-column-description
 *            object:property-change:accessible-table-column-header
 *            object:property-change:accessible-table-row-description
 *            object:property-change:accessible-table-row-header
 *            object:property-change:accessible-table-summary
 *
 *    (other object events)
 *
 *            object:state-changed 
 *            object:children-changed
 *            object:visible-data-changed
 *            object:selection-changed
 *            object:text-selection-changed
 *            object:text-changed
 *            object:text-caret-moved
 *            object:row-inserted
 *            object:row-reordered
 *            object:row-deleted
 *            object:column-inserted
 *            object:column-reordered
 *            object:column-deleted
 *            object:model-changed
 *            object:active-descendant-changed
 *
 *  (window events)
 *
 *            window:minimize
 *            window:maximize
 *            window:restore
 *            window:close
 *            window:create
 *            window:reparent
 *            window:desktop-create
 *            window:desktop-destroy
 *            window:activate
 *            window:deactivate
 *            window:raise
 *            window:lower
 *            window:move
 *            window:resize
 *            window:shade
 *            window:unshade
 *            window:restyle
 *
 *  (other events)
 *
 *            focus:
 *            mouse:abs
 *            mouse:rel
 *            mouse:b1p
 *            mouse:b1r
 *            mouse:b2p
 *            mouse:b2r
 *            mouse:b3p
 *            mouse:b3r
 *
 * NOTE: this character string may be UTF-8, but should not contain byte 
 * value 56
 *            (ascii ':'), except as a delimiter, since non-UTF-8 string
 *            delimiting functions are used internally.
 *            In general, listening to
 *            toolkit-specific events is not recommended.
 *
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_register (AtspiEventListener *listener,
				             const gchar              *event_type,
				             GError **error)
{
  /* TODO: Keep track of which events have been registered, so that we
 * deregister all of them when the event listener is destroyed */

  return atspi_event_listener_register_from_callback (listener->callback,
                                                      listener->user_data,
                                                      listener->cb_destroyed,
                                                      event_type, error);
}

/**
 * atspi_event_listener_register_full:
 * @listener: The #AtspiEventListener to register against an event type.
 * @event_type: a character string indicating the type of events for which
 *            notification is requested.  See #atspi_event_listener_register
 * for a description of the format and legal event types.
* @properties: (element-type gchar*) (transfer none) (allow-none): a list of
 *             properties that should be sent along with the event. The
 *             properties are valued for the duration of the event callback.k
 *             TODO: Document.
 *
 * Adds an in-process callback function to an existing #AtspiEventListener.
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_register_full (AtspiEventListener *listener,
				             const gchar              *event_type,
				             GArray *properties,
				             GError **error)
{
  /* TODO: Keep track of which events have been registered, so that we
 * deregister all of them when the event listener is destroyed */

  return atspi_event_listener_register_from_callback_full (listener->callback,
                                                           listener->user_data,
                                                           listener->cb_destroyed,
                                                           event_type,
                                                           properties,
                                                           error);
}

static void
notify_event_deregistered (EventListenerEntry *e)
{
  A11yAtspiRegistry *registry_proxy =
    a11y_atspi_registry_proxy_new_sync (_atspi_bus (), G_DBUS_PROXY_FLAGS_NONE,
                                        atspi_bus_registry,
                                        atspi_path_registry,
                                        NULL, NULL);
  if (registry_proxy)
    {
      a11y_atspi_registry_call_deregister_event_sync (registry_proxy,
                                                      e->event_type,
                                                      NULL, NULL);
      g_object_unref (registry_proxy);
    }
}

static void
notify_event_registered (EventListenerEntry *e)
{
  A11yAtspiRegistry *registry_proxy =
    a11y_atspi_registry_proxy_new_sync (_atspi_bus (), G_DBUS_PROXY_FLAGS_NONE,
                                        atspi_bus_registry,
                                        atspi_path_registry,
                                        NULL, NULL);
  if (registry_proxy)
    {
      GVariant *properties = g_variant_new_strv ((const gchar * const *) e->properties->data,
                                                 e->properties->len);
      /* Don't use the generated method here, so that we can use g_variant_new()
       * to generate a variant with the right signature.
       */
      g_dbus_proxy_call_sync (G_DBUS_PROXY (registry_proxy),
                              "RegisterEvent",
                              g_variant_new ("(s@as)", e->event_type,
                                             properties),
                              G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
      g_object_unref (registry_proxy);
    }
}

/**
 * atspi_event_listener_register_from_callback:
 * @callback: (scope notified): the #AtspiEventListenerCB to be registered 
 * against an event type.
 * @user_data: (closure): User data to be passed to the callback.
 * @callback_destroyed: A #GDestroyNotify called when the callback is destroyed.
 * @event_type: a character string indicating the type of events for which
 *            notification is requested.  See #atspi_event_listener_register
 * for a description of the format.
 * 
 * Registers an #AtspiEventListenerCB against an @event_type.
 *
 * Returns: #TRUE if successfull, otherwise #FALSE.
 *
 **/
gboolean
atspi_event_listener_register_from_callback (AtspiEventListenerCB callback,
				             void *user_data,
				             GDestroyNotify callback_destroyed,
				             const gchar              *event_type,
				             GError **error)
{
  return atspi_event_listener_register_from_callback_full (callback,
                                                           user_data,
                                                           callback_destroyed,
                                                           event_type, NULL,
                                                           error);
}

static GArray *
copy_event_properties (GArray *src)
{
  gint i;

  GArray *dst = g_array_new (FALSE, FALSE, sizeof (char *));

  if (!src)
    return dst;
  for (i = 0; i < src->len; i++)
    {
      gchar *dup = g_strdup (g_array_index (src, char *, i));
    g_array_append_val (dst, dup);
    }
  return dst;
}

#define OLD_EVENT_VARIANT_TYPE "(siiv(so))"
#define NEW_EVENT_VARIANT_TYPE "(siiva{sv})"

static void
handle_event (GDBusConnection *connection,
              const char *sender_name,
              const char *object_path,
              const char *interface_name,
              const char *signal_name,
              GVariant *parameters,
              gpointer user_data)
{
  GVariant *event_data, *properties = NULL;
  gchar *converted_type, *detail, *name;
  const char *category;
  AtspiEvent e;
  char *p;

  memset (&e, 0, sizeof (e));

  category = g_utf8_strrchr (interface_name, -1, '.');
  if (category == NULL)
    return;

  category++;

  if (g_variant_is_of_type (parameters, G_VARIANT_TYPE (OLD_EVENT_VARIANT_TYPE)))
    g_variant_get (parameters, OLD_EVENT_VARIANT_TYPE,
                   &detail, &e.detail1, &e.detail2, &event_data, NULL, NULL);
  else if (g_variant_is_of_type (parameters, G_VARIANT_TYPE (NEW_EVENT_VARIANT_TYPE)))
    g_variant_get (parameters, "(siiv@a{sv})",
                   &detail, &e.detail1, &e.detail2, &event_data, &properties);

  converted_type = convert_name_from_dbus (category, FALSE);
  name = convert_name_from_dbus (signal_name, FALSE);
  detail = convert_name_from_dbus (detail, TRUE);

  if (strcasecmp (category, name) != 0)
    {
      p = g_strconcat (converted_type, ":", name, NULL);
      g_free (converted_type);
      converted_type = p;
    }
  else if (detail [0] == '\0')
    {
      p = g_strconcat (converted_type, ":",  NULL);
      g_free (converted_type);
      converted_type = p;
    }

  if (detail[0] != '\0')
    {
      p = g_strconcat (converted_type, ":", detail, NULL);
      g_free (converted_type);
      converted_type = p;
    }

  e.type = converted_type;
  e.source = _atspi_ref_accessible (sender_name, object_path);
  if (e.source == NULL)
    {
      g_warning ("Got no valid source accessible for signal for signal %s from interface %s\n", signal_name, interface_name);
      g_free (converted_type);
      g_free (name);
      g_free (detail);
      return;
    }

  if (g_variant_is_of_type (event_data, G_VARIANT_TYPE ("(iiii)")))
    {
      AtspiRect rect;
      g_variant_get (event_data, "(iiii)",
                     &rect.x, &rect.y, &rect.width, &rect.height);
      g_value_init (&e.any_data, ATSPI_TYPE_RECT);
      g_value_set_boxed (&e.any_data, &rect);
    }
  else if (g_variant_is_of_type (event_data, G_VARIANT_TYPE ("(so)")))
      {
        AtspiAccessible *accessible = _atspi_dbus_return_accessible_from_variant (event_data);
	g_value_init (&e.any_data, ATSPI_TYPE_ACCESSIBLE);
	g_value_set_instance (&e.any_data, accessible);
        g_clear_object (&accessible); /* value now owns it */
      }
  else if (g_variant_is_of_type (event_data, G_VARIANT_TYPE_STRING))
    {
      g_value_init (&e.any_data, G_TYPE_STRING);
      g_value_set_string (&e.any_data, g_variant_get_string (event_data, NULL));
    }

  if (properties)
    _atspi_dbus_update_cache_from_variant (e.source, properties);

  if (!strncmp (e.type, "object:children-changed", 23))
    cache_process_children_changed (&e);
  else if (!strncmp (e.type, "object:property-change", 22))
    cache_process_property_change (&e);
  else if (!strncmp (e.type, "object:state-changed", 20))
    cache_process_state_changed (&e);
  else if (!strncmp (e.type, "focus", 5))
    /* BGO#663992 - TODO: figure out the real problem */
    e.source->cached_properties &= ~(ATSPI_CACHE_STATES);

  _atspi_send_event (&e);

  g_free (converted_type);
  g_free (name);
  g_free (detail);
  g_object_unref (e.source);
  g_value_unset (&e.any_data);
  g_variant_unref (event_data);
  if (properties)
    g_variant_unref (properties);
}

/**
 * atspi_event_listener_register_from_callback_full:
 * @callback: (scope async): an #AtspiEventListenerCB function pointer.
 * @user_data: (closure callback)
 * @callback_destroyed: (destroy callback)
 * @event_type:
 * @properties: (element-type utf8)
 * @error:
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 *
 **/
gboolean
atspi_event_listener_register_from_callback_full (AtspiEventListenerCB callback,
				                  void *user_data,
				                  GDestroyNotify callback_destroyed,
				                  const gchar              *event_type,
				                  GArray *properties,
				                  GError **error)
{
  EventListenerEntry *e;
  gchar *interface;

  if (!callback)
    {
      return FALSE;
    }

  if (!event_type)
  {
    g_warning ("called atspi_event_listener_register_from_callback with a NULL event_type");
    return FALSE;
  }

  e = g_new0 (EventListenerEntry, 1);
  if (!convert_event_type_to_dbus (event_type, &e->category, &e->name, &e->detail))
    {
      listener_entry_free (e);
      return FALSE;
    }

  e->event_type = g_strdup (event_type);
  e->callback = callback;
  e->user_data = user_data;
  e->callback_destroyed = callback_destroyed;
  callback_ref (callback == remove_datum ? (gpointer)user_data : (gpointer)callback,
                callback_destroyed);
  e->properties = copy_event_properties (properties);
  event_listeners = g_list_prepend (event_listeners, e);

  interface = g_strdup_printf ("org.a11y.atspi.Event.%s", e->category);
  e->signal_subscription =
    g_dbus_connection_signal_subscribe (_atspi_bus (),
                                        NULL, interface, e->name, NULL,
                                        e->detail, G_DBUS_SIGNAL_FLAGS_NONE,
                                        handle_event, NULL, NULL);
  if (e->detail)
    e->arg0path_signal_subscription =
      g_dbus_connection_signal_subscribe (_atspi_bus (),
                                          NULL, interface, e->name, NULL,
                                          e->detail, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH,
                                          handle_event, NULL, NULL);
  g_free (interface);

  notify_event_registered (e);
  return TRUE;
}

void
_atspi_reregister_event_listeners ()
{
  GList *l;
  EventListenerEntry *e;

  for (l = event_listeners; l; l = l->next)
    {
      e = l->data;
      notify_event_registered (e);
    }
}

void
_atspi_register_default_event_listeners (void)
{
  g_dbus_connection_signal_subscribe (_atspi_bus (),
                                      NULL, ATSPI_DBUS_INTERFACE_EVENT_OBJECT,
                                      "ChildrenChanged", NULL, NULL,
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      handle_event, NULL, NULL);
  g_dbus_connection_signal_subscribe (_atspi_bus (),
                                      NULL, ATSPI_DBUS_INTERFACE_EVENT_OBJECT,
                                      "PropertyChange", NULL, NULL,
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      handle_event, NULL, NULL);
  g_dbus_connection_signal_subscribe (_atspi_bus (),
                                      NULL, ATSPI_DBUS_INTERFACE_EVENT_OBJECT,
                                      "StateChanged", NULL, NULL,
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      handle_event, NULL, NULL);
}

/**
 * atspi_event_listener_register_no_data: (skip)
 * @callback: (scope notified): the #AtspiEventListenerSimpleCB to be
 *            registered against an event type.
 * @callback_destroyed: A #GDestroyNotify called when the callback is destroyed.
 * @event_type: a character string indicating the type of events for which
 *            notification is requested.  Format is
 *            EventClass:major_type:minor_type:detail
 *            where all subfields other than EventClass are optional.
 *            EventClasses include "object", "window", "mouse",
 *            and toolkit events (e.g. "Gtk", "AWT").
 *            Examples: "focus:", "Gtk:GtkWidget:button_press_event".
 *
 * Registers an #AtspiEventListenetSimpleCB. The method is similar to 
 * #atspi_event_listener_register, but @callback takes no user_data.
 *
 * Returns: #TRUE if successfull, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_register_no_data (AtspiEventListenerSimpleCB callback,
				 GDestroyNotify callback_destroyed,
				 const gchar              *event_type,
				 GError **error)
{
  return atspi_event_listener_register_from_callback (remove_datum, callback,
                                                      callback_destroyed,
                                                      event_type, error);
}

static gboolean
is_superset (const gchar *super, const gchar *sub)
{
  if (!super || !super [0])
    return TRUE;
  return (strcmp (super, sub) == 0);
}

/**
 * atspi_event_listener_deregister:
 * @listener: The #AtspiEventListener to deregister.
 * @event_type: a string specifying the event type for which this
 *             listener is to be deregistered.
 *
 * Deregisters an #AtspiEventListener from the registry, for a specific
 *             event type.
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_deregister (AtspiEventListener *listener,
				               const gchar              *event_type,
				               GError **error)
{
  return atspi_event_listener_deregister_from_callback (listener->callback,
                                                        listener->user_data,
                                                        event_type, error);
}

/**
 * atspi_event_listener_deregister_from_callback:
 * @callback: (scope call): the #AtspiEventListenerCB registered against an
 *            event type.
 * @user_data: (closure): User data that was passed in for this callback.
 * @event_type: a string specifying the event type for which this
 *             listener is to be deregistered.
 *
 * Deregisters an #AtspiEventListenerCB from the registry, for a specific
 *             event type.
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_deregister_from_callback (AtspiEventListenerCB callback,
				               void *user_data,
				               const gchar              *event_type,
				               GError **error)
{
  char *category, *name, *detail;
  GList *l;

  if (!convert_event_type_to_dbus (event_type, &category, &name, &detail))
  {
    return FALSE;
  }
  if (!callback)
    {
      return FALSE;
    }

  for (l = event_listeners; l;)
  {
    EventListenerEntry *e = l->data;
    if (e->callback == callback &&
        e->user_data == user_data &&
        is_superset (category, e->category) &&
        is_superset (name, e->name) &&
        is_superset (detail, e->detail))
    {
      gboolean need_replace;
      need_replace = (l == event_listeners);
      l = g_list_remove (l, e);
      if (need_replace)
        event_listeners = l;

      g_dbus_connection_signal_unsubscribe (_atspi_bus (), e->signal_subscription);
      if (e->arg0path_signal_subscription)
        g_dbus_connection_signal_unsubscribe (_atspi_bus (), e->arg0path_signal_subscription);

      notify_event_deregistered (e);
      listener_entry_free (e);
    }
    else l = g_list_next (l);
  }
  g_free (category);
  g_free (name);
  g_free (detail);
  return TRUE;
}

/**
 * atspi_event_listener_deregister_no_data: (skip)
 * @callback: (scope call): the #AtspiEventListenerSimpleCB registered against
 *            an event type.
 * @event_type: a string specifying the event type for which this
 *             listener is to be deregistered.
 *
 * deregisters an #AtspiEventListenerSimpleCB from the registry, for a specific
 *             event type.
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_deregister_no_data (AtspiEventListenerSimpleCB callback,
				   const gchar              *event_type,
				   GError **error)
{
  return atspi_event_listener_deregister_from_callback (remove_datum, callback,
                                                        event_type,
                                                        error);
}

static AtspiEvent *
atspi_event_copy (AtspiEvent *src)
{
  AtspiEvent *dst = g_new0 (AtspiEvent, 1);
  dst->type = g_strdup (src->type);
  dst->source = g_object_ref (src->source);
  dst->detail1 = src->detail1;
  dst->detail2 = src->detail2;
  g_value_init (&dst->any_data, G_VALUE_TYPE (&src->any_data));
  g_value_copy (&src->any_data, &dst->any_data);
  return dst;
}

static void
atspi_event_free (AtspiEvent *event)
{
  g_object_unref (event->source);
  g_free (event->type);
  g_value_unset (&event->any_data);
  g_free (event);
}

static gboolean
detail_matches_listener (const char *event_detail, const char *listener_detail)
{
  if (!listener_detail)
    return TRUE;

  if (!event_detail)
    return (listener_detail ? FALSE : TRUE);

  return !(listener_detail [strcspn (listener_detail, ":")] == '\0'
               ? strncmp (listener_detail, event_detail,
                          strcspn (event_detail, ":"))
               : strcmp (listener_detail, event_detail));
}

void
_atspi_send_event (AtspiEvent *e)
{
  char *category, *name, *detail;
  GList *l;
  GList *called_listeners = NULL;

  /* Ensure that the value is set to avoid a Python exception */
  /* TODO: Figure out how to do this without using a private field */
  if (e->any_data.g_type == 0)
  {
    g_value_init (&e->any_data, G_TYPE_INT);
    g_value_set_int (&e->any_data, 0);
  }

  if (!convert_event_type_to_dbus (e->type, &category, &name, &detail))
  {
    g_warning ("Atspi: Couldn't parse event: %s\n", e->type);
    return;
  }
  for (l = event_listeners; l; l = g_list_next (l))
  {
    EventListenerEntry *entry = l->data;
    if (!strcmp (category, entry->category) &&
        (entry->name == NULL || !strcmp (name, entry->name)) &&
        detail_matches_listener (detail, entry->detail))
    {
      GList *l2;
      for (l2 = called_listeners; l2; l2 = l2->next)
      {
        EventListenerEntry *e2 = l2->data;
        if (entry->callback == e2->callback && entry->user_data == e2->user_data)
          break;
      }
      if (!l2)
      {
        entry->callback (atspi_event_copy (e), entry->user_data);
        called_listeners = g_list_prepend (called_listeners, entry);
      }
    }
  }
  if (detail) g_free (detail);
  g_free (name);
  g_free (category);
  g_list_free (called_listeners);
}

G_DEFINE_BOXED_TYPE (AtspiEvent, atspi_event, atspi_event_copy, atspi_event_free)
