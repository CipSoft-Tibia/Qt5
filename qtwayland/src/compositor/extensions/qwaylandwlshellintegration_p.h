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

#ifndef QWAYLANDWLSHELLINTEGRATION_H
#define QWAYLANDWLSHELLINTEGRATION_H

#include <QtWaylandCompositor/private/qwaylandquickshellsurfaceitem_p.h>

#include <QtWaylandCompositor/QWaylandWlShellSurface>

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

namespace QtWayland {

class WlShellIntegration : public QWaylandQuickShellIntegration
{
    Q_OBJECT
public:
    WlShellIntegration(QWaylandQuickShellSurfaceItem *item);
    ~WlShellIntegration() override;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private Q_SLOTS:
    void handleStartMove(QWaylandSeat *seat);
    void handleStartResize(QWaylandSeat *seat, QWaylandWlShellSurface::ResizeEdge edges);
    void handleSetDefaultTopLevel();
    void handleSetTransient(QWaylandSurface *parentSurface, const QPoint &relativeToParent, bool inactive);
    void handleSetMaximized(QWaylandOutput *output);
    void handleSetFullScreen(QWaylandWlShellSurface::FullScreenMethod method, uint framerate, QWaylandOutput *output);
    void handleSetPopup(QWaylandSeat *seat, QWaylandSurface *parent, const QPoint &relativeToParent);
    void handleShellSurfaceDestroyed();
    void handleSurfaceHasContentChanged();
    void handleRedraw();
    void adjustOffsetForNextFrame(const QPointF &offset);
    void handleFullScreenSizeChanged();
    void handleMaximizedSizeChanged();

private:
    enum class GrabberState {
        Default,
        Resize,
        Move
    };

    void handlePopupClosed();
    void handlePopupRemoved();
    qreal devicePixelRatio() const;

    QWaylandQuickShellSurfaceItem *m_item = nullptr;
    QPointer<QWaylandWlShellSurface> m_shellSurface;
    GrabberState grabberState = GrabberState::Default;
    struct {
        QWaylandSeat *seat = nullptr;
        QPointF initialOffset;
        bool initialized = false;
    } moveState;
    struct {
        QWaylandSeat *seat = nullptr;
        QWaylandWlShellSurface::ResizeEdge resizeEdges;
        QSizeF initialSize;
        QPointF initialMousePos;
        bool initialized = false;
    } resizeState;

    bool isPopup = false;

    enum class State {
        Windowed,
        Maximized,
        FullScreen
    };

    State currentState = State::Windowed;
    State nextState = State::Windowed;

    struct {
        QWaylandOutput *output = nullptr;
        QMetaObject::Connection sizeChangedConnection; // Depending on whether maximized or fullscreen,
                                                       // will be hooked to geometry-changed or available-
                                                       // geometry-changed.
    } nonwindowedState;

    QPointF normalPosition;
    QPointF finalPosition;

    bool filterMouseMoveEvent(QMouseEvent *event);
    bool filterMouseReleaseEvent(QMouseEvent *event);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDWLSHELLINTEGRATION_H
