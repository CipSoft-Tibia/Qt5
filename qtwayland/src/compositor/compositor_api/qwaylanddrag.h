/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
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

#ifndef QWAYLANDDRAG_H
#define QWAYLANDDRAG_H

#include <QtWaylandCompositor/qtwaylandcompositorglobal.h>

#include <QtCore/QObject>
#include <QtCore/QPointF>

QT_REQUIRE_CONFIG(draganddrop);

QT_BEGIN_NAMESPACE

class QWaylandDragPrivate;
class QWaylandSurface;
class QWaylandSeat;

class Q_WAYLAND_COMPOSITOR_EXPORT QWaylandDrag : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandDrag)

    Q_PROPERTY(QWaylandSurface *icon READ icon NOTIFY iconChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY iconChanged)

public:
    explicit QWaylandDrag(QWaylandSeat *seat);

    QWaylandSurface *icon() const;
    QWaylandSurface *origin() const;
    QWaylandSeat *seat() const;
    bool visible() const;

public Q_SLOTS:
    void dragMove(QWaylandSurface *target, const QPointF &pos);
    void drop();
    void cancelDrag();

Q_SIGNALS:
    void iconChanged();
    void dragStarted(); // QWaylandSurface *icon????
};

QT_END_NAMESPACE

#endif // QWAYLANDDRAG_H
