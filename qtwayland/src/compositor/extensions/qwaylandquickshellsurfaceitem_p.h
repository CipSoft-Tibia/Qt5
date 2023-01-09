/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QWAYLANDQUICKSHELLSURFACEITEM_P_H
#define QWAYLANDQUICKSHELLSURFACEITEM_P_H

#include <QtWaylandCompositor/QWaylandQuickShellSurfaceItem>
#include <QtWaylandCompositor/QWaylandQuickShellIntegration>
#include <QtWaylandCompositor/private/qwaylandquickitem_p.h>
#include <QtCore/QBasicTimer>

#include <functional>

QT_BEGIN_NAMESPACE

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

class QWaylandShellSurface;
class QWaylandQuickShellSurfaceItem;

class Q_WAYLAND_COMPOSITOR_EXPORT QWaylandQuickShellSurfaceItemPrivate : public QWaylandQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QWaylandQuickShellSurfaceItem)
public:
    QWaylandQuickShellSurfaceItemPrivate() {}
    QWaylandQuickShellSurfaceItem *maybeCreateAutoPopup(QWaylandShellSurface* shellSurface);
    static QWaylandQuickShellSurfaceItemPrivate *get(QWaylandQuickShellSurfaceItem *item) { return item->d_func(); }

    QWaylandQuickShellIntegration *m_shellIntegration = nullptr;
    QWaylandShellSurface *m_shellSurface = nullptr;
    QQuickItem *m_moveItem = nullptr;
    bool m_autoCreatePopupItems =
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            true;
#else
            false;
#endif

};

class Q_WAYLAND_COMPOSITOR_EXPORT QWaylandQuickShellEventFilter : public QObject
{
    Q_OBJECT
public:
    typedef std::function<void()> CallbackFunction;
    static void startFilter(QWaylandClient *client, CallbackFunction closePopupCallback);
    static void cancelFilter();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void stopFilter();

    QWaylandQuickShellEventFilter(QObject *parent = nullptr);
    bool eventFilter(QObject *, QEvent *) override;
    bool eventFilterInstalled = false;
    bool waitForRelease = false;
    QPointer<QWaylandClient> client;
    CallbackFunction closePopups = nullptr;
    QBasicTimer mousePressTimeout;
    static QWaylandQuickShellEventFilter *self;
};

QT_END_NAMESPACE

#endif // QWAYLANDQUICKSHELLSURFACEITEM_P_H
