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
#include "devkit-plugin.h"

#define DEVKIT_BUS_NAME              "org.freedesktop.DeviceKit.Disks"
#define DEVKIT_DEVICE_INTERFACE_NAME "org.freedesktop.DeviceKit.Disks.Device"
#define DEVKIT_INTERFACE_NAME        "org.freedesktop.DeviceKit.Disks"
#define DEVKIT_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define DEVKIT_OBJECT_PATH           "/org/freedesktop/DeviceKit/Disks"


/*
 * Info about a single sensor
 */
typedef struct _DevInfo{
        gchar *path;
        gboolean changed;
        gdouble temp;
        DBusGProxy *sensor_proxy;
} DevInfo;

const gchar *plugin_name = "devkit-disks";

GHashTable *devices = NULL;

/* This is a global variable to allow for all the communication to DeviceKit to
 * require only one connection. */
DBusGConnection *connection;


/* This is the handler for the Changed() signal emitted by DeviceKit-Disks. */
static void devkit_changed_signal_cb(DBusGProxy *sensor_proxy) {
        const gchar *path = dbus_g_proxy_get_path(sensor_proxy);
        DevInfo *info;

        info = g_hash_table_lookup(devices, path);
        if (info)
        {
                info->changed = TRUE;
        }
}

static void devkit_plugin_get_sensors(GList **sensors) {
        DBusGProxy *proxy, *sensor_proxy;
        GError *error = NULL;
        GPtrArray *paths;
        guint i;
        DevInfo *info;

        g_type_init();

        /* This connection will be used for everything, including the obtaining
         * of sensor data. */
        connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
        if (connection == NULL)
        {
                g_printerr("Failed to open connection to DBUS: %s",
                           error->message);
                g_error_free(error);
                return;
        }

        /* This is the proxy which is only used once during the enumeration of
         * the device object paths. */
        proxy = dbus_g_proxy_new_for_name(connection,
                                          DEVKIT_BUS_NAME,
                                          DEVKIT_OBJECT_PATH,
                                          DEVKIT_INTERFACE_NAME);
        if (proxy == NULL) {
                g_printerr("Failed to open connection to DeviceKit: %s",
                           error->message);
                g_error_free(error);
                return;
        }

        /* The object paths of the disks are enumerated and placed in an array
         * of object paths. */
        if (!dbus_g_proxy_call(proxy, "EnumerateDevices", &error,
                               G_TYPE_INVALID,
                               dbus_g_type_get_collection("GPtrArray",
                                                          DBUS_TYPE_G_OBJECT_PATH),
                               &paths,
                               G_TYPE_INVALID)) {
                g_printerr("Failed to enumerate disk devices: %s",
                           error->message);
                g_error_free(error);
                return;
        }
        g_object_unref(proxy);

        for (i = 0; i < paths->len; i++) {
                /* This proxy is used to get the required data in order to build
                 * up the list of sensors. */
                GValue temp = {0}, model = {0}, id = {0};
                gchar *path = (gchar *)g_ptr_array_index(paths, i);

                sensor_proxy = dbus_g_proxy_new_for_name(connection,
                                                         DEVKIT_BUS_NAME,
                                                         path,
                                                         DEVKIT_PROPERTIES_INTERFACE);

                if (sensor_proxy == NULL) {
                        continue;
                }

                if (dbus_g_proxy_call(sensor_proxy, "Get", NULL, G_TYPE_STRING,
                                      DEVKIT_DEVICE_INTERFACE_NAME,
                                      G_TYPE_STRING,
                                      "drive-ata-smart-temperature-kelvin",
                                      G_TYPE_INVALID,
                                      G_TYPE_VALUE, &temp, G_TYPE_INVALID)) {
                        /* If the temperature property returns 0, we know this
                         * disk doesn't have a sensor, because the return
                         * value's unit is in Kelvin */
                        if (g_value_get_double(&temp) == 0.0) {
                                g_object_unref(sensor_proxy);
                                continue;
                        }
                        dbus_g_proxy_call(sensor_proxy, "Get", NULL,
                                          G_TYPE_STRING, DEVKIT_DEVICE_INTERFACE_NAME,
                                          G_TYPE_STRING, "drive-model",
                                          G_TYPE_INVALID,
                                          G_TYPE_VALUE, &model,
                                          G_TYPE_INVALID);

                        dbus_g_proxy_call(sensor_proxy, "Get", NULL,
                                          G_TYPE_STRING, DEVKIT_DEVICE_INTERFACE_NAME,
                                          G_TYPE_STRING, "device-file",
                                          G_TYPE_INVALID,
                                          G_TYPE_VALUE, &id,
                                          G_TYPE_INVALID);
                        g_object_unref(sensor_proxy);

                        sensor_proxy = dbus_g_proxy_new_for_name(connection,
                                                                 DEVKIT_BUS_NAME,
                                                                 path,
                                                                 DEVKIT_DEVICE_INTERFACE_NAME);

                        /* Use the Changed() signal emitted from DeviceKit to
                         * get the temperature without always polling. */
                        dbus_g_proxy_add_signal(sensor_proxy, "Changed",
                                                G_TYPE_INVALID);

                        dbus_g_proxy_connect_signal(sensor_proxy, "Changed",
                                                    G_CALLBACK(devkit_changed_signal_cb),
                                                    path, NULL);

                        info = g_malloc(sizeof(*info));
                        if (devices == NULL)
                        {
                                devices = g_hash_table_new(g_str_hash,
                                                           g_str_equal);
                        }
                        info->path = g_strdup(path);
                        info->sensor_proxy = sensor_proxy;
                        /* Set the device status changed as TRUE because we need
                         * to get the initial temperature reading. */
                        info->changed = TRUE;
                        g_hash_table_insert(devices, info->path, info);

                        /* Write the sensor data. */
                        sensors_applet_plugin_add_sensor(sensors,
                                                         path,
                                                         g_value_get_string(&id),
                                                         g_value_get_string(&model),
                                                         TEMP_SENSOR,
                                                         FALSE,
                                                         HDD_ICON,
                                                         DEFAULT_GRAPH_COLOR);
                        g_value_unset(&model);
                        g_value_unset(&id);
                } else {
                        g_object_unref(sensor_proxy);
                }
                g_value_unset(&temp);
                g_free(path);
        }
        g_ptr_array_free(paths, TRUE);
}

static gdouble devkit_plugin_get_sensor_value(const gchar *path,
                                              const gchar *id,
                                              SensorType type,
                                              GError **error) {
        GValue temperature = { 0, };
        DBusGProxy *sensor_proxy;
        guint count;
        DevInfo *info;

        info = (DevInfo *)g_hash_table_lookup(devices, path);
        if (info == NULL)
        {
                g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, 0,
                            "Error finding disk with path %s", path);
                return 0.0;
        }

        /* If the device has changed, we find the new temperature and return
         * it. */
        if (info->changed)
        {
                sensor_proxy = dbus_g_proxy_new_for_name(connection,
                                                         DEVKIT_BUS_NAME,
                                                         path,
                                                         DEVKIT_PROPERTIES_INTERFACE);

                if (dbus_g_proxy_call(sensor_proxy, "Get", error,
                                      G_TYPE_STRING, DEVKIT_DEVICE_INTERFACE_NAME,
                                      G_TYPE_STRING, "drive-ata-smart-temperature-kelvin", G_TYPE_INVALID,
                                      G_TYPE_VALUE, &temperature,
                                      G_TYPE_INVALID))
                {
                        info->temp = g_value_get_double(&temperature) - 273.15;
                        info->changed = FALSE;
                }
                g_object_unref(sensor_proxy);
        }
        return info->temp;

}

static GList *devkit_plugin_init(void) {
        GList *sensors = NULL;

        devkit_plugin_get_sensors(&sensors);

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
