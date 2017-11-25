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

#include <stdlib.h> /* for malloc */
#include "atspi-private.h"

#include "xml/a11y-atspi-table.h"

static A11yAtspiTable *
get_table_proxy (AtspiTable *table)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (table), (AtspiAccessibleProxyInit) a11y_atspi_table_proxy_new_sync,
     "a11y-atspi-table-proxy");
}

/**
 * atspi_table_get_caption:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 *
 * Gets an accessible representation of the caption for an #AtspiTable.
 *
 * Returns: (transfer full): an #AtspiAccessible object that serves as
 * the table's caption.
 *
 **/
AtspiAccessible *
atspi_table_get_caption (AtspiTable *obj, GError **error)
{
  AtspiAccessible *retval = NULL;
  GVariant *variant;

  g_return_val_if_fail (obj != NULL, NULL);

  variant = a11y_atspi_table_get_caption (get_table_proxy (obj));
  if (variant)
    retval = _atspi_dbus_return_accessible_from_variant (variant);

  return retval;
}

/**
 * atspi_table_get_summary:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 *
 * Gets an accessible object which summarizes the contents of an #AtspiTable.
 *
 * Returns: (transfer full): an #AtspiAccessible object that serves as the
 *          table's summary (often a reduced #AtspiTable).
 **/
AtspiAccessible *
atspi_table_get_summary (AtspiTable *obj, GError **error)
{
  AtspiAccessible *retval = NULL;
  GVariant *variant;

  g_return_val_if_fail (obj != NULL, NULL);

  variant = a11y_atspi_table_get_summary (get_table_proxy (obj));
  if (variant)
    retval = _atspi_dbus_return_accessible_from_variant (variant);

  return retval;
}

/**
 * atspi_table_get_n_rows:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 *
 * Gets the number of rows in an #AtspiTable,
 *        exclusive of any rows that are programmatically hidden, but inclusive
 *        of rows that may be outside of the current scrolling window or viewport.
 *
 * Returns: a #gint indicating the number of rows in the table.
 **/
gint
atspi_table_get_n_rows (AtspiTable *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, -1);

  return a11y_atspi_table_get_nrows (get_table_proxy (obj));
}

/**
 * atspi_table_get_n_columns:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 *
 * Gets the number of columns in an #AtspiTable,
 *        exclusive of any columns that are programmatically hidden, but inclusive
 *        of columns that may be outside of the current scrolling window or viewport.
 *
 * Returns: a #gint indicating the number of columns in the table.
 **/
gint
atspi_table_get_n_columns (AtspiTable *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, -1);

  return a11y_atspi_table_get_ncolumns (get_table_proxy (obj));
}

/**
 * atspi_table_get_accessible_at:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the specified table row, zero-indexed.
 * @column: the specified table column, zero-indexed.
 *
 * Gets the table cell at the specified row and column indices.
 * To get the accessible object at a particular (x, y) screen 
 * coordinate, use #atspi_component_get_accessible_at_point.
 *
 * Returns: (transfer full): an #AtspiAccessible object representing the
 *          specified table cell.
 **/
AtspiAccessible *
atspi_table_get_accessible_at (AtspiTable *obj,
                                 gint row,
                                 gint column,
                                 GError **error)
{
  AtspiAccessible *accessible = NULL;
  GVariant *variant = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_table_call_get_accessible_at_sync (get_table_proxy (obj),
                                                    row, column,
                                                    &variant, NULL, error))
    {
      accessible = _atspi_dbus_return_accessible_from_variant (variant);
      g_variant_unref (variant);
    }

  return accessible;
}

/**
 * atspi_table_get_index_at:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the specified table row, zero-indexed.
 * @column: the specified table column, zero-indexed.
 *
 * Gets the 1-D child index corresponding to the specified 2-D row and
 * column indices. To get the accessible object at a particular (x, y) screen 
 * coordinate, use #atspi_component_get_accessible_at_point.
 *
 * @see #atspi_table_get_row_at_index, #atspi_table_get_column_at_index
 *
 * Returns: a #gint which serves as the index of a specified cell in the
 *          table, in a form usable by #atspi_get_child_at_index.
 **/
gint
atspi_table_get_index_at (AtspiTable *obj,
                            gint row,
                            gint column,
                            GError **error)
{
  gint retval = -1;

  g_return_val_if_fail (obj != NULL, -1);

  a11y_atspi_table_call_get_index_at_sync (get_table_proxy (obj),
                                           row, column,
                                           &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_get_row_at_index:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @index: the specified child index, zero-indexed.
 *
 * Gets the table row index occupied by the child at a particular 1-D 
 * child index.
 *
 * @see #atspi_table_get_index_at, #atspi_table_get_column_at_index
 *
 * Returns: a #gint indicating the first row spanned by the child of a
 *          table, at the specified 1-D (zero-offset) @index.
 **/
gint
atspi_table_get_row_at_index (AtspiTable *obj,
                               gint index,
                               GError **error)
{
  gint retval = -1;

  g_return_val_if_fail (obj != NULL, -1);

  a11y_atspi_table_call_get_row_at_index_sync (get_table_proxy (obj),
                                               index,
                                               &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_get_column_at_index:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @index: the specified child index, zero-indexed.
 *
 * Gets the table column index occupied by the child at a particular 1-D
 * child index.
 *
 * @see #atspi_table_get_index_at, #atspi_table_get_row_at_index
 *
 * Returns: a #gint indicating the first column spanned by the child of a
 *          table, at the specified 1-D (zero-offset) @index.
 **/
gint
atspi_table_get_column_at_index (AtspiTable *obj,
                                  gint index,
                                  GError **error)
{
  gint retval = -1;

  g_return_val_if_fail (obj != NULL, -1);

  a11y_atspi_table_call_get_column_at_index_sync (get_table_proxy (obj),
                                                  index,
                                                  &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_get_row_description:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the specified table row, zero-indexed.
 *
 * Gets a text description of a particular table row.  This differs from
 * #atspi_table_get_row_header, which returns an #AtspiAccessible.
 *
 * Returns: a UTF-8 string describing the specified table row, if available.
 **/
gchar *
atspi_table_get_row_description (AtspiTable *obj,
				   gint  row,
				   GError **error)
{
  char *retval = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  a11y_atspi_table_call_get_row_description_sync (get_table_proxy (obj),
                                                  row,
                                                  &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_get_column_description:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @column: the specified table column, zero-indexed.
 *
 * Gets a text description of a particular table column.  This differs from
 * #atspi_table_get_column_header, which returns an #Accessible.
 *
 * Returns: a UTF-8 string describing the specified table column, if available.
 **/
gchar *
atspi_table_get_column_description (AtspiTable *obj,
				      gint         column, GError **error)
{
  char *retval = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  a11y_atspi_table_call_get_row_description_sync (get_table_proxy (obj),
                                                  column,
                                                  &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_get_row_extent_at:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the specified table row, zero-indexed.
 * @column: the specified table column, zero-indexed.
 *
 * Gets the number of rows spanned by the table cell at the specific row
 * and column. (some tables can have cells which span multiple rows
 * and/or columns).
 *
 * Returns: a #gint indicating the number of rows spanned by the specified cell.
 **/
gint
atspi_table_get_row_extent_at (AtspiTable *obj,
                                gint         row,
                                gint         column,
                                GError **error)
{
  gint retval = -1;

  g_return_val_if_fail (obj != NULL, -1);

  a11y_atspi_table_call_get_row_extent_at_sync (get_table_proxy (obj),
                                                row, column,
                                                &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_get_column_extent_at:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the specified table row, zero-indexed.
 * @column: the specified table column, zero-indexed.
 *
 * Gets the number of columns spanned by the table cell at the specific
 * row and column (some tables can have cells which span multiple
 * rows and/or columns).
 *
 * Returns: a #gint indicating the number of columns spanned by the specified cell.
 **/
gint
atspi_table_get_column_extent_at (AtspiTable *obj,
                                   gint         row,
                                   gint         column,
                                   GError **error)
{
  gint retval = -1;

  g_return_val_if_fail (obj != NULL, -1);

  a11y_atspi_table_call_get_column_extent_at_sync (get_table_proxy (obj),
                                                   row, column,
                                                   &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_get_row_header:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the specified table row, zero-indexed.
 *
 * Gets the header associated with a table row, if available. This differs from
 * #atspi_table_get_row_description, which returns a string.
 *
 * Returns: (transfer full): an #AtspiAccessible representation of the specified
 *          table row, if available.
 **/
AtspiAccessible *
atspi_table_get_row_header (AtspiTable *obj,
			      gint         row,
			      GError **error)
{
  AtspiAccessible *accessible = NULL;
  GVariant *variant = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_table_call_get_row_header_sync (get_table_proxy (obj),
                                                 row,
                                                 &variant, NULL, error))
    {
      accessible = _atspi_dbus_return_accessible_from_variant (variant);
      g_variant_unref (variant);
    }

  return accessible;
}

/**
 * atspi_table_get_column_header:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @column: the specified table column, zero-indexed.
 *
 * Gets the header associated with a table column, if available.
 * This differs from #atspi_table_get_column_description, which
 * returns a string.
 *
 * Returns: (transfer full): an #AtspiAccessible representation of the
 *          specified table column, if available.
 **/
AtspiAccessible *
atspi_table_get_column_header (AtspiTable *obj,
				 gint column,
				 GError **error)
{
  AtspiAccessible *accessible = NULL;
  GVariant *variant = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_table_call_get_column_header_sync (get_table_proxy (obj),
                                                    column,
                                                    &variant, NULL, error))
    {
      accessible = _atspi_dbus_return_accessible_from_variant (variant);
      g_variant_unref (variant);
    }

  return accessible;
}

/**
 * atspi_table_get_n_selected_rows:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 *
 * Query a table to find out how many rows are currently selected. 
 * Not all tables support row selection.
 *
 * Returns: a #gint indicating the number of rows currently selected.
 **/
gint
atspi_table_get_n_selected_rows (AtspiTable *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, -1);

  return a11y_atspi_table_get_nselected_rows (get_table_proxy (obj));
}

/**
 * atspi_table_get_selected_rows:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 *
 * Queries a table for a list of indices of rows which are currently selected.
 *
 * Returns: (element-type gint) (transfer full): an array of #gint values,
 *          specifying which rows are currently selected.
 **/
GArray *
atspi_table_get_selected_rows (AtspiTable *obj,
                                 GError **error)
{
  GVariant *variant = NULL;
  GArray *rows = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_table_call_get_selected_rows_sync (get_table_proxy (obj),
                                                    &variant, NULL, error))
    {
      gsize length;
      const gint32 *array = g_variant_get_fixed_array (variant, &length, sizeof (gint32));

      rows = g_array_new (TRUE, TRUE, sizeof (gint32));
      g_array_append_vals (rows, array, length);
      g_variant_unref (variant);
    }

  return rows;
}

/**
 * atspi_table_get_selected_columns:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 *
 * Queries a table for a list of indices of columns which are currently
 * selected.
 *
 * Returns: (element-type gint) (transfer full): an array of #gint values,
 *          specifying which columns are currently selected.
 **/
GArray *
atspi_table_get_selected_columns (AtspiTable *obj,
                                 GError **error)
{
  GArray *columns = NULL;
  GVariant *variant = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  if (a11y_atspi_table_call_get_selected_columns_sync (get_table_proxy (obj),
                                                       &variant, NULL, error))
    {
      gsize length;
      const gint32 *array = g_variant_get_fixed_array (variant, &length, sizeof (gint32));

      columns = g_array_new (TRUE, TRUE, sizeof (gint32));
      g_array_append_vals (columns, array, length);
      g_variant_unref (variant);
    }

  return columns;
}

/**
 * atspi_table_get_n_selected_columns:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 *
 * Queries a table to find out how many columns are currently selected. 
 * Not all tables support column selection.
 *
 * Returns: a #gint indicating the number of columns currently selected.
 **/
gint
atspi_table_get_n_selected_columns (AtspiTable *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, -1);

  return a11y_atspi_table_get_nselected_columns (get_table_proxy (obj));
}

/**
 * atspi_table_is_row_selected:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the zero-indexed row number of the row being queried.
 *
 * Determines whether a table row is selected.  Not all tables support 
 * row selection.
 *
 * Returns: #TRUE if the specified row is currently selected, #FALSE if not.
 **/
gboolean
atspi_table_is_row_selected (AtspiTable *obj,
                               gint row,
                               GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_table_call_is_row_selected_sync (get_table_proxy (obj),
                                              row,
                                              &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_is_column_selected:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @column: the zero-indexed column number of the column being queried.
 *
 * Determines whether specified table column is selected.
 * Not all tables support column selection.
 *
 * Returns: #TRUE if the specified column is currently selected, #FALSE if not.
 **/
gboolean
atspi_table_is_column_selected (AtspiTable *obj,
                                  gint column,
                                  GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_table_call_is_column_selected_sync (get_table_proxy (obj),
                                                 column,
                                                 &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_add_row_selection:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the zero-indexed row number of the row being selected.
 *
 * Selects the specified row, adding it to the current row selection.
 * Not all tables support row selection.
 *
 * Returns: #TRUE if the specified row was successfully selected, #FALSE if not.
 **/
gboolean
atspi_table_add_row_selection (AtspiTable *obj,
				 gint row,
				 GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_table_call_add_row_selection_sync (get_table_proxy (obj),
                                                row,
                                                &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_add_column_selection:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @column: the zero-indexed column number of the column being selected.
 *
 * Selects the specified column, adding it to the current column selection.
 * Not all tables support column selection.
 *
 * Returns: #TRUE if the specified column was successfully selected, #FALSE if not.
 **/
gboolean
atspi_table_add_column_selection (AtspiTable *obj,
				    gint column,
				    GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_table_call_add_column_selection_sync (get_table_proxy (obj),
                                                   column,
                                                   &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_remove_row_selection:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the zero-indexed number of the row being de-selected.
 *
 * De-selects the specified row, removing it from the current row selection.
 * Not all tables support row selection.
 *
 * Returns: #TRUE if the specified row was successfully de-selected,
 * #FALSE if not.
 **/
gboolean
atspi_table_remove_row_selection (AtspiTable *obj,
				    gint row,
				    GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_table_call_remove_row_selection_sync (get_table_proxy (obj),
                                                   row,
                                                   &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_remove_column_selection:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @column: the zero-indexed column number of the column being de-selected.
 *
 * De-selects the specified column, removing it from the current column
 * selection.
 * Not all tables support column selection.
 *
 * Returns: #TRUE if the specified column was successfully de-selected,
 * #FALSE if not.
 **/
gboolean
atspi_table_remove_column_selection (AtspiTable *obj,
				       gint column,
				       GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_table_call_remove_column_selection_sync (get_table_proxy (obj),
                                                      column,
                                                      &retval, NULL, error);
  return retval;
}

/**
 * atspi_table_get_row_column_extents_at_index:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @index: the index of the #AtspiTable child whose row/column 
 * extents are requested.
 * @row: (out): back-filled with the first table row associated with
 * the cell with child index.
 * @col: (out): back-filled with the first table column associated 
 * with the cell with child index.
 * @row_extents: (out): back-filled with the number of table rows 
 * across which child i extends.
 * @col_extents: (out): back-filled with the number of table columns
 * across which child i extends.
 * @is_selected: (out): a boolean which is back-filled with #TRUE
 * if the child at index i corresponds to a selected table cell,
 * #FALSE otherwise.
 *
 * Given a child index, determines the row and column indices and 
 * extents, and whether the cell is currently selected.  If
 * the child at index is not a cell (for instance, if it is 
 * a summary, caption, etc.), #FALSE is returned.
 *
 * Example:
 * If the #AtspiTable child at index '6' extends across columns 5 and 6 of
 * row 2 of an #AtspiTable instance, and is currently selected, then
 * 
 * retval = atspi_table_get_row_column_extents_at_index (table, 6,
 *                                             row, col, 
 *                                             row_extents,
 *                                             col_extents,
 *                                             is_selected);
 * 
 * will return #TRUE, and after the call
 * row, col, row_extents, col_extents,
 * and is_selected will contain 2, 5, 1, 2, and 
 * #TRUE, respectively.
 *
 * Returns: #TRUE if the index is associated with a valid table
 * cell, #FALSE if the index does not correspond to a cell.  If 
 * #FALSE is returned, the values of the out parameters are 
 * undefined.
 **/
gboolean
atspi_table_get_row_column_extents_at_index (AtspiTable *obj,
					    gint index, gint *row, gint *col, 
					    gint *row_extents, gint *col_extents, 
					    gboolean *is_selected, GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_table_call_get_row_column_extents_at_index_sync (get_table_proxy (obj),
                                                              index,
                                                              &retval, row, col,
                                                              row_extents, col_extents, is_selected,
                                                              NULL, error);
  return retval;
}


/**
 * atspi_table_is_selected:
 * @obj: a pointer to the #AtspiTable implementor on which to operate.
 * @row: the zero-indexed row of the cell being queried.
 * @column: the zero-indexed column of the cell being queried.
 *
 * Determines whether the cell at a specific row and column is selected.
 *
 * Returns: #TRUE if the specified cell is currently selected, #FALSE if not.
 **/
gboolean
atspi_table_is_selected (AtspiTable *obj,
                            gint row,
                            gint column,
                            GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_table_call_is_selected_sync (get_table_proxy (obj),
                                          row, column,
                                          &retval, NULL, error);
  return retval;
}

static void
atspi_table_base_init (AtspiTable *klass)
{
}

GType
atspi_table_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiTable),
      (GBaseInitFunc) atspi_table_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiTable", &tinfo, 0);

  }
  return type;
}
