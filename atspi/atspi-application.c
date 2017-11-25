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

#include "atspi-private.h"

#include "xml/a11y-atspi-application.h"
#include "xml/a11y-atspi-cache.h"

typedef struct {
  A11yAtspiApplication *application_proxy;
  A11yAtspiCache *cache_proxy;
  GDBusConnection *private_bus;
  guint watch_id;
} AtspiApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (AtspiApplication, atspi_application, G_TYPE_OBJECT)

static void
atspi_application_init (AtspiApplication *application)
{
  gettimeofday (&application->time_added, NULL);
  application->cache = ATSPI_CACHE_UNDEFINED;
  application->hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

static void
dispose_accessible (gpointer key, gpointer obj_data, gpointer data)
{
  g_object_run_dispose (obj_data);
}

static void
atspi_application_dispose (GObject *object)
{
  AtspiApplication *application = ATSPI_APPLICATION (object);
  AtspiApplicationPrivate *priv = atspi_application_get_instance_private (application);

  if (priv->private_bus)
  {
    g_dbus_connection_close_sync (priv->private_bus, NULL, NULL);
    g_clear_object (&priv->private_bus);
  }

  g_clear_object (&priv->application_proxy);
  g_clear_object (&priv->cache_proxy);

  if (application->hash)
  {
    g_hash_table_foreach (application->hash, dispose_accessible, NULL);
    g_hash_table_unref (application->hash);
    application->hash = NULL;
  }

  if (application->root)
  {
    g_clear_object (&application->root->parent.app);
    g_object_unref (application->root);
    application->root = NULL;
  }

  if (priv->watch_id)
  {
    g_bus_unwatch_name (priv->watch_id);
    priv->watch_id = 0;
  }

  G_OBJECT_CLASS (atspi_application_parent_class)->dispose (object);
}

static void
atspi_application_class_init (AtspiApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = atspi_application_dispose;
}

static void
add_accessible_from_variant (GVariant *variant)
{
  GVariantIter iter;
  GVariant *value;
  AtspiAccessible *accessible;
  gboolean children_cached = FALSE;
  gint count, index;

  g_variant_iter_init (&iter, variant);
  value = g_variant_iter_next_value (&iter);

  /* get accessible */
  accessible = _atspi_dbus_return_accessible_from_variant (value);
  g_variant_unref (value);
  if (!accessible)
    return;

  /* Get application: TODO */
  value = g_variant_iter_next_value (&iter);
  g_variant_unref (value);
  value = g_variant_iter_next_value (&iter);

  /* get parent */
  if (accessible->accessible_parent)
    g_object_unref (accessible->accessible_parent);
  accessible->accessible_parent = _atspi_dbus_return_accessible_from_variant (value);

  g_variant_unref (value);
  value = g_variant_iter_next_value (&iter);

  if (g_variant_is_of_type (value, G_VARIANT_TYPE_INT32))
    {
      /* Get index in parent */
      index = g_variant_get_int32 (value);
      if (index >= 0 && accessible->accessible_parent)
        {
          if (index >= accessible->accessible_parent->children->len)
            g_ptr_array_set_size (accessible->accessible_parent->children, index + 1);
          g_ptr_array_index (accessible->accessible_parent->children, index) = g_object_ref (accessible);
        }

      g_variant_unref (value);
      value = g_variant_iter_next_value (&iter);

      /* get child count */
      count = g_variant_get_int32 (value);
      if (count >= 0)
        {
          g_ptr_array_set_size (accessible->children, count);
          children_cached = TRUE;
        }
    }
  else if (g_variant_is_of_type (value, G_VARIANT_TYPE_ARRAY))
    {
      /* It's the old API with a list of children */
      GVariant *node;

      while ((node = g_variant_iter_next_value (&iter)))
        {
          AtspiAccessible *child = _atspi_dbus_return_accessible_from_variant (node);
          g_ptr_array_remove (accessible->children, child);
          g_ptr_array_add (accessible->children, child);
          g_variant_unref (node);
        }

      children_cached = TRUE;
    }

  g_variant_unref (value);
  value = g_variant_iter_next_value (&iter);

  /* interfaces */
  _atspi_dbus_set_interfaces (accessible, g_variant_get_strv (value, NULL));

  g_variant_unref (value);
  value = g_variant_iter_next_value (&iter);

  /* name */
  if (accessible->name)
    g_free (accessible->name);
  accessible->name = g_variant_dup_string (value, NULL);

  g_variant_unref (value);
  value = g_variant_iter_next_value (&iter);

  /* role */
  accessible->role = g_variant_get_uint32 (value);

  g_variant_unref (value);
  value = g_variant_iter_next_value (&iter);

  /* description */
  if (accessible->description)
    g_free (accessible->description);
  accessible->description = g_variant_dup_string (value, NULL);

  g_variant_unref (value);
  value = g_variant_iter_next_value (&iter);

  /* state */
  _atspi_dbus_set_state (accessible, value);

  g_variant_unref (value);

  _atspi_accessible_add_cache (accessible, ATSPI_CACHE_NAME | ATSPI_CACHE_ROLE |
                               ATSPI_CACHE_PARENT | ATSPI_CACHE_DESCRIPTION);
  if (!atspi_state_set_contains (accessible->states,
                                 ATSPI_STATE_MANAGES_DESCENDANTS) &&
      children_cached)
    _atspi_accessible_add_cache (accessible, ATSPI_CACHE_CHILDREN);

  /* This is a bit of a hack since the cache holds a ref, so we don't need
   * the one provided for us anymore */
  g_object_unref (accessible);
}

static void
handle_cache_remove_accessible (A11yAtspiCache *cache_proxy,
                                GVariant *variant,
                                gpointer user_data)
{
  AtspiApplication *app = user_data;
  AtspiAccessible *accessible = _atspi_dbus_return_accessible_from_variant (variant);
  if (!accessible)
    return;

  g_hash_table_remove (app->hash, accessible->parent.path);
  g_object_run_dispose (G_OBJECT (accessible));
  g_object_unref (accessible);
}

static void
handle_cache_add_accessible (A11yAtspiCache *cache_proxy,
                             GVariant *variant,
                             gpointer user_data)
{
  add_accessible_from_variant (variant);
}

static void
handle_name_vanished (GDBusConnection *bus,
                      const char *name,
                      gpointer user_data)
{
  AtspiApplication *app = user_data;
  g_object_run_dispose (G_OBJECT (app));
}

static void
handle_get_items (GObject *source,
                  GAsyncResult *res,
                  gpointer user_data)
{
  GDBusConnection *connection = G_DBUS_CONNECTION (source);
  AtspiApplication *app = user_data;
  AtspiApplicationPrivate *priv = atspi_application_get_instance_private (app);
  GVariant *variant = NULL, *nodes = NULL, *node = NULL;
  GVariantIter nodes_iter;
  GError *error = NULL;

  variant = g_dbus_connection_call_finish (connection, res, &error);
  if (error != NULL)
    {
      if (!g_error_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN))
        g_warning ("AT-SPI: Error in GetItems, sender=%s, error=%s",
                   g_dbus_proxy_get_name (G_DBUS_PROXY (priv->cache_proxy)), error->message);
      g_error_free (error);
      return;
    }

  nodes = g_variant_get_child_value (variant, 0);
  g_variant_iter_init (&nodes_iter, nodes);
  while ((node = g_variant_iter_next_value (&nodes_iter)))
    {
      add_accessible_from_variant (node);
      g_variant_unref (node);
    }

  g_variant_unref (nodes);
  g_variant_unref (variant);
}

static void
handle_get_bus_address (GObject *source,
                        GAsyncResult *res,
                        gpointer user_data)
{
  A11yAtspiApplication *proxy = A11Y_ATSPI_APPLICATION (source);
  AtspiApplication *app = user_data;
  AtspiApplicationPrivate *priv = atspi_application_get_instance_private (app);
  GError *error = NULL;
  gchar *address = NULL;

  if (a11y_atspi_application_call_get_application_bus_address_finish
      (proxy, &address, res, NULL))
    {
      priv->private_bus = g_dbus_connection_new_for_address_sync
        (address,
         G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
         G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
         NULL, NULL, &error);
      g_free (address);

      if (priv->private_bus)
        proxy = a11y_atspi_application_proxy_new_sync (priv->private_bus, G_DBUS_PROXY_FLAGS_NONE,
                                                       app->bus_name, ATSPI_DBUS_PATH_ROOT,
                                                       NULL, &error);

      if (error != NULL)
        {
          if (!g_error_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_FILE_NOT_FOUND))
            g_warning ("Unable to open bus connection: %s", error->message);
          g_clear_error (&error);
          g_clear_object (&priv->private_bus);
        }
      else
        {
          g_set_object (&priv->application_proxy, proxy);
        }
    }

  priv->cache_proxy =
    a11y_atspi_cache_proxy_new_sync (g_dbus_proxy_get_connection (G_DBUS_PROXY (priv->application_proxy)),
                                     G_DBUS_PROXY_FLAGS_NONE,
                                     app->bus_name, "/org/a11y/atspi/cache",
                                     NULL, NULL);
  g_signal_connect (priv->cache_proxy, "add-accessible",
                    G_CALLBACK (handle_cache_add_accessible), app);
  g_signal_connect (priv->cache_proxy, "remove-accessible",
                    G_CALLBACK (handle_cache_remove_accessible), app);

  /* Call GetItems() manually here, since we can expect two different signatures
   * for the return value.
   */
  g_dbus_connection_call (g_dbus_proxy_get_connection (G_DBUS_PROXY (priv->cache_proxy)),
                          g_dbus_proxy_get_name (G_DBUS_PROXY (priv->cache_proxy)),
                          g_dbus_proxy_get_object_path (G_DBUS_PROXY (priv->cache_proxy)),
                          g_dbus_proxy_get_interface_name (G_DBUS_PROXY (priv->cache_proxy)),
                          "GetItems", NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                          handle_get_items, app);
}

A11yAtspiApplication *
atspi_application_get_application_proxy (AtspiApplication *app)
{
  AtspiApplicationPrivate *priv = atspi_application_get_instance_private (app);
  return priv->application_proxy;
}

AtspiApplication *
_atspi_application_new (const gchar *bus_name)
{
  AtspiApplication *application;
  AtspiApplicationPrivate *priv;
  
  application = g_object_new (ATSPI_TYPE_APPLICATION, NULL);
  application->bus_name = g_strdup (bus_name);
  application->root = NULL;

  priv = atspi_application_get_instance_private (application);
  priv->application_proxy =
    a11y_atspi_application_proxy_new_sync (_atspi_bus (), G_DBUS_PROXY_FLAGS_NONE,
                                           bus_name, atspi_path_root,
                                           NULL, NULL);
  priv->watch_id =
    g_bus_watch_name_on_connection (_atspi_bus (), bus_name,
                                    G_BUS_NAME_WATCHER_FLAGS_NONE,
                                    NULL, handle_name_vanished, application, NULL);
  a11y_atspi_application_call_get_application_bus_address
    (priv->application_proxy,
     NULL, handle_get_bus_address, application);

  return application;
}
