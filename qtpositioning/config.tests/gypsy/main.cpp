// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <gypsy/gypsy-satellite.h>
#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-device.h>
#include <gconf/gconf-client.h>

int main()
{
    GypsyControl *control = gypsy_control_get_default();
    GypsyDevice *device = gypsy_device_new("test");
    GypsySatellite *satellite = gypsy_satellite_new("test");

    GConfClient *client = gconf_client_get_default();
    g_object_unref(client);

    return 0;
}
