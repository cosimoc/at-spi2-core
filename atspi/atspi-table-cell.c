/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2001, 2002 Sun Microsystems Inc.,
 * Copyright 2001, 2002 Ximian, Inc.
 * Copyright 2013 SUSE LLC.
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

#include <stdlib.h> /* for malloc */
#include "atspi-private.h"

#include "xml/a11y-atspi-table-cell.h"

static A11yAtspiTableCell *
get_table_cell_proxy (AtspiTableCell *cell)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (cell), (AtspiAccessibleProxyInit) a11y_atspi_table_cell_proxy_new_sync,
     "a11y-atspi-table-cell-proxy");
}

static GPtrArray *
get_object_array_and_unref (GVariant *variant)
{
  GVariantIter iter;
  GVariant *value;
  GPtrArray *array = g_ptr_array_new ();

  g_variant_iter_init (&iter, variant);
  while ((value = g_variant_iter_next_value (&iter)))
    {
      AtspiAccessible *accessible = _atspi_dbus_return_accessible_from_variant (value);
      g_ptr_array_add (array, accessible);
      g_variant_unref (value);
    }

  g_variant_unref (variant);

  return array;
}

/**
 * atspi_table_cell_get_column_span:
 * @obj: a GObject instance that implements AtspiTableCellIface
 *
 * Returns the number of columns occupied by this cell accessible.
 *
 * Returns: a gint representing the number of columns occupied by this cell,
 * or 0 if the cell does not implement this method.
 */
gint
atspi_table_cell_get_column_span (AtspiTableCell *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, -1);

  return a11y_atspi_table_cell_get_column_span (get_table_cell_proxy (obj));
}

/**
 * atspi_table_cell_get_column_header_cells:
 * @obj: a GObject instance that implements AtspiTableCellIface
 *
 * Returns the column headers as an array of cell accessibles.
 *
 * Returns: (element-type AtspiAccessible) (transfer full): a GPtrArray of
 * AtspiAccessibles representing the column header cells.
  */
GPtrArray *
atspi_table_cell_get_column_header_cells (AtspiTableCell *obj, GError **error)
{
  GPtrArray *retval = NULL;
  GVariant *variant = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_table_cell_call_get_column_header_cells_sync (get_table_cell_proxy (obj),
                                                               &variant, NULL, error))
    retval = get_object_array_and_unref (variant);

  return retval;
}

/**
 * atspi_table_cell_get_row_span:
 * @obj: a GObject instance that implements AtspiTableCellIface
 *
 * Returns the number of rows occupied by this cell accessible.
 *
 * Returns: a gint representing the number of rows occupied by this cell,
 * or 0 if the cell does not implement this method.
 */
gint
atspi_table_cell_get_row_span (AtspiTableCell *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, -1);

  return a11y_atspi_table_cell_get_row_span (get_table_cell_proxy (obj));
}

/**
 * atspi_table_cell_get_row_header_cells:
 * @obj: a GObject instance that implements AtspiTableCellIface
 *
 * Returns the row headers as an array of cell accessibles.
 *
 * Returns: (element-type AtspiAccessible) (transfer full): a GPtrArray of
 * AtspiAccessibles representing the row header cells.
 */
GPtrArray *
atspi_table_cell_get_row_header_cells (AtspiTableCell *obj, GError **error)
{
  GPtrArray *retval = NULL;
  GVariant *variant = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_table_cell_call_get_row_header_cells_sync (get_table_cell_proxy (obj),
                                                            &variant, NULL, error))
    retval = get_object_array_and_unref (variant);

  return retval;
}

/**
 * atspi_table_cell_get_position:
 * @obj: a GObject instance that implements AtspiTableCellIface
 * @row: (out): the row of the given cell.
 * @column: (out): the column of the given cell.
 *
 * Retrieves the tabular position of this cell.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 */
gint
atspi_table_cell_get_position (AtspiTableCell *obj,
                               gint *row,
                               gint *column,
                               GError **error)
{
  GVariant *variant;

  g_return_val_if_fail (obj != NULL, -1);

  variant = a11y_atspi_table_cell_get_position (get_table_cell_proxy (obj));
  g_variant_get (variant, "(ii)", row, column);
  g_variant_unref (variant);

  return TRUE;
}

/**
 * atspi_table_cell_get_row_column_span:
 * @obj: a GObject instance that implements AtspiTableCellIface
 * @row: (out): the row index of the given cell.
 * @column: (out): the column index of the given cell.
 * @row_span: (out): the number of rows occupied by this cell.
 * @column_span: (out): the number of columns occupied by this cell.
 *
 * Gets the row and column indexes and extents of this cell accessible.
 */
void
atspi_table_cell_get_row_column_span (AtspiTableCell *obj,
                                       gint *row,
                                       gint *column,
                                       gint *row_span,
                                       gint *column_span,
                                       GError **error)
{
  gboolean result;

  if (row)
    *row = -1;
  if (column)
    *column = -1;
  if (row_span)
    *row_span = -1;
  if (column_span)
    *column_span = -1;

  g_return_if_fail (obj != NULL);

  a11y_atspi_table_cell_call_get_row_column_span_sync (get_table_cell_proxy (obj),
                                                       &result,
                                                       row, column,
                                                       row_span, column_span,
                                                       NULL, error);
}

/**
 * atspi_table_cell_get_table:
 * @obj: a GObject instance that implements AtspiTableCellIface
 *
 * Returns a reference to the accessible of the containing table.
 *
 * Returns: (transfer full): the AtspiAccessible for the containing table.
 */
AtspiAccessible *
atspi_table_cell_get_table (AtspiTableCell *obj, GError **error)
{
  AtspiAccessible *retval = NULL;
  GVariant *variant;

  g_return_val_if_fail (obj != NULL, NULL);

  variant = a11y_atspi_table_cell_get_table (get_table_cell_proxy (obj));
  retval = _atspi_dbus_return_accessible_from_variant (variant);
  g_variant_unref (variant);
	  
  return retval;
}

static void
atspi_table_cell_base_init (AtspiTableCell *klass)
{
}

GType
atspi_table_cell_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiTableCell),
      (GBaseInitFunc) atspi_table_cell_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiTableCell", &tinfo, 0);

  }
  return type;
}
