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

#include "xml/a11y-atspi-selection.h"

static A11yAtspiSelection *
get_selection_proxy (AtspiSelection *selection)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (selection), (AtspiAccessibleProxyInit) a11y_atspi_selection_proxy_new_sync,
     "a11y-atspi-selection-proxy");
}

/**
 * atspi_selection_get_n_selected_children:
 * @obj: a pointer to the #AtspiSelection implementor on which to operate.
 *
 * Gets the number of children of an #AtspiSelection implementor which are
 *        currently selected.
 *
 * Returns: a #gint indicating the number of #Accessible children
 *        of the #AtspiSelection implementor which are currently selected.
 *
 **/
gint
atspi_selection_get_n_selected_children (AtspiSelection *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, -1);

  return a11y_atspi_selection_get_nselected_children (get_selection_proxy (obj));
}

/**
 * atspi_selection_get_selected_child:
 * @obj: a pointer to the #AtspiSelection on which to operate.
 * @selected_child_index: a #gint indicating which of the selected
 *      children is specified.
 *
 * Gets the i-th selected #AtspiAccessible child of an #AtspiSelection.
 *      Note that @selected_child_index refers to the index in the list 
 *      of 'selected'
 *      children and generally differs from that used in
 *      #atspi_accessible_get_child_at_index or returned by
 *      #atspi_accessible_get_index_in_parent.
 *      @selected_child_index must lie between 0
 *      and #atspi_selection_get_n_selected_children - 1, inclusive.
 *
 * Returns: (transfer full): a pointer to a selected #AtspiAccessible child
 *          object, specified by @selected_child_index.
 *
 **/
AtspiAccessible *
atspi_selection_get_selected_child (AtspiSelection *obj,
                                      gint selected_child_index, GError **error)
{
  AtspiAccessible *accessible = NULL;
  GVariant *variant = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_selection_call_get_selected_child_sync (get_selection_proxy (obj),
                                                         selected_child_index,
                                                         &variant, NULL, error))
    {
      accessible = _atspi_dbus_return_accessible_from_variant (variant);
      g_variant_unref (variant);
    }

  return accessible;
}

/**
 * atspi_selection_select_child:
 * @obj: a pointer to the #AtspiSelection on which to operate.
 * @child_index: a #gint indicating which child of the #Accessible
 *              is to be selected.
 *
 * Adds a child to the selected children list of an #AtspiSelection.
 *         For #AtspiSelection implementors that only allow
 *         single selections, this may replace the (single) current
 *         selection.
 *
 * Returns: #TRUE if the child was successfully selected, #FALSE otherwise.
 **/
gboolean
atspi_selection_select_child (AtspiSelection *obj,
                              gint child_index,
                              GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_selection_call_select_child_sync (get_selection_proxy (obj),
                                               child_index,
                                               &retval, NULL, error);
  return retval;
}

/**
 * atspi_selection_deselect_selected_child:
 * @obj: a pointer to the #AtspiSelection on which to operate.
 * @selected_child_index: a #gint indicating which of the selected children
 *              of the #Accessible is to be selected.
 *
 * Removes a child from the selected children list of an #AtspiSelection.
 *          Note that @child_index is the index in the selected-children list,
 *          not the index in the parent container.  @selectedChildIndex in this
 *          method, and @child_index in #atspi_selection_select_child
 *          are asymmetric.
 *
 * Returns: #TRUE if the child was successfully deselected, #FALSE otherwise.
 **/
gboolean
atspi_selection_deselect_selected_child (AtspiSelection *obj,
                                         gint selected_child_index,
                                         GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_selection_call_deselect_selected_child_sync (get_selection_proxy (obj),
                                                          selected_child_index,
                                                          &retval, NULL, error);
  return retval;
}

/**
 * atspi_selection_deselect_child:
 * @obj: a pointer to the #AtspiSelection on which to operate.
 * @child_index: a #gint indicating which of the children
 *              of the #AtspiAccessible is to be de-selected.
 *
 * Deselects a specific child of an #AtspiSelection.
 *          Note that @child_index is the index of the child
 *          in the parent container.
 * 
 * See #atspi_selection_deselect_selected_child
 * 
 * Returns: #TRUE if the child was successfully deselected, #FALSE otherwise.
 **/
gboolean
atspi_selection_deselect_child (AtspiSelection *obj,
				gint child_index,
				GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_selection_call_deselect_child_sync (get_selection_proxy (obj),
                                                 child_index,
                                                 &retval, NULL, error);
  return retval;
}

/**
 * atspi_selection_is_child_selected:
 * @obj: a pointer to the #AtspiSelection implementor on which to operate.
 * @child_index: an index into the #AtspiSelection's list of children.
 *
 * Determines whether a particular child of an #AtspiSelection implementor
 *        is currently selected.  Note that @child_index is the index into the
 *        standard #AtspiAccessible container's list of children.
 *
 * Returns: #TRUE if the specified child is currently selected,
 *          #FALSE otherwise.
 **/
gboolean
atspi_selection_is_child_selected (AtspiSelection *obj,
                                   gint child_index,
                                   GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_selection_call_is_child_selected_sync (get_selection_proxy (obj),
                                                    child_index,
                                                    &retval, NULL, error);
  return retval;
}

/**
 * atspi_selection_select_all:
 * @obj: a pointer to the #AtspiSelection implementor on which to operate.
 *
 * Attempts to select all of the children of an #AtspiSelection implementor.
 * Not all #AtspiSelection implementors support this operation.
 *
 * Returns: #TRUE if successful, #FALSE otherwise.
 *
 **/
gboolean
atspi_selection_select_all (AtspiSelection *obj, GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_selection_call_select_all_sync (get_selection_proxy (obj),
                                             &retval, NULL, error);
  return retval;
}

/**
 * atspi_selection_clear_selection:
 * @obj: a pointer to the #AtspiSelection implementor on which to operate.
 *
 * Clears the current selection, removing all selected children from the
 *       specified #AtspiSelection implementor's selection list.
 *
 * Returns: #TRUE if successful, #FALSE otherwise.
 *
 **/
gboolean
atspi_selection_clear_selection (AtspiSelection *obj, GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_selection_call_clear_selection_sync (get_selection_proxy (obj),
                                                  &retval, NULL, error);
  return retval;
}

static void
atspi_selection_base_init (AtspiSelection *klass)
{
}

GType
atspi_selection_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiSelection),
      (GBaseInitFunc) atspi_selection_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiSelection", &tinfo, 0);

  }
  return type;
}
