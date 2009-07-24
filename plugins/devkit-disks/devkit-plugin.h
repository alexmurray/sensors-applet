/*
 * Copyright (C) 2009 Pramod Dematagoda <pmd.lotr.gandalf@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DEVKIT_PLUGIN_H
#define DEVKIT_PLUGIN_H

#include <sensors-applet/sensors-applet-plugin.h>

/* This structure contains the data on the sensors in the following order:-
 * 1) The object path of the device.
 * 2) The status of the device, whether a property has changed or not. 
 * 3) The temperature reading as of the last poll to DeviceKit. 
 * 4) The next structure. 
 */
typedef struct _Dev_Info{
  const gchar *dev_path;
  gboolean dev_changed;
  gdouble old_temp;
  struct _Dev_Info *next;
} Dev_Info;

#endif /* DEVKIT_PLUGIN_H */
