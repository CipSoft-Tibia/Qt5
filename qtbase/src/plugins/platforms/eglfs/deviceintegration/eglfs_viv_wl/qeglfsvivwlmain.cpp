// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qeglfsdeviceintegration_p.h"
#include "qeglfsvivwlintegration.h"

QT_BEGIN_NAMESPACE

class QEglFSVivWaylandIntegrationPlugin : public QEglFSDeviceIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QEglFSDeviceIntegrationFactoryInterface_iid FILE "eglfs_viv_wl.json")

public:
    QEglFSDeviceIntegration *create() override { return new QEglFSVivWaylandIntegration; }
};

QT_END_NAMESPACE

#include "qeglfsvivwlmain.moc"
