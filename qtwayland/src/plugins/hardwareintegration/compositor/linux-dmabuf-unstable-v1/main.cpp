/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWaylandCompositor/private/qwlclientbufferintegrationfactory_p.h>
#include <QtWaylandCompositor/private/qwlclientbufferintegrationplugin_p.h>
#include "linuxdmabufclientbufferintegration.h"

QT_BEGIN_NAMESPACE

class QWaylandDmabufClientBufferIntegrationPlugin : public QtWayland::ClientBufferIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QtWaylandClientBufferIntegrationFactoryInterface_iid FILE "linux-dmabuf-unstable-v1.json")
public:
    QtWayland::ClientBufferIntegration *create(const QString& key, const QStringList& paramList) override;
};

QtWayland::ClientBufferIntegration *QWaylandDmabufClientBufferIntegrationPlugin::create(const QString& key, const QStringList& paramList)
{
    Q_UNUSED(paramList);
    Q_UNUSED(key);
    return new LinuxDmabufClientBufferIntegration();
}

QT_END_NAMESPACE

#include "main.moc"
