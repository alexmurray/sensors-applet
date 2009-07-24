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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>
#include <dbus/dbus-glib.h>


#define DEVKIT_BUS_NAME              "org.freedesktop.DeviceKit.Disks"
#define DEVKIT_DEVICE_INTERFACE_NAME "org.freedesktop.DeviceKit.Disks.Device"
#define DEVKIT_INTERFACE_NAME        "org.freedesktop.DeviceKit.Disks"
#define DEVKIT_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define DEVKIT_OBJECT_PATH           "/org/freedesktop/DeviceKit/Disks"

#include "devkit-plugin.h"

const gchar *plugin_name = "devkit-disks";

Dev_Info *first_dev_struct = NULL;

/* This is a global variable to allow for all the communication to DeviceKit to require only one connection. */
DBusGConnection *connection;


/* This is the handler for the Changed() signal emitted by DeviceKit-Disks. */
static void devkit_changed_signal_cb (DBusGProxy *sensor_proxy){
  Dev_Info *movable_dev_ptr = first_dev_struct;

  const gchar *sensor_path = dbus_g_proxy_get_path (sensor_proxy);

  while (movable_dev_ptr != NULL){
    if (g_strcmp0(movable_dev_ptr->dev_path, sensor_path) == 0){
      movable_dev_ptr->dev_changed = TRUE;
      return;
  }
    else
      movable_dev_ptr = movable_dev_ptr->next;
  }

}

static void devkit_plugin_get_sensors ( GList **sensors ){
  DBusGProxy *proxy, *sensor_proxy;
  GError *error = NULL;
  GPtrArray *dev_paths;
  guint array_index;
  GValue temp = { 0, }, model = { 0, }, id = { 0, };
  Dev_Info *movable_dev_ptr;

  g_type_init ();
  

  /* This connection will stick throughout the use of the plugin, this means that one connection can be used for everything, including the obtaining of sensor data. */
  if ( (connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error)) == NULL )
    {
      g_printerr ("Failed to open connection to DBUS: %s", error->message);
      g_error_free (error);
      return;
    }
  
  /* This is the proxy which is only used once during the enumeration of the device object paths. */
  if ( (proxy = dbus_g_proxy_new_for_name (connection, DEVKIT_BUS_NAME, DEVKIT_OBJECT_PATH, DEVKIT_INTERFACE_NAME)) == NULL )
    {
      g_printerr ("Failed to open connection to DeviceKit: %s", error->message);
      g_error_free (error);
      return;
    }
  
  /* The object paths of the disks are enumerated and placed in an array of g object paths. */
  if ( (dbus_g_proxy_call (proxy, "EnumerateDevices", &error, G_TYPE_INVALID,
			   dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH), &dev_paths, 
			   G_TYPE_INVALID) ) == FALSE)
    {
      g_printerr ("Failed to enumerate disk devices: %s", error->message);
      g_error_free (error);
      return;
    }
  
  g_object_unref (proxy);

  for (array_index = 0; array_index < dev_paths->len; array_index++){

    /* This proxy is used to get the required data in order to build up the list of sensors. */
    if ( (sensor_proxy = dbus_g_proxy_new_for_name (connection, DEVKIT_BUS_NAME,
						    (gchar *) g_ptr_array_index (dev_paths, array_index ),
						    DEVKIT_PROPERTIES_INTERFACE)) != NULL ){
    
      if ( dbus_g_proxy_call ( sensor_proxy, "Get", NULL, G_TYPE_STRING,
			       DEVKIT_DEVICE_INTERFACE_NAME,
			       G_TYPE_STRING, "drive-ata-smart-temperature-kelvin", G_TYPE_INVALID,
			       G_TYPE_VALUE, &temp, G_TYPE_INVALID ) != FALSE ){
      
	/* If the temperature property returns 0, we know this disk doesn't have a sensor, because the return value's unit is the Kelvin, which is impossible in real world conditions. We therefore skip this device. */
	if ( g_value_get_double (&temp) ){
	  
	  dbus_g_proxy_call ( sensor_proxy, "Get", NULL, G_TYPE_STRING,
			      DEVKIT_DEVICE_INTERFACE_NAME,
			      G_TYPE_STRING, "drive-model", G_TYPE_INVALID,
			      G_TYPE_VALUE, &model, G_TYPE_INVALID );
	  
	  dbus_g_proxy_call ( sensor_proxy, "Get", NULL, G_TYPE_STRING,
			      DEVKIT_DEVICE_INTERFACE_NAME,
			      G_TYPE_STRING, "device-file", G_TYPE_INVALID,
			      G_TYPE_VALUE, &id, G_TYPE_INVALID );

	  g_object_unref (sensor_proxy);

	  sensor_proxy = dbus_g_proxy_new_for_name (connection, DEVKIT_BUS_NAME,
						    (gchar *) g_ptr_array_index (dev_paths, array_index ),
						    DEVKIT_DEVICE_INTERFACE_NAME);

	  /* Experimental, use the Changed() signal emitted from DeviceKit to get the temperature without always polling. */
	  dbus_g_proxy_add_signal (sensor_proxy, "Changed", G_TYPE_INVALID);

	  dbus_g_proxy_connect_signal (sensor_proxy,"Changed", G_CALLBACK(devkit_changed_signal_cb), (gchar *) g_ptr_array_index (dev_paths, array_index ), NULL);


	  if (first_dev_struct == NULL){
	    first_dev_struct = (Dev_Info *) g_malloc (sizeof(Dev_Info));
	    movable_dev_ptr = first_dev_struct;
	  }
	  else{
	    movable_dev_ptr->next = (Dev_Info *) g_malloc (sizeof(Dev_Info));
	    movable_dev_ptr = movable_dev_ptr->next;
	  }

	  movable_dev_ptr->dev_path = dbus_g_proxy_get_path (sensor_proxy);
	  /* Set the device status changed as TRUE because we need to get the initial temperature reading. */
	  movable_dev_ptr->dev_changed = TRUE;
	  movable_dev_ptr->next = NULL;
	  /* End of signal code. */

	  
	  /* Write the sensor data. */
	  sensors_applet_plugin_add_sensor (sensors,
					    (gchar *) g_ptr_array_index (dev_paths, array_index ),
					    g_value_get_string ( &id ),
					    g_value_get_string ( &model ),
					    TEMP_SENSOR,
					    FALSE,
					    HDD_ICON,
					    DEFAULT_GRAPH_COLOR);
	  g_value_unset (&model);
	  g_value_unset (&id);
	}    
	else
	  g_object_unref (sensor_proxy);
	
	g_value_unset (&temp);
	
      }

    }
    g_free (g_ptr_array_index (dev_paths, array_index));
  
  }
  
  g_ptr_array_free (dev_paths, TRUE);
  
}

static gdouble devkit_plugin_get_sensor_value(const gchar *path, 
					      const gchar *id, 
					      SensorType type,
					      GError **error) {


  GValue temperature = { 0, };
  DBusGProxy *sensor_proxy;
  guint count;
  Dev_Info *movable_dev_ptr = first_dev_struct;


  while (movable_dev_ptr != NULL){
    /* If we get a match and the device has changed, we find the new temperature and return it. */
    if (g_strcmp0(movable_dev_ptr->dev_path, path) == 0 && movable_dev_ptr->dev_changed == TRUE){

      sensor_proxy = dbus_g_proxy_new_for_name (connection, DEVKIT_BUS_NAME,
						path,
						DEVKIT_PROPERTIES_INTERFACE);
      
      dbus_g_proxy_call ( sensor_proxy, "Get", error, G_TYPE_STRING,
			  DEVKIT_DEVICE_INTERFACE_NAME,
			  G_TYPE_STRING, "drive-ata-smart-temperature-kelvin", G_TYPE_INVALID,
			  G_TYPE_VALUE, &temperature, G_TYPE_INVALID );

      movable_dev_ptr->old_temp = g_value_get_double (&temperature);
      movable_dev_ptr->dev_changed = FALSE;

      g_object_unref (sensor_proxy);

      return ( (g_value_get_double (&temperature)) - 273.15 );
    }
    /* If we get a match and the device hasn't changed, we return the old temperature. */
    else if (g_strcmp0(movable_dev_ptr->dev_path, path) == 0 && movable_dev_ptr->dev_changed == FALSE){
      return ((movable_dev_ptr->old_temp) - 273.15);
    }

    else
      movable_dev_ptr = movable_dev_ptr->next;
  }

  /* In extreme cases, return 0, this should not happen and would be the result of a bug. */
  return 0;

}

static GList *devkit_plugin_init(void) {
  GList *sensors = NULL;
  devkit_plugin_get_sensors (&sensors);

  return sensors;
}

const gchar *sensors_applet_plugin_name(void) 
{
        return plugin_name;
}

GList *sensors_applet_plugin_init(void) 
{
        return devkit_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path, 
					       const gchar *id, 
					       SensorType type,
					       GError **error) {
        return devkit_plugin_get_sensor_value(path, id, type, error);
}
