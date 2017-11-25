/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2002 Ximian, Inc.
 *           2002 Sun Microsystems Inc.
 * Copyright 2010, 2011 Novell, Inc.
 *
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

#ifndef _ATSPI_ACCESSIBLE_PRIVATE_H_
#define _ATSPI_ACCESSIBLE_PRIVATE_H_

G_BEGIN_DECLS

#include "atspi-accessible.h"

#include "xml/a11y-atspi-accessible.h"

struct _AtspiAccessiblePrivate
{
  GHashTable *cache;
  guint cache_ref_count;
  A11yAtspiAccessible *accessible_proxy;
};

GHashTable *
_atspi_accessible_ref_cache (AtspiAccessible *accessible);

void
_atspi_accessible_unref_cache (AtspiAccessible *accessible);

GVariant *
_atspi_accessible_get_state (AtspiAccessible *accessible);

A11yAtspiAccessible *
_atspi_accessible_get_accessible_proxy (AtspiAccessible *accessible);

typedef gpointer (* AtspiAccessibleProxyInit) (GDBusConnection *bus,
                                               GDBusProxyFlags flags,
                                               const char *bus_name,
                                               const char *object_path,
                                               GCancellable *cancellable,
                                               GError **error);
gpointer
atspi_accessible_get_iface_proxy (AtspiAccessible *accessible,
                                  AtspiAccessibleProxyInit init_func,
                                  const char *key);

G_END_DECLS

#endif	/* _ATSPI_ACCESSIBLE_H_ */
