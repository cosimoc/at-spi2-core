/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2008, 2010 Codethink Ltd.
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

#include <config.h>
#include <string.h>
#include <ctype.h>

#include "paths.h"
#include "registry.h"

#include "xml/a11y-atspi-accessible.h"
#include "xml/a11y-atspi-application.h"
#include "xml/a11y-atspi-cache.h"
#include "xml/a11y-atspi-component.h"
#include "xml/a11y-atspi-registry.h"
#include "xml/a11y-atspi-socket.h"

typedef struct {
  A11yAtspiAccessible *accessible_skel;
  A11yAtspiApplication *application_skel;
  A11yAtspiCache *cache_skel;
  A11yAtspiComponent *component_skel;
  A11yAtspiRegistry *registry_skel;
  A11yAtspiSocket *socket_skel;

  guint bus_filter_id;
} SpiRegistryPrivate;

typedef struct event_data event_data;
struct event_data
{
  gchar *bus_name;
  gchar **data;
  gchar **properties;
};

static void handle_name_vanished (GDBusConnection *bus,
                                  const gchar *name,
                                  gpointer user_data);

/*---------------------------------------------------------------------------*/

G_DEFINE_TYPE_WITH_PRIVATE (SpiRegistry, spi_registry, G_TYPE_OBJECT)

static void
spi_registry_finalize (GObject *object)
{
  SpiRegistry *registry = SPI_REGISTRY (object);
  SpiRegistryPrivate *priv = spi_registry_get_instance_private (registry);

  g_clear_object (&priv->accessible_skel);
  g_clear_object (&priv->application_skel);
  g_clear_object (&priv->cache_skel);
  g_clear_object (&priv->component_skel);
  g_clear_object (&priv->registry_skel);
  g_clear_object (&priv->socket_skel);

  g_dbus_connection_remove_filter (registry->bus, priv->bus_filter_id);

  G_OBJECT_CLASS (spi_registry_parent_class)->finalize (object);
}

static void
spi_registry_class_init (SpiRegistryClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->finalize = spi_registry_finalize;
}

static void
spi_registry_init (SpiRegistry *registry)
{
  registry->apps = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
}

/*---------------------------------------------------------------------------*/

static GVariant *
append_reference (const char * name, const char * path)
{
  return g_variant_new ("(so)", name, path);
}

/*---------------------------------------------------------------------------*/

static gboolean
find_index_of_reference (GPtrArray *arr, const gchar *name, const gchar * path, guint *index)
{
  gboolean found = FALSE;
  guint i = 0;

  for (i = 0; i < arr->len; i++)
    {
      GDBusProxy *proxy = g_ptr_array_index (arr, i);
      if ((g_strcmp0 (g_dbus_proxy_get_name (proxy), name) == 0) &&
          (g_strcmp0 (g_dbus_proxy_get_object_path (proxy), path) == 0))
        {
          found = TRUE;
          break;
        }
    }

  *index = i;
  return found;
}

/*
 * Emits an AT-SPI event.
 * AT-SPI events names are split into three parts:
 * class:major:minor
 * This is mapped onto D-Bus events as:
 * D-Bus Interface:Signal Name:Detail argument
 *
 * Marshals a basic type into the 'any_data' attribute of
 * the AT-SPI event.
 */
static void
emit_event (GDBusConnection *bus,
            const char *klass,
            const char *major,
            const char *minor,
            gint detail1,
            gint detail2,
            const char *name,
            const char *path)
{
  g_dbus_connection_emit_signal (bus, NULL, SPI_DBUS_PATH_ROOT, klass, major,
                                 g_variant_new ("(siiv(so))",
                                                minor, detail1, detail2,
                                                g_variant_new ("(so)",
                                                               name, path),
                                                g_dbus_connection_get_unique_name (bus),
                                                SPI_DBUS_PATH_ROOT),
                                 NULL);
}

/*
 * Children changed signal converter and forwarder.
 *
 * Klass (Interface) org.a11y.atspi.Event.Object
 * Major is the signal name.
 * Minor is 'add' or 'remove'
 * detail1 is the index.
 * detail2 is 0.
 * any_data is the child reference.
 */

static void
children_added_listener (GDBusConnection * bus,
                         gint              index,
                         const gchar     * name,
                         const gchar     * path)
{
  emit_event (bus, SPI_DBUS_INTERFACE_EVENT_OBJECT, "ChildrenChanged", "add", index, 0,
              name, path);
}

static void
children_removed_listener (GDBusConnection * bus,
                           gint              index,
                           const gchar     * name,
                           const gchar     * path)
{
  emit_event (bus, SPI_DBUS_INTERFACE_EVENT_OBJECT, "ChildrenChanged", "remove", index, 0,
              name, path);
}

static void
clear_bus_watch_name (gpointer data)
{
  guint watch_id = GPOINTER_TO_UINT (data);
  g_bus_unwatch_name (watch_id);
}

static void
set_id (SpiRegistry *reg, A11yAtspiApplication *proxy)
{
  a11y_atspi_application_set_id (proxy, reg->id);
  /* TODO: This will cause problems if we cycle through 2^31 ids */
  reg->id++;
}

static void
remove_application (SpiRegistry *reg, guint index)
{
  SpiRegistryPrivate *priv = spi_registry_get_instance_private (reg);
  A11yAtspiApplication *proxy = g_ptr_array_index (reg->apps, index);
  const char *name = g_dbus_proxy_get_name (G_DBUS_PROXY (proxy));
  const char *path = g_dbus_proxy_get_object_path (G_DBUS_PROXY (proxy));

  spi_remove_device_listeners (reg->dec, name);
  children_removed_listener (reg->bus, index, name, path);
  g_ptr_array_remove_index (reg->apps, index);
  a11y_atspi_accessible_set_child_count (priv->accessible_skel, reg->apps->len);
}

static gboolean
event_is_subtype (gchar **needle, gchar **haystack)
{
  while (*haystack && **haystack)
    {
      if (g_strcmp0 (*needle, *haystack))
        return FALSE;
      needle++;
      haystack++;
    }
  return TRUE;
}

static gboolean
needs_mouse_poll (char **event)
{
  if (g_strcmp0 (event [0], "Mouse") != 0)
    return FALSE;
  if (!event [1] || !event [1][0])
    return TRUE;
  return (g_strcmp0 (event [1], "Abs") == 0);
}

static void
remove_events (SpiRegistry *registry, const char *bus_name, const char *event)
{
  SpiRegistryPrivate *priv = spi_registry_get_instance_private (registry);
  gchar **remove_data;
  GList *list;
  gboolean mouse_found = FALSE;

  remove_data = g_strsplit (event, ":", 3);
  if (!remove_data)
    {
      return;
    }

  for (list = registry->events; list;)
    {
      event_data *evdata = list->data;
      list = list->next;
      if (!g_strcmp0 (evdata->bus_name, bus_name) &&
          event_is_subtype (evdata->data, remove_data))
        {
          g_strfreev (evdata->data);
          g_strfreev (evdata->properties);
          g_free (evdata->bus_name);
          g_free (evdata);
          registry->events = g_list_remove (registry->events, evdata);
        }
      else
        {
          if (needs_mouse_poll (evdata->data))
            mouse_found = TRUE;
        }
    }

  if (!mouse_found)
    spi_device_event_controller_stop_poll_mouse ();

  g_strfreev (remove_data);

  a11y_atspi_registry_emit_event_listener_deregistered (priv->registry_skel,
                                                        bus_name, event);
}

static void
handle_name_vanished (GDBusConnection *bus,
                      const gchar *name,
                      gpointer user_data)
{
  SpiRegistry *reg = user_data;
  guint i;

  for (i = 0; i < reg->apps->len; i++)
    {
      GDBusProxy *app  = g_ptr_array_index (reg->apps, i);
      if (!g_strcmp0 (g_dbus_proxy_get_name (app), name))
        {
          remove_application (reg, i);
          i--;
        }
    }

  remove_events (reg, name, "");
}

/*
 * Converts names of the form "active-descendant-changed" to
 *" ActiveDescendantChanged"
 */
static gchar *
ensure_proper_format (const char *name)
{
  gchar *ret = (gchar *) g_malloc (strlen (name) * 2 + 2);
  gchar *p = ret;
  gboolean need_upper = TRUE;

  if (!ret)
    return NULL;
  while (*name)
    {
      if (need_upper)
        {
          *p++ = toupper (*name);
          need_upper = FALSE;
        }
      else if (*name == '-')
        need_upper = TRUE;
      else if (*name == ':')
        {
          need_upper = TRUE;
          *p++ = *name;
        }
      else
        *p++ = *name;
      name++;
    }
  *p = '\0';
  return ret;
}

/* org.at_spi.Socket interface */
/*---------------------------------------------------------------------------*/

static void
add_application (SpiRegistry *reg,
                 A11yAtspiApplication *proxy)
{
  SpiRegistryPrivate *priv = spi_registry_get_instance_private (reg);
  guint watch_id;

  g_ptr_array_add (reg->apps, proxy);
  a11y_atspi_accessible_set_child_count (priv->accessible_skel, reg->apps->len);
  children_added_listener (reg->bus, reg->apps->len - 1,
                           g_dbus_proxy_get_name (G_DBUS_PROXY (proxy)),
                           g_dbus_proxy_get_object_path (G_DBUS_PROXY (proxy)));

  watch_id = g_bus_watch_name_on_connection (reg->bus, g_dbus_proxy_get_name (G_DBUS_PROXY (proxy)),
                                             G_BUS_NAME_WATCHER_FLAGS_NONE,
                                             NULL, handle_name_vanished, reg, NULL);
  g_object_set_data_full (G_OBJECT (proxy), "bus-watch-name-id",
                          GUINT_TO_POINTER (watch_id), clear_bus_watch_name);

  set_id (reg, proxy);
}

static void
application_proxy_prepared (GObject *source,
                            GAsyncResult *res,
                            gpointer user_data)
{
  GDBusMethodInvocation *invocation = user_data;
  SpiRegistry *reg = g_object_get_data (G_OBJECT (invocation), "spi-registry");
  SpiRegistryPrivate *priv = spi_registry_get_instance_private (reg);
  A11yAtspiApplication *proxy = a11y_atspi_application_proxy_new_finish (res, NULL);

  if (proxy)
    add_application (reg, proxy);

  a11y_atspi_socket_complete_embed (priv->socket_skel, invocation,
                                    append_reference (g_dbus_connection_get_unique_name (reg->bus),
                                                      SPI_DBUS_PATH_ROOT));
}

static gboolean
handle_socket_embed (A11yAtspiSocket *skel,
                     GDBusMethodInvocation *invocation,
                     GVariant *plug,
                     gpointer user_data)
{
  SpiRegistry *reg = SPI_REGISTRY (user_data);
  const gchar *app_name, *obj_path;

  g_variant_get (plug, "(&s&o)", &app_name, &obj_path);
  a11y_atspi_application_proxy_new (reg->bus,
                                    G_DBUS_PROXY_FLAGS_NONE,
                                    app_name, obj_path, NULL,
                                    application_proxy_prepared, invocation);
  g_object_set_data (G_OBJECT (invocation), "spi-registry", reg);
  return TRUE;
}

static gboolean
handle_socket_unembed (A11yAtspiSocket *skel,
                       GDBusMethodInvocation *invocation,
                       GVariant *plug,
                       gpointer user_data)
{
  SpiRegistry *reg = SPI_REGISTRY (user_data);
  guint index;
  const gchar *app_name, *obj_path;

  g_variant_get (plug, "(&s&o)", &app_name, &obj_path);

  if (find_index_of_reference (reg->apps, app_name, obj_path, &index))
      remove_application (reg, index);

  a11y_atspi_socket_complete_unembed (skel, invocation);
  return TRUE;
}

/* org.at_spi.Component interface */
/*---------------------------------------------------------------------------*/

static gboolean
handle_component_contains (A11yAtspiComponent *component,
                           GDBusMethodInvocation *invocation,
                           gint x, gint y, guint coord_type,
                           gpointer user_data)
{
  a11y_atspi_component_complete_contains (component, invocation, FALSE);
  return TRUE;
}

static gboolean
handle_component_get_accessible_at_point (A11yAtspiComponent *component,
                                          GDBusMethodInvocation *invocation,
                                          gint x, gint y, guint coord_type,
                                          gpointer user_data)
{
  SpiRegistry *reg = user_data;
  a11y_atspi_component_complete_get_accessible_at_point
    (component, invocation,
     append_reference (g_dbus_connection_get_unique_name (reg->bus),
                       SPI_DBUS_PATH_NULL));
  return TRUE;
}

static gboolean
handle_component_get_extents (A11yAtspiComponent *component,
                              GDBusMethodInvocation *invocation,
                              guint coord_type,
                              gpointer user_data)
{
  a11y_atspi_component_complete_get_extents (component, invocation,
                                             g_variant_new ("(iiii)", 0, 0, 1024, 768));
  return TRUE;
}

static gboolean
handle_component_get_position (A11yAtspiComponent *component,
                               GDBusMethodInvocation *invocation,
                               guint coord_type,
                               gpointer user_data)
{
  a11y_atspi_component_complete_get_position (component, invocation, 0, 0);
  return TRUE;
}

static gboolean
handle_component_get_size (A11yAtspiComponent *component,
                           GDBusMethodInvocation *invocation,
                           gpointer user_data)
{
  /* TODO - Get the screen size */
  a11y_atspi_component_complete_get_size (component, invocation, 1024, 768);
  return TRUE;
}

#define LAYER_WIDGET 3

static gboolean
handle_component_get_layer (A11yAtspiComponent *component,
                            GDBusMethodInvocation *invocation,
                            gpointer user_data)
{
  a11y_atspi_component_complete_get_layer (component, invocation, LAYER_WIDGET);
  return TRUE;
}

static gboolean
handle_component_get_mdizorder (A11yAtspiComponent *component,
                                GDBusMethodInvocation *invocation,
                                gpointer user_data)
{
  a11y_atspi_component_complete_get_mdizorder (component, invocation, 0);
  return TRUE;
}

static gboolean
handle_component_grab_focus (A11yAtspiComponent *component,
                             GDBusMethodInvocation *invocation,
                             gpointer user_data)
{
  a11y_atspi_component_complete_grab_focus (component, invocation, FALSE);
  return TRUE;
}

static gboolean
handle_component_get_alpha (A11yAtspiComponent *component,
                            GDBusMethodInvocation *invocation,
                            gpointer user_data)
{
  a11y_atspi_component_complete_get_alpha (component, invocation, 1.0);
  return TRUE;
}

/* org.at_spi.Accessible interface */
/*---------------------------------------------------------------------------*/

static gboolean
handle_accessible_get_child_at_index (A11yAtspiAccessible *skel,
                                      GDBusMethodInvocation *invocation,
                                      gint index,
                                      gpointer user_data)
{
  SpiRegistry *reg = SPI_REGISTRY (user_data);
  GVariant *variant;

  if (index < 0 || index >= reg->apps->len)
    {
      variant = append_reference (SPI_DBUS_NAME_REGISTRY, SPI_DBUS_PATH_NULL);
    }
  else
    {
      GDBusProxy *proxy = g_ptr_array_index (reg->apps, index);
      variant = append_reference (g_dbus_proxy_get_name (proxy),
                                  g_dbus_proxy_get_object_path (proxy));
    }

  a11y_atspi_accessible_complete_get_child_at_index (skel, invocation, variant);
  return TRUE;
}

static gboolean
handle_accessible_get_children (A11yAtspiAccessible *skel,
                                GDBusMethodInvocation *invocation,
                                gpointer user_data)
{
  SpiRegistry *reg = SPI_REGISTRY (user_data);
  GVariantBuilder builder;
  int i;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(so)"));
  for (i = 0; i < reg->apps->len; i++)
    {
      GDBusProxy *proxy = g_ptr_array_index (reg->apps, i);
      g_variant_builder_add_value (&builder,
                                   append_reference (g_dbus_proxy_get_name (proxy),
                                                     g_dbus_proxy_get_object_path (proxy)));
    }

  a11y_atspi_accessible_complete_get_children (skel, invocation,
                                               g_variant_builder_end (&builder));
  return TRUE;
}

static gboolean
handle_accessible_get_index_in_parent (A11yAtspiAccessible *skel,
                                       GDBusMethodInvocation *invocation,
                                       gpointer user_data)
{
  a11y_atspi_accessible_complete_get_index_in_parent (skel, invocation, 0);
  return TRUE;
}

static gboolean
handle_accessible_get_relation_set (A11yAtspiAccessible *skel,
                                    GDBusMethodInvocation *invocation,
                                    gpointer user_data)
{
  a11y_atspi_accessible_complete_get_relation_set (skel, invocation,
                                                   g_variant_new_array (G_VARIANT_TYPE ("(ua(so))"),
                                                                        NULL, 0));
  return TRUE;
}

static gboolean
handle_accessible_get_role (A11yAtspiAccessible *skel,
                            GDBusMethodInvocation *invocation,
                            gpointer user_data)
{
/* TODO: Get DESKTOP_FRAME from somewhere */
  a11y_atspi_accessible_complete_get_role (skel, invocation, 14);
  return TRUE;
}

static gboolean
handle_accessible_get_role_name (A11yAtspiAccessible *skel,
                                 GDBusMethodInvocation *invocation,
                                 gpointer user_data)
{
  a11y_atspi_accessible_complete_get_role_name (skel, invocation, "desktop frame");
  return TRUE;
}

static gboolean
handle_accessible_get_localized_role_name (A11yAtspiAccessible *skel,
                                           GDBusMethodInvocation *invocation,
                                           gpointer user_data)
{
  /* TODO - Localize this */
  a11y_atspi_accessible_complete_get_localized_role_name (skel, invocation, "desktop frame");
  return TRUE;
}

static gboolean
handle_accessible_get_state (A11yAtspiAccessible *skel,
                             GDBusMethodInvocation *invocation,
                             gpointer user_data)
{
  guint32 states[2] = { 0, 1 };
  a11y_atspi_accessible_complete_get_state (skel, invocation,
                                            g_variant_new_fixed_array (G_VARIANT_TYPE_UINT32,
                                                                       states, 2, sizeof (guint32)));
  return TRUE;
}

static gboolean
handle_accessible_get_attributes (A11yAtspiAccessible *skel,
                                  GDBusMethodInvocation *invocation,
                                  gpointer user_data)
{
  a11y_atspi_accessible_complete_get_attributes (skel, invocation,
                                                 g_variant_new_array (G_VARIANT_TYPE ("{ss}"),
                                                                      NULL, 0));
  return TRUE;
}

static gboolean
handle_accessible_get_application (A11yAtspiAccessible *skel,
                                   GDBusMethodInvocation *invocation,
                                   gpointer user_data)
{
  SpiRegistry *reg = user_data;
  a11y_atspi_accessible_complete_get_application (skel, invocation,
                                                  append_reference (g_dbus_connection_get_unique_name (reg->bus),
                                                                    SPI_DBUS_PATH_NULL));
  return TRUE;
}

static gboolean
handle_accessible_get_interfaces (A11yAtspiAccessible *skel,
                                  GDBusMethodInvocation *invocation,
                                  gpointer user_data)
{
  const char *interfaces[3] = { SPI_DBUS_INTERFACE_ACCESSIBLE,
                                SPI_DBUS_INTERFACE_COMPONENT,
                                NULL };
  a11y_atspi_accessible_complete_get_interfaces (skel, invocation, interfaces);
  return TRUE;
}

static gboolean
handle_cache_get_items (A11yAtspiCache *skel,
                        GDBusMethodInvocation *invocation,
                        gpointer user_data)
{
  a11y_atspi_cache_complete_get_items (skel, invocation,
                                       g_variant_new_array (G_VARIANT_TYPE ("((so)(so)(so)iiassusau)"),
                                                            NULL, 0));
  return TRUE;
}

/* I would rather these two be signals, but I'm not sure that dbus-python
 * supports emitting signals except for a service, so implementing as both
 * a method call and signal for now.
 */
static gboolean
handle_registry_register_event (A11yAtspiRegistry *skel,
                                GDBusMethodInvocation *invocation,
                                GVariant *event,
                                gpointer user_data)
{
  SpiRegistry *registry = SPI_REGISTRY (user_data);
  const char *orig_name;
  gchar *name;
  event_data *evdata;

  evdata = g_new0 (event_data, 1);
  evdata->bus_name = g_strdup (g_dbus_method_invocation_get_sender (invocation));

  g_variant_get (event, "(&s^as)", &orig_name, &evdata->properties);
  name = ensure_proper_format (orig_name);
  evdata->data = g_strsplit (name, ":", 3);

  registry->events = g_list_append (registry->events, evdata);

  if (needs_mouse_poll (evdata->data))
    spi_device_event_controller_start_poll_mouse (registry);

  a11y_atspi_registry_emit_event_listener_registered (skel, evdata->bus_name,
                                                      name, (const char * const * )evdata->properties);
  g_free (name);

  a11y_atspi_registry_complete_register_event (skel, invocation);
  return TRUE;
}

static gboolean
handle_registry_deregister_event (A11yAtspiRegistry *skel,
                                  GDBusMethodInvocation *invocation,
                                  const char *event,
                                  gpointer user_data)
{
  SpiRegistry *registry = SPI_REGISTRY (user_data);
  gchar *name;
  const char *sender = g_dbus_method_invocation_get_sender (invocation);

  name = ensure_proper_format (event);
  remove_events (registry, sender, name);
  g_free (name);

  a11y_atspi_registry_complete_deregister_event (skel, invocation);
  return TRUE;
}

static gboolean
handle_registry_get_registered_events (A11yAtspiRegistry *skel,
                                       GDBusMethodInvocation *invocation,
                                       gpointer user_data)
{
  SpiRegistry *registry = SPI_REGISTRY (user_data);
  GVariantBuilder builder;
  GList *list;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(ss)"));
  for (list = registry->events; list; list = list->next)
    {
      event_data *evdata = list->data;
      gchar *str = g_strconcat (evdata->data [0],
                                ":", (evdata->data [1]? evdata->data [1]: ""),
                                ":", (evdata->data [1] && evdata->data [2]? evdata->data [2]: ""), NULL);

      g_variant_builder_add (&builder, "(ss)", evdata->bus_name, str);
      g_free (str);
    }

  a11y_atspi_registry_complete_get_registered_events (skel, invocation,
                                                      g_variant_builder_end (&builder));
  return TRUE;
}

static GDBusMessage *
registry_dbus_filter (GDBusConnection *connection,
                      GDBusMessage *message,
                      gboolean incoming,
                      gpointer user_data)
{
  GDBusMessage *copy;
  GVariant *body;
  const GVariant *array[2] = { NULL, };

  if (!incoming)
    return message;

  if (g_dbus_message_get_message_type (message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL &&
      g_strcmp0 (g_dbus_message_get_interface (message), SPI_DBUS_INTERFACE_REGISTRY) == 0 &&
      g_strcmp0 (g_dbus_message_get_path (message), SPI_DBUS_PATH_REGISTRY) == 0 &&
      g_strcmp0 (g_dbus_message_get_member (message), "RegisterEvent") == 0 &&
      g_strcmp0 (g_dbus_message_get_signature (message), "sas") == 0)
    {
      copy = g_dbus_message_copy (message, NULL);
      if (!copy)
        goto out;

      /* Adjust for broken signature on RegisterEvent method */
      body = g_dbus_message_get_body (message);
      array[0] = body;
      g_dbus_message_set_body (copy, g_variant_new_tuple ((GVariant * const *) array, 1));
      g_dbus_message_set_signature (copy, "(sas)");

      g_object_unref (message);
      message = copy;
    }

 out:
  return message;
}

/*---------------------------------------------------------------------------*/

SpiRegistry *
spi_registry_new (GDBusConnection *bus)
{
  SpiRegistry *reg = g_object_new (SPI_REGISTRY_TYPE, NULL);
  SpiRegistryPrivate *priv = spi_registry_get_instance_private (reg);

  reg->bus = bus;

  priv->bus_filter_id =
    g_dbus_connection_add_filter (reg->bus, registry_dbus_filter, reg, NULL);

  /* Export org.a11y.atspi.Registry */
  priv->registry_skel = a11y_atspi_registry_skeleton_new ();
  g_signal_connect (priv->registry_skel, "handle-deregister-event",
                    G_CALLBACK (handle_registry_deregister_event), reg);
  g_signal_connect (priv->registry_skel, "handle-get-registered-events",
                    G_CALLBACK (handle_registry_get_registered_events), reg);
  g_signal_connect (priv->registry_skel, "handle-register-event",
                    G_CALLBACK (handle_registry_register_event), reg);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->registry_skel),
                                    bus, SPI_DBUS_PATH_REGISTRY, NULL);

  /* Export org.a11y.atspi.Cache */
  priv->cache_skel = a11y_atspi_cache_skeleton_new ();
  g_signal_connect (priv->cache_skel, "handle-get-items",
                    G_CALLBACK (handle_cache_get_items), reg);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->cache_skel),
                                    bus, SPI_DBUS_PATH_CACHE, NULL);

  /* Export org.a11y.atspi.Accessible */
  priv->accessible_skel = a11y_atspi_accessible_skeleton_new ();
  a11y_atspi_accessible_set_name (priv->accessible_skel, "main");
  a11y_atspi_accessible_set_description (priv->accessible_skel, "");
  a11y_atspi_accessible_set_parent (priv->accessible_skel,
                                    append_reference ("", SPI_DBUS_PATH_NULL));
  g_signal_connect (priv->accessible_skel, "handle-get-child-at-index",
                    G_CALLBACK (handle_accessible_get_child_at_index), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-children",
                    G_CALLBACK (handle_accessible_get_children), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-index-in-parent",
                    G_CALLBACK (handle_accessible_get_index_in_parent), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-relation-set",
                    G_CALLBACK (handle_accessible_get_relation_set), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-role",
                    G_CALLBACK (handle_accessible_get_role), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-role-name",
                    G_CALLBACK (handle_accessible_get_role_name), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-localized-role-name",
                    G_CALLBACK (handle_accessible_get_localized_role_name), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-state",
                    G_CALLBACK (handle_accessible_get_state), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-attributes",
                    G_CALLBACK (handle_accessible_get_attributes), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-application",
                    G_CALLBACK (handle_accessible_get_application), reg);
  g_signal_connect (priv->accessible_skel, "handle-get-interfaces",
                    G_CALLBACK (handle_accessible_get_interfaces), reg);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->accessible_skel),
                                    bus, SPI_DBUS_PATH_ROOT, NULL);

  /* Export org.a11y.atspi.Application */
  priv->application_skel = a11y_atspi_application_skeleton_new ();
  a11y_atspi_application_set_toolkit_name (priv->application_skel, "at-spi-registry");
  a11y_atspi_application_set_toolkit_version (priv->application_skel, "2.0");
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->application_skel),
                                    bus, SPI_DBUS_PATH_ROOT, NULL);

  /* Export org.a11y.atspi.Component */
  priv->component_skel = a11y_atspi_component_skeleton_new ();
  g_signal_connect (priv->component_skel, "handle-contains",
                    G_CALLBACK (handle_component_contains), reg);
  g_signal_connect (priv->component_skel, "handle-get-accessible-at-point",
                    G_CALLBACK (handle_component_get_accessible_at_point), reg);
  g_signal_connect (priv->component_skel, "handle-get-extents",
                    G_CALLBACK (handle_component_get_extents), reg);
  g_signal_connect (priv->component_skel, "handle-get-position",
                    G_CALLBACK (handle_component_get_position), reg);
  g_signal_connect (priv->component_skel, "handle-get-size",
                    G_CALLBACK (handle_component_get_size), reg);
  g_signal_connect (priv->component_skel, "handle-get-layer",
                    G_CALLBACK (handle_component_get_layer), reg);
  g_signal_connect (priv->component_skel, "handle-get-mdizorder",
                    G_CALLBACK (handle_component_get_mdizorder), reg);
  g_signal_connect (priv->component_skel, "handle-grab-focus",
                    G_CALLBACK (handle_component_grab_focus), reg);
  g_signal_connect (priv->component_skel, "handle-get-alpha",
                    G_CALLBACK (handle_component_get_alpha), reg);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->component_skel),
                                    bus, SPI_DBUS_PATH_ROOT, NULL);

  /* Export org.a11y.atspi.Socket */
  priv->socket_skel = a11y_atspi_socket_skeleton_new ();
  g_signal_connect (priv->socket_skel, "handle-embed",
                    G_CALLBACK (handle_socket_embed), reg);
  g_signal_connect (priv->socket_skel, "handle-unembed",
                    G_CALLBACK (handle_socket_unembed), reg);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->socket_skel),
                                    bus, SPI_DBUS_PATH_ROOT, NULL);
  a11y_atspi_socket_emit_available
    (priv->socket_skel, append_reference (SPI_DBUS_NAME_REGISTRY, SPI_DBUS_PATH_ROOT));

  return reg;
}

/*END------------------------------------------------------------------------*/
