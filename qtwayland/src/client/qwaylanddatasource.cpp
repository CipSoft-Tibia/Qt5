/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwaylanddatasource_p.h"
#include "qwaylanddataoffer_p.h"
#include "qwaylanddatadevicemanager_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandmimehelper_p.h"

#include <QtCore/QFile>

#include <QtCore/QDebug>

#include <unistd.h>
#include <signal.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandDataSource::QWaylandDataSource(QWaylandDataDeviceManager *dataDeviceManager, QMimeData *mimeData)
    : QtWayland::wl_data_source(dataDeviceManager->create_data_source())
    , m_mime_data(mimeData)
{
    if (!mimeData)
        return;
    const auto formats = QInternalMimeData::formatsHelper(mimeData);
    for (const QString &format : formats) {
        offer(format);
    }
}

QWaylandDataSource::~QWaylandDataSource()
{
    destroy();
}

QMimeData * QWaylandDataSource::mimeData() const
{
    return m_mime_data;
}

void QWaylandDataSource::data_source_cancelled()
{
    Q_EMIT cancelled();
}

void QWaylandDataSource::data_source_send(const QString &mime_type, int32_t fd)
{
    QByteArray content = QWaylandMimeHelper::getByteArray(m_mime_data, mime_type);
    if (!content.isEmpty()) {
        // Create a sigpipe handler that does nothing, or clients may be forced to terminate
        // if the pipe is closed in the other end.
        struct sigaction action, oldAction;
        action.sa_handler = SIG_IGN;
        sigemptyset (&action.sa_mask);
        action.sa_flags = 0;

        sigaction(SIGPIPE, &action, &oldAction);
        write(fd, content.constData(), content.size());
        sigaction(SIGPIPE, &oldAction, nullptr);
    }
    close(fd);
}

void QWaylandDataSource::data_source_target(const QString &mime_type)
{
    m_accepted = !mime_type.isEmpty();
    Q_EMIT dndResponseUpdated(m_accepted, m_dropAction);
}

void QWaylandDataSource::data_source_action(uint32_t action)
{
    Qt::DropAction qtAction = Qt::IgnoreAction;

    if (action == WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE)
        qtAction = Qt::MoveAction;
    else if (action == WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY)
        qtAction = Qt::CopyAction;

    m_dropAction = qtAction;
    Q_EMIT dndResponseUpdated(m_accepted, m_dropAction);
}

void QWaylandDataSource::data_source_dnd_finished()
{
    Q_EMIT finished();
}

void QWaylandDataSource::data_source_dnd_drop_performed()
{

    Q_EMIT dndDropped(m_accepted, m_dropAction);
}

}

QT_END_NAMESPACE
