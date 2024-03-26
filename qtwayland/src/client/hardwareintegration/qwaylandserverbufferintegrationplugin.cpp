// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandserverbufferintegrationplugin_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandServerBufferIntegrationPlugin::QWaylandServerBufferIntegrationPlugin(QObject *parent)
    : QObject(parent)
{
}
QWaylandServerBufferIntegrationPlugin::~QWaylandServerBufferIntegrationPlugin()
{
}

}

QT_END_NAMESPACE

#include "moc_qwaylandserverbufferintegrationplugin_p.cpp"
