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

/*
 *
 * AtspiComponent function implementations
 *
 */

#include "atspi-private.h"
#include "atspi-accessible-private.h"

#include "xml/a11y-atspi-component.h"

static A11yAtspiComponent *
get_component_proxy (AtspiComponent *component)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (component), (AtspiAccessibleProxyInit) a11y_atspi_component_proxy_new_sync,
     "a11y-atspi-component-proxy");
}

void
atspi_rect_free (AtspiRect *rect)
{
  g_free (rect);
}

AtspiRect *
atspi_rect_copy (AtspiRect *src)
{
  AtspiRect *dst = g_new (AtspiRect, 1);
  dst->x = src->x;
  dst->y = src->y;
  dst->height = src->height;
  dst->width = src->width;
  return dst;
}

G_DEFINE_BOXED_TYPE (AtspiRect, atspi_rect, atspi_rect_copy, atspi_rect_free)

AtspiPoint *
atspi_point_copy (AtspiPoint *src)
{
  AtspiPoint *dst = g_new (AtspiPoint, 1);
  dst->x = src->x;
  dst->y = src->y;
  return dst;
}

G_DEFINE_BOXED_TYPE (AtspiPoint, atspi_point, atspi_point_copy, g_free)

/**
 * atspi_component_contains:
 * @obj: a pointer to the #AtspiComponent to query.
 * @x: a #gint specifying the x coordinate in question.
 * @y: a #gint specifying the y coordinate in question.
 * @ctype: the desired coordinate system of the point (@x, @y)
 *         (e.g. CSPI_COORD_TYPE_WINDOW, CSPI_COORD_TYPE_SCREEN).
 *
 * Queries whether a given #AtspiComponent contains a particular point.
 *
 * Returns: #TRUE if the specified component contains the point (@x, @y),
 *          #FALSE otherwise.
 **/
gboolean
atspi_component_contains (AtspiComponent *obj,
                              gint x,
                              gint y,
                              AtspiCoordType ctype, GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_component_call_contains_sync (get_component_proxy (obj), x, y, ctype,
                                           &retval, NULL, error);
  return retval;
}

/**
 * atspi_component_get_accessible_at_point:
 * @obj: a pointer to the #AtspiComponent to query.
 * @x: a #gint specifying the x coordinate of the point in question.
 * @y: a #gint specifying the y coordinate of the point in question.
 * @ctype: the coordinate system of the point (@x, @y)
 *         (e.g. ATSPI_COORD_TYPE_WINDOW, ATSPI_COORD_TYPE_SCREEN).
 *
 * Gets the accessible child at a given coordinate within an #AtspiComponent.
 *
 * Returns: (nullable) (transfer full): a pointer to an
 *          #AtspiAccessible child of the specified component which
 *          contains the point (@x, @y), or NULL if no child contains
 *          the point.
 **/
AtspiAccessible *
atspi_component_get_accessible_at_point (AtspiComponent *obj,
                                          gint x,
                                          gint y,
                                          AtspiCoordType ctype, GError **error)
{
  AtspiAccessible *accessible = NULL;
  GVariant *variant;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_component_call_get_accessible_at_point_sync (get_component_proxy (obj),
                                                              x, y, ctype,
                                                              &variant, NULL, error))
    {
      accessible = _atspi_dbus_return_accessible_from_variant (variant);
      g_variant_unref (variant);
    }

  return accessible;
}

/**
 * atspi_component_get_extents:
 * @obj: a pointer to the #AtspiComponent to query.
 * @ctype: the desired coordinate system into which to return the results,
 *         (e.g. ATSPI_COORD_TYPE_WINDOW, ATSPI_COORD_TYPE_SCREEN).
 *
 * Gets the bounding box of the specified #AtspiComponent.
 *
 * Returns: An #AtspiRect giving the accessible's extents.
 **/
AtspiRect *
atspi_component_get_extents (AtspiComponent *obj,
                                AtspiCoordType ctype, GError **error)
{
  AtspiRect bbox;
  AtspiAccessible *accessible;
  GVariant *variant;

  bbox.x = bbox.y = bbox.width = bbox.height = -1;
  g_return_val_if_fail (obj != NULL, atspi_rect_copy (&bbox));

  accessible = ATSPI_ACCESSIBLE (obj);
  if (accessible->priv->cache && ctype == ATSPI_COORD_TYPE_SCREEN)
  {
    GValue *val = g_hash_table_lookup (accessible->priv->cache, "Component.ScreenExtents");
    if (val)
    {
      return g_value_dup_boxed (val);
    }
  }

  if (a11y_atspi_component_call_get_extents_sync (get_component_proxy (obj), ctype,
                                                  &variant, NULL, error))
    {
      g_variant_get (variant, "(iiii)",
                     &bbox.x, &bbox.y, &bbox.width, &bbox.height);
      g_variant_unref (variant);
    }
  return atspi_rect_copy (&bbox);
}

/**
 * atspi_component_get_position:
 * @obj: a pointer to the #AtspiComponent to query.
 * @ctype: the desired coordinate system into which to return the results,
 *         (e.g. ATSPI_COORD_TYPE_WINDOW, ATSPI_COORD_TYPE_SCREEN).
 *
 * Gets the minimum x and y coordinates of the specified #AtspiComponent.
 *
 * returns: An #AtspiPoint giving the @obj's position.
 **/
AtspiPoint *
atspi_component_get_position (AtspiComponent *obj,
                                 AtspiCoordType ctype, GError **error)
{
  AtspiPoint ret;

  ret.x = ret.y = -1;
  g_return_val_if_fail (obj != NULL, atspi_point_copy (&ret));

  a11y_atspi_component_call_get_position_sync (get_component_proxy (obj), ctype,
                                               &ret.x, &ret.y, NULL, error);
  return atspi_point_copy (&ret);
}

/**
 * atspi_component_get_size:
 * @obj: a pointer to the #AtspiComponent to query.
 *
 * Gets the size of the specified #AtspiComponent.
 *
 * returns: An #AtspiPoint giving the @obj's size.
 **/
AtspiPoint *
atspi_component_get_size (AtspiComponent *obj, GError **error)
{
  AtspiPoint ret;

  ret.x = ret.y = -1;
  g_return_val_if_fail (obj != NULL, atspi_point_copy (&ret));

  a11y_atspi_component_call_get_size_sync (get_component_proxy (obj),
                                           &ret.x, &ret.y, NULL, error);
  return atspi_point_copy (&ret);
}

/**
 * atspi_component_get_layer:
 * @obj: a pointer to the #AtspiComponent to query.
 *
 * Queries which layer the component is painted into, to help determine its 
 *      visibility in terms of stacking order.
 *
 * Returns: the #AtspiComponentLayer into which this component is painted.
 **/
AtspiComponentLayer
atspi_component_get_layer (AtspiComponent *obj, GError **error)
{
  guint32 retval = -1;

  a11y_atspi_component_call_get_layer_sync (get_component_proxy (obj),
                                            &retval, NULL, error);
  return retval;
}

/**
 * atspi_component_get_mdi_z_order:
 * @obj: a pointer to the #AtspiComponent to query.
 *
 * Queries the z stacking order of a component which is in the MDI or window
 *       layer. (Bigger z-order numbers mean nearer the top)
 *
 * Returns: a #gshort indicating the stacking order of the component 
 *       in the MDI layer, or -1 if the component is not in the MDI layer.
 **/
gshort
atspi_component_get_mdi_z_order (AtspiComponent *obj, GError **error)
{
  gint16 retval = -1;

  a11y_atspi_component_call_get_mdizorder_sync (get_component_proxy (obj),
                                                &retval, NULL, error);
  return retval;
}

/**
 * atspi_component_grab_focus:
 * @obj: a pointer to the #AtspiComponent on which to operate.
 *
 * Attempts to set the keyboard input focus to the specified
 *         #AtspiComponent.
 *
 * Returns: #TRUE if successful, #FALSE otherwise.
 *
 **/
gboolean
atspi_component_grab_focus (AtspiComponent *obj, GError **error)
{
  gboolean retval = FALSE;

  a11y_atspi_component_call_grab_focus_sync (get_component_proxy (obj),
                                             &retval, NULL, error);
  return retval;
}

/**
 * atspi_component_get_alpha:
 * @obj: The #AtspiComponent to be queried.
 *
 * Gets the opacity/alpha value of a component, if alpha blending is in use.
 *
 * Returns: the opacity value of a component, as a #gdouble between 0.0 and 1.0. 
 **/
gdouble      
atspi_component_get_alpha    (AtspiComponent *obj, GError **error)
{
  gdouble retval = 1;

  a11y_atspi_component_call_get_alpha_sync (get_component_proxy (obj),
                                            &retval, NULL, error);
  return retval;
}

/**
 * atspi_component_set_extents:
 * @obj: a pointer to the #AtspiComponent to move.
 * @x: the new vertical position to which the component should be moved.
 * @y: the new horizontal position to which the component should be moved.
 * @width: the width to which the component should be resized.
 * @height: the height to which the component should be resized.
 * @ctype: the coordinate system in which the position is specified.
 *         (e.g. ATSPI_COORD_TYPE_WINDOW, ATSPI_COORD_TYPE_SCREEN).
 *
 * Moves and resizes the specified component.
 *
 * Returns: #TRUE if successful; #FALSE otherwise.
 **/
gboolean
atspi_component_set_extents (AtspiComponent *obj,
                             gint x,
                             gint y,
                             gint width,
                             gint height,
                             AtspiCoordType ctype,
                             GError **error)
{
  gboolean retval = FALSE;
  AtspiAccessible *aobj = ATSPI_ACCESSIBLE (obj);

  g_return_val_if_fail (obj != NULL, FALSE);

  if (!aobj->parent.app || !aobj->parent.app->bus_name)
  {
    g_set_error_literal (error, ATSPI_ERROR, ATSPI_ERROR_APPLICATION_GONE,
                          _("The application no longer exists"));
    return FALSE;
  }

  a11y_atspi_component_call_set_extents_sync (get_component_proxy (obj),
                                              x, y, width, height, ctype,
                                              &retval, NULL, error);
  return retval;
}

/**
 * atspi_component_set_position:
 * @obj: a pointer to the #AtspiComponent to move.
 * @x: the new vertical position to which the component should be moved.
 * @y: the new horizontal position to which the component should be moved.
 * @ctype: the coordinate system in which the position is specified.
 *         (e.g. ATSPI_COORD_TYPE_WINDOW, ATSPI_COORD_TYPE_SCREEN).
 *
 * Moves the component to the specified position.
 *
 * Returns: #TRUE if successful; #FALSE otherwise.
 **/
gboolean
atspi_component_set_position (AtspiComponent *obj,
                              gint x,
                              gint y,
                              AtspiCoordType ctype,
                              GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_component_call_set_position_sync (get_component_proxy (obj),
                                               x, y, ctype,
                                               &retval, NULL, error);
  return retval;
}

/**
 * atspi_component_set_size:
 * @obj: a pointer to the #AtspiComponent to query.
 * @width: the width to which the component should be resized.
 * @height: the height to which the component should be resized.
 *
 * Resizes the specified component to the given coordinates.
 *
 * Returns: #TRUE if successful; #FALSE otherwise.
 **/
gboolean
atspi_component_set_size (AtspiComponent *obj,
                          gint width,
                          gint height,
                          GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_component_call_set_size_sync (get_component_proxy (obj),
                                           width, height,
                                           &retval, NULL, error);
  return retval;
}

static void
atspi_component_base_init (AtspiComponent *klass)
{
}

GType
atspi_component_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiComponent),
      (GBaseInitFunc) atspi_component_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiComponent", &tinfo, 0);

  }
  return type;
}
