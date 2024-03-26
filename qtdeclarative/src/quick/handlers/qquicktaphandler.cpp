// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktaphandler_p.h"
#include "qquicksinglepointhandler_p_p.h"
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/qquickwindow.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>
#include <QtGui/qstylehints.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTapHandler, "qt.quick.handler.tap")

quint64 QQuickTapHandler::m_multiTapInterval(0);
// single tap distance is the same as the drag threshold
int QQuickTapHandler::m_mouseMultiClickDistanceSquared(-1);
int QQuickTapHandler::m_touchMultiTapDistanceSquared(-1);

/*!
    \qmltype TapHandler
    \instantiates QQuickTapHandler
    \inherits SinglePointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Handler for taps and clicks.

    TapHandler is a handler for taps on a touchscreen or clicks on a mouse.

    Detection of a valid tap gesture depends on \l gesturePolicy.  The default
    value is DragThreshold, which requires the press and release to be close
    together in both space and time.  In this case, DragHandler is able to
    function using only a passive grab, and therefore does not interfere with
    event delivery to any other Items or Input Handlers.  So the default
    gesturePolicy is useful when you want to modify behavior of an existing
    control or Item by adding a TapHandler with bindings and/or JavaScript
    callbacks.

    Note that buttons (such as QPushButton) are often implemented not to care
    whether the press and release occur close together: if you press the button
    and then change your mind, you need to drag all the way off the edge of the
    button in order to cancel the click.  For this use case, set the
    \l gesturePolicy to \c TapHandler.ReleaseWithinBounds.

    \snippet pointerHandlers/tapHandlerButton.qml 0

    For multi-tap gestures (double-tap, triple-tap etc.), the distance moved
    must not exceed QStyleHints::mouseDoubleClickDistance() with mouse and
    QStyleHints::touchDoubleTapDistance() with touch, and the time between
    taps must not exceed QStyleHints::mouseDoubleClickInterval().

    \sa MouseArea, {Qt Quick Examples - Pointer Handlers}
*/

QQuickTapHandler::QQuickTapHandler(QQuickItem *parent)
    : QQuickSinglePointHandler(parent)
    , m_longPressThreshold(QGuiApplication::styleHints()->mousePressAndHoldInterval())
{
    if (m_mouseMultiClickDistanceSquared < 0) {
        m_multiTapInterval = qApp->styleHints()->mouseDoubleClickInterval();
        m_mouseMultiClickDistanceSquared = qApp->styleHints()->mouseDoubleClickDistance();
        m_mouseMultiClickDistanceSquared *= m_mouseMultiClickDistanceSquared;
        m_touchMultiTapDistanceSquared = qApp->styleHints()->touchDoubleTapDistance();
        m_touchMultiTapDistanceSquared *= m_touchMultiTapDistanceSquared;
    }
}

bool QQuickTapHandler::wantsEventPoint(const QPointerEvent *event, const QEventPoint &point)
{
    if (!QQuickDeliveryAgentPrivate::isMouseEvent(event) &&
            !QQuickDeliveryAgentPrivate::isTouchEvent(event) &&
            !QQuickDeliveryAgentPrivate::isTabletEvent(event))
        return false;
    // If the user has not violated any constraint, it could be a tap.
    // Otherwise we want to give up the grab so that a competing handler
    // (e.g. DragHandler) gets a chance to take over.
    // Don't forget to emit released in case of a cancel.
    bool ret = false;
    bool overThreshold = d_func()->dragOverThreshold(point);
    if (overThreshold && m_gesturePolicy != DragWithinBounds) {
        if (m_longPressTimer.isActive())
            qCDebug(lcTapHandler) << objectName() << "drag threshold exceeded";
        m_longPressTimer.stop();
        m_holdTimer.invalidate();
    }
    switch (point.state()) {
    case QEventPoint::Pressed:
    case QEventPoint::Released:
        ret = parentContains(point);
        break;
    case QEventPoint::Updated:
        ret = point.id() == this->point().id();
        switch (m_gesturePolicy) {
        case DragThreshold:
            ret = ret && !overThreshold && parentContains(point);
            break;
        case WithinBounds:
        case DragWithinBounds:
            ret = ret && parentContains(point);
            break;
        case ReleaseWithinBounds:
            // no change to ret: depends only whether it's the already-tracking point ID
            break;
        }
        break;
    case QEventPoint::Stationary:
        // If the point hasn't moved since last time, the return value should be the same as last time.
        // If we return false here, QQuickPointerHandler::handlePointerEvent() will call setActive(false).
        ret = point.id() == this->point().id();
        break;
    case QEventPoint::Unknown:
        break;
    }
    // If this is the grabber, returning false from this function will cancel the grab,
    // so onGrabChanged(this, CancelGrabExclusive, point) and setPressed(false) will be called.
    // But when m_gesturePolicy is DragThreshold, we don't get an exclusive grab, but
    // we still don't want to be pressed anymore.
    if (!ret && point.id() == this->point().id())
        setPressed(false, true, const_cast<QPointerEvent *>(event), const_cast<QEventPoint &>(point));
    return ret;
}

void QQuickTapHandler::handleEventPoint(QPointerEvent *event, QEventPoint &point)
{
    const bool isTouch = QQuickDeliveryAgentPrivate::isTouchEvent(event);
    switch (point.state()) {
    case QEventPoint::Pressed:
        setPressed(true, false, event, point);
        break;
    case QEventPoint::Released: {
        if (isTouch || (static_cast<const QSinglePointEvent *>(event)->buttons() & acceptedButtons()) == Qt::NoButton)
            setPressed(false, false, event, point);
        break;
    }
    default:
        break;
    }

    QQuickSinglePointHandler::handleEventPoint(event, point);

    // If TapHandler only needs a passive grab, it should not block other items and handlers from reacting.
    // If the point is accepted, QQuickItemPrivate::localizedTouchEvent() would skip it.
    if (isTouch && m_gesturePolicy == DragThreshold)
        point.setAccepted(false);
}

/*!
    \qmlproperty real QtQuick::TapHandler::longPressThreshold

    The time in seconds that an \l eventPoint must be pressed in order to
    trigger a long press gesture and emit the \l longPressed() signal, if the
    value is greater than \c 0. If the point is released before this time
    limit, a tap can be detected if the \l gesturePolicy constraint is
    satisfied. If \c longPressThreshold is \c 0, the timer is disabled and the
    signal will not be emitted. If \c longPressThreshold is set to \c undefined,
    the default value is used instead, and can be read back from this property.

    The default value is QStyleHints::mousePressAndHoldInterval() converted to
    seconds.
*/
qreal QQuickTapHandler::longPressThreshold() const
{
    return m_longPressThreshold / qreal(1000);
}

void QQuickTapHandler::setLongPressThreshold(qreal longPressThreshold)
{
    if (longPressThreshold < 0) {
        resetLongPressThreshold();
        return;
    }
    int ms = qRound(longPressThreshold * 1000);
    if (m_longPressThreshold == ms)
        return;

    m_longPressThreshold = ms;
    emit longPressThresholdChanged();
}

void QQuickTapHandler::resetLongPressThreshold()
{
    int ms = QGuiApplication::styleHints()->mousePressAndHoldInterval();
    if (m_longPressThreshold == ms)
        return;

    m_longPressThreshold = ms;
    emit longPressThresholdChanged();
}

void QQuickTapHandler::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_longPressTimer.timerId()) {
        m_longPressTimer.stop();
        qCDebug(lcTapHandler) << objectName() << "longPressed";
        m_longPressed = true;
        emit longPressed();
    } else if (event->timerId() == m_doubleTapTimer.timerId()) {
        m_doubleTapTimer.stop();
        qCDebug(lcTapHandler) << objectName() << "double-tap timer expired; taps:" << m_tapCount;
        Q_ASSERT(m_exclusiveSignals == (SingleTap | DoubleTap));
        if (m_tapCount == 1)
            emit singleTapped(m_singleTapReleasedPoint, m_singleTapReleasedButton);
        else if (m_tapCount == 2)
            emit doubleTapped(m_singleTapReleasedPoint, m_singleTapReleasedButton);
    }
}

/*!
    \qmlproperty enumeration QtQuick::TapHandler::gesturePolicy

    The spatial constraint for a tap or long press gesture to be recognized,
    in addition to the constraint that the release must occur before
    \l longPressThreshold has elapsed. If these constraints are not satisfied,
    the \l tapped signal is not emitted, and \l tapCount is not incremented.
    If the spatial constraint is violated, \l pressed transitions immediately
    from true to false, regardless of the time held.

    The \c gesturePolicy also affects grab behavior as described below.

    \table
    \header
        \li Constant
        \li Description
    \row
        \li \c TapHandler.DragThreshold
            \image pointerHandlers/tapHandlerOverlappingButtons.webp
            Grab on press: \e passive
        \li (the default value) The \l eventPoint must not move significantly.
            If the mouse, finger or stylus moves past the system-wide drag
            threshold (QStyleHints::startDragDistance), the tap gesture is
            canceled, even if the device or finger is still pressed. This policy
            can be useful whenever TapHandler needs to cooperate with other
            input handlers (for example \l DragHandler) or event-handling Items
            (for example \l {Qt Quick Controls}), because in this case TapHandler
            will not take the exclusive grab, but merely a
            \l {QPointerEvent::addPassiveGrabber()}{passive grab}.
            That is, \c DragThreshold is especially useful to \e augment
            existing behavior: it reacts to tap/click/long-press even when
            another item or handler is already reacting, perhaps even in a
            different layer of the UI. The following snippet shows one
            TapHandler as used in one component; but if we stack up two
            instances of the component, you will see the handlers in both of them
            react simultaneously when a press occurs over both of them, because
            the passive grab does not stop event propagation:
            \quotefromfile pointerHandlers/tapHandlerOverlappingButtons.qml
            \skipto Item
            \printuntil component Button
            \skipto TapHandler
            \printuntil }
            \skipuntil Text {
            \skipuntil }
            \printuntil Button
            \printuntil Button
            \printuntil }

    \row
        \li \c TapHandler.WithinBounds
            \image pointerHandlers/tapHandlerButtonWithinBounds.webp
            Grab on press: \e exclusive
        \li If the \l eventPoint leaves the bounds of the \c parent Item, the tap
            gesture is canceled. The TapHandler will take the
            \l {QPointerEvent::setExclusiveGrabber}{exclusive grab} on
            press, but will release the grab as soon as the boundary constraint
            is no longer satisfied.
            \snippet pointerHandlers/tapHandlerButtonWithinBounds.qml 1

    \row
        \li \c TapHandler.ReleaseWithinBounds
            \image pointerHandlers/tapHandlerButtonReleaseWithinBounds.webp
            Grab on press: \e exclusive
        \li At the time of release (the mouse button is released or the finger
            is lifted), if the \l eventPoint is outside the bounds of the
            \c parent Item, a tap gesture is not recognized. This corresponds to
            typical behavior for button widgets: you can cancel a click by
            dragging outside the button, and you can also change your mind by
            dragging back inside the button before release. Note that it's
            necessary for TapHandler to take the
            \l {QPointerEvent::setExclusiveGrabber}{exclusive grab} on press
            and retain it until release in order to detect this gesture.
            \snippet pointerHandlers/tapHandlerButtonReleaseWithinBounds.qml 1

    \row
        \li \c TapHandler.DragWithinBounds
            \image pointerHandlers/dragReleaseMenu.webp
            Grab on press: \e exclusive
        \li On press, TapHandler takes the
            \l {QPointerEvent::setExclusiveGrabber}{exclusive grab}; after that,
            the \l eventPoint can be dragged within the bounds of the \c parent
            item, while the \l timeHeld property keeps counting, and the
            \l longPressed() signal will be emitted regardless of drag distance.
            However, like \c WithinBounds, if the point leaves the bounds,
            the tap gesture is \l {PointerHandler::}{canceled()}, \l active()
            becomes \c false, and \l timeHeld stops counting. This is suitable
            for implementing press-drag-release components, such as menus, in
            which a single TapHandler detects press, \c timeHeld drives an
            "opening" animation, and then the user can drag to a menu item and
            release, while never leaving the bounds of the parent scene containing
            the menu. This value was added in Qt 6.3.
            \snippet pointerHandlers/dragReleaseMenu.qml 1
    \endtable

    The \l {Qt Quick Examples - Pointer Handlers} demonstrates some use cases for these.

    \note If you find that TapHandler is reacting in cases that conflict with
    some other behavior, the first thing you should try is to think about which
    \c gesturePolicy is appropriate. If you cannot fix it by changing \c gesturePolicy,
    some cases are better served by adjusting \l {PointerHandler::}{grabPermissions},
    either in this handler, or in another handler that should \e prevent TapHandler
    from reacting.
*/
void QQuickTapHandler::setGesturePolicy(QQuickTapHandler::GesturePolicy gesturePolicy)
{
    if (m_gesturePolicy == gesturePolicy)
        return;

    m_gesturePolicy = gesturePolicy;
    emit gesturePolicyChanged();
}

/*!
    \qmlproperty enumeration QtQuick::TapHandler::exclusiveSignals
    \since 6.5

    Determines the exclusivity of the singleTapped() and doubleTapped() signals.

    \value NotExclusive (the default) singleTapped() and doubleTapped() are
    emitted immediately when the user taps once or twice, respectively.

    \value SingleTap singleTapped() is emitted immediately when the user taps
    once, and doubleTapped() is never emitted.

    \value DoubleTap doubleTapped() is emitted immediately when the user taps
    twice, and singleTapped() is never emitted.

    \value (SingleTap | DoubleTap) Both signals are delayed until
    QStyleHints::mouseDoubleClickInterval(), such that either singleTapped()
    or doubleTapped() can be emitted, but not both. But if 3 or more taps
    occur within \c mouseDoubleClickInterval, neither signal is emitted.

    \note The remaining signals such as tapped() and tapCountChanged() are
    always emitted immediately, regardless of this property.
*/
void QQuickTapHandler::setExclusiveSignals(QQuickTapHandler::ExclusiveSignals exc)
{
    if (m_exclusiveSignals == exc)
        return;

    m_exclusiveSignals = exc;
    emit exclusiveSignalsChanged();
}

/*!
    \qmlproperty bool QtQuick::TapHandler::pressed
    \readonly

    Holds true whenever the mouse or touch point is pressed,
    and any movement since the press is compliant with the current
    \l gesturePolicy. When the \l eventPoint is released or the policy is
    violated, \e pressed will change to false.
*/
void QQuickTapHandler::setPressed(bool press, bool cancel, QPointerEvent *event, QEventPoint &point)
{
    if (m_pressed != press) {
        qCDebug(lcTapHandler) << objectName() << "pressed" << m_pressed << "->" << press
                              << (cancel ? "CANCEL" : "") << point << "gp" << m_gesturePolicy;
        m_pressed = press;
        connectPreRenderSignal(press);
        updateTimeHeld();
        if (press) {
            if (m_longPressThreshold > 0)
                m_longPressTimer.start(m_longPressThreshold, this);
            m_holdTimer.start();
        } else {
            m_longPressTimer.stop();
            m_holdTimer.invalidate();
        }
        if (press) {
            // on press, grab before emitting changed signals
            if (m_gesturePolicy == DragThreshold)
                setPassiveGrab(event, point, press);
            else
                setExclusiveGrab(event, point, press);
        }
        if (!cancel && !press && parentContains(point)) {
            if (m_longPressed) {
                qCDebug(lcTapHandler) << objectName() << "long press threshold" << longPressThreshold() << "exceeded:" << point.timeHeld();
            } else {
                // Assuming here that pointerEvent()->timestamp() is in ms.
                const quint64 ts = event->timestamp();
                const quint64 interval = ts - m_lastTapTimestamp;
                const auto distanceSquared = QVector2D(point.scenePosition() - m_lastTapPos).lengthSquared();
                const auto singleTapReleasedButton = event->isSinglePointEvent() ? static_cast<QSinglePointEvent *>(event)->button() : Qt::NoButton;
                if ((interval < m_multiTapInterval && distanceSquared <
                        (event->device()->type() == QInputDevice::DeviceType::Mouse ?
                         m_mouseMultiClickDistanceSquared : m_touchMultiTapDistanceSquared))
                      && m_singleTapReleasedButton == singleTapReleasedButton) {
                    ++m_tapCount;
                } else {
                    m_singleTapReleasedButton = singleTapReleasedButton;
                    m_singleTapReleasedPoint = point;
                    m_tapCount = 1;
                }
                qCDebug(lcTapHandler) << objectName() << "tapped" << m_tapCount << "times; interval since last:" << interval
                                      << "sec; distance since last:" << qSqrt(distanceSquared);
                auto button = event->isSinglePointEvent() ? static_cast<QSinglePointEvent *>(event)->button() : Qt::NoButton;
                emit tapped(point, button);
                emit tapCountChanged();
                switch (m_exclusiveSignals) {
                case NotExclusive:
                    if (m_tapCount == 1)
                        emit singleTapped(point, button);
                    else if (m_tapCount == 2)
                        emit doubleTapped(point, button);
                    break;
                case SingleTap:
                    if (m_tapCount == 1)
                        emit singleTapped(point, button);
                    break;
                case DoubleTap:
                    if (m_tapCount == 2)
                        emit doubleTapped(point, button);
                    break;
                case (SingleTap | DoubleTap):
                    if (m_tapCount == 1) {
                        qCDebug(lcTapHandler) << objectName() << "waiting to emit singleTapped:" << m_multiTapInterval << "ms";
                        m_doubleTapTimer.start(m_multiTapInterval, this);
                    }
                }
                qCDebug(lcTapHandler) << objectName() << "tap" << m_tapCount << "after" << event->timestamp() - m_lastTapTimestamp << "ms";

                m_lastTapTimestamp = ts;
                m_lastTapPos = point.scenePosition();
            }
        }
        m_longPressed = false;
        emit pressedChanged();
        if (!press && m_gesturePolicy != DragThreshold) {
            // on release, ungrab after emitting changed signals
            setExclusiveGrab(event, point, press);
        }
        if (cancel) {
            emit canceled(point);
            setExclusiveGrab(event, point, false);
            // In case there is a filtering parent (Flickable), we should not give up the passive grab,
            // so that it can continue to filter future events.
            d_func()->reset();
            emit pointChanged();
        }
    }
}

void QQuickTapHandler::onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition,
                                     QPointerEvent *ev, QEventPoint &point)
{
    QQuickSinglePointHandler::onGrabChanged(grabber, transition, ev, point);
    bool isCanceled = transition == QPointingDevice::CancelGrabExclusive || transition == QPointingDevice::CancelGrabPassive;
    if (grabber == this && (isCanceled || point.state() == QEventPoint::Released))
        setPressed(false, isCanceled, ev, point);
}

void QQuickTapHandler::connectPreRenderSignal(bool conn)
{
    auto par = parentItem();
    if (!par)
        return;
    if (conn)
        connect(par->window(), &QQuickWindow::beforeSynchronizing, this, &QQuickTapHandler::updateTimeHeld);
    else
        disconnect(par->window(), &QQuickWindow::beforeSynchronizing, this, &QQuickTapHandler::updateTimeHeld);
}

void QQuickTapHandler::updateTimeHeld()
{
    emit timeHeldChanged();
}

/*!
    \qmlproperty int QtQuick::TapHandler::tapCount
    \readonly

    The number of taps which have occurred within the time and space
    constraints to be considered a single gesture. The counter is reset to 1
    if the button changed. For example, to detect a triple-tap, you can write:

    \qml
    Rectangle {
        width: 100; height: 30
        signal tripleTap
        TapHandler {
            acceptedButtons: Qt.AllButtons
            onTapped: if (tapCount == 3) tripleTap()
        }
    }
    \endqml
*/

/*!
    \qmlproperty real QtQuick::TapHandler::timeHeld
    \readonly

    The amount of time in seconds that a pressed point has been held, without
    moving beyond the drag threshold. It will be updated at least once per
    frame rendered, which enables rendering an animation showing the progress
    towards an action which will be triggered by a long-press. It is also
    possible to trigger one of a series of actions depending on how long the
    press is held.

    A value of less than zero means no point is being held within this
    handler's \l [QML] Item.

    \note If \l gesturePolicy is set to \c TapHandler.DragWithinBounds,
    \c timeHeld does not stop counting even when the pressed point is moved
    beyond the drag threshold, but only when the point leaves the \l {Item::}
    {parent} item's \l {QtQuick::Item::contains()}{bounds}.
*/

/*!
    \qmlsignal QtQuick::TapHandler::tapped(eventPoint eventPoint, Qt::MouseButton button)

    This signal is emitted each time the \c parent Item is tapped.

    That is, if you press and release a touchpoint or button within a time
    period less than \l longPressThreshold, while any movement does not exceed
    the drag threshold, then the \c tapped signal will be emitted at the time
    of release.  The \a eventPoint signal parameter contains information
    from the release event about the point that was tapped, and \a button
    is the \l {Qt::MouseButton}{mouse button} that was clicked, or \c NoButton
    on a touchscreen.

    \snippet pointerHandlers/tapHandlerOnTapped.qml 0
*/

/*!
    \qmlsignal QtQuick::TapHandler::singleTapped(eventPoint eventPoint, Qt::MouseButton button)
    \since 5.11

    This signal is emitted when the \c parent Item is tapped once.
    After an amount of time greater than QStyleHints::mouseDoubleClickInterval,
    it can be tapped again; but if the time until the next tap is less,
    \l tapCount will increase. The \a eventPoint signal parameter contains
    information from the release event about the point that was tapped, and
    \a button is the \l {Qt::MouseButton}{mouse button} that was clicked, or
    \c NoButton on a touchscreen.
*/

/*!
    \qmlsignal QtQuick::TapHandler::doubleTapped(eventPoint eventPoint, Qt::MouseButton button)
    \since 5.11

    This signal is emitted when the \c parent Item is tapped twice within a
    short span of time (QStyleHints::mouseDoubleClickInterval()) and distance
    (QStyleHints::mouseDoubleClickDistance() or
    QStyleHints::touchDoubleTapDistance()). This signal always occurs after
    \l singleTapped, \l tapped, and \l tapCountChanged. The \a eventPoint
    signal parameter contains information from the release event about the
    point that was tapped, and \a button is the
    \l {Qt::MouseButton}{mouse button} that was clicked, or \c NoButton
    on a touchscreen.
*/

/*!
    \qmlsignal QtQuick::TapHandler::longPressed()

    This signal is emitted when the \c parent Item is pressed and held for a
    time period greater than \l longPressThreshold. That is, if you press and
    hold a touchpoint or button, while any movement does not exceed the drag
    threshold, then the \c longPressed signal will be emitted at the time that
    \l timeHeld exceeds \l longPressThreshold.
*/

/*!
    \qmlsignal QtQuick::TapHandler::tapCountChanged()

    This signal is emitted when the \c parent Item is tapped once or more (within
    a specified time and distance span) and when the present \c tapCount differs
    from the previous \c tapCount.
*/
QT_END_NAMESPACE

#include "moc_qquicktaphandler_p.cpp"
