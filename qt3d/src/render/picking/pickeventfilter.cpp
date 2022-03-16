/****************************************************************************
**
** Copyright (C) 2015 Paul Lemire paul.lemire350@gmail.com
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#include "pickeventfilter_p.h"

#include <QtCore/QMutexLocker>
#include <QtGui/QHoverEvent>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

namespace Render {

PickEventFilter::PickEventFilter(QObject *parent)
    : QObject(parent)
{
}

PickEventFilter::~PickEventFilter()
{
}

/*!
    \internal
    Called from a worker thread in the thread pool so be sure to
    mutex protect the data.
*/
QList<QPair<QObject *, QMouseEvent> > PickEventFilter::pendingMouseEvents()
{
    QMutexLocker locker(&m_mutex);
    QList<QPair<QObject*, QMouseEvent>> pendingEvents(m_pendingMouseEvents);
    m_pendingMouseEvents.clear();
    return pendingEvents;
}

QList<QKeyEvent> PickEventFilter::pendingKeyEvents()
{
    QMutexLocker locker(&m_mutex);
    QList<QKeyEvent> pendingEvents(m_pendingKeyEvents);
    m_pendingKeyEvents.clear();
    return pendingEvents;
}

/*!
    \internal
    Called from the main thread.
*/
bool PickEventFilter::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj);
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove: {
        QMutexLocker locker(&m_mutex);
        m_pendingMouseEvents.push_back({obj, QMouseEvent(*static_cast<QMouseEvent *>(e))});
    } break;
    case QEvent::HoverMove: {
        QMutexLocker locker(&m_mutex);
        QHoverEvent *he = static_cast<QHoverEvent *>(e);
        m_pendingMouseEvents.push_back({obj, QMouseEvent(QEvent::MouseMove,
                                                   he->pos(), Qt::NoButton, Qt::NoButton,
                                                   he->modifiers())});
    } break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        QMutexLocker locker(&m_mutex);
        m_pendingKeyEvents.push_back(QKeyEvent(*static_cast<QKeyEvent *>(e)));
    }
    default:
        break;
    }
    return false;
}

} // Render

} // Qt3DRender

QT_END_NAMESPACE
