/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WLDATASOURCE_H
#define WLDATASOURCE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWaylandCompositor/private/qwayland-server-wayland.h>
#include <QtWaylandCompositor/private/qtwaylandcompositorglobal_p.h>
#include <QObject>
#include <QtCore/QList>

QT_REQUIRE_CONFIG(wayland_datadevice);

QT_BEGIN_NAMESPACE

namespace QtWayland {

class DataOffer;
class DataDevice;
class DataDeviceManager;

class DataSource : public QObject, public QtWaylandServer::wl_data_source
{
public:
    DataSource(struct wl_client *client, uint32_t id, uint32_t time);
    ~DataSource() override;
    uint32_t time() const;
    QList<QString> mimeTypes() const;

    void accept(const QString &mimeType);
    void send(const QString &mimeType,int fd);
    void cancel();

    void setManager(DataDeviceManager *mgr);
    void setDevice(DataDevice *device);

    static DataSource *fromResource(struct ::wl_resource *resource);

protected:
    void data_source_offer(Resource *resource, const QString &mime_type) override;
    void data_source_destroy(Resource *resource) override;
    void data_source_destroy_resource(Resource *resource) override;

private:
    uint32_t m_time;
    QList<QString> m_mimeTypes;

    DataDevice *m_device = nullptr;
    DataDeviceManager *m_manager = nullptr;
};

}

QT_END_NAMESPACE

#endif // WLDATASOURCE_H
