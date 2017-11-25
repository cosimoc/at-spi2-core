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

#include "xml/a11y-atspi-image.h"

static A11yAtspiImage *
get_image_proxy (AtspiImage *image)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (image), (AtspiAccessibleProxyInit) a11y_atspi_image_proxy_new_sync,
     "a11y-atspi-image-proxy");
}

/**
 * atspi_image_get_image_description:
 * @obj: a pointer to the #AtspiImage implementor on which to operate.
 *
 * Gets the description of the image displayed in an #AtspiImage object.
 *
 * Returns: a UTF-8 string describing the image.
 **/
gchar *
atspi_image_get_image_description (AtspiImage *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, NULL);

  return a11y_atspi_image_dup_image_description (get_image_proxy (obj));
}

/**
 * atspi_image_get_image_size:
 * @obj: a pointer to the #AtspiImage to query.
 *
 * Gets the size of the image displayed in a specified #AtspiImage object.
 *
 * Returns: a pointer to an #AtspiPoint where x corresponds to
 * the image's width and y corresponds to the image's height.
 *
 **/
AtspiPoint *
atspi_image_get_image_size (AtspiImage *obj, GError **error)
{
  AtspiPoint ret;

  ret.x = ret.y = -1;
  g_return_val_if_fail (obj != NULL, atspi_point_copy (&ret));

  a11y_atspi_image_call_get_image_size_sync (get_image_proxy (obj),
                                             &ret.x, &ret.y,
                                             NULL, error);
  return atspi_point_copy (&ret);
}

/**
 * atspi_image_get_image_position:
 * @obj: a pointer to the #AtspiImage implementor to query.
 * @ctype: the desired coordinate system into which to return the results,
 *         (e.g. ATSPI_COORD_TYPE_WINDOW, ATSPI_COORD_TYPE_SCREEN).
 *
 * Gets the minimum x and y coordinates of the image displayed in a
 *         specified #AtspiImage implementor.
 *
 * Returns: a pointer to an #AtspiPoint where x and y correspond to the
 * minimum coordinates of the displayed image. 
 * 
 **/
AtspiPoint *
atspi_image_get_image_position (AtspiImage *obj,
                                AtspiCoordType ctype,
                                GError **error)
{
  AtspiPoint ret;

  ret.x = ret.y = 0;
  g_return_val_if_fail (obj != NULL, atspi_point_copy (&ret));

  a11y_atspi_image_call_get_image_position_sync (get_image_proxy (obj),
                                                 ctype,
                                                 &ret.x, &ret.y,
                                                 NULL, error);
  return atspi_point_copy (&ret);
}

/**
 * atspi_image_get_image_extents:
 * @obj: a pointer to the #AtspiImage implementor to query.
 * @ctype: the desired coordinate system into which to return the results,
 *         (e.g. ATSPI_COORD_TYPE_WINDOW, ATSPI_COORD_TYPE_SCREEN).
 *
 * Gets the bounding box of the image displayed in a
 *         specified #AtspiImage implementor.
 *
 * Returns: a pointer to an #AtspiRect corresponding to the image's bounding box. The minimum x and y coordinates, 
 * width, and height are specified.
 **/
AtspiRect *
atspi_image_get_image_extents (AtspiImage *obj,
			       AtspiCoordType ctype,
			       GError **error)
{
  GVariant *variant = NULL;
  AtspiRect bbox;

  bbox.x = bbox.y = bbox.width = bbox.height = -1;
  g_return_val_if_fail (obj != NULL, atspi_rect_copy (&bbox));

  if (a11y_atspi_image_call_get_image_extents_sync (get_image_proxy (obj),
                                                    ctype,
                                                    &variant, NULL, error))
    {
      g_variant_get (variant, "(iiii)", &bbox.x, &bbox.y,
                     &bbox.width, &bbox.height);
      g_variant_unref (variant);
    }

  return atspi_rect_copy (&bbox);
}

/**
 * atspi_image_get_image_locale:
 * @obj: a pointer to the #AtspiImage to query.
 *
 * Gets the locale associated with an image and its textual representation.
 *
 * Returns: A POSIX LC_MESSAGES-style locale value for image description and text.
 **/
gchar *
atspi_image_get_image_locale  (AtspiImage *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, g_strdup ("C"));

  return a11y_atspi_image_dup_image_locale (get_image_proxy (obj));
}

static void
atspi_image_base_init (AtspiImage *klass)
{
}

GType
atspi_image_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiImage),
      (GBaseInitFunc) atspi_image_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiImage", &tinfo, 0);

  }
  return type;
}
