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

#include "xml/a11y-atspi-action.h"

static A11yAtspiAction *
get_action_proxy (AtspiAction *action)
{
  return atspi_accessible_get_iface_proxy
    (ATSPI_ACCESSIBLE (action), (AtspiAccessibleProxyInit) a11y_atspi_action_proxy_new_sync,
     "a11y-atspi-action-proxy");
}

/**
 * atspi_action_get_n_actions:
 * @obj: a pointer to the #AtspiAction to query.
 *
 * Get the number of actions invokable on an #AtspiAction implementor.
 *
 * Returns: an integer indicating the number of invocable actions.
 **/
gint
atspi_action_get_n_actions (AtspiAction *obj, GError **error)
{
  g_return_val_if_fail (obj != NULL, -1);

  return a11y_atspi_action_get_nactions (get_action_proxy (obj));
}

/**
 * atspi_action_get_description: (rename-to atspi_action_get_action_description)
 * @obj: a pointer to the #AtspiAction implementor to query.
 * @i: an integer indicating which action to query.
 *
 * Get the description of '@i-th' action invocable on an
 *      object implementing #AtspiAction.
 *
 * Returns: a UTF-8 string describing the '@i-th' invocable action.
 *
 * Deprecated: 2.10: Use atspi_action_get_action_description instead.
 **/
gchar *
atspi_action_get_description (AtspiAction *obj, int i, GError **error)
{
  return atspi_action_get_action_description (obj, i, error);
}

/**
 * atspi_action_get_action_description:
 * @obj: a pointer to the #AtspiAction implementor to query.
 * @i: an integer indicating which action to query.
 *
 * Get the description of '@i-th' action invocable on an
 *      object implementing #AtspiAction.
 *
 * Returns: a UTF-8 string describing the '@i-th' invocable action.
 **/
gchar *
atspi_action_get_action_description (AtspiAction *obj, int i, GError **error)
{
  char *retval = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  a11y_atspi_action_call_get_description_sync (get_action_proxy (obj),
                                               i, &retval, NULL, error);
  return retval;
}

/**
 * atspi_action_get_key_binding:
 * @obj: a pointer to the #AtspiAction implementor to query.
 * @i: an integer indicating which action to query.
 *
 * Get the keybindings for the @i-th action invocable on an
 *      object implementing #AtspiAction, if any are defined.
 *      The keybindings string format is as follows:
 *        there are multiple parts to a keybinding string (typically 3).
 *        They are delimited with ";".  The first is the action's
 *        keybinding which is usable if the object implementing the action
 *        is currently posted to the screen, e.g. if a menu is posted 
 *        then these keybindings for the corresponding menu-items are
 *        available.  The second keybinding substring is the full key sequence
 *        necessary to post the action's widget and activate it, e.g. for
 *        a menu item such as "File->Open" it would both post the menu and
 *        activate the item.  Thus the second keybinding string is available
 *        during the lifetime of the containing toplevel window as a whole,
 *        whereas the first keybinding string only works while the object
 *        implementing AtkAction is posted.  The third (and optional)
 *        keybinding string is the "keyboard shortcut" which invokes the 
 *        action without posting any menus. 
 *        Meta-keys are indicated by the conventional strings
 *        "&lt;Control&gt;", "&lt;Alt&gt;", "&lt;Shift&gt;", "&lt;Mod2&gt;",
 *        etc. (we use the same string as gtk_accelerator_name() in 
 *        gtk+-2.X.
 *
 * Returns: a UTF-8 string which can be parsed to determine the @i-th
 *       invocable action's keybindings.
 **/
gchar *
atspi_action_get_key_binding (AtspiAction *obj, gint i, GError **error)
{
  char *retval = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  a11y_atspi_action_call_get_key_binding_sync (get_action_proxy (obj),
                                               i, &retval, NULL, error);
  return retval;
}

/**
 * atspi_action_get_name: (rename-to atspi_action_get_action_name)
 * @obj: a pointer to the #AtspiAction implementor to query.
 * @i: an integer indicating which action to query.
 *
 * Get the name of the '@i-th' action invocable on an
 *      object implementing #AtspiAction.
 *
 * Returns: the non-localized name of the action, as a UTF-8 string.
 *
 * Deprecated: 2.10: Use atspi_action_get_action_name instead.
 **/
gchar *
atspi_action_get_name (AtspiAction *obj, gint i, GError **error)
{
  return atspi_action_get_action_name (obj, i, error);
}

/**
 * atspi_action_get_action_name:
 * @obj: a pointer to the #AtspiAction implementor to query.
 * @i: an integer indicating which action to query.
 *
 * Get the name of the '@i-th' action invocable on an
 *      object implementing #AtspiAction.
 *
 * Returns: the non-localized name of the action, as a UTF-8 string.
 **/
gchar *
atspi_action_get_action_name (AtspiAction *obj, gint i, GError **error)
{
  char *retval = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  a11y_atspi_action_call_get_name_sync (get_action_proxy (obj),
                                        i, &retval, NULL, error);
  return retval;
}

/**
 * atspi_action_get_localized_name:
 * @obj: a pointer to the #AtspiAction implementor to query.
 * @i: an integer indicating which action to query.
 *
 * Get the name of the '@i-th' action invocable on an
 *      object implementing #AtspiAction.
 *
 * Returns: the name of the action, as a UTF-8 string.
 **/
gchar *
atspi_action_get_localized_name (AtspiAction *obj, gint i, GError **error)
{
  char *retval = NULL;

  g_return_val_if_fail (obj != NULL, NULL);

  a11y_atspi_action_call_get_localized_name_sync (get_action_proxy (obj),
                                                  i, &retval, NULL, error);

  return retval;
}

/**
 * atspi_action_do_action:
 * @obj: a pointer to the #AtspiAction to query.
 * @i: an integer specifying which action to invoke.
 *
 * Invoke the action indicated by #index.
 *
 * Returns: #TRUE if the action is successfully invoked, otherwise #FALSE.
 **/
gboolean
atspi_action_do_action (AtspiAction *obj, gint i, GError **error)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (obj != NULL, FALSE);

  a11y_atspi_action_call_do_action_sync (get_action_proxy (obj),
                                         i, &retval, NULL, error);
  return retval;
}

static void
atspi_action_base_init (AtspiAction *klass)
{
}

GType
atspi_action_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo tinfo =
    {
      sizeof (AtspiAction),
      (GBaseInitFunc) atspi_action_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "AtspiAction", &tinfo, 0);

  }
  return type;
}
