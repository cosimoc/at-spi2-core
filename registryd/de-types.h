/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2009  Codethink Ltd
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

#ifndef SPI_DE_TYPES_H_
#define SPI_DE_TYPES_H_

typedef unsigned long Accessibility_ControllerEventMask;

typedef enum {
    Accessibility_KEY_PRESSED_EVENT,
    Accessibility_KEY_RELEASED_EVENT,
    Accessibility_BUTTON_PRESSED_EVENT,
    Accessibility_BUTTON_RELEASED_EVENT,
} Accessibility_EventType;

typedef enum {
    Accessibility_KEY_PRESSED,
    Accessibility_KEY_RELEASED,
} Accessibility_KeyEventType;

typedef enum {
    Accessibility_KEY_PRESS,
    Accessibility_KEY_RELEASE,
    Accessibility_KEY_PRESSRELEASE,
    Accessibility_KEY_SYM,
    Accessibility_KEY_STRING,
} Accessibility_KeySynthType;

typedef struct _Accessibility_DeviceEvent Accessibility_DeviceEvent;
struct _Accessibility_DeviceEvent
{
  Accessibility_EventType type;
  guint32 id;
  guint32 hw_code;
  guint32 modifiers;
  guint32 timestamp;
  char * event_string;
  gboolean is_text;
};

typedef struct _Accessibility_EventListenerMode Accessibility_EventListenerMode;
struct _Accessibility_EventListenerMode
{
  gboolean synchronous;
  gboolean preemptive;
  gboolean global;
};

typedef struct _Accessibility_KeyDefinition Accessibility_KeyDefinition;
struct _Accessibility_KeyDefinition
{
  gint32 keycode;
  gint32 keysym;
  char *keystring;
  gint32 unused;
};

#endif /* SPI_DE_TYPES_H_ */
