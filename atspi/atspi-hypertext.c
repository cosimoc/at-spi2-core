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

#include "atspi-private.h"

#include "xml/a11y-atspi-hypertext.h"

static A11yAtspiHypertext *
get_hypertext_proxy (AtspiHypertext *hypertext)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (hypertext), (AtspiAccessibleProxyInit) a11y_atspi_hypertext_proxy_new_sync,
     "a11y-atspi-hypertext-proxy");
}

/**
 * atspi_hypertext_get_n_links:
 * @obj: a pointer to the #AtspiHypertext implementor on which to operate.
 *
 * Gets the total number of #AtspiHyperlink objects that an
 * #AtspiHypertext implementor has.
 *
 * Returns: a #gint indicating the number of #AtspiHyperlink objects
 *        of the #AtspiHypertext implementor, or -1 if
 *        the number cannot be determined (for example, if the
 *        #AtspiHypertext object is so large that it is not
 *        all currently in the memory cache).
 **/
gint
atspi_hypertext_get_n_links (AtspiHypertext *obj, GError **error)
{
  gint retval = 0;

  g_return_val_if_fail (obj != NULL, 0);

  a11y_atspi_hypertext_call_get_nlinks_sync (get_hypertext_proxy (obj),
                                             &retval, NULL, error);
  return retval;
}

/**
 * atspi_hypertext_get_link:
 * @obj: a pointer to the #AtspiHypertext implementor on which to operate.
 * @link_index: a (zero-index) #gint indicating which hyperlink to query.
 *
 * Gets the #AtspiHyperlink object at a specified index.
 *
 * Returns: (nullable) (transfer full): the #AtspiHyperlink object
 *          specified by @link_index.
 **/
AtspiHyperlink *
atspi_hypertext_get_link (AtspiHypertext *obj, gint link_index, GError **error)
{
  GVariant *variant = NULL;
  AtspiHyperlink *hyperlink = NULL;
	
  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_hypertext_call_get_link_sync (get_hypertext_proxy (obj),
                                               link_index,
                                               &variant, NULL, error))
    {
      hyperlink = _atspi_dbus_return_hyperlink_from_variant (variant);
      g_variant_unref (variant);
    }

  return hyperlink;
}

/**
 * atspi_hypertext_get_link_index:
 * @obj: a pointer to the #AtspiHypertext implementor on which to operate.
 * @character_offset: a #gint specifying the character offset to query.
 *
 * Gets the index of the #AtspiHyperlink object at a specified
 *        character offset.
 *
 * Returns: the linkIndex of the #AtspiHyperlink active at
 *        character offset @character_offset, or -1 if there is
 *        no hyperlink at the specified character offset.
 **/
int
atspi_hypertext_get_link_index (AtspiHypertext *obj,
                                gint             character_offset,
                                GError **error)
{
  gint retval = -1;

  g_return_val_if_fail (obj != NULL, 0);

  a11y_atspi_hypertext_call_get_link_index_sync (get_hypertext_proxy (obj),
                                                 character_offset,
                                                 &retval, NULL, error);
  return retval;
}

static void
atspi_hypertext_base_init (AtspiHypertext *klass)
{
}

GType
atspi_hypertext_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiHypertext),
      (GBaseInitFunc) atspi_hypertext_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiHypertext", &tinfo, 0);

  }
  return type;
}
