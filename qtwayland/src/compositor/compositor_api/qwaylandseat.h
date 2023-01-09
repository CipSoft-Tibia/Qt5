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

#ifndef QWAYLANDSEAT_H
#define QWAYLANDSEAT_H

#include <QtCore/qnamespace.h>
#include <QtCore/QPoint>
#include <QtCore/QString>

#include <QtWaylandCompositor/qtwaylandcompositorglobal.h>
#include <QtWaylandCompositor/qwaylandcompositorextension.h>
#include <QtWaylandCompositor/qwaylandkeyboard.h>

QT_BEGIN_NAMESPACE

class QWaylandCompositor;
class QWaylandSurface;
class QKeyEvent;
class QTouchEvent;
class QWaylandView;
class QInputEvent;
class QWaylandSeatPrivate;
class QWaylandDrag;
class QWaylandKeyboard;
class QWaylandPointer;
class QWaylandTouch;

class Q_WAYLAND_COMPOSITOR_EXPORT QWaylandSeat : public QWaylandObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandSeat)

#if QT_CONFIG(draganddrop)
    Q_PROPERTY(QWaylandDrag *drag READ drag CONSTANT)
#endif
    Q_PROPERTY(QWaylandKeymap *keymap READ keymap CONSTANT)
public:
    enum CapabilityFlag {
        // The order should match the enum WL_SEAT_CAPABILITY_*
        Pointer = 0x01,
        Keyboard = 0x02,
        Touch = 0x04,

        DefaultCapabilities = Pointer | Keyboard | Touch
    };
    Q_DECLARE_FLAGS(CapabilityFlags, CapabilityFlag)
    Q_ENUM(CapabilityFlags)

    QWaylandSeat(QWaylandCompositor *compositor, CapabilityFlags capabilityFlags = DefaultCapabilities);
    ~QWaylandSeat() override;
    virtual void initialize();
    bool isInitialized() const;

    void sendMousePressEvent(Qt::MouseButton button);
    void sendMouseReleaseEvent(Qt::MouseButton button);
    void sendMouseMoveEvent(QWaylandView *surface , const QPointF &localPos, const QPointF &outputSpacePos = QPointF());
    void sendMouseWheelEvent(Qt::Orientation orientation, int delta);

    void sendKeyPressEvent(uint code);
    void sendKeyReleaseEvent(uint code);

    void sendFullKeyEvent(QKeyEvent *event);
    Q_INVOKABLE void sendKeyEvent(int qtKey, bool pressed);

    uint sendTouchPointEvent(QWaylandSurface *surface, int id, const QPointF &point, Qt::TouchPointState state);
    Q_INVOKABLE uint sendTouchPointPressed(QWaylandSurface *surface, int id, const QPointF &position);
    Q_INVOKABLE uint sendTouchPointReleased(QWaylandSurface *surface, int id, const QPointF &position);
    Q_INVOKABLE uint sendTouchPointMoved(QWaylandSurface *surface, int id, const QPointF &position);
    Q_INVOKABLE void sendTouchFrameEvent(QWaylandClient *client);
    Q_INVOKABLE void sendTouchCancelEvent(QWaylandClient *client);

    void sendFullTouchEvent(QWaylandSurface *surface, QTouchEvent *event);

    QWaylandPointer *pointer() const;
    //Normally set by the mouse device,
    //But can be set manually for use with touch or can reset unset the current mouse focus;
    QWaylandView *mouseFocus() const;
    void setMouseFocus(QWaylandView *view);

    QWaylandKeyboard *keyboard() const;
    QWaylandSurface *keyboardFocus() const;
    bool setKeyboardFocus(QWaylandSurface *surface);
    QWaylandKeymap *keymap();

    QWaylandTouch *touch() const;

    QWaylandCompositor *compositor() const;

#if QT_CONFIG(draganddrop)
    QWaylandDrag *drag() const;
#endif

    QWaylandSeat::CapabilityFlags capabilities() const;

    virtual bool isOwner(QInputEvent *inputEvent) const;

    static QWaylandSeat *fromSeatResource(struct ::wl_resource *resource);

Q_SIGNALS:
    void mouseFocusChanged(QWaylandView *newFocus, QWaylandView *oldFocus);
    void keyboardFocusChanged(QWaylandSurface *newFocus, QWaylandSurface *oldFocus);
    void cursorSurfaceRequest(QWaylandSurface *surface, int hotspotX, int hotspotY);

private:
    void handleMouseFocusDestroyed();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWaylandSeat::CapabilityFlags)

QT_END_NAMESPACE

#endif // QWAYLANDSEAT_H
