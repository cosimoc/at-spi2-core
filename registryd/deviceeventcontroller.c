/* AT-SPI - Assistive Technology Service Provider Interface
 *
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2001, 2003 Sun Microsystems Inc.,
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

/* deviceeventcontroller.c: implement the DeviceEventController interface */

#include "config.h"

#undef  SPI_XKB_DEBUG
#undef  SPI_DEBUG
#undef  SPI_KEYEVENT_DEBUG

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/time.h>

#include <glib.h>

#include "paths.h"
#include "de-types.h"
#include "de-marshaller.h"
#include "keymasks.h"

#ifdef HAVE_X11
#include "display.h"
#include "event-source.h"
#endif

#include "deviceeventcontroller.h"

#include "xml/a11y-atspi-device-event-controller.h"
#include "xml/a11y-atspi-device-event-listener.h"

static int spi_error_code = 0;
struct _SpiPoint {
    gint x;
    gint y;
};
typedef struct _SpiPoint SpiPoint;

static unsigned int mouse_mask_state = 0;
static unsigned int key_modifier_mask =
  SPI_KEYMASK_MOD1 | SPI_KEYMASK_MOD2 | SPI_KEYMASK_MOD3 | SPI_KEYMASK_MOD4 |
  SPI_KEYMASK_MOD5 | SPI_KEYMASK_SHIFT | SPI_KEYMASK_SHIFTLOCK |
  SPI_KEYMASK_CONTROL | SPI_KEYMASK_NUMLOCK;
static unsigned int _numlock_physical_mask = SPI_KEYMASK_MOD2; /* a guess, will be reset */

static gboolean have_mouse_listener = FALSE;
static gboolean have_mouse_event_listener = FALSE;


typedef struct {
  guint                             ref_count : 30;
  guint                             pending_add : 1;
  guint                             pending_remove : 1;

  Accessibility_ControllerEventMask mod_mask;
  guint32               key_val;  /* KeyCode */
} DEControllerGrabMask;


gboolean spi_controller_update_key_grabs               (SpiDEController           *controller,
							       Accessibility_DeviceEvent *recv);

static gboolean eventtype_seq_contains_event (guint32 types,
					      const Accessibility_DeviceEvent *event);
static gboolean spi_dec_poll_mouse_moving (gpointer data);
static gboolean spi_dec_poll_mouse_idle (gpointer data);

typedef struct {
  A11yAtspiDeviceEventController *dec_skel;
  guint bus_filter_id;
} SpiDEControllerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SpiDEController, spi_device_event_controller, G_TYPE_OBJECT)

static gint
spi_dec_plat_get_keycode (SpiDEController *controller,
                          gint keysym,
                          gchar *key_str,
                          gboolean fix,
                          guint *modmask)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.get_keycode)
    return klass->plat.get_keycode (controller, keysym, key_str, fix, modmask);
  else
    return keysym;
}

static guint
spi_dec_plat_mouse_check (SpiDEController *controller, 
		     int *x, int *y, gboolean *moved)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.mouse_check)
    return klass->plat.mouse_check (controller, x, y, moved);
  else
    return 0;
}

static gboolean
spi_dec_plat_grab_key (SpiDEController *controller, guint key_val, Accessibility_ControllerEventMask mod_mask)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.grab_key)
    return klass->plat.grab_key (controller, key_val, mod_mask);
  else
    return FALSE;
}

static void
spi_dec_plat_ungrab_key (SpiDEController *controller, guint key_val, Accessibility_ControllerEventMask mod_mask)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.ungrab_key)
    klass->plat.ungrab_key (controller, key_val, mod_mask);
}

static gboolean
spi_dec_plat_synth_keycode_press (SpiDEController *controller,
			 unsigned int keycode)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.synth_keycode_press)
    return klass->plat.synth_keycode_press (controller, keycode);
  else
    return FALSE;
}

static gboolean
spi_dec_plat_synth_keycode_release (SpiDEController *controller,
			   unsigned int keycode)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.synth_keycode_release)
    return klass->plat.synth_keycode_release (controller, keycode);
  else
    return FALSE;
}

static gboolean
spi_dec_plat_lock_modifiers (SpiDEController *controller, unsigned modifiers)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.lock_modifiers)
    return klass->plat.lock_modifiers (controller, modifiers);
  else
    return FALSE;
}

static gboolean
spi_dec_plat_unlock_modifiers (SpiDEController *controller, unsigned modifiers)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.unlock_modifiers)
    return klass->plat.unlock_modifiers (controller, modifiers);
  else
    return FALSE;
}

static gboolean
spi_dec_plat_synth_keystring (SpiDEController *controller, guint synth_type, gint keycode, const char *keystring)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.synth_keystring)
    return klass->plat.synth_keystring (controller, synth_type, keycode, keystring);
  else
    return FALSE;
}

static void
spi_dec_plat_emit_modifier_event (SpiDEController *controller, guint prev_mask, 
			     guint current_mask)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.emit_modifier_event)
    klass->plat.emit_modifier_event (controller, prev_mask, current_mask);
}

static void
spi_dec_plat_generate_mouse_event (SpiDEController *controller,
                                   gint x,
                                   gint y,
                                   const char *eventName)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
  if (klass->plat.generate_mouse_event)
    klass->plat.generate_mouse_event (controller, x, y, eventName);
}

static DEControllerGrabMask *
spi_grab_mask_clone (DEControllerGrabMask *grab_mask)
{
  DEControllerGrabMask *clone = g_new (DEControllerGrabMask, 1);

  memcpy (clone, grab_mask, sizeof (DEControllerGrabMask));

  clone->ref_count = 1;
  clone->pending_add = TRUE;
  clone->pending_remove = FALSE;

  return clone;
}

static void
spi_grab_mask_free (DEControllerGrabMask *grab_mask)
{
  g_free (grab_mask);
}

static gint
spi_grab_mask_compare_values (gconstpointer p1, gconstpointer p2)
{
  DEControllerGrabMask *l1 = (DEControllerGrabMask *) p1;
  DEControllerGrabMask *l2 = (DEControllerGrabMask *) p2;

  if (p1 == p2)
    {
      return 0;
    }
  else
    { 
      return ((l1->mod_mask != l2->mod_mask) || (l1->key_val != l2->key_val));
    }
}

void
spi_dec_dbus_emit (SpiDEController *controller, const char *interface,
                   const char *name, const char *minor, int a1, int a2)
{
  g_dbus_connection_emit_signal (controller->bus, NULL, SPI_DBUS_PATH_ROOT,
                                 interface, name,
                                 g_variant_new ("(siiv(so))",
                                                minor, a1, a2,
                                                g_variant_new_int32 (0),
                                                g_dbus_connection_get_unique_name (controller->bus),
                                                SPI_DBUS_PATH_ROOT),
                                 NULL);
}

static gboolean
spi_dec_poll_mouse_moved (gpointer data)
{
  SpiDEController *controller = SPI_DEVICE_EVENT_CONTROLLER(data);
  int x, y;
  gboolean moved;
  guint mask_return;

  mask_return = spi_dec_plat_mouse_check (controller, &x, &y, &moved);

  if ((mask_return & key_modifier_mask) !=
      (mouse_mask_state & key_modifier_mask)) 
    {
      spi_dec_plat_emit_modifier_event (controller, mouse_mask_state, mask_return);
      mouse_mask_state = mask_return;
    }

  return moved;
}

static gboolean
spi_dec_poll_mouse_idle (gpointer data)
{
  if (!have_mouse_event_listener && !have_mouse_listener)
    return FALSE;
  else if (!spi_dec_poll_mouse_moved (data))
    return TRUE;
  else
    {
      guint id;
      id = g_timeout_add (20, spi_dec_poll_mouse_moving, data);
      g_source_set_name_by_id (id, "[at-spi2-core] spi_dec_poll_mouse_moving");
      return FALSE;	    
    }
}

static gboolean
spi_dec_poll_mouse_moving (gpointer data)
{
  if (!have_mouse_event_listener && !have_mouse_listener)
    return FALSE;
  else if (spi_dec_poll_mouse_moved (data))
    return TRUE;
  else
    {
      guint id;
      id = g_timeout_add (100, spi_dec_poll_mouse_idle, data);
      g_source_set_name_by_id (id, "[at-spi2-core] check_release");
      return FALSE;
    }
}

/**
 * Eventually we can use this to make the marshalling of mask types
 * more sane, but for now we just use this to detect 
 * the use of 'virtual' masks such as numlock and convert them to
 * system-specific mask values (i.e. ModMask).
 * 
 **/
static Accessibility_ControllerEventMask
spi_dec_translate_mask (Accessibility_ControllerEventMask mask)
{
  Accessibility_ControllerEventMask tmp_mask;
  gboolean has_numlock;

  has_numlock = (mask & SPI_KEYMASK_NUMLOCK);
  tmp_mask = mask;
  if (has_numlock)
    {
      tmp_mask = mask ^ SPI_KEYMASK_NUMLOCK;
      tmp_mask |= _numlock_physical_mask;
    }
 
  return tmp_mask;
}

static A11yAtspiDeviceEventListener *
get_listener_proxy (DEControllerListener *listener, SpiDEController *controller)
{
  if (!listener->proxy)
    {
      listener->proxy = G_DBUS_PROXY (a11y_atspi_device_event_listener_proxy_new_sync
                                      (controller->bus,
                                       G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                       listener->bus_name, listener->path, NULL, NULL));
      g_dbus_proxy_set_default_timeout (listener->proxy, 3000);
    }

  return A11Y_ATSPI_DEVICE_EVENT_LISTENER (listener->proxy);
}

static DEControllerKeyListener *
spi_dec_key_listener_new (const char *bus_name,
			  const char *path,
			  GSList *keys,
			  const Accessibility_ControllerEventMask mask,
			  const guint32 types,
			  const Accessibility_EventListenerMode  *mode)
{
  DEControllerKeyListener *key_listener = g_new0 (DEControllerKeyListener, 1);
  key_listener->listener.bus_name = g_strdup(bus_name);
  key_listener->listener.path = g_strdup(path);
  key_listener->listener.type = SPI_DEVICE_TYPE_KBD;
  key_listener->keys = keys;
  key_listener->mask = spi_dec_translate_mask (mask);
  key_listener->listener.types = types;
  if (mode)
  {
    key_listener->mode = (Accessibility_EventListenerMode *) g_malloc(sizeof(Accessibility_EventListenerMode));
    memcpy(key_listener->mode, mode, sizeof(*mode));
  }
  else
    key_listener->mode = NULL;

#ifdef SPI_DEBUG
  g_print ("new listener, with mask %x, is_global %d, keys %p (%d)\n",
	   (unsigned int) key_listener->mask,
           (int) (mode ? mode->global : 0),
	   (void *) key_listener->keys,
	   (int) (key_listener->keys ? g_slist_length(key_listener->keys) : 0));
#endif

  return key_listener;	
}

static DEControllerListener *
spi_dec_listener_new (const char *bus_name,
		      const char *path,
		      guint32 types)
{
  DEControllerListener *listener = g_new0 (DEControllerListener, 1);
  listener->bus_name = g_strdup(bus_name);
  listener->path = g_strdup(path);
  listener->type = SPI_DEVICE_TYPE_MOUSE;
  listener->types = types;
  return listener;
}

static GSList *keylist_clone (GSList *s)
{
  GSList *d = NULL;
  GSList *l;

  for (l = s; l; l = g_slist_next(l))
  {
    Accessibility_KeyDefinition *kd = (Accessibility_KeyDefinition *)g_malloc(sizeof(Accessibility_KeyDefinition));
    if (kd)
    {
      Accessibility_KeyDefinition *kds = (Accessibility_KeyDefinition *)l->data;
      kd->keycode = kds->keycode;
      kd->keysym = kds->keysym;
      kd->keystring = g_strdup(kds->keystring);
      d = g_slist_append(d, kd);
    }
  }
  return d;
}

static void keylist_free(GSList *keys)
{
  GSList *l;

  for (l = keys; l; l = g_slist_next(l))
  {
    Accessibility_KeyDefinition *kd = (Accessibility_KeyDefinition *)l->data;
    g_free(kd->keystring);
    g_free(kd);
  }
  g_slist_free (keys);
}

static void
spi_key_listener_data_free (DEControllerKeyListener *key_listener)
{
  keylist_free(key_listener->keys);
  if (key_listener->mode) g_free(key_listener->mode);
  g_free (key_listener);
}

static void
spi_dec_listener_free (DEControllerListener    *listener)
{
  if (listener->type == SPI_DEVICE_TYPE_KBD) 
    spi_key_listener_data_free ((DEControllerKeyListener *) listener);
  else
  {
    g_free (listener->bus_name);
    g_free (listener->path);
    g_clear_object (&listener->proxy);
  }
}

static void
_register_keygrab (SpiDEController      *controller,
		   DEControllerGrabMask *grab_mask)
{
  GList *l;

  l = g_list_find_custom (controller->keygrabs_list, grab_mask,
			  spi_grab_mask_compare_values);
  if (l)
    {
      DEControllerGrabMask *cur_mask = l->data;

      cur_mask->ref_count++;
      if (cur_mask->pending_remove)
        {
          cur_mask->pending_remove = FALSE;
	}
    }
  else
    {
      controller->keygrabs_list =
        g_list_prepend (controller->keygrabs_list,
			spi_grab_mask_clone (grab_mask));
    }
}

static void
_deregister_keygrab (SpiDEController      *controller,
		     DEControllerGrabMask *grab_mask)
{
  GList *l;

  l = g_list_find_custom (controller->keygrabs_list, grab_mask,
			  spi_grab_mask_compare_values);

  if (l)
    {
      DEControllerGrabMask *cur_mask = l->data;

      cur_mask->ref_count--;
      if (cur_mask->ref_count <= 0)
        {
          cur_mask->pending_remove = TRUE;
	}
    }
}

static void
handle_keygrab (SpiDEController         *controller,
		DEControllerKeyListener *key_listener,
		void                   (*process_cb) (SpiDEController *controller,
						      DEControllerGrabMask *grab_mask))
{
  DEControllerGrabMask grab_mask = { 0 };

  grab_mask.mod_mask = key_listener->mask;
  if (g_slist_length (key_listener->keys) == 0) /* special case means AnyKey/AllKeys */
    {
      grab_mask.key_val = 0L; /* AnyKey */
#ifdef SPI_DEBUG
      fprintf (stderr, "AnyKey grab!");
#endif
      process_cb (controller, &grab_mask);
    }
  else
    {
      GSList *l;

      for (l = key_listener->keys; l; l = g_slist_next(l))
        {
	  Accessibility_KeyDefinition *keydef = l->data;
	  long key_val;
	  key_val = spi_dec_plat_get_keycode (controller, keydef->keysym, keydef->keystring, FALSE, NULL);
	  if (!key_val)
	    key_val = keydef->keycode;
	  grab_mask.key_val = key_val;
	  process_cb (controller, &grab_mask);
	}
    }
}

static gboolean
spi_controller_register_global_keygrabs (SpiDEController         *controller,
					 DEControllerKeyListener *key_listener)
{
  handle_keygrab (controller, key_listener, _register_keygrab);
  return spi_controller_update_key_grabs (controller, NULL);
}

static void
spi_controller_deregister_global_keygrabs (SpiDEController         *controller,
					   DEControllerKeyListener *key_listener)
{
  handle_keygrab (controller, key_listener, _deregister_keygrab);
  spi_controller_update_key_grabs (controller, NULL);
}

static GVariant *
append_keystroke_listener (DEControllerKeyListener *listener)
{
  GVariantBuilder builder;
  GSList *kl;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(iisi)"));
  for (kl = listener->keys; kl; kl = kl->next)
    {
      Accessibility_KeyDefinition *kd = kl->data;
      g_variant_builder_add (&builder, "(iisi)", kd->keycode, kd->keysym,
                             kd->keystring, kd->unused);
    }

  return g_variant_new ("(souu@a(iisi)u(bbb))",
                        listener->listener.bus_name, listener->listener.path,
                        listener->listener.type, listener->listener.types,
                        g_variant_builder_end (&builder), listener->mask,
                        FALSE, FALSE, FALSE);
}

static GVariant *
append_mouse_listener (DEControllerListener *listener)
{
  return g_variant_new ("(sou)",
                        listener->bus_name, listener->path, listener->types);
}

static gboolean
spi_controller_register_device_listener (SpiDEController      *controller,
					 DEControllerListener *listener)
{
  DEControllerKeyListener *key_listener;
  gboolean retval;
  
  switch (listener->type) {
  case SPI_DEVICE_TYPE_KBD:
      key_listener = (DEControllerKeyListener *) listener;

      controller->key_listeners = g_list_prepend (controller->key_listeners,
						  key_listener);
      if (key_listener->mode->global)
        {
	  retval = spi_controller_register_global_keygrabs (controller, key_listener);
	}
      else
	  retval = TRUE;
      if (retval)
        a11y_atspi_device_event_listener_emit_keystroke_listener_registered
          (get_listener_proxy (listener, controller),
           append_keystroke_listener (key_listener));
      break;
  case SPI_DEVICE_TYPE_MOUSE:
      controller->mouse_listeners = g_list_prepend (controller->mouse_listeners, listener);
      if (!have_mouse_listener)
        {
          have_mouse_listener = TRUE;
          if (!have_mouse_event_listener) {
            guint id;
            id = g_timeout_add (100, spi_dec_poll_mouse_idle, controller);
            g_source_set_name_by_id (id, "[at-spi2-core] spi_dec_poll_mouse_idle");
          }
        }
      a11y_atspi_device_event_listener_emit_device_listener_registered
        (get_listener_proxy (listener, controller),
         append_mouse_listener (listener));
      break;
  default:
      break;
  }
  return FALSE;
}

gboolean
spi_controller_notify_mouselisteners (SpiDEController                 *controller,
				      const Accessibility_DeviceEvent *event)
{
  GList   *l, *notify = NULL;
  gboolean is_consumed;
#ifdef SPI_KEYEVENT_DEBUG
  gboolean found = FALSE;
#endif
  if (!controller->mouse_listeners)
    return FALSE;

  for (l = controller->mouse_listeners; l; l = l->next)
    {
       DEControllerListener *listener = l->data;

       if (eventtype_seq_contains_event (listener->types, event))
         {
	   /* we clone (don't dup) the listener, to avoid refcount inc. */
	   notify = g_list_prepend (notify, listener);
#ifdef SPI_KEYEVENT_DEBUG
           found = TRUE;
#endif
         }
    }

#ifdef SPI_KEYEVENT_DEBUG
  if (!found)
    {
      g_print ("no match for event\n");
    }
#endif

  is_consumed = FALSE;
  for (l = notify; l && !is_consumed; l = l->next)
    {
      DEControllerListener *listener = l->data;
      a11y_atspi_device_event_listener_call_notify_event_sync (get_listener_proxy (listener, controller),
                                                               spi_dbus_marshal_deviceEvent (event),
                                                               &is_consumed, NULL, NULL);
    }

  g_list_free (notify);

#ifdef SPI_DEBUG
  if (is_consumed) g_message ("consumed\n");
#endif
  return is_consumed;
}

static gboolean
key_set_contains_key (GSList                          *key_set,
			  const Accessibility_DeviceEvent *key_event)
{
  gint i;
  gint len;
  GSList *l;

  if (!key_set)
    {
#ifdef SPI_DEBUG
      g_print ("null key set!\n");
#endif
      return TRUE;
    }

  len = g_slist_length (key_set);
  
  if (len == 0) /* special case, means "all keys/any key" */
    {
#ifdef SPI_DEBUG
      g_print ("anykey\n");	    
#endif
      return TRUE;
    }

  for (l = key_set,i = 0; l; l = g_slist_next(l),i++)
    {
      Accessibility_KeyDefinition *kd = l->data;
#ifdef SPI_KEYEVENT_DEBUG	    
      g_print ("key_set[%d] event = %d, code = %d; key_event %d, code %d, string %s\n",
                i,
                (int) kd->keysym,
                (int) kd->keycode,
                (int) key_event->id,
                (int) key_event->hw_code,
                key_event->event_string); 
#endif
      if (kd->keysym == (guint32) key_event->id)
        {
          return TRUE;
	}
      if (kd->keycode == (guint32) key_event->hw_code)
        {
          return TRUE;
	}
      if (key_event->event_string && key_event->event_string[0] &&
	  !strcmp (kd->keystring, key_event->event_string))
        {
          return TRUE;
	}
    }

  return FALSE;
}

static gboolean
eventtype_seq_contains_event (guint32 types,
				  const Accessibility_DeviceEvent *event)
{
  if (types == 0) /* special case, means "all events/any event" */
    {
      return TRUE;
    }

  return (types & (1 << event->type));
}

static gboolean
spi_key_event_matches_listener (const Accessibility_DeviceEvent *key_event,
				DEControllerKeyListener         *listener,
				gboolean                    is_system_global)
{
  if (((key_event->modifiers & 0xFF) == (guint16) (listener->mask & 0xFF)) &&
       key_set_contains_key (listener->keys, key_event) &&
       eventtype_seq_contains_event (listener->listener.types, key_event) && 
      (is_system_global == listener->mode->global))
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static GList *
spi_controller_get_keylisteners_notify_list (SpiDEController           *controller,
                                             Accessibility_DeviceEvent *key_event,
                                             gboolean                   is_system_global)
{
  GList *l, *notify = NULL;

  if (!controller->key_listeners)
    return NULL;

  /* set the NUMLOCK event mask bit if appropriate: see bug #143702 */
  if (key_event->modifiers & _numlock_physical_mask)
      key_event->modifiers |= SPI_KEYMASK_NUMLOCK;

  for (l = controller->key_listeners; l; l = l->next)
    {
       DEControllerKeyListener *key_listener = l->data;

       if (spi_key_event_matches_listener (key_event, key_listener, is_system_global))
         notify = g_list_prepend (notify, key_listener);
    }

#ifdef SPI_KEYEVENT_DEBUG
  if (!notify)
    g_print ("no match for event\n");
#endif

  return notify;
}

typedef struct {
  GList *notify;
  GList *current;
  GVariant *event;
} NotifyKeyListenersData;

static void
notify_keylisteners_data_free (gpointer _data)
{
  NotifyKeyListenersData *data = _data;
  g_clear_pointer (&data->event, g_variant_unref);
  g_list_free (data->notify);
  g_free (data);
}

static void
notify_event_done (GObject *proxy,
                   GAsyncResult *res,
                   gpointer user_data)
{
  GTask *task = user_data;
  NotifyKeyListenersData *data = g_task_get_task_data (task);
  DEControllerKeyListener *key_listener = data->current->data;
  gboolean is_consumed = FALSE;

  a11y_atspi_device_event_listener_call_notify_event_finish
    (A11Y_ATSPI_DEVICE_EVENT_LISTENER (proxy), &is_consumed, res, NULL);
  is_consumed &= key_listener->mode->preemptive;

  if (is_consumed)
    {
      g_task_return_boolean (task, TRUE);
      g_object_unref (task);
      return;
    }

  data->current = data->current->next;
  if (!data->current)
    {
      g_task_return_boolean (task, FALSE);
      g_object_unref (task);
      return;
    }

  a11y_atspi_device_event_listener_call_notify_event
    (get_listener_proxy (data->current->data, g_task_get_source_object (task)),
     data->event, NULL, notify_event_done, task);
}

static void
spi_controller_begin_notify_keylisteners (SpiDEController     *controller,
                                          GVariant            *event,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data)
{
  NotifyKeyListenersData *data = g_new0 (NotifyKeyListenersData, 1);
  Accessibility_DeviceEvent key_event;
  GTask *task;
  GList *notify = NULL;

  spi_dbus_demarshal_deviceEvent (event, &key_event);
  notify = spi_controller_get_keylisteners_notify_list (controller, &key_event, FALSE);

  task = g_task_new (controller, NULL, callback, user_data);
  data->event = g_variant_ref (event);
  data->current = data->notify = notify;
  g_task_set_task_data (task, data, notify_keylisteners_data_free);

  if (notify)
    {
      DEControllerKeyListener *key_listener = notify->data;
      a11y_atspi_device_event_listener_call_notify_event
        (get_listener_proxy ((DEControllerListener *) key_listener, controller),
         data->event, NULL, notify_event_done, g_object_ref (task));
    }
  else
    {
#ifdef SPI_KEYEVENT_DEBUG
      g_print ("no match for event\n");
#endif
      g_task_return_boolean (task, FALSE);
    }

  g_object_unref (task);
  g_free (key_event.event_string);
}

gboolean
spi_controller_notify_keylisteners (SpiDEController                 *controller,
				    Accessibility_DeviceEvent       *key_event,
				    gboolean                    is_system_global)
{
  gboolean is_consumed;
  GList *l, *notify = NULL;

  notify = spi_controller_get_keylisteners_notify_list (controller, key_event, is_system_global);
  is_consumed = FALSE;

  for (l = notify; l && !is_consumed; l = l->next)
    {
      DEControllerKeyListener *key_listener = l->data;
      a11y_atspi_device_event_listener_call_notify_event_sync (get_listener_proxy ((DEControllerListener *) key_listener, controller),
                                                               spi_dbus_marshal_deviceEvent (key_event),
                                                               &is_consumed, NULL, NULL);
      is_consumed &= key_listener->mode->preemptive;
    }

  g_list_free (notify);

#ifdef SPI_DEBUG
  if (is_consumed) g_message ("consumed\n");
#endif
  return is_consumed;
}

gboolean
spi_clear_error_state (void)
{
	gboolean retval = spi_error_code != 0;
	spi_error_code = 0;
	return retval;
}

gboolean
spi_controller_update_key_grabs (SpiDEController           *controller,
				 Accessibility_DeviceEvent *recv)
{
  GList *l, *next;
  gboolean   update_failed = FALSE;
  long keycode = 0;
  
  g_return_val_if_fail (controller != NULL, FALSE);

  /*
   * masks known to work with default RH 7.1+:
   * 0 (no mods), LockMask, Mod1Mask, Mod2Mask, ShiftMask,
   * ShiftMask|LockMask, Mod1Mask|LockMask, Mod2Mask|LockMask,
   * ShiftMask|Mod1Mask, ShiftMask|Mod2Mask, Mod1Mask|Mod2Mask,
   * ShiftMask|LockMask|Mod1Mask, ShiftMask|LockMask|Mod2Mask,
   *
   * ControlMask grabs are broken, must be in use already
   */
  if (recv)
    keycode = spi_dec_plat_get_keycode (controller, recv->id, NULL, TRUE, NULL);
  for (l = controller->keygrabs_list; l; l = next)
    {
      gboolean do_remove;
      gboolean re_issue_grab;
      DEControllerGrabMask *grab_mask = l->data;

      next = l->next;

      re_issue_grab = recv &&
	      (recv->modifiers & grab_mask->mod_mask) &&
	      (grab_mask->key_val == keycode);

#ifdef SPI_DEBUG
      fprintf (stderr, "mask=%lx %lx (%c%c) %s\n",
	       (long int) grab_mask->key_val,
	       (long int) grab_mask->mod_mask,
	       grab_mask->pending_add ? '+' : '.',
	       grab_mask->pending_remove ? '-' : '.',
	       re_issue_grab ? "re-issue": "");
#endif

      do_remove = FALSE;

      if (grab_mask->pending_add && grab_mask->pending_remove)
        {
          do_remove = TRUE;
	}
      else if (grab_mask->pending_remove)
        {
#ifdef SPI_DEBUG
      fprintf (stderr, "ungrabbing, mask=%x\n", grab_mask->mod_mask);
#endif
	  spi_dec_plat_ungrab_key (controller,
		               grab_mask->key_val,
		               grab_mask->mod_mask);

          do_remove = TRUE;
	}
      else if (grab_mask->pending_add || re_issue_grab)
        {

#ifdef SPI_DEBUG
	  fprintf (stderr, "grab %d with mask %x\n", grab_mask->key_val, grab_mask->mod_mask);
#endif
	  update_failed = spi_dec_plat_grab_key (controller,
		                               grab_mask->key_val,
		                               grab_mask->mod_mask);
	  if (update_failed) {
		  while (grab_mask->ref_count > 0) --grab_mask->ref_count;
		  do_remove = TRUE;
	  }
	}

      grab_mask->pending_add = FALSE;
      grab_mask->pending_remove = FALSE;

      if (do_remove)
        {
          g_assert (grab_mask->ref_count <= 0);

	  controller->keygrabs_list = g_list_delete_link (
	    controller->keygrabs_list, l);

	  spi_grab_mask_free (grab_mask);
	}

    } 

  return ! update_failed;
}

/*
 * Implemented GObject::finalize
 */
static void
spi_device_event_controller_object_finalize (GObject *object)
{
  SpiDEController *controller = SPI_DEVICE_EVENT_CONTROLLER (object);
  SpiDEControllerPrivate *priv = spi_device_event_controller_get_instance_private (controller);
  SpiDEControllerClass *klass;

  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (controller);
#ifdef SPI_DEBUG
  fprintf(stderr, "spi_device_event_controller_object_finalize called\n");
#endif
  if (klass->plat.finalize)
    klass->plat.finalize (controller);

  g_clear_object (&priv->dec_skel);

  g_dbus_connection_remove_filter (controller->bus, priv->bus_filter_id);

  G_OBJECT_CLASS (spi_device_event_controller_parent_class)->finalize (object);
}

/*
 * DBus Accessibility::DEController::RegisterKeystrokeListener
 *     method implementation
 */
static gboolean
handle_register_keystroke_listener (A11yAtspiDeviceEventController *skel,
                                    GDBusMethodInvocation *invocation,
                                    const gchar *listener,
                                    GVariant *keys_variant,
                                    guint mask,
                                    guint type,
                                    GVariant *mode_variant,
                                    gpointer user_data)
{
  SpiDEController *controller = user_data;
  GSList *keys = NULL;
  GVariant *value;
  GVariantIter iter;
  DEControllerKeyListener *dec_listener;
  Accessibility_EventListenerMode mode;
  gboolean ret;

  g_variant_iter_init (&iter, keys_variant);
  while ((value = g_variant_iter_next_value (&iter)))
    {
      Accessibility_KeyDefinition *kd = g_new0 (Accessibility_KeyDefinition, 1);

      g_variant_get (value, "(iisi)", &kd->keycode, &kd->keysym,
                     &kd->keystring, NULL);
      keys = g_slist_append (keys, kd);
      g_variant_unref (value);
    }

  g_variant_get (mode_variant, "(bbb)", &mode.synchronous, &mode.preemptive,
                 &mode.global);

  dec_listener = spi_dec_key_listener_new (g_dbus_method_invocation_get_sender (invocation),
                                           listener, keys, mask, type, &mode);
  ret = spi_controller_register_device_listener (controller, (DEControllerListener *) dec_listener);

  a11y_atspi_device_event_controller_complete_register_keystroke_listener
    (skel, invocation, ret);

#ifdef SPI_DEBUG
  fprintf (stderr, "registering keystroke listener %s:%s with maskVal %lu\n",
	   g_dbus_method_invocation_get_sender (invocation), listener, (unsigned long) mask);
#endif

  return TRUE;
}

/*
 * DBus Accessibility::DEController::RegisterDeviceEventListener
 *     method implementation
 */
static gboolean
handle_register_device_event_listener (A11yAtspiDeviceEventController *skel,
                                       GDBusMethodInvocation *invocation,
                                       const gchar *listener_path,
                                       guint types,
                                       gpointer user_data)
{
  SpiDEController *controller = user_data;
  DEControllerListener *dec_listener;
  gboolean ret;

  dec_listener = spi_dec_listener_new (g_dbus_method_invocation_get_sender (invocation),
                                       listener_path, types);
  ret = spi_controller_register_device_listener (controller, dec_listener);

  a11y_atspi_device_event_controller_complete_register_device_event_listener
    (skel, invocation, ret);
  return TRUE;
}



static GList *
spi_controller_deregister_device_listener_link (SpiDEController *controller,
                                                GList *link)
{
  DEControllerListener *mouse_listener = link->data;
  GList *ret;

  a11y_atspi_device_event_listener_emit_device_listener_deregistered
    (get_listener_proxy (mouse_listener, controller),
     append_mouse_listener (mouse_listener));

  ret = link->next;
  controller->mouse_listeners = g_list_delete_link (controller->mouse_listeners, link);
  if (!controller->mouse_listeners)
    have_mouse_listener = FALSE;

  return ret;
}

static void
spi_controller_deregister_device_listener (SpiDEController *controller,
                                           DEControllerListener *listener)
{
  GList *l = controller->mouse_listeners;

  while (l != NULL)
    {
      DEControllerListener *mouse_listener = l->data;
      if (!strcmp (listener->bus_name, mouse_listener->bus_name) &&
          !strcmp (listener->path, mouse_listener->path))
        l = spi_controller_deregister_device_listener_link (controller, l);
      else
        l = l->next;
    }
}

static GList *
spi_deregister_controller_key_listener_link (SpiDEController *controller,
                                             GList *link)
{
  DEControllerKeyListener *key_listener = link->data;
  GList *ret;

  a11y_atspi_device_event_listener_emit_keystroke_listener_deregistered
    (get_listener_proxy ((DEControllerListener *) key_listener, controller),
     append_keystroke_listener (key_listener));

  ret = link->next;
  controller->key_listeners = g_list_delete_link (controller->key_listeners, link);

  return ret;
}

static void
spi_deregister_controller_key_listener (SpiDEController            *controller,
					DEControllerKeyListener    *key_listener)
{
  GList *l;

  a11y_atspi_device_event_listener_emit_keystroke_listener_deregistered
    (get_listener_proxy ((DEControllerListener *) key_listener, controller),
     append_keystroke_listener (key_listener));

  /* special case, copy keyset from existing controller list entry */
  if (g_slist_length(key_listener->keys) == 0)
    {
      for (l = controller->key_listeners; l != NULL; l = l->next)
        {
          DEControllerKeyListener *listener = l->data;
          if (!strcmp (key_listener->listener.bus_name, listener->listener.bus_name) &&
              !strcmp (key_listener->listener.path, listener->listener.path))
            {
              /* TODO: FIXME aggregate keys in case the listener is registered twice */
              keylist_free (key_listener->keys);
              key_listener->keys = keylist_clone (listener->keys);
            }
        }
    }

  spi_controller_deregister_global_keygrabs (controller, key_listener);

  l = controller->key_listeners;
  while (l != NULL)
    {
      DEControllerKeyListener *listener = l->data;
      if (!strcmp (key_listener->listener.bus_name, listener->listener.bus_name) &&
          !strcmp (key_listener->listener.path, listener->listener.path))
        l = spi_deregister_controller_key_listener_link (controller, l);
      else
        l = l->next;
    }
}

void
spi_remove_device_listeners (SpiDEController *controller, const char *bus_name)
{
  GList *l;

  l = controller->mouse_listeners;
  while (l != NULL)
    {
      DEControllerListener *listener = l->data;
      if (!strcmp (listener->bus_name, bus_name))
        l = spi_controller_deregister_device_listener_link (controller, l);
      else
        l = l->next;
    }

  l = controller->key_listeners;
  while (l != NULL)
    {
      DEControllerKeyListener *key_listener = l->data;
      if (!strcmp (key_listener->listener.bus_name, bus_name))
        l = spi_deregister_controller_key_listener_link (controller, l);
      else
        l = l->next;
    }
}

/*
 * DBus Accessibility::DEController::DeregisterKeystrokeListener
 *     method implementation
 */
static gboolean
handle_deregister_keystroke_listener (A11yAtspiDeviceEventController *skel,
                                      GDBusMethodInvocation *invocation,
                                      const gchar *listener,
                                      GVariant *keys_variant,
                                      guint mask,
                                      guint type,
                                      gpointer user_data)
{
  SpiDEController *controller = user_data;
  GSList *keys = NULL;
  GVariant *value;
  GVariantIter iter;
  DEControllerKeyListener *key_listener;

  g_variant_iter_init (&iter, keys_variant);
  while ((value = g_variant_iter_next_value (&iter)))
    {
      Accessibility_KeyDefinition *kd = g_new0 (Accessibility_KeyDefinition, 1);

      g_variant_get (value, "(iisi)", &kd->keycode, &kd->keysym,
                     &kd->keystring, NULL);
      keys = g_slist_append (keys, kd);
      g_variant_unref (value);
    }

  key_listener = spi_dec_key_listener_new (g_dbus_method_invocation_get_sender (invocation),
                                           listener, keys, mask, type, NULL);
#ifdef SPI_DEREGISTER_DEBUG
  fprintf (stderr, "deregistering keystroke listener %p with maskVal %lu\n",
	   (void *) l, (unsigned long) mask->value);
#endif

  spi_deregister_controller_key_listener (controller, key_listener);

  spi_dec_listener_free ((DEControllerListener *) key_listener);

  a11y_atspi_device_event_controller_complete_deregister_keystroke_listener (skel, invocation);
  return TRUE;
}

/*
 * DBus Accessibility::DEController::DeregisterDeviceEventListener
 *     method implementation
 */
static gboolean
handle_deregister_device_event_listener (A11yAtspiDeviceEventController *skel,
                                         GDBusMethodInvocation *invocation,
                                         const gchar *listener_path,
                                         guint types,
                                         gpointer user_data)
{
  SpiDEController *controller = user_data;
  DEControllerListener *listener;

  listener = spi_dec_listener_new (g_dbus_method_invocation_get_sender (invocation),
                                   listener_path, types);
  spi_controller_deregister_device_listener (controller, listener);

  a11y_atspi_device_event_controller_complete_deregister_device_event_listener (skel, invocation);
  return TRUE;
}

static gboolean
handle_get_keystroke_listeners (A11yAtspiDeviceEventController *skel,
                                GDBusMethodInvocation *invocation,
                                gpointer user_data)
{
  SpiDEController *controller = user_data;
  GVariantBuilder builder;
  GList *l;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(souua(iisi)u(bbb))"));
  for (l = controller->key_listeners; l; l = l->next)
    {
      GVariant *variant = append_keystroke_listener (l->data);
      g_variant_builder_add_value (&builder, variant);
    }

  a11y_atspi_device_event_controller_complete_get_keystroke_listeners
    (skel, invocation, g_variant_builder_end (&builder));
  return TRUE;
}

static gboolean
handle_get_device_event_listeners (A11yAtspiDeviceEventController *skel,
                                   GDBusMethodInvocation *invocation,
                                   gpointer user_data)
{
  SpiDEController *controller = user_data;
  GVariantBuilder builder;
  GList *l;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(sou)"));
  for (l = controller->key_listeners; l; l = l->next)
    {
      GVariant *variant = append_mouse_listener (l->data);
      g_variant_builder_add_value (&builder, variant);
    }

  a11y_atspi_device_event_controller_complete_get_device_event_listeners
    (skel, invocation, g_variant_builder_end (&builder));
  return TRUE;
}

static unsigned
get_modifier_state (SpiDEController *controller)
{
	return mouse_mask_state;
}

gboolean
spi_dec_synth_keysym (SpiDEController *controller, long keysym)
{
	long key_synth_code;
	unsigned int modifiers, synth_mods, lock_mods;

	key_synth_code = spi_dec_plat_get_keycode (controller, keysym, NULL, TRUE, &synth_mods);

	if ((key_synth_code == 0) || (synth_mods == 0xFF)) return FALSE;

	/* TODO: set the modifiers accordingly! */
	modifiers = get_modifier_state (controller);
	/* side-effect; we may unset mousebutton modifiers here! */

	lock_mods = 0;
	if (synth_mods != modifiers) {
		lock_mods = synth_mods & ~modifiers;
		spi_dec_plat_lock_modifiers (controller, lock_mods);
	}
	spi_dec_plat_synth_keycode_press (controller, key_synth_code);
	spi_dec_plat_synth_keycode_release (controller, key_synth_code);

	if (synth_mods != modifiers) 
		spi_dec_plat_unlock_modifiers (controller, lock_mods);
	return TRUE;
}



/*
 * DBus Accessibility::DEController::RegisterKeystrokeListener
 *     method implementation
 */
static gboolean
handle_generate_keyboard_event (A11yAtspiDeviceEventController *skel,
                                GDBusMethodInvocation *invocation,
                                gint keycode,
                                const char *keystring,
                                guint type,
                                gpointer user_data)
{
  SpiDEController *controller = user_data;

#ifdef SPI_DEBUG
	fprintf (stderr, "synthesizing keystroke %ld, type %d\n",
		 (long) keycode, (int) type);
#endif
  /* TODO: hide/wrap/remove X dependency */

  /*
   * TODO: when initializing, query for XTest extension before using,
   * and fall back to XSendEvent() if XTest is not available.
   */
  
  switch (type)
    {
      case Accessibility_KEY_PRESS:
	      spi_dec_plat_synth_keycode_press (controller, keycode);
	      break;
      case Accessibility_KEY_PRESSRELEASE:
	      spi_dec_plat_synth_keycode_press (controller, keycode);
      case Accessibility_KEY_RELEASE:
	      spi_dec_plat_synth_keycode_release (controller, keycode);
	      break;
      case Accessibility_KEY_SYM:
#ifdef SPI_XKB_DEBUG	      
	      fprintf (stderr, "KeySym synthesis\n");
#endif
	      /* 
	       * note: we are using long for 'keycode'
	       * in our arg list; it can contain either
	       * a keycode or a keysym.
	       */
	      spi_dec_synth_keysym (controller, keycode);
	      break;
      case Accessibility_KEY_STRING:
	      if (!spi_dec_plat_synth_keystring (controller, type, keycode, keystring))
		      fprintf (stderr, "Keystring synthesis failure, string=%s\n",
			       keystring);
	      break;
    }

  a11y_atspi_device_event_controller_complete_generate_keyboard_event (skel, invocation);
  return TRUE;
}

/* Accessibility::DEController::GenerateMouseEvent */
static gboolean
handle_generate_mouse_event (A11yAtspiDeviceEventController *skel,
                             GDBusMethodInvocation *invocation,
                             gint x,
                             gint y,
                             const char *event_name,
                             gpointer user_data)
{
  SpiDEController *dec = user_data;

#ifdef SPI_DEBUG
  fprintf (stderr, "generating mouse %s event at %ld, %ld\n",
	   event_name, (long int) x, (long int) y);
#endif

  spi_dec_plat_generate_mouse_event (dec, x, y, event_name);
  a11y_atspi_device_event_controller_complete_generate_mouse_event (skel, invocation);
  return TRUE;
}

static void
notify_keylisteners_done (GObject *source,
                          GAsyncResult *res,
                          gpointer user_data)
{
  GDBusMethodInvocation *invocation = user_data;
  SpiDEController *controller = SPI_DEVICE_EVENT_CONTROLLER (source);
  SpiDEControllerPrivate *priv = spi_device_event_controller_get_instance_private (controller);
  gboolean is_consumed;

  is_consumed = g_task_propagate_boolean (G_TASK (res), NULL);
#ifdef SPI_DEBUG
  g_print ("consumed %d\n", is_consumed);
#endif
  if (invocation)
    a11y_atspi_device_event_controller_complete_notify_listeners_sync
      (priv->dec_skel, invocation, is_consumed);
}

/* Accessibility::DEController::NotifyListenersSync */
static gboolean
handle_notify_listeners_sync (A11yAtspiDeviceEventController *skel,
                              GDBusMethodInvocation *invocation,
                              GVariant *variant,
                              gpointer user_data)
{
  SpiDEController *controller = user_data;

#ifdef SPI_DEBUG
  char *str = g_variant_print (variant, TRUE);
  g_print ("notifylistening listeners synchronously: controller %p, event %s\n",
	   controller, str);
  g_free (str);
#endif
  spi_controller_begin_notify_keylisteners (controller, variant,
                                            notify_keylisteners_done, invocation);
  return TRUE;
}

static gboolean
handle_notify_listeners_async (A11yAtspiDeviceEventController *skel,
                               GDBusMethodInvocation *invocation,
                               GVariant *variant,
                               gpointer user_data)
{
  SpiDEController *controller = user_data;

#ifdef SPI_DEBUG
  char *str = g_variant_print (variant, TRUE);
  g_print ("notifylistening listeners asynchronously: controller %p, event %s\n",
	   controller, str);
  g_free (str);
#endif
  spi_controller_begin_notify_keylisteners (controller, variant,
                                            notify_keylisteners_done, NULL);

  a11y_atspi_device_event_controller_complete_notify_listeners_async (skel, invocation);
  return TRUE;
}

static void
spi_device_event_controller_class_init (SpiDEControllerClass *klass)
{
  GObjectClass * object_class = (GObjectClass *) klass;

  object_class->finalize = spi_device_event_controller_object_finalize;

#ifdef HAVE_X11
  if (g_getenv ("DISPLAY"))
    spi_dec_setup_x11 (klass);
#endif
}

static void
spi_device_event_controller_init (SpiDEController *device_event_controller)
{
  SpiDEControllerClass *klass;
  klass = SPI_DEVICE_EVENT_CONTROLLER_GET_CLASS (device_event_controller);

  if (klass->plat.init)
    klass->plat.init (device_event_controller);
}

/*---------------------------------------------------------------------------*/

#define DEVICE_EVENT_OLD_TYPE "(uinnisb)"
#define DEVICE_EVENT_NEW_TYPE "(uiuuisb)"

static GVariant *
adjust_device_event (GVariant *event)
{
  guint type, hw_code, modifiers;
  gint id, timestamp;
  gboolean is_text;
  const char *event_string;

  g_variant_get (event, "((uinni&sb))", &type, &id, &hw_code, &modifiers,
                 &timestamp, &event_string, &is_text);
  return g_variant_new ("(" DEVICE_EVENT_NEW_TYPE ")", type, id, hw_code, modifiers,
                        timestamp, event_string, is_text);
}

static GDBusMessage *
dec_dbus_filter (GDBusConnection *connection,
                 GDBusMessage *message,
                 gboolean incoming,
                 gpointer user_data)
{
  GDBusMessage *copy;
  GVariant *body;

  if (!incoming)
    return message;

  if (g_dbus_message_get_message_type (message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL &&
      g_strcmp0 (g_dbus_message_get_interface (message), SPI_DBUS_INTERFACE_DEC) == 0 &&
      g_strcmp0 (g_dbus_message_get_path (message), SPI_DBUS_PATH_DEC) == 0 &&
      (g_strcmp0 (g_dbus_message_get_member (message), "NotifyListenersSync") == 0 ||
       g_strcmp0 (g_dbus_message_get_member (message), "NotifyListenersAsync") == 0) &&
      g_strcmp0 (g_dbus_message_get_signature (message), DEVICE_EVENT_OLD_TYPE) == 0)
    {
      copy = g_dbus_message_copy (message, NULL);
      if (!copy)
        goto out;

      /* Adjust for old signature of NotifyListenersSync/NotifyListenersAsync */
      body = g_dbus_message_get_body (message);
      g_dbus_message_set_body (copy, adjust_device_event (body));
      g_dbus_message_set_signature (copy, DEVICE_EVENT_NEW_TYPE);

      g_object_unref (message);
      message = copy;
    }

 out:
  return message;
}

SpiDEController *
spi_registry_dec_new (SpiRegistry *reg, GDBusConnection *bus)
{
  SpiDEController *dec = g_object_new (SPI_DEVICE_EVENT_CONTROLLER_TYPE, NULL);
  SpiDEControllerPrivate *priv = spi_device_event_controller_get_instance_private (dec);

  dec->registry = g_object_ref (reg);
  reg->dec = g_object_ref (dec);
  dec->bus = bus;

  priv->bus_filter_id =
    g_dbus_connection_add_filter (dec->bus, dec_dbus_filter, dec, NULL);

  priv->dec_skel = a11y_atspi_device_event_controller_skeleton_new ();
  g_signal_connect (priv->dec_skel, "handle-deregister-device-event-listener",
                    G_CALLBACK (handle_deregister_device_event_listener), dec);
  g_signal_connect (priv->dec_skel, "handle-deregister-keystroke-listener",
                    G_CALLBACK (handle_deregister_keystroke_listener), dec);
  g_signal_connect (priv->dec_skel, "handle-generate-keyboard-event",
                    G_CALLBACK (handle_generate_keyboard_event), dec);
  g_signal_connect (priv->dec_skel, "handle-generate-mouse-event",
                    G_CALLBACK (handle_generate_mouse_event), dec);
  g_signal_connect (priv->dec_skel, "handle-get-device-event-listeners",
                    G_CALLBACK (handle_get_device_event_listeners), dec);
  g_signal_connect (priv->dec_skel, "handle-get-keystroke-listeners",
                    G_CALLBACK (handle_get_keystroke_listeners), dec);
  g_signal_connect (priv->dec_skel, "handle-notify-listeners-async",
                    G_CALLBACK (handle_notify_listeners_async), dec);
  g_signal_connect (priv->dec_skel, "handle-notify-listeners-sync",
                    G_CALLBACK (handle_notify_listeners_sync), dec);
  g_signal_connect (priv->dec_skel, "handle-register-device-event-listener",
                    G_CALLBACK (handle_register_device_event_listener), dec);
  g_signal_connect (priv->dec_skel, "handle-register-keystroke-listener",
                    G_CALLBACK (handle_register_keystroke_listener), dec);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->dec_skel),
                                    bus, SPI_DBUS_PATH_DEC, NULL);

  return dec;
}

void
spi_device_event_controller_start_poll_mouse (SpiRegistry *registry)
{
  if (!have_mouse_event_listener)
    {
      have_mouse_event_listener = TRUE;
      if (!have_mouse_listener) {
        guint id;
        id = g_timeout_add (100, spi_dec_poll_mouse_idle, registry->dec);
        g_source_set_name_by_id (id, "[at-spi2-core] spi_dec_poll_mouse_idle");
      }
    }
}

void
spi_device_event_controller_stop_poll_mouse (void)
{
  have_mouse_event_listener = FALSE;
}
