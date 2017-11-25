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

#include "atspi-private.h"

#include "xml/a11y-atspi-editable-text.h"

static A11yAtspiEditableText *
get_editable_text_proxy (AtspiEditableText *editable_text)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (editable_text), (AtspiAccessibleProxyInit) a11y_atspi_editable_text_proxy_new_sync,
     "a11y-atspi-editable-text-proxy");
}

/**
 * atspi_editable_text_set_text_contents:
 * @obj: a pointer to the #AtspiEditableText object to modify.
 * @new_contents: a character string, encoded in UTF-8, which is to
 *      become the new text contents of the #AtspiEditableText object.
 *
 * Replace the entire text contents of an #AtspiEditableText object.
 *
 * Returns: #TRUE if the operation was successful, otherwise #FALSE.
 **/
gboolean
atspi_editable_text_set_text_contents (AtspiEditableText *obj,
                                       const gchar *new_contents,
                                       GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_editable_text_call_set_text_contents_sync (get_editable_text_proxy (obj),
                                                        new_contents,
                                                        &retval, NULL, error);
  return retval;
}

/**
 * atspi_editable_text_insert_text:
 * @obj: a pointer to the #AtspiEditableText object to modify.
 * @position: a #gint indicating the character offset at which to insert
 *       the new text.  
 * @text: a string representing the text to insert, in UTF-8 encoding.
 * @length:  the number of characters of text to insert. If the character
 * count of text is less than or equal to length, the entire contents
 * of text will be inserted.
 *
 * Inserts text into an #AtspiEditableText object.
 * As with all character offsets, the specified @position may not be the
 * same as the resulting byte offset, since the text is in a
 * variable-width encoding.
 *
 * Returns: #TRUE if the operation was successful, otherwise #FALSE.
 **/
gboolean
atspi_editable_text_insert_text (AtspiEditableText *obj,
                                 gint position,
                                 const gchar *text,
                                 gint length,
                                 GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_editable_text_call_insert_text_sync (get_editable_text_proxy (obj),
                                                  position, text, length,
                                                  &retval, NULL, error);
  return retval;
}

/**
 * atspi_editable_text_copy_text:
 * @obj: a pointer to the #AtspiEditableText object to modify.
 * @start_pos: a #gint indicating the starting character offset
 *       of the text to copy.
 * @end_pos: a #gint indicating the offset of the first character
 *       past the end of the text section to be copied.
 *
 * Copies text from an #AtspiEditableText object into the system clipboard.
 *
 * see: #atspi_editable_text_paste_text 
 *
 * Returns: #TRUE if the operation was successful, otherwise #FALSE.
 **/
gboolean
atspi_editable_text_copy_text (AtspiEditableText *obj,
                               gint start_pos,
                               gint end_pos,
                               GError **error)
{
  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_editable_text_call_copy_text_sync (get_editable_text_proxy (obj),
                                                start_pos, end_pos,
                                                NULL, error);
  return TRUE;
}

/**
 * atspi_editable_text_cut_text:
 * @obj: a pointer to the #AtspiEditableText object to modify.
 * @start_pos: a #gint indicating the starting character offset
 *       of the text to cut.
 * @end_pos: a #gint indicating the offset of the first character
 *       past the end of the text section to be cut.
 *
 * Deletes text from an #AtspiEditableText object, copying the
 *       excised portion into the system clipboard.
 *
 * see: #atspi_editable_text_paste_text
 *
 * Returns: #TRUE if operation was successful, #FALSE otherwise.
 **/
gboolean
atspi_editable_text_cut_text (AtspiEditableText *obj,
                              gint start_pos,
                              gint end_pos,
                              GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_editable_text_call_cut_text_sync (get_editable_text_proxy (obj),
                                               start_pos, end_pos,
                                               &retval, NULL, error);
  return retval;
}

/**
 * atspi_editable_text_delete_text:
 * @obj: a pointer to the #AtspiEditableText object to modify.
 * @start_pos: a #gint indicating the starting character offset
 *       of the text to delete.
 * @end_pos: a #gint indicating the offset of the first character
 *       past the end of the text section to be deleted.
 *
 * Deletes text from an #AtspiEditableText object, without copying the
 *       excised portion into the system clipboard.
 *
 * see: #atspi_editable_text_cut_text
 *
 * Returns: #TRUE if the operation was successful, otherwise #FALSE.
 **/
gboolean
atspi_editable_text_delete_text (AtspiEditableText *obj,
                                 gint start_pos,
                                 gint end_pos,
                                 GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_editable_text_call_delete_text_sync (get_editable_text_proxy (obj),
                                                  start_pos, end_pos,
                                                  &retval, NULL, error);
  return retval;
}

/**
 * atspi_editable_text_paste_text:
 * @obj: a pointer to the #AtspiEditableText object to modify.
 * @position: a #gint indicating the character offset at which to insert
 *       the new text.  
 *
 * Inserts text from the system clipboard into an #AtspiEditableText object.
 * As with all character offsets, the specified @position may not be the
 *       same as the resulting byte offset, since the text is in a
 *       variable-width encoding.
 *
 * Returns: #TRUE if the operation was successful, otherwise #FALSE.
 **/
gboolean
atspi_editable_text_paste_text (AtspiEditableText *obj,
                                gint position,
                                GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_editable_text_call_paste_text_sync (get_editable_text_proxy (obj),
                                                 position,
                                                 &retval, NULL, error);
  return retval;
}

static void
atspi_editable_text_base_init (AtspiEditableText *klass)
{
}

GType
atspi_editable_text_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiEditableText),
      (GBaseInitFunc) atspi_editable_text_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiEditableText", &tinfo, 0);

  }
  return type;
}
