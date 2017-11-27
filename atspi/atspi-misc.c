/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2001, 2002 Sun Microsystems Inc.,
 * Copyright 2001, 2002 Ximian, Inc.
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

/*
 *
 * Basic SPI initialization and event loop function prototypes
 *
 */

#include "atspi-private.h"
#ifdef HAVE_X11
#include "X11/Xlib.h"
#endif
#include <stdio.h>
#include <string.h>

#include "xml/a11y-atspi-registry.h"

static GDBusConnection *bus = NULL;
static GHashTable *live_refs = NULL;
static gboolean allow_sync = TRUE;

GMainLoop *atspi_main_loop;
GMainContext *atspi_main_context;
gboolean atspi_no_cache;

const char *atspi_path_dec = ATSPI_DBUS_PATH_DEC;
const char *atspi_path_registry = ATSPI_DBUS_PATH_REGISTRY;
const char *atspi_path_root = ATSPI_DBUS_PATH_ROOT;
const char *atspi_bus_registry = ATSPI_DBUS_NAME_REGISTRY;
const char *atspi_interface_accessible = ATSPI_DBUS_INTERFACE_ACCESSIBLE;
const char *atspi_interface_action = ATSPI_DBUS_INTERFACE_ACTION;
const char *atspi_interface_application = ATSPI_DBUS_INTERFACE_APPLICATION;
const char *atspi_interface_collection = ATSPI_DBUS_INTERFACE_COLLECTION;
const char *atspi_interface_component = ATSPI_DBUS_INTERFACE_COMPONENT;
const char *atspi_interface_dec = ATSPI_DBUS_INTERFACE_DEC;
const char *atspi_interface_device_event_listener = ATSPI_DBUS_INTERFACE_DEVICE_EVENT_LISTENER;
const char *atspi_interface_document = ATSPI_DBUS_INTERFACE_DOCUMENT;
const char *atspi_interface_editable_text = ATSPI_DBUS_INTERFACE_EDITABLE_TEXT;
const char *atspi_interface_event_object = ATSPI_DBUS_INTERFACE_EVENT_OBJECT;
const char *atspi_interface_hyperlink = ATSPI_DBUS_INTERFACE_HYPERLINK;
const char *atspi_interface_hypertext = ATSPI_DBUS_INTERFACE_HYPERTEXT;
const char *atspi_interface_image = ATSPI_DBUS_INTERFACE_IMAGE;
const char *atspi_interface_registry = ATSPI_DBUS_INTERFACE_REGISTRY;
const char *atspi_interface_selection = ATSPI_DBUS_INTERFACE_SELECTION;
const char *atspi_interface_table = ATSPI_DBUS_INTERFACE_TABLE;
const char *atspi_interface_table_cell = ATSPI_DBUS_INTERFACE_TABLE_CELL;
const char *atspi_interface_text = ATSPI_DBUS_INTERFACE_TEXT;
const char *atspi_interface_cache = ATSPI_DBUS_INTERFACE_CACHE;
const char *atspi_interface_value = ATSPI_DBUS_INTERFACE_VALUE;

static const char *interfaces[] =
{
  ATSPI_DBUS_INTERFACE_ACCESSIBLE,
  ATSPI_DBUS_INTERFACE_ACTION,
  ATSPI_DBUS_INTERFACE_APPLICATION,
  ATSPI_DBUS_INTERFACE_COLLECTION,
  ATSPI_DBUS_INTERFACE_COMPONENT,
  ATSPI_DBUS_INTERFACE_DOCUMENT,
  ATSPI_DBUS_INTERFACE_EDITABLE_TEXT,
  ATSPI_DBUS_INTERFACE_HYPERLINK,
  ATSPI_DBUS_INTERFACE_HYPERTEXT,
  ATSPI_DBUS_INTERFACE_IMAGE,
  "org.a11y.atspi.LoginHelper",
  ATSPI_DBUS_INTERFACE_SELECTION,
  ATSPI_DBUS_INTERFACE_TABLE,
  ATSPI_DBUS_INTERFACE_TABLE_CELL,
  ATSPI_DBUS_INTERFACE_TEXT,
  ATSPI_DBUS_INTERFACE_VALUE,
  NULL
};

gint
_atspi_get_iface_num (const char *iface)
{
  /* TODO: Use a binary search or hash to improve performance */
  int i;

  for (i = 0; interfaces[i]; i++)
  {
    if (!strcmp(iface, interfaces[i])) return i;
  }
  return -1;
}

GHashTable *
_atspi_get_live_refs (void)
{
  if (!live_refs) 
    {
      live_refs = g_hash_table_new (g_direct_hash, g_direct_equal);
    }
  return live_refs;
}

/* TODO: Add an application parameter */
GDBusConnection *
_atspi_bus ()
{
  if (!bus)
    atspi_init ();
  if (!bus)
    g_error ("AT-SPI: Couldn't connect to accessibility bus. Is at-spi-bus-launcher running?");
  return bus;
}

#define APP_IS_REGISTRY(app) (!strcmp (app->bus_name, atspi_bus_registry))

static AtspiAccessible *desktop;

static void
cleanup ()
{
  GHashTable *refs;
  gint i;

  refs = live_refs;
  live_refs = NULL;
  if (refs)
    {
      g_hash_table_destroy (refs);
    }

  if (bus)
    {
      g_dbus_connection_close (bus, NULL, NULL, NULL);
      g_clear_object (&bus);
    }

  _atspi_dbus_cleanup ();

  if (!desktop)
    return;

  /* TODO: Do we need this code, or should we just dispose the desktop? */
  for (i = desktop->children->len - 1; i >= 0; i--)
  {
    AtspiAccessible *child = g_ptr_array_index (desktop->children, i);
    g_object_run_dispose (G_OBJECT (child->parent.app));
    g_object_run_dispose (G_OBJECT (child));
  }

  g_object_run_dispose (G_OBJECT (desktop->parent.app));
  g_object_unref (desktop);
  desktop = NULL;
}

static gboolean atspi_inited = FALSE;

static GHashTable *app_hash = NULL;

static AtspiApplication *
_atspi_application_get (const char *bus_name)
{
  AtspiApplication *app = NULL;
  char *bus_name_dup;

  if (!app_hash)
  {
    app_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_object_unref);
    if (!app_hash) return NULL;
  }
  app = g_hash_table_lookup (app_hash, bus_name);
  if (app) return app;
  bus_name_dup = g_strdup (bus_name);
  if (!bus_name_dup) return NULL;
  // TODO: change below to something that will send state-change:defunct notification if necessary */
  app = _atspi_application_new (bus_name);
  g_hash_table_insert (app_hash, bus_name_dup, app);

  return app;
}

static AtspiAccessible *
ref_accessible (const char *app_name, const char *path)
{
  AtspiApplication *app;
  AtspiAccessible *a;

  if (!strcmp (path, ATSPI_DBUS_PATH_NULL))
    return NULL;

  app = _atspi_application_get (app_name);

  if (!strcmp (path, "/org/a11y/atspi/accessible/root"))
  {
    if (!app->root)
    {
      app->root = _atspi_accessible_new (app, atspi_path_root);
      app->root->accessible_parent = atspi_get_desktop (0);
      g_ptr_array_add (app->root->accessible_parent->children, g_object_ref (app->root));
    }
    return g_object_ref (app->root);
  }

  a = g_hash_table_lookup (app->hash, path);
  if (a)
  {
    return g_object_ref (a);
  }
  a = _atspi_accessible_new (app, path);
  if (!a)
    return NULL;
  g_hash_table_insert (app->hash, g_strdup (a->parent.path), g_object_ref (a));
  return a;
}

static AtspiHyperlink *
ref_hyperlink (const char *app_name, const char *path)
{
  AtspiApplication *app = _atspi_application_get (app_name);
  AtspiHyperlink *hyperlink;

  if (!strcmp (path, ATSPI_DBUS_PATH_NULL))
    return NULL;

  hyperlink = g_hash_table_lookup (app->hash, path);
  if (hyperlink)
  {
    return g_object_ref (hyperlink);
  }
  hyperlink = _atspi_hyperlink_new (app, path);
  g_hash_table_insert (app->hash, g_strdup (hyperlink->parent.path), hyperlink);
  /* TODO: This should be a weak ref */
  g_object_ref (hyperlink);	/* for the hash */
  return hyperlink;
}

static gboolean
add_app_to_desktop (AtspiAccessible *a, const char *bus_name)
{
  AtspiAccessible *obj = ref_accessible (bus_name, atspi_path_root);
  /* The app will be added to the desktop as a side-effect of calling
   * ref_accessible */
  g_object_unref (obj);
  return (obj != NULL);
}

void
get_reference_from_iter (DBusMessageIter *iter, const char **app_name, const char **path)
{
  DBusMessageIter iter_struct;

  dbus_message_iter_recurse (iter, &iter_struct);
  dbus_message_iter_get_basic (&iter_struct, app_name);
  dbus_message_iter_next (&iter_struct);
  dbus_message_iter_get_basic (&iter_struct, path);
  dbus_message_iter_next (iter);
}

/* TODO: Do we stil need this function? */
static AtspiAccessible *
ref_accessible_desktop (AtspiApplication *app)
{
  GVariantIter iter;
  GVariant *variant = NULL;
  GError *error = NULL;
  gchar *bus_name_dup;
  A11yAtspiAccessible *accessible_proxy;
  const char *app_name, *path;

  if (desktop)
  {
    g_object_ref (desktop);
    return desktop;
  }
  desktop = _atspi_accessible_new (app, atspi_path_root);
  if (!desktop)
  {
    return NULL;
  }
  g_hash_table_insert (app->hash, g_strdup (desktop->parent.path),
                       g_object_ref (desktop));
  app->root = g_object_ref (desktop);
  desktop->name = g_strdup ("main");

  accessible_proxy = _atspi_accessible_get_accessible_proxy (desktop);
  if (!a11y_atspi_accessible_call_get_children_sync (accessible_proxy,
                                                     &variant, NULL, &error))
    {
      g_warning ("Couldn't get application list: %s", error->message);
      g_clear_error (&error);
      return NULL;
    }

  g_variant_iter_init (&iter, variant);
  while (g_variant_iter_loop (&iter, "(so)", &app_name, &path))
    add_app_to_desktop (desktop, app_name);

  /* Record the alternate name as an alias for org.a11y.atspi.Registry */
  bus_name_dup = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (accessible_proxy));
  if (bus_name_dup)
    g_hash_table_insert (app_hash, bus_name_dup, app);

  g_variant_unref (variant);

  return g_object_ref (desktop);
}

AtspiAccessible *
_atspi_ref_accessible (const char *app, const char *path)
{
  AtspiApplication *a = _atspi_application_get (app);
  if (!a)
    return NULL;
  if ( APP_IS_REGISTRY(a))
  {
    if (!a->root)
      g_object_unref (ref_accessible_desktop (a));	/* sets a->root */
    return g_object_ref (a->root);
  }
  return ref_accessible (app, path);
}

AtspiAccessible *
_atspi_dbus_return_accessible_from_variant (GVariant *variant)
{
  const char *app_name, *path;

  g_variant_get (variant, "(&s&o)", &app_name, &path);
  return ref_accessible (app_name, path);
}

AtspiHyperlink *
_atspi_dbus_return_hyperlink_from_variant (GVariant *variant)
{
  const char *app_name, *path;

  g_variant_get (variant, "(&s&o)", &app_name, &path);
  return ref_hyperlink (app_name, path);
}

static void
handle_registry_name_owner_changed (A11yAtspiRegistry *registry_proxy)
{
  gchar *name = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (registry_proxy));

  if (name)
    {
      _atspi_reregister_event_listeners ();
      _atspi_reregister_device_listeners ();
    }

  g_free (name);
}

/*
 * Returns a 'canonicalized' value for DISPLAY,
 * with the screen number stripped off if present.
 *
 * TODO: Avoid having duplicate functions for this here and in at-spi2-atk
 */
static gchar *
spi_display_name (void)
{
  char *canonical_display_name = NULL;
  const gchar *display_env = g_getenv ("AT_SPI_DISPLAY");

  if (!display_env)
    {
      display_env = g_getenv ("DISPLAY");
      if (!display_env || !display_env[0])
        return NULL;
      else
        {
          gchar *display_p, *screen_p;
          canonical_display_name = g_strdup (display_env);
          display_p = g_utf8_strrchr (canonical_display_name, -1, ':');
          screen_p = g_utf8_strrchr (canonical_display_name, -1, '.');
          if (screen_p && display_p && (screen_p > display_p))
            {
              *screen_p = '\0';
            }
        }
    }
  else
    {
      canonical_display_name = g_strdup (display_env);
    }

  return canonical_display_name;
}

/**
 * atspi_init:
 *
 * Connects to the accessibility registry and initializes the SPI.
 *
 * Returns: 0 on success, 1 if already initialized, or an integer error code.  
 **/
int
atspi_init (void)
{
  const gchar *no_cache;
  A11yAtspiRegistry *registry_proxy;

  if (atspi_inited)
    {
      return 1;
    }

  atspi_inited = TRUE;

  _atspi_get_live_refs();

  bus = atspi_get_a11y_gdbus ();
  if (!bus)
    return 2;

  _atspi_register_default_event_listeners ();
  registry_proxy = a11y_atspi_registry_proxy_new_sync (bus, G_DBUS_PROXY_FLAGS_NONE,
                                                       atspi_bus_registry,
                                                       atspi_path_registry,
                                                       NULL, NULL);
  g_signal_connect (registry_proxy, "notify::g-name-owner",
                    G_CALLBACK (handle_registry_name_owner_changed), NULL);

  no_cache = g_getenv ("ATSPI_NO_CACHE");
  if (no_cache && g_strcmp0 (no_cache, "0") != 0)
    atspi_no_cache = TRUE;

  return 0;
}

/**
 * atspi_is_initialized:
 *
 * Indicates whether AT-SPI has been initialized.
 *
 * Returns: %True if initialized; %False otherwise.
 */
gboolean
atspi_is_initialized ()
{
  return atspi_inited;
}

/**
 * atspi_event_main:
 *
 * Starts/enters the main event loop for the AT-SPI services.
 *
 * NOTE: This method does not return control; it is exited via a call to
 * #atspi_event_quit from within an event handler.
 *
 **/
void
atspi_event_main (void)
{
  atspi_main_loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (atspi_main_loop);
  atspi_main_loop = NULL;
}

/**
 * atspi_event_quit:
 *
 * Quits the last main event loop for the AT-SPI services,
 * See: #atspi_event_main
 **/
void
atspi_event_quit (void)
{
  g_main_loop_quit (atspi_main_loop);
}

/**
 * atspi_exit:
 *
 * Disconnects from #AtspiRegistry instances and releases 
 * any floating resources. Call only once at exit.
 *
 * Returns: 0 if there were no leaks, otherwise other integer values.
 **/
int
atspi_exit (void)
{
  int leaked;

  if (!atspi_inited)
    {
      return 0;
    }

  atspi_inited = FALSE;

  if (live_refs)
    {
      leaked = g_hash_table_size (live_refs);
    }
  else
    {
      leaked = 0;
    }

  cleanup ();

  return leaked;
}

GHashTable *
_atspi_dbus_return_hash_from_variant (GVariant *variant)
{
  GVariantIter iter;
  char *name = NULL, *value = NULL;
  GHashTable *ret = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           (GDestroyNotify) g_free,
                                           (GDestroyNotify) g_free);

  g_variant_iter_init (&iter, variant);
  while (g_variant_iter_next (&iter, "{ss}", &name, &value))
    g_hash_table_insert (ret, name, value);

  return ret;
}

GArray *
_atspi_dbus_return_attribute_array_from_variant (GVariant *variant)
{
  GVariantIter iter;
  const gchar *name, *value;
  GArray *ret = g_array_new (TRUE, TRUE, sizeof (gchar *));

  g_variant_iter_init (&iter, variant);
  while (g_variant_iter_loop (&iter, "{ss}", &name, &value))
    {
      gchar *str = g_strdup_printf ("%s:%s", name, value);
      ret = g_array_append_val (ret, str);
    }

  return ret;
}

void
_atspi_dbus_set_interfaces (AtspiAccessible *accessible, const gchar **interfaces)
{
  gint idx;

  accessible->interfaces = 0;

  for (idx = 0; interfaces[idx] != NULL; idx++)
    {
      const char *iface = interfaces[idx];
      gint n;

      if (strcmp (iface, "org.freedesktop.DBus.Introspectable") == 0)
        continue;

      n = _atspi_get_iface_num (iface);
      if (n == -1)
        g_warning ("AT-SPI: Unknown interface %s", iface);
      else
        accessible->interfaces |= (1 << n);
  }
  _atspi_accessible_add_cache (accessible, ATSPI_CACHE_INTERFACES);
}

void
_atspi_dbus_set_state (AtspiAccessible *accessible, GVariant *variant)
{
  const guint32 *states;
  gsize count;

  states = g_variant_get_fixed_array (variant, &count, sizeof (guint32));
  if (count != 2)
  {
    g_warning ("AT-SPI: expected 2 values in states array; got %" G_GSIZE_FORMAT "\n", count);
    if (!accessible->states)
      accessible->states = _atspi_state_set_new_internal (accessible, 0);
  }
  else
  {
    guint64 val = ((guint64)states [1]) << 32;
    val += states [0];
    if (!accessible->states)
      accessible->states = _atspi_state_set_new_internal (accessible, val);
    else
      accessible->states->states = val;
  }
  _atspi_accessible_add_cache (accessible, ATSPI_CACHE_STATES);
}

GQuark
_atspi_error_quark (void)
{
  return g_quark_from_static_string ("atspi_error");
}

/*
 * Gets the IOR from the XDisplay.
 */
#ifdef HAVE_X11
static char *
get_accessibility_bus_address_x11 (void)
{
  Atom AT_SPI_BUS;
  Atom actual_type;
  Display *bridge_display = NULL;
  int actual_format;
  char *data;
  unsigned char *data_x11 = NULL;
  unsigned long nitems;
  unsigned long leftover;
  char *display_name;

  display_name = spi_display_name ();
  if (!display_name)
    return NULL;

  bridge_display = XOpenDisplay (display_name);
  g_free (display_name);

  if (!bridge_display)
    {
      g_warning ("Could not open X display");
      return NULL;
    }
      
  AT_SPI_BUS = XInternAtom (bridge_display, "AT_SPI_BUS", False);
  XGetWindowProperty (bridge_display,
		      XDefaultRootWindow (bridge_display),
		      AT_SPI_BUS, 0L,
		      (long) BUFSIZ, False,
		      (Atom) 31, &actual_type, &actual_format,
		      &nitems, &leftover, &data_x11);
  XCloseDisplay (bridge_display);

  data = g_strdup ((gchar *)data_x11);
  XFree (data_x11);
  return data;
}
#endif

static char *
get_accessibility_bus_address_dbus (void)
{
  DBusConnection *session_bus = NULL;
  DBusMessage *message;
  DBusMessage *reply;
  DBusError error;
  char *address = NULL;

  session_bus = dbus_bus_get (DBUS_BUS_SESSION, NULL);
  if (!session_bus)
    return NULL;

  message = dbus_message_new_method_call ("org.a11y.Bus",
					  "/org/a11y/bus",
					  "org.a11y.Bus",
					  "GetAddress");

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (session_bus,
						     message,
						     -1,
						     &error);
  dbus_message_unref (message);

  if (!reply)
  {
    g_warning ("Error retrieving accessibility bus address: %s: %s",
               error.name, error.message);
    dbus_error_free (&error);
    goto out;
  }
  
  {
    const char *tmp_address;
    if (!dbus_message_get_args (reply,
				NULL,
				DBUS_TYPE_STRING,
				&tmp_address,
				DBUS_TYPE_INVALID))
      {
	dbus_message_unref (reply);
        goto out;
      }
    address = g_strdup (tmp_address);
    dbus_message_unref (reply);
  }
  
out:
  dbus_connection_unref (session_bus);
  return address;
}

static GDBusConnection *a11y_bus;

char *
_atspi_dbus_get_address (void)
{
  char *address = NULL;
  const char *address_env = NULL;

  address_env = g_getenv ("AT_SPI_BUS_ADDRESS");
  if (address_env != NULL && *address_env != 0)
    address = g_strdup (address_env);
#ifdef HAVE_X11
  if (!address)
    address = get_accessibility_bus_address_x11 ();
#endif
  if (!address)
    address = get_accessibility_bus_address_dbus ();

  return address;
}

/**
 * atspi_get_a11y_gdbus: (skip)
 */
GDBusConnection *
atspi_get_a11y_gdbus (void)
{
  GError *error = NULL;
  char *address = NULL;

  if (a11y_bus && !g_dbus_connection_is_closed (a11y_bus))
    return a11y_bus;

  address = _atspi_dbus_get_address ();
  if (!address)
    return NULL;

  a11y_bus = g_dbus_connection_new_for_address_sync (address, G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                     G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                     NULL, NULL, &error);
  g_free (address);

  if (!a11y_bus)
    {
      if (!g_getenv("SSH_CONNECTION"))
        g_warning ("Couldn't connect to accessibility bus: %s", error->message);
      g_error_free (error);
      return NULL;
    }
  
  /* Simulate a weak ref on the bus */
  g_object_add_weak_pointer (G_OBJECT (a11y_bus), (gpointer *) &a11y_bus);

  return a11y_bus;
}

/**
 * atspi_set_timeout:
 * @val: The timeout value, in milliseconds, or -1 to disable the timeout.
 * @startup_time: The amount of time, in milliseconds, to allow to pass
 * before enforcing timeouts on an application. Can be used to prevent
 * timeout exceptions if an application is likely to block for an extended
 * period of time on initialization. -1 can be passed to disable this
 * behavior.
 *
 * Set the timeout used for method calls. If this is not set explicitly,
 * a default of 0.8 ms is used.
 * Note that at-spi2-registryd currently uses a timeout of 3 seconds when
 * sending a keyboard event notification. This means that, if an AT makes
 * a call in response to the keyboard notification and the application
 * being called does not respond before the timeout is reached,
 * at-spi2-registryd will time out on the keyboard event notification and
 * pass the key onto the application (ie, reply to indicate that the key
 * was not consumed), so this may make it undesirable to set a timeout
 * larger than 3 seconds.
 *
 * By default, the normal timeout is set to 800 ms, and the application startup
 * timeout is set to 15 seconds.
 *
 * Deprecated: 2.28: this function does nothing
 */
void
atspi_set_timeout (gint val, gint startup_time)
{
}

/**
 * atspi_set_main_context:
 * @cnx: The #GMainContext to use.
 *
 * Sets the main loop context that AT-SPI should assume is in use when
 * setting an idle callback.
 * This function should be called by application-side implementors (ie,
 * at-spi2-atk) when it is desirable to re-enter the main loop.
 */
void
atspi_set_main_context (GMainContext *cnx)
{
  if (atspi_main_context == cnx)
    return;
  atspi_main_context = cnx;
}

#ifdef DEBUG_REF_COUNTS
static void
print_disposed (gpointer key, gpointer value, gpointer data)
{
  AtspiAccessible *accessible = key;
  if (accessible->parent.app)
    return;
  g_print ("disposed: %s %d\n", accessible->name, accessible->role);
}

void
debug_disposed ()
{
  g_hash_table_foreach (live_refs, print_disposed, NULL);
}
#endif

gchar *
_atspi_name_compat (gchar *name)
{
  gchar *p = name;

  while (*p)
  {
    if (*p == '-')
      *p = ' ';
    p++;
  }
  return name;
}

/**
 * atspi_role_get_name:
 * @role: an #AtspiRole object to query.
 *
 * Gets a localizable string that indicates the name of an #AtspiRole.
 * <em>DEPRECATED.</em>
 *
 * Returns: a localizable string name for an #AtspiRole enumerated type.
 **/
gchar *
atspi_role_get_name (AtspiRole role)
{
  gchar *retval = NULL;
  GTypeClass *type_class;
  GEnumValue *value;

  type_class = g_type_class_ref (ATSPI_TYPE_ROLE);
  g_return_val_if_fail (G_IS_ENUM_CLASS (type_class), NULL);

  value = g_enum_get_value (G_ENUM_CLASS (type_class), role);

  if (value)
    {
      retval = g_strdup (value->value_nick);
    }

  g_type_class_unref (type_class);

  if (retval)
    return _atspi_name_compat (retval);

  return NULL;
}

void
_atspi_dbus_update_cache_from_variant (AtspiAccessible *accessible, GVariant *variant)
{
  GHashTable *cache = _atspi_accessible_ref_cache (accessible);
  GVariantIter iter;
  const char *key;
  GVariant *value;

  g_variant_iter_init (&iter, variant);
  while (g_variant_iter_loop (&iter, "{sv}", &key, &value))
    {
      GValue *val = NULL;

      if (!strcmp (key, "interfaces"))
        {
          _atspi_dbus_set_interfaces (accessible, g_variant_get_strv (value, NULL));
        }
      else if (!strcmp (key, "Attributes"))
        {
          if (!g_variant_is_of_type (value, G_VARIANT_TYPE ("a{ss}")))
            continue;
          val = g_new0 (GValue, 1);
          g_value_init (val, G_TYPE_HASH_TABLE);
          g_value_take_boxed (val, _atspi_dbus_return_hash_from_variant (value));
        }
      else if (!strcmp (key, "Component.ScreenExtents"))
        {
          AtspiRect extents;

          if (!g_variant_is_of_type (value, G_VARIANT_TYPE ("(iiii")))
            continue;
          g_variant_get (value, "(iiii)", &extents.x, &extents.y,
                         &extents.width, &extents.height);
          val = g_new0 (GValue, 1);;
          g_value_init (val, ATSPI_TYPE_RECT);
          g_value_set_boxed (val, &extents);
        }
      if (val)
        g_hash_table_insert (cache, g_strdup (key), val);
    }

  _atspi_accessible_unref_cache (accessible);
}

gboolean
_atspi_get_allow_sync ()
{
  return allow_sync;
}

gboolean
_atspi_set_allow_sync (gboolean val)
{
  gboolean ret = allow_sync;

  allow_sync = val;
  return ret;
}

void
_atspi_set_error_no_sync (GError **error)
{
  g_set_error_literal (error, ATSPI_ERROR, ATSPI_ERROR_SYNC_NOT_ALLOWED,
                        _("Attempted synchronous call where prohibited"));
}
