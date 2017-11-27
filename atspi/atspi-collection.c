/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2007 IBM Corp.
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

#include "xml/a11y-atspi-collection.h"

/* TODO: Improve documentation and implement some missing functions */

/**
 * atspi_collection_is_ancestor_of:
 *
 * Not yet implemented.
 *
 **/
gboolean
atspi_collection_is_ancestor_of (AtspiCollection *collection,
                                 AtspiAccessible *test,
                                 GError **error)
{
  g_warning ("Atspi: TODO: Implement is_ancestor_of");
  return FALSE;
}

static A11yAtspiCollection *
get_collection_proxy (AtspiCollection *collection)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (collection), (AtspiAccessibleProxyInit) a11y_atspi_collection_proxy_new_sync,
     "a11y-atspi-collection-proxy");
}

static GArray *
return_accessibles (GVariant *variant)
{
  GVariant *value;
  GVariantIter iter;
  GArray *ret = g_array_new (TRUE, TRUE, sizeof (AtspiAccessible *));

  g_variant_iter_init (&iter, variant);
  while ((value = g_variant_iter_next_value (&iter)))
    {
      AtspiAccessible *accessible = _atspi_dbus_return_accessible_from_variant (value);
      ret = g_array_append_val (ret, accessible);
      g_variant_unref (value);
    }
  return ret;
}

/**
 * atspi_collection_get_matches:
 * @collection: A pointer to the #AtspiCollection to query.
 * @rule: An #AtspiMatchRule describing the match criteria.
 * @sortby: An #AtspiCollectionSortOrder specifying the way the results are to
 *          be sorted.
 * @count: The maximum number of results to return, or 0 for no limit.
 * @traverse: Not supported.
 *
 * Gets all #AtspiAccessible objects from the @collection matching a given
 * @rule.  
 *
 * Returns: (element-type AtspiAccessible*) (transfer full): All 
 *          #AtspiAccessible objects matching the given match rule.
 **/
GArray *
atspi_collection_get_matches (AtspiCollection *collection,
                              AtspiMatchRule *rule,
                              AtspiCollectionSortOrder sortby,
                              gint count,
                              gboolean traverse,
                              GError **error)
{
  A11yAtspiCollection *collection_proxy = get_collection_proxy (collection);
  GVariant *rule_variant = _atspi_match_rule_to_variant (rule), *variant = NULL;
  GArray *accessibles = NULL;
  gboolean res;

  res = a11y_atspi_collection_call_get_matches_sync (collection_proxy, rule_variant,
                                                     sortby, count, traverse,
                                                     &variant, NULL, error);
  g_variant_unref (rule_variant);

  if (res)
    {
      accessibles = return_accessibles (variant);
      g_variant_unref (variant);
    }

  return accessibles;
}

/**
 * atspi_collection_get_matches_to:
 * @collection: A pointer to the #AtspiCollection to query.
 * @current_object: The object at which to start searching.
 * @rule: An #AtspiMatchRule describing the match criteria.
 * @sortby: An #AtspiCollectionSortOrder specifying the way the results are to
 *          be sorted.
 * @tree: An #AtspiCollectionTreeTraversalType specifying restrictions on
 *          the objects to be traversed.
 * @limit_scope: If #TRUE, only descendants of @current_object's parent
 *          will be returned. Otherwise (if #FALSE), any accessible may be
 *          returned if it would preceed @current_object in a flattened
 *          hierarchy.
 * @count: The maximum number of results to return, or 0 for no limit.
 * @traverse: Not supported.
 *
 * Gets all #AtspiAccessible objects from the @collection, after 
 * @current_object, matching a given @rule.  
 *
 * Returns: (element-type AtspiAccessible*) (transfer full): All
 *          #AtspiAccessible objects matching the given match rule after
 *          @current_object.
 **/
GArray *
atspi_collection_get_matches_to (AtspiCollection *collection,
                              AtspiAccessible *current_object,
                              AtspiMatchRule *rule,
                              AtspiCollectionSortOrder sortby,
                              AtspiCollectionTreeTraversalType tree,
                              gboolean limit_scope,
                              gint count,
                              gboolean traverse,
                              GError **error)
{
  A11yAtspiCollection *collection_proxy = get_collection_proxy (collection);
  GVariant *rule_variant = _atspi_match_rule_to_variant (rule), *variant = NULL;
  GArray *accessibles = NULL;
  gboolean res;

  res = a11y_atspi_collection_call_get_matches_to_sync (collection_proxy, current_object->parent.path,
                                                        rule_variant, sortby, tree,
                                                        limit_scope, count, traverse,
                                                        &variant, NULL, error);
  g_variant_unref (rule_variant);

  if (res)
    {
      accessibles = return_accessibles (variant);
      g_variant_unref (variant);
    }

  return accessibles;
}

/**
 * atspi_collection_get_matches_from:
 * @collection: A pointer to the #AtspiCollection to query.
 * @current_object: Upon reaching this object, searching should stop.
 * @rule: An #AtspiMatchRule describing the match criteria.
 * @sortby: An #AtspiCollectionSortOrder specifying the way the results are to
 *          be sorted.
 * @tree: An #AtspiCollectionTreeTraversalType specifying restrictions on
 *          the objects to be traversed.
 * @count: The maximum number of results to return, or 0 for no limit.
 * @traverse: Not supported.
 *
 * Gets all #AtspiAccessible objects from the @collection, before  
 * @current_object, matching a given @rule.  
 *
 * Returns: (element-type AtspiAccessible*) (transfer full): All 
 *          #AtspiAccessible objects matching the given match rule that preceed
 *          @current_object.
 **/
GArray *
atspi_collection_get_matches_from (AtspiCollection *collection,
                              AtspiAccessible *current_object,
                              AtspiMatchRule *rule,
                              AtspiCollectionSortOrder sortby,
                              AtspiCollectionTreeTraversalType tree,
                              gint count,
                              gboolean traverse,
                              GError **error)
{
  A11yAtspiCollection *collection_proxy = get_collection_proxy (collection);
  GVariant *rule_variant = _atspi_match_rule_to_variant (rule), *variant = NULL;
  GArray *accessibles = NULL;
  gboolean res;

  res = a11y_atspi_collection_call_get_matches_from_sync (collection_proxy, current_object->parent.path,
                                                          rule_variant, sortby, tree,
                                                          count, traverse,
                                                          &variant, NULL, error);
  g_variant_unref (rule_variant);

  if (res)
    {
      accessibles = return_accessibles (variant);
      g_variant_unref (variant);
    }

  return accessibles;
}

/**
 * atspi_collection_get_active_descendant:
 *
* Returns: (transfer full): The active descendant of the given object.
 * Not yet implemented.
 **/
AtspiAccessible *
atspi_collection_get_active_descendant (AtspiCollection *collection, GError **error)
{
  g_warning ("atspi: TODO: Implement get_active_descendants");
  return NULL;
}

static void
atspi_collection_base_init (AtspiCollection *klass)
{
}

GType
atspi_collection_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiCollection),
      (GBaseInitFunc) atspi_collection_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiCollection", &tinfo, 0);

  }
  return type;
}
