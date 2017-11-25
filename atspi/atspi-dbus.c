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

#include <config.h>
#include <dbus/dbus.h>
#include <glib.h>

#include "atspi-misc-private.h"

static DBusConnection *a11y_bus;
static dbus_int32_t a11y_dbus_slot = -1;

static void
a11y_bus_free (void *data)
{
  if (data == a11y_bus)
    {
      a11y_bus = NULL;
      dbus_connection_free_data_slot (&a11y_dbus_slot);
    }
}

/**
 * atspi_get_a11y_bus: (skip)
 */
DBusConnection *
atspi_get_a11y_bus (void)
{
  DBusError error;
  char *address = NULL;

  if (a11y_bus && dbus_connection_get_is_connected (a11y_bus))
    return a11y_bus;

  if (a11y_dbus_slot == -1)
    if (!dbus_connection_allocate_data_slot (&a11y_dbus_slot))
      g_warning ("at-spi: Unable to allocate D-Bus slot");

  address = _atspi_dbus_get_address ();

  dbus_error_init (&error);
  a11y_bus = dbus_connection_open_private (address, &error);
  g_free (address);

  if (!a11y_bus)
    {
      if (!g_getenv("SSH_CONNECTION"))
        g_warning ("Couldn't connect to accessibility bus: %s", error.message);
      dbus_error_free (&error);
      return NULL;
    }
  else
    {
      if (!dbus_bus_register (a11y_bus, &error))
	{
	  g_warning ("Couldn't register with accessibility bus: %s", error.message);
          dbus_error_free (&error);
          dbus_connection_close (a11y_bus);
          dbus_connection_unref (a11y_bus);
          a11y_bus = NULL;
	  return NULL;
	}
    }
  
  /* Simulate a weak ref on the bus */
  dbus_connection_set_data (a11y_bus, a11y_dbus_slot, a11y_bus, a11y_bus_free);

  return a11y_bus;
}

void
_atspi_dbus_cleanup (void)
{
  if (a11y_bus)
    {
      dbus_connection_close (a11y_bus);
      dbus_connection_unref (a11y_bus);
      a11y_bus = NULL;
    }
}
