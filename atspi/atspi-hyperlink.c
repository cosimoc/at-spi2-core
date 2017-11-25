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

#include "xml/a11y-atspi-hyperlink.h"

typedef struct {
  A11yAtspiHyperlink *hyperlink_proxy;
} AtspiHyperlinkPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (AtspiHyperlink, atspi_hyperlink, ATSPI_TYPE_OBJECT)

static void
atspi_hyperlink_init (AtspiHyperlink *hyperlink)
{
  AtspiApplication *app = hyperlink->parent.app;
  AtspiHyperlinkPrivate *priv = atspi_hyperlink_get_instance_private (hyperlink);
  priv->hyperlink_proxy = a11y_atspi_hyperlink_proxy_new_sync (g_dbus_proxy_get_connection (G_DBUS_PROXY (atspi_application_get_application_proxy (app))),
                                                               G_DBUS_PROXY_FLAGS_NONE,
                                                               app->bus_name, hyperlink->parent.path,
                                                               NULL, NULL);
}

static void
atspi_hyperlink_finalize (GObject *object)
{
  AtspiHyperlink *hyperlink = ATSPI_HYPERLINK (object);
  AtspiHyperlinkPrivate *priv = atspi_hyperlink_get_instance_private (hyperlink);

  g_clear_object (&priv->hyperlink_proxy);

  G_OBJECT_CLASS (atspi_hyperlink_parent_class)->finalize (object);
}

static void
atspi_hyperlink_class_init (AtspiHyperlinkClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->finalize = atspi_hyperlink_finalize;
}

AtspiHyperlink *
_atspi_hyperlink_new (AtspiApplication *app, const gchar *path)
{
  AtspiHyperlink *hyperlink;
  
  hyperlink = g_object_new (ATSPI_TYPE_HYPERLINK, NULL);
  hyperlink->parent.app = g_object_ref (app);
  hyperlink->parent.path = g_strdup (path);

  return hyperlink;
}

/**
 * atspi_hyperlink_get_n_anchors:
 * @obj: a pointer to the #AtspiHyperlink object on which to operate.
 *
 * Gets the total number of anchors which an #AtspiHyperlink implementor has.
 * Though typical hyperlinks have only one anchor, client-side image maps and
 * other hypertext objects may potentially activate or refer to multiple
 * URIs.  For each anchor there is a corresponding URI and object.
 *
 * see: #atspi_hyperlink_get_uri and #atspi_hyperlink_get_object.
 *
 * Returns: a #gint indicating the number of anchors in this hyperlink.
 **/
gint
atspi_hyperlink_get_n_anchors (AtspiHyperlink *obj, GError **error)
{
  AtspiHyperlinkPrivate *priv;

  g_return_val_if_fail (obj != NULL, -1);
  priv = atspi_hyperlink_get_instance_private (obj);

  return a11y_atspi_hyperlink_get_nanchors (priv->hyperlink_proxy);
}

/**
 * atspi_hyperlink_get_uri:
 * @obj: a pointer to the #AtspiHyperlink implementor on which to operate.
 * @i: a (zero-index) integer indicating which hyperlink anchor to query.
 *
 * Gets the URI associated with a particular hyperlink anchor.
 *
 * Returns: a UTF-8 string giving the URI of the @ith hyperlink anchor.
 **/
gchar *
atspi_hyperlink_get_uri (AtspiHyperlink *obj, int i, GError **error)
{
  AtspiHyperlinkPrivate *priv;
  char *retval = NULL;

  g_return_val_if_fail (obj != NULL, NULL);
  priv = atspi_hyperlink_get_instance_private (obj);

  a11y_atspi_hyperlink_call_get_uri_sync (priv->hyperlink_proxy, i,
                                          &retval, NULL, error);
  if (!retval)
    retval = g_strdup ("");

  return retval;
}

/**
 * atspi_hyperlink_get_object:
 * @obj: a pointer to the #AtspiHyperlink implementor on which to operate.
 * @i: a (zero-index) #gint indicating which hyperlink anchor to query.
 *
 * Gets the object associated with a particular hyperlink anchor, as an
 * #AtspiAccessible.
 *
 * Returns: (transfer full): an #AtspiAccessible that represents the object
 *        associated with the @ith anchor of the specified #AtspiHyperlink.
 **/
AtspiAccessible*
atspi_hyperlink_get_object (AtspiHyperlink *obj, gint i, GError **error)
{
  AtspiHyperlinkPrivate *priv;
  GVariant *variant;
  AtspiAccessible *accessible = NULL;

  g_return_val_if_fail (obj != NULL, NULL);
  priv = atspi_hyperlink_get_instance_private (obj);

  if (a11y_atspi_hyperlink_call_get_object_sync (priv->hyperlink_proxy, i,
                                                 &variant, NULL, error))
    {
      accessible = _atspi_dbus_return_accessible_from_variant (variant);
      g_variant_unref (variant);
    }

  return accessible;
}

/**
 * atspi_hyperlink_get_index_range:
 * @obj: a pointer to the #AtspiHyperlink implementor on which to operate.
 *
 *
 * Gets the starting and ending character offsets of the text range
 * associated with an #AtspiHyperlink, in its originating #AtspiHypertext.
 **/
AtspiRange *
atspi_hyperlink_get_index_range (AtspiHyperlink *obj, GError **error)
{
  AtspiHyperlinkPrivate *priv;
  AtspiRange *ret;

  ret = g_new (AtspiRange, 1);
  ret->start_offset = ret->end_offset = -1;

  g_return_val_if_fail (obj != NULL, ret);
  priv = atspi_hyperlink_get_instance_private (obj);

  ret->start_offset = a11y_atspi_hyperlink_get_start_index (priv->hyperlink_proxy);
  ret->end_offset = a11y_atspi_hyperlink_get_end_index (priv->hyperlink_proxy);

  return ret;
}

/**
 * atspi_hyperlink_get_start_index:
 * @obj: a pointer to the #AtspiHyperlink implementor on which to operate.
 *
 *
 * Gets the starting character offset of the text range associated with
 *       an #AtspiHyperlink, in its originating #AtspiHypertext.
 **/
gint
atspi_hyperlink_get_start_index (AtspiHyperlink *obj, GError **error)
{
  AtspiHyperlinkPrivate *priv;

  g_return_val_if_fail (obj != NULL, -1);
  priv = atspi_hyperlink_get_instance_private (obj);

  return a11y_atspi_hyperlink_get_start_index (priv->hyperlink_proxy);
}
/**
 * atspi_hyperlink_get_end_index:
 * @obj: a pointer to the #AtspiHyperlink implementor on which to operate.
 *
 *
 * Gets the ending character offset of the text range associated with
 *       an #AtspiHyperlink, in its originating #AtspiHypertext.
 **/
gint
atspi_hyperlink_get_end_index (AtspiHyperlink *obj, GError **error)
{
  AtspiHyperlinkPrivate *priv;

  g_return_val_if_fail (obj != NULL, -1);
  priv = atspi_hyperlink_get_instance_private (obj);

  return a11y_atspi_hyperlink_get_start_index (priv->hyperlink_proxy);
}


/**
 * atspi_hyperlink_is_valid:
 * @obj: a pointer to the #AtspiHyperlink on which to operate.
 *
 * Tells whether an #AtspiHyperlink object is still valid with respect to its
 *          originating hypertext object.
 *
 * Returns: #TRUE if the specified #AtspiHyperlink is still valid with respect
 *          to its originating #AtspiHypertext object, #FALSE otherwise.
 **/
gboolean
atspi_hyperlink_is_valid (AtspiHyperlink *obj, GError **error)
{
  AtspiHyperlinkPrivate *priv;
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);
  priv = atspi_hyperlink_get_instance_private (obj);

  a11y_atspi_hyperlink_call_is_valid_sync (priv->hyperlink_proxy,
                                           &retval, NULL, error);
  return retval;
}
