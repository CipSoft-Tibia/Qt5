/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickdial_p.h"
#include "qquickdeferredexecute_p_p.h"

#include <QtCore/qmath.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Dial
    \inherits Control
    \instantiates QQuickDial
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-input
    \brief Circular dial that is rotated to set a value.

    The Dial is similar to a traditional dial knob that is found on devices
    such as stereos or industrial equipment. It allows the user to specify a
    value within a range.

    \image qtquickcontrols2-dial-no-wrap.gif

    The value of the dial is set with the \l value property. The range is
    set with the \l from and \l to properties. To enable or disable wrapping,
    use the \l wrap property.

    The dial can be manipulated with a keyboard. It supports the following
    actions:

    \table
    \header \li \b {Action} \li \b {Key}
    \row \li Decrease \l value by \l stepSize \li \c Qt.Key_Left
    \row \li Decrease \l value by \l stepSize \li \c Qt.Key_Down
    \row \li Set \l value to \l from \li \c Qt.Key_Home
    \row \li Increase \l value by \l stepSize \li \c Qt.Key_Right
    \row \li Increase \l value by \l stepSize \li \c Qt.Key_Up
    \row \li Set \l value to \l to \li \c Qt.Key_End
    \endtable

    \include qquickdial.qdocinc inputMode

    \sa {Customizing Dial}, {Input Controls}
*/

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlsignal QtQuick.Controls::Dial::moved()

    This signal is emitted when the dial has been interactively moved
    by the user by either touch, mouse, or keys.
*/

static const qreal startAngleRadians = (M_PI * 2.0) * (4.0 / 6.0);
static const qreal startAngle = -140;
static const qreal endAngleRadians = (M_PI * 2.0) * (5.0 / 6.0);
static const qreal endAngle = 140;

class QQuickDialPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickDial)

public:
    qreal valueAt(qreal position) const;
    qreal snapPosition(qreal position) const;
    qreal positionAt(const QPointF &point) const;
    qreal circularPositionAt(const QPointF &point) const;
    qreal linearPositionAt(const QPointF &point) const;
    void setPosition(qreal position);
    void updatePosition();
    bool isLargeChange(const QPointF &eventPos, qreal proposedPosition) const;
    bool isHorizontalOrVertical() const;

    void handlePress(const QPointF &point) override;
    void handleMove(const QPointF &point) override;
    void handleRelease(const QPointF &point) override;
    void handleUngrab() override;

    void cancelHandle();
    void executeHandle(bool complete = false);

    qreal from = 0;
    qreal to = 1;
    qreal value = 0;
    qreal position = 0;
    qreal angle = startAngle;
    qreal stepSize = 0;
    bool pressed = false;
    QPointF pressPoint;
    qreal positionBeforePress = 0;
    QQuickDial::SnapMode snapMode = QQuickDial::NoSnap;
    QQuickDial::InputMode inputMode = QQuickDial::Circular;
    bool wrap = false;
    bool live = true;
    QQuickDeferredPointer<QQuickItem> handle;
};

qreal QQuickDialPrivate::valueAt(qreal position) const
{
    return from + (to - from) * position;
}

qreal QQuickDialPrivate::snapPosition(qreal position) const
{
    const qreal range = to - from;
    if (qFuzzyIsNull(range))
        return position;

    const qreal effectiveStep = stepSize / range;
    if (qFuzzyIsNull(effectiveStep))
        return position;

    return qRound(position / effectiveStep) * effectiveStep;
}

qreal QQuickDialPrivate::positionAt(const QPointF &point) const
{
    return inputMode == QQuickDial::Circular ? circularPositionAt(point) : linearPositionAt(point);
}

qreal QQuickDialPrivate::circularPositionAt(const QPointF &point) const
{
    qreal yy = height / 2.0 - point.y();
    qreal xx = point.x() - width / 2.0;
    qreal angle = (xx || yy) ? std::atan2(yy, xx) : 0;

    if (angle < M_PI / -2)
        angle = angle + M_PI * 2;

    qreal normalizedAngle = (startAngleRadians - angle) / endAngleRadians;
    return normalizedAngle;
}

qreal QQuickDialPrivate::linearPositionAt(const QPointF &point) const
{
    // This value determines the range (either horizontal or vertical)
    // within which the dial can be dragged.
    // The larger this value is, the further the drag distance
    // must be to go from a position of e.g. 0.0 to 1.0.
    qreal dragArea = 0;

    // The linear input mode uses a "relative" input system,
    // where the distance from the press point is used to calculate
    // the change in position. Moving the mouse above the press
    // point increases the position (when inputMode is Vertical),
    // and vice versa. This prevents the dial from jumping when clicked.
    qreal dragDistance = 0;

    if (inputMode == QQuickDial::Horizontal) {
        dragArea = width * 2;
        dragDistance = pressPoint.x() - point.x();
    } else {
        dragArea = height * 2;
        dragDistance = point.y() - pressPoint.y();
    }
    const qreal normalisedDifference = dragDistance / dragArea;
    return qBound(qreal(0), positionBeforePress - normalisedDifference, qreal(1));
}

void QQuickDialPrivate::setPosition(qreal pos)
{
    Q_Q(QQuickDial);
    pos = qBound<qreal>(qreal(0), pos, qreal(1));
    if (qFuzzyCompare(position, pos))
        return;

    position = pos;

    angle = startAngle + position * qAbs(endAngle - startAngle);

    emit q->positionChanged();
    emit q->angleChanged();
}

void QQuickDialPrivate::updatePosition()
{
    qreal pos = 0;
    if (!qFuzzyCompare(from, to))
        pos = (value - from) / (to - from);
    setPosition(pos);
}

bool QQuickDialPrivate::isLargeChange(const QPointF &eventPos, qreal proposedPosition) const
{
    return qAbs(proposedPosition - position) >= qreal(0.5) && eventPos.y() >= height / 2;
}

bool QQuickDialPrivate::isHorizontalOrVertical() const
{
    return inputMode == QQuickDial::Horizontal || inputMode == QQuickDial::Vertical;
}

void QQuickDialPrivate::handlePress(const QPointF &point)
{
    Q_Q(QQuickDial);
    QQuickControlPrivate::handlePress(point);
    pressPoint = point;
    positionBeforePress = position;
    q->setPressed(true);
}

void QQuickDialPrivate::handleMove(const QPointF &point)
{
    Q_Q(QQuickDial);
    QQuickControlPrivate::handleMove(point);
    const qreal oldPos = position;
    qreal pos = positionAt(point);
    if (snapMode == QQuickDial::SnapAlways)
        pos = snapPosition(pos);

    if (wrap || (!wrap && (isHorizontalOrVertical() || !isLargeChange(point, pos)))) {
        if (live)
            q->setValue(valueAt(pos));
        else
            setPosition(pos);
        if (!qFuzzyCompare(pos, oldPos))
            emit q->moved();
    }
}

void QQuickDialPrivate::handleRelease(const QPointF &point)
{
    Q_Q(QQuickDial);
    QQuickControlPrivate::handleRelease(point);
    if (q->keepMouseGrab() || q->keepTouchGrab()) {
        const qreal oldPos = position;
        qreal pos = positionAt(point);
        if (snapMode != QQuickDial::NoSnap)
            pos = snapPosition(pos);

        if (wrap || (!wrap && (isHorizontalOrVertical() || !isLargeChange(point, pos))))
            q->setValue(valueAt(pos));
        if (!qFuzzyCompare(pos, oldPos))
            emit q->moved();

        q->setKeepMouseGrab(false);
        q->setKeepTouchGrab(false);
    }

    q->setPressed(false);
    pressPoint = QPointF();
    positionBeforePress = 0;
}

void QQuickDialPrivate::handleUngrab()
{
    Q_Q(QQuickDial);
    QQuickControlPrivate::handleUngrab();
    pressPoint = QPointF();
    positionBeforePress = 0;
    q->setPressed(false);
}

static inline QString handleName() { return QStringLiteral("handle"); }

void QQuickDialPrivate::cancelHandle()
{
    Q_Q(QQuickDial);
    quickCancelDeferred(q, handleName());
}

void QQuickDialPrivate::executeHandle(bool complete)
{
    Q_Q(QQuickDial);
    if (handle.wasExecuted())
        return;

    if (!handle || complete)
        quickBeginDeferred(q, handleName(), handle);
    if (complete)
        quickCompleteDeferred(q, handleName(), handle);
}

QQuickDial::QQuickDial(QQuickItem *parent)
    : QQuickControl(*(new QQuickDialPrivate), parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::from

    This property holds the starting value for the range. The default value is \c 0.0.

    \sa to, value
*/
qreal QQuickDial::from() const
{
    Q_D(const QQuickDial);
    return d->from;
}

void QQuickDial::setFrom(qreal from)
{
    Q_D(QQuickDial);
    if (qFuzzyCompare(d->from, from))
        return;

    d->from = from;
    emit fromChanged();
    if (isComponentComplete()) {
        setValue(d->value);
        d->updatePosition();
    }
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::to

    This property holds the end value for the range. The default value is
    \c 1.0.

    \sa from, value
*/
qreal QQuickDial::to() const
{
    Q_D(const QQuickDial);
    return d->to;
}

void QQuickDial::setTo(qreal to)
{
    Q_D(QQuickDial);
    if (qFuzzyCompare(d->to, to))
        return;

    d->to = to;
    emit toChanged();
    if (isComponentComplete()) {
        setValue(d->value);
        d->updatePosition();
    }
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::value

    This property holds the value in the range \c from - \c to. The default
    value is \c 0.0.

    \sa position, live
*/
qreal QQuickDial::value() const
{
    Q_D(const QQuickDial);
    return d->value;
}

void QQuickDial::setValue(qreal value)
{
    Q_D(QQuickDial);
    if (isComponentComplete())
        value = d->from > d->to ? qBound(d->to, value, d->from) : qBound(d->from, value, d->to);

    if (qFuzzyCompare(d->value, value))
        return;

    d->value = value;
    d->updatePosition();
    emit valueChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::position
    \readonly

    This property holds the logical position of the handle.

    The position is expressed as a fraction of the control's angle range (the
    range within which the handle can be moved) in the range \c {0.0 - 1.0}.

    \sa value, angle
*/
qreal QQuickDial::position() const
{
    Q_D(const QQuickDial);
    return d->position;
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::angle
    \readonly

    This property holds the angle of the handle.

    The range is from \c -140 degrees to \c 140 degrees.

    \sa position
*/
qreal QQuickDial::angle() const
{
    Q_D(const QQuickDial);
    return d->angle;
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::stepSize

    This property holds the step size.

    The step size determines the amount by which the dial's value
    is increased and decreased when interacted with via the keyboard.
    For example, a step size of \c 0.2, will result in the dial's
    value increasing and decreasing in increments of \c 0.2.

    The step size is only respected for touch and mouse interaction
    when \l snapMode is set to a value other than \c Dial.NoSnap.

    The default value is \c 0.0, which results in an effective step
    size of \c 0.1 for keyboard interaction.

    \sa snapMode, increase(), decrease()
*/
qreal QQuickDial::stepSize() const
{
    Q_D(const QQuickDial);
    return d->stepSize;
}

void QQuickDial::setStepSize(qreal step)
{
    Q_D(QQuickDial);
    if (qFuzzyCompare(d->stepSize, step))
        return;

    d->stepSize = step;
    emit stepSizeChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Dial::snapMode

    This property holds the snap mode.

    The snap mode works with the \l stepSize to allow the handle to snap to
    certain points along the dial.

    Possible values:
    \value Dial.NoSnap The dial does not snap (default).
    \value Dial.SnapAlways The dial snaps while the handle is dragged.
    \value Dial.SnapOnRelease The dial does not snap while being dragged, but only after the handle is released.

    \sa stepSize
*/
QQuickDial::SnapMode QQuickDial::snapMode() const
{
    Q_D(const QQuickDial);
    return d->snapMode;
}

void QQuickDial::setSnapMode(SnapMode mode)
{
    Q_D(QQuickDial);
    if (d->snapMode == mode)
        return;

    d->snapMode = mode;
    emit snapModeChanged();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty enumeration QtQuick.Controls::Dial::inputMode

    This property holds the input mode.

    \include qquickdial.qdocinc inputMode

    The default value is \c Dial.Circular.
*/
QQuickDial::InputMode QQuickDial::inputMode() const
{
    Q_D(const QQuickDial);
    return d->inputMode;
}

void QQuickDial::setInputMode(QQuickDial::InputMode mode)
{
    Q_D(QQuickDial);
    if (d->inputMode == mode)
        return;

    d->inputMode = mode;
    emit inputModeChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Dial::wrap

    This property holds whether the dial wraps when dragged.

    For example, when this property is set to \c true, dragging the dial past
    the \l to position will result in the handle being positioned at the
    \l from position, and vice versa:

    \image qtquickcontrols2-dial-wrap.gif

    When this property is \c false, it's not possible to drag the dial across
    the from and to values.

    \image qtquickcontrols2-dial-no-wrap.gif

    The default value is \c false.
*/
bool QQuickDial::wrap() const
{
    Q_D(const QQuickDial);
    return d->wrap;
}

void QQuickDial::setWrap(bool wrap)
{
    Q_D(QQuickDial);
    if (d->wrap == wrap)
        return;

    d->wrap = wrap;
    emit wrapChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Dial::pressed

    This property holds whether the dial is pressed.

    The dial will be pressed when either the mouse is pressed over it, or a key
    such as \c Qt.Key_Left is held down. If you'd prefer not to have the dial
    be pressed upon key presses (due to styling reasons, for example), you can
    use the \l {Keys}{Keys attached property}:

    \code
    Dial {
        Keys.onLeftPressed: {}
    }
    \endcode

    This will result in pressed only being \c true upon mouse presses.
*/
bool QQuickDial::isPressed() const
{
    Q_D(const QQuickDial);
    return d->pressed;
}

void QQuickDial::setPressed(bool pressed)
{
    Q_D(QQuickDial);
    if (d->pressed == pressed)
        return;

    d->pressed = pressed;
    setAccessibleProperty("pressed", pressed);
    emit pressedChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::Dial::handle

    This property holds the handle of the dial.

    The handle acts as a visual indicator of the position of the dial.

    \sa {Customizing Dial}
*/
QQuickItem *QQuickDial::handle() const
{
    QQuickDialPrivate *d = const_cast<QQuickDialPrivate *>(d_func());
    if (!d->handle)
        d->executeHandle();
    return d->handle;
}

void QQuickDial::setHandle(QQuickItem *handle)
{
    Q_D(QQuickDial);
    if (handle == d->handle)
        return;

    if (!d->handle.isExecuting())
        d->cancelHandle();

    delete d->handle;
    d->handle = handle;
    if (d->handle && !d->handle->parentItem())
        d->handle->setParentItem(this);
    if (!d->handle.isExecuting())
        emit handleChanged();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty bool QtQuick.Controls::Dial::live

    This property holds whether the dial provides live updates for the \l value
    property while the handle is dragged.

    The default value is \c true.

    \sa value
*/
bool QQuickDial::live() const
{
    Q_D(const QQuickDial);
    return d->live;
}

void QQuickDial::setLive(bool live)
{
    Q_D(QQuickDial);
    if (d->live == live)
        return;

    d->live = live;
    emit liveChanged();
}

/*!
    \qmlmethod void QtQuick.Controls::Dial::increase()

    Increases the value by \l stepSize, or \c 0.1 if stepSize is not defined.

    \sa stepSize
*/
void QQuickDial::increase()
{
    Q_D(QQuickDial);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setValue(d->value + step);
}

/*!
    \qmlmethod void QtQuick.Controls::Dial::decrease()

    Decreases the value by \l stepSize, or \c 0.1 if stepSize is not defined.

    \sa stepSize
*/
void QQuickDial::decrease()
{
    Q_D(QQuickDial);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setValue(d->value - step);
}

void QQuickDial::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickDial);
    const qreal oldValue = d->value;
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Down:
        setPressed(true);
        if (isMirrored())
            increase();
        else
            decrease();
        break;

    case Qt::Key_Right:
    case Qt::Key_Up:
        setPressed(true);
        if (isMirrored())
            decrease();
        else
            increase();
        break;

    case Qt::Key_Home:
        setPressed(true);
        setValue(isMirrored() ? d->to : d->from);
        break;

    case Qt::Key_End:
        setPressed(true);
        setValue(isMirrored() ? d->from : d->to);
        break;

    default:
        event->ignore();
        QQuickControl::keyPressEvent(event);
        break;
    }
    if (!qFuzzyCompare(d->value, oldValue))
        emit moved();
}

void QQuickDial::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    setPressed(false);
}

void QQuickDial::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickDial);
    QQuickControl::mousePressEvent(event);
    d->handleMove(event->localPos());
    setKeepMouseGrab(true);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickDial::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickDial);
    switch (event->type()) {
    case QEvent::TouchUpdate:
        for (const QTouchEvent::TouchPoint &point : event->touchPoints()) {
            if (!d->acceptTouch(point))
                continue;

            switch (point.state()) {
            case Qt::TouchPointMoved:
                if (!keepTouchGrab()) {
                    bool overXDragThreshold = QQuickWindowPrivate::dragOverThreshold(point.pos().x() - d->pressPoint.x(), Qt::XAxis, &point);
                    setKeepTouchGrab(overXDragThreshold);

                    if (!overXDragThreshold) {
                        bool overYDragThreshold = QQuickWindowPrivate::dragOverThreshold(point.pos().y() - d->pressPoint.y(), Qt::YAxis, &point);
                        setKeepTouchGrab(overYDragThreshold);
                    }
                }
                if (keepTouchGrab())
                    d->handleMove(point.pos());
                break;

            default:
                QQuickControl::touchEvent(event);
                break;
            }
        }
        break;

    default:
        QQuickControl::touchEvent(event);
        break;
    }
}
#endif

#if QT_CONFIG(wheelevent)
void QQuickDial::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickDial);
    QQuickControl::wheelEvent(event);
    if (d->wheelEnabled) {
        const qreal oldValue = d->value;
        const QPointF angle = event->angleDelta();
        const qreal delta = (qFuzzyIsNull(angle.y()) ? angle.x() : (event->inverted() ? -angle.y() : angle.y())) / QWheelEvent::DefaultDeltasPerStep;
        const qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
        setValue(oldValue + step * delta);
        event->setAccepted(!qFuzzyCompare(d->value, oldValue));
    }
}
#endif

void QQuickDial::mirrorChange()
{
    QQuickControl::mirrorChange();
    emit angleChanged();
}

void QQuickDial::componentComplete()
{
    Q_D(QQuickDial);
    d->executeHandle(true);
    QQuickControl::componentComplete();
    setValue(d->value);
    d->updatePosition();
}

#if QT_CONFIG(accessibility)
void QQuickDial::accessibilityActiveChanged(bool active)
{
    QQuickControl::accessibilityActiveChanged(active);

    Q_D(QQuickDial);
    if (active)
        setAccessibleProperty("pressed", d->pressed);
}

QAccessible::Role QQuickDial::accessibleRole() const
{
    return QAccessible::Dial;
}
#endif

QT_END_NAMESPACE
