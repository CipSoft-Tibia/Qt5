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

#include "qquickrangeslider_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickdeferredexecute_p_p.h"

#include <QtCore/qscopedpointer.h>
#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RangeSlider
    \inherits Control
    \instantiates QQuickRangeSlider
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-input
    \ingroup qtquickcontrols2-focusscopes
    \brief Used to select a range of values by sliding two handles along a track.

    \image qtquickcontrols2-rangeslider.gif

    RangeSlider is used to select a range specified by two values, by sliding
    each handle along a track.

    In the example below, custom \l from and \l to values are set, and the
    initial positions of the \l first and \l second handles are set:

    \code
    RangeSlider {
        from: 1
        to: 100
        first.value: 25
        second.value: 75
    }
    \endcode

    In order to perform an action when the value for a particular handle changes,
    use the following syntax:

    \code
    first.onMoved: console.log("first.value changed to " + first.value)
    \endcode

    The \l {first.position} and \l {second.position} properties are expressed as
    fractions of the control's size, in the range \c {0.0 - 1.0}.
    The \l {first.visualPosition} and \l {second.visualPosition} properties are
    the same, except that they are reversed in a
    \l {Right-to-left User Interfaces}{right-to-left} application.
    The \c visualPosition is useful for positioning the handles when styling
    RangeSlider. In the example above, \l {first.visualPosition} will be \c 0.24
    in a left-to-right application, and \c 0.76 in a right-to-left application.

    For a slider that allows the user to select a single value, see \l Slider.

    \sa {Customizing RangeSlider}, {Input Controls},
        {Focus Management in Qt Quick Controls 2}
*/

class QQuickRangeSliderNodePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickRangeSliderNode)
public:
    QQuickRangeSliderNodePrivate(qreal value, QQuickRangeSlider *slider)
        : value(value),
          slider(slider)
    {
    }

    bool isFirst() const;

    void setPosition(qreal position, bool ignoreOtherPosition = false);
    void updatePosition(bool ignoreOtherPosition = false);

    void cancelHandle();
    void executeHandle(bool complete = false);

    static QQuickRangeSliderNodePrivate *get(QQuickRangeSliderNode *node);

    qreal value = 0;
    bool isPendingValue = false;
    qreal pendingValue = 0;
    qreal position = 0;
    QQuickDeferredPointer<QQuickItem> handle;
    QQuickRangeSlider *slider = nullptr;
    bool pressed = false;
    bool hovered = false;
    int touchId = -1;
};

bool QQuickRangeSliderNodePrivate::isFirst() const
{
    return this == get(slider->first());
}

void QQuickRangeSliderNodePrivate::setPosition(qreal position, bool ignoreOtherPosition)
{
    Q_Q(QQuickRangeSliderNode);

    const qreal min = isFirst() || ignoreOtherPosition ? 0.0 : qMax<qreal>(0.0, slider->first()->position());
    const qreal max = !isFirst() || ignoreOtherPosition ? 1.0 : qMin<qreal>(1.0, slider->second()->position());
    position = qBound(min, position, max);
    if (!qFuzzyCompare(this->position, position)) {
        this->position = position;
        emit q->positionChanged();
        emit q->visualPositionChanged();
    }
}

void QQuickRangeSliderNodePrivate::updatePosition(bool ignoreOtherPosition)
{
    qreal pos = 0;
    if (!qFuzzyCompare(slider->from(), slider->to()))
        pos = (value - slider->from()) / (slider->to() - slider->from());
    setPosition(pos, ignoreOtherPosition);
}

static inline QString handleName() { return QStringLiteral("handle"); }

void QQuickRangeSliderNodePrivate::cancelHandle()
{
    Q_Q(QQuickRangeSliderNode);
    quickCancelDeferred(q, handleName());
}

void QQuickRangeSliderNodePrivate::executeHandle(bool complete)
{
    Q_Q(QQuickRangeSliderNode);
    if (handle.wasExecuted())
        return;

    if (!handle || complete)
        quickBeginDeferred(q, handleName(), handle);
    if (complete)
        quickCompleteDeferred(q, handleName(), handle);
}

QQuickRangeSliderNodePrivate *QQuickRangeSliderNodePrivate::get(QQuickRangeSliderNode *node)
{
    return node->d_func();
}

QQuickRangeSliderNode::QQuickRangeSliderNode(qreal value, QQuickRangeSlider *slider)
    : QObject(*(new QQuickRangeSliderNodePrivate(value, slider)), slider)
{
}

QQuickRangeSliderNode::~QQuickRangeSliderNode()
{
}

qreal QQuickRangeSliderNode::value() const
{
    Q_D(const QQuickRangeSliderNode);
    return d->value;
}

void QQuickRangeSliderNode::setValue(qreal value)
{
    Q_D(QQuickRangeSliderNode);
    if (!d->slider->isComponentComplete()) {
        d->pendingValue = value;
        d->isPendingValue = true;
        return;
    }

    // First, restrict the first value to be within to and from.
    const qreal smaller = qMin(d->slider->to(), d->slider->from());
    const qreal larger = qMax(d->slider->to(), d->slider->from());
    value = qBound(smaller, value, larger);

    // Then, ensure that it doesn't go past the other value,
    // a check that depends on whether or not the range is inverted.
    const bool invertedRange = d->slider->from() > d->slider->to();
    if (d->isFirst()) {
        if (invertedRange) {
            if (value < d->slider->second()->value())
                value = d->slider->second()->value();
        } else {
            if (value > d->slider->second()->value())
                value = d->slider->second()->value();
        }
    } else {
        if (invertedRange) {
            if (value > d->slider->first()->value())
                value = d->slider->first()->value();
        } else {
            if (value < d->slider->first()->value())
                value = d->slider->first()->value();
        }
    }

    if (!qFuzzyCompare(d->value, value)) {
        d->value = value;
        d->updatePosition();
        emit valueChanged();
    }
}

qreal QQuickRangeSliderNode::position() const
{
    Q_D(const QQuickRangeSliderNode);
    return d->position;
}

qreal QQuickRangeSliderNode::visualPosition() const
{
    Q_D(const QQuickRangeSliderNode);
    if (d->slider->orientation() == Qt::Vertical || d->slider->isMirrored())
        return 1.0 - d->position;
    return d->position;
}

QQuickItem *QQuickRangeSliderNode::handle() const
{
    QQuickRangeSliderNodePrivate *d = const_cast<QQuickRangeSliderNodePrivate *>(d_func());
    if (!d->handle)
        d->executeHandle();
    return d->handle;
}

void QQuickRangeSliderNode::setHandle(QQuickItem *handle)
{
    Q_D(QQuickRangeSliderNode);
    if (d->handle == handle)
        return;

    if (!d->handle.isExecuting())
        d->cancelHandle();

    const qreal oldImplicitHandleWidth = implicitHandleWidth();
    const qreal oldImplicitHandleHeight = implicitHandleHeight();

    QQuickControlPrivate::get(d->slider)->removeImplicitSizeListener(d->handle);
    delete d->handle;
    d->handle = handle;

    if (handle) {
        if (!handle->parentItem())
            handle->setParentItem(d->slider);

        QQuickItem *firstHandle = QQuickRangeSliderNodePrivate::get(d->slider->first())->handle;
        QQuickItem *secondHandle = QQuickRangeSliderNodePrivate::get(d->slider->second())->handle;
        if (firstHandle && secondHandle) {
            // The order of property assignments in QML is undefined,
            // but we need the first handle to be before the second due
            // to focus order constraints, so check for that here.
            const QList<QQuickItem *> childItems = d->slider->childItems();
            const int firstIndex = childItems.indexOf(firstHandle);
            const int secondIndex = childItems.indexOf(secondHandle);
            if (firstIndex != -1 && secondIndex != -1 && firstIndex > secondIndex) {
                firstHandle->stackBefore(secondHandle);
                // Ensure we have some way of knowing which handle is above
                // the other when it comes to mouse presses, and also that
                // they are rendered in the correct order.
                secondHandle->setZ(secondHandle->z() + 1);
            }
        }

        handle->setActiveFocusOnTab(true);
        QQuickControlPrivate::get(d->slider)->addImplicitSizeListener(handle);
    }

    if (!qFuzzyCompare(oldImplicitHandleWidth, implicitHandleWidth()))
        emit implicitHandleWidthChanged();
    if (!qFuzzyCompare(oldImplicitHandleHeight, implicitHandleHeight()))
        emit implicitHandleHeightChanged();
    if (!d->handle.isExecuting())
        emit handleChanged();
}

bool QQuickRangeSliderNode::isPressed() const
{
    Q_D(const QQuickRangeSliderNode);
    return d->pressed;
}

void QQuickRangeSliderNode::setPressed(bool pressed)
{
    Q_D(QQuickRangeSliderNode);
    if (d->pressed == pressed)
        return;

    d->pressed = pressed;
    d->slider->setAccessibleProperty("pressed", pressed || d->slider->second()->isPressed());
    emit pressedChanged();
}

bool QQuickRangeSliderNode::isHovered() const
{
    Q_D(const QQuickRangeSliderNode);
    return d->hovered;
}

void QQuickRangeSliderNode::setHovered(bool hovered)
{
    Q_D(QQuickRangeSliderNode);
    if (d->hovered == hovered)
        return;

    d->hovered = hovered;
    emit hoveredChanged();
}

qreal QQuickRangeSliderNode::implicitHandleWidth() const
{
    Q_D(const QQuickRangeSliderNode);
    if (!d->handle)
        return 0;
    return d->handle->implicitWidth();
}

qreal QQuickRangeSliderNode::implicitHandleHeight() const
{
    Q_D(const QQuickRangeSliderNode);
    if (!d->handle)
        return 0;
    return d->handle->implicitHeight();
}

void QQuickRangeSliderNode::increase()
{
    Q_D(QQuickRangeSliderNode);
    qreal step = qFuzzyIsNull(d->slider->stepSize()) ? 0.1 : d->slider->stepSize();
    setValue(d->value + step);
}

void QQuickRangeSliderNode::decrease()
{
    Q_D(QQuickRangeSliderNode);
    qreal step = qFuzzyIsNull(d->slider->stepSize()) ? 0.1 : d->slider->stepSize();
    setValue(d->value - step);
}

static const qreal defaultFrom = 0.0;
static const qreal defaultTo = 1.0;

class QQuickRangeSliderPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickRangeSlider)

public:
    QQuickRangeSliderNode *pressedNode(int touchId = -1) const;

#if QT_CONFIG(quicktemplates2_multitouch)
    bool acceptTouch(const QTouchEvent::TouchPoint &point) override;
#endif
    void handlePress(const QPointF &point) override;
    void handleMove(const QPointF &point) override;
    void handleRelease(const QPointF &point) override;
    void handleUngrab() override;

    void updateHover(const QPointF &pos);

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    bool live = true;
    qreal from = defaultFrom;
    qreal to = defaultTo;
    qreal stepSize = 0;
    qreal touchDragThreshold = -1;
    QQuickRangeSliderNode *first = nullptr;
    QQuickRangeSliderNode *second = nullptr;
    QPointF pressPoint;
    Qt::Orientation orientation = Qt::Horizontal;
    QQuickRangeSlider::SnapMode snapMode = QQuickRangeSlider::NoSnap;
};

static qreal valueAt(const QQuickRangeSlider *slider, qreal position)
{
    return slider->from() + (slider->to() - slider->from()) * position;
}

static qreal snapPosition(const QQuickRangeSlider *slider, qreal position)
{
    const qreal range = slider->to() - slider->from();
    if (qFuzzyIsNull(range))
        return position;

    const qreal effectiveStep = slider->stepSize() / range;
    if (qFuzzyIsNull(effectiveStep))
        return position;

    return qRound(position / effectiveStep) * effectiveStep;
}

static qreal positionAt(const QQuickRangeSlider *slider, QQuickItem *handle, const QPointF &point)
{
    if (slider->orientation() == Qt::Horizontal) {
        const qreal hw = handle ? handle->width() : 0;
        const qreal offset = hw / 2;
        const qreal extent = slider->availableWidth() - hw;
        if (!qFuzzyIsNull(extent)) {
            if (slider->isMirrored())
                return (slider->width() - point.x() - slider->rightPadding() - offset) / extent;
            return (point.x() - slider->leftPadding() - offset) / extent;
        }
    } else {
        const qreal hh = handle ? handle->height() : 0;
        const qreal offset = hh / 2;
        const qreal extent = slider->availableHeight() - hh;
        if (!qFuzzyIsNull(extent))
            return (slider->height() - point.y() - slider->bottomPadding() - offset) / extent;
    }
    return 0;
}

QQuickRangeSliderNode *QQuickRangeSliderPrivate::pressedNode(int touchId) const
{
    if (touchId == -1)
        return first->isPressed() ? first : (second->isPressed() ? second : nullptr);
    if (QQuickRangeSliderNodePrivate::get(first)->touchId == touchId)
        return first;
    if (QQuickRangeSliderNodePrivate::get(second)->touchId == touchId)
        return second;
    return nullptr;
}

#if QT_CONFIG(quicktemplates2_multitouch)
bool QQuickRangeSliderPrivate::acceptTouch(const QTouchEvent::TouchPoint &point)
{
    int firstId = QQuickRangeSliderNodePrivate::get(first)->touchId;
    int secondId = QQuickRangeSliderNodePrivate::get(second)->touchId;

    if (((firstId == -1 || secondId == -1) && point.state() == Qt::TouchPointPressed) || point.id() == firstId || point.id() == secondId) {
        touchId = point.id();
        return true;
    }

    return false;
}
#endif

void QQuickRangeSliderPrivate::handlePress(const QPointF &point)
{
    Q_Q(QQuickRangeSlider);
    QQuickControlPrivate::handlePress(point);
    pressPoint = point;

    QQuickItem *firstHandle = first->handle();
    QQuickItem *secondHandle = second->handle();
    const bool firstHit = firstHandle && !first->isPressed() && firstHandle->contains(q->mapToItem(firstHandle, point));
    const bool secondHit = secondHandle && !second->isPressed() && secondHandle->contains(q->mapToItem(secondHandle, point));
    QQuickRangeSliderNode *hitNode = nullptr;
    QQuickRangeSliderNode *otherNode = nullptr;

    if (firstHit && secondHit) {
        // choose highest
        hitNode = firstHandle->z() > secondHandle->z() ? first : second;
        otherNode = firstHandle->z() > secondHandle->z() ? second : first;
    } else if (firstHit) {
        hitNode = first;
        otherNode = second;
    } else if (secondHit) {
        hitNode = second;
        otherNode = first;
    } else {
        // find the nearest
        const qreal firstPos = positionAt(q, firstHandle, point);
        const qreal secondPos = positionAt(q, secondHandle, point);
        const qreal firstDistance = qAbs(firstPos - first->position());
        const qreal secondDistance = qAbs(secondPos - second->position());

        if (qFuzzyCompare(firstDistance, secondDistance)) {
            // same distance => choose the one that can be moved towards the press position
            const bool inverted = from > to;
            if ((!inverted && firstPos < first->position()) || (inverted && firstPos > first->position())) {
                hitNode = first;
                otherNode = second;
            } else {
                hitNode = second;
                otherNode = first;
            }
        } else if (firstDistance < secondDistance) {
            hitNode = first;
            otherNode = second;
        } else {
            hitNode = second;
            otherNode = first;
        }
    }

    if (hitNode) {
        hitNode->setPressed(true);
        if (QQuickItem *handle = hitNode->handle())
            handle->setZ(1);
        QQuickRangeSliderNodePrivate::get(hitNode)->touchId = touchId;
    }
    if (otherNode) {
        if (QQuickItem *handle = otherNode->handle())
            handle->setZ(0);
    }
}

void QQuickRangeSliderPrivate::handleMove(const QPointF &point)
{
    Q_Q(QQuickRangeSlider);
    QQuickControlPrivate::handleMove(point);
    QQuickRangeSliderNode *pressedNode = QQuickRangeSliderPrivate::pressedNode(touchId);
    if (pressedNode) {
        const qreal oldPos = pressedNode->position();
        qreal pos = positionAt(q, pressedNode->handle(), point);
        if (snapMode == QQuickRangeSlider::SnapAlways)
            pos = snapPosition(q, pos);
        if (live)
            pressedNode->setValue(valueAt(q, pos));
        else
            QQuickRangeSliderNodePrivate::get(pressedNode)->setPosition(pos);

        if (!qFuzzyCompare(pressedNode->position(), oldPos))
            emit pressedNode->moved();
    }
}

void QQuickRangeSliderPrivate::handleRelease(const QPointF &point)
{
    Q_Q(QQuickRangeSlider);
    QQuickControlPrivate::handleRelease(point);
    pressPoint = QPointF();

    QQuickRangeSliderNode *pressedNode = QQuickRangeSliderPrivate::pressedNode(touchId);
    if (!pressedNode)
        return;
    QQuickRangeSliderNodePrivate *pressedNodePrivate = QQuickRangeSliderNodePrivate::get(pressedNode);

    if (q->keepMouseGrab() || q->keepTouchGrab()) {
        const qreal oldPos = pressedNode->position();
        qreal pos = positionAt(q, pressedNode->handle(), point);
        if (snapMode != QQuickRangeSlider::NoSnap)
            pos = snapPosition(q, pos);
        qreal val = valueAt(q, pos);
        if (!qFuzzyCompare(val, pressedNode->value()))
            pressedNode->setValue(val);
        else if (snapMode != QQuickRangeSlider::NoSnap)
            pressedNodePrivate->setPosition(pos);
        q->setKeepMouseGrab(false);
        q->setKeepTouchGrab(false);

        if (!qFuzzyCompare(pressedNode->position(), oldPos))
            emit pressedNode->moved();
    }
    pressedNode->setPressed(false);
    pressedNodePrivate->touchId = -1;
}

void QQuickRangeSliderPrivate::handleUngrab()
{
    QQuickControlPrivate::handleUngrab();
    pressPoint = QPointF();
    first->setPressed(false);
    second->setPressed(false);
    QQuickRangeSliderNodePrivate::get(first)->touchId = -1;
    QQuickRangeSliderNodePrivate::get(second)->touchId = -1;
}

void QQuickRangeSliderPrivate::updateHover(const QPointF &pos)
{
    Q_Q(QQuickRangeSlider);
    QQuickItem *firstHandle = first->handle();
    QQuickItem *secondHandle = second->handle();
    first->setHovered(firstHandle && firstHandle->isEnabled() && firstHandle->contains(q->mapToItem(firstHandle, pos)));
    second->setHovered(secondHandle && secondHandle->isEnabled() && secondHandle->contains(q->mapToItem(secondHandle, pos)));
}

void QQuickRangeSliderPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    QQuickControlPrivate::itemImplicitWidthChanged(item);
    if (item == first->handle())
        emit first->implicitHandleWidthChanged();
    else if (item == second->handle())
        emit second->implicitHandleWidthChanged();
}

void QQuickRangeSliderPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    QQuickControlPrivate::itemImplicitHeightChanged(item);
    if (item == first->handle())
        emit first->implicitHandleHeightChanged();
    else if (item == second->handle())
        emit second->implicitHandleHeightChanged();
}

QQuickRangeSlider::QQuickRangeSlider(QQuickItem *parent)
    : QQuickControl(*(new QQuickRangeSliderPrivate), parent)
{
    Q_D(QQuickRangeSlider);
    d->first = new QQuickRangeSliderNode(0.0, this);
    d->second = new QQuickRangeSliderNode(1.0, this);

    setFlag(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif
}

QQuickRangeSlider::~QQuickRangeSlider()
{
    Q_D(QQuickRangeSlider);
    d->removeImplicitSizeListener(d->first->handle());
    d->removeImplicitSizeListener(d->second->handle());
}

/*!
    \qmlproperty real QtQuick.Controls::RangeSlider::from

    This property holds the starting value for the range. The default value is \c 0.0.

    \sa to, first.value, second.value
*/
qreal QQuickRangeSlider::from() const
{
    Q_D(const QQuickRangeSlider);
    return d->from;
}

void QQuickRangeSlider::setFrom(qreal from)
{
    Q_D(QQuickRangeSlider);
    if (qFuzzyCompare(d->from, from))
        return;

    d->from = from;
    emit fromChanged();

    if (isComponentComplete()) {
        d->first->setValue(d->first->value());
        d->second->setValue(d->second->value());
    }
}

/*!
    \qmlproperty real QtQuick.Controls::RangeSlider::to

    This property holds the end value for the range. The default value is \c 1.0.

    \sa from, first.value, second.value
*/
qreal QQuickRangeSlider::to() const
{
    Q_D(const QQuickRangeSlider);
    return d->to;
}

void QQuickRangeSlider::setTo(qreal to)
{
    Q_D(QQuickRangeSlider);
    if (qFuzzyCompare(d->to, to))
        return;

    d->to = to;
    emit toChanged();

    if (isComponentComplete()) {
        d->first->setValue(d->first->value());
        d->second->setValue(d->second->value());
    }
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty qreal QtQuick.Controls::RangeSlider::touchDragThreshold

    This property holds the threshold (in logical pixels) at which a touch drag event will be initiated.
    The mouse drag threshold won't be affected.
    The default value is \c Qt.styleHints.startDragDistance.

    \sa QStyleHints

*/
qreal QQuickRangeSlider::touchDragThreshold() const
{
    Q_D(const QQuickRangeSlider);
    return d->touchDragThreshold;
}

void QQuickRangeSlider::setTouchDragThreshold(qreal touchDragThreshold)
{
    Q_D(QQuickRangeSlider);
    if (d->touchDragThreshold == touchDragThreshold)
        return;

    d->touchDragThreshold = touchDragThreshold;
    emit touchDragThresholdChanged();
}

void QQuickRangeSlider::resetTouchDragThreshold()
{
    setTouchDragThreshold(-1);
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlmethod real QtQuick.Controls::RangeSlider::valueAt(real position)

    Returns the value for the given \a position.

    \sa first.value, second.value, first.position, second.position, live
*/
qreal QQuickRangeSlider::valueAt(qreal position) const
{
    Q_D(const QQuickRangeSlider);
    const qreal value = (d->to - d->from) * position;
    if (qFuzzyIsNull(d->stepSize))
        return d->from + value;
    return d->from + qRound(value / d->stepSize) * d->stepSize;
}

/*!
    \qmlpropertygroup QtQuick.Controls::RangeSlider::first
    \qmlproperty real QtQuick.Controls::RangeSlider::first.value
    \qmlproperty real QtQuick.Controls::RangeSlider::first.position
    \qmlproperty real QtQuick.Controls::RangeSlider::first.visualPosition
    \qmlproperty Item QtQuick.Controls::RangeSlider::first.handle
    \qmlproperty bool QtQuick.Controls::RangeSlider::first.pressed
    \qmlproperty bool QtQuick.Controls::RangeSlider::first.hovered
    \qmlproperty real QtQuick.Controls::RangeSlider::first.implicitHandleWidth
    \qmlproperty real QtQuick.Controls::RangeSlider::first.implicitHandleHeight

    \table
    \header
        \li Property
        \li Description
    \row
        \li value
        \li This property holds the value of the first handle in the range
            \c from - \c to.

            If \l from is greater than \l to, the value of the first handle
            must be greater than the second, and vice versa.

            The default value is \c 0.0.
    \row
        \li handle
        \li This property holds the first handle item.
    \row
        \li visualPosition
        \li This property holds the visual position of the first handle.

            The position is expressed as a fraction of the control's size, in the range
            \c {0.0 - 1.0}. When the control is \l {Control::mirrored}{mirrored}, the
            value is equal to \c {1.0 - position}. This makes the value suitable for
            visualizing the slider, taking right-to-left support into account.
    \row
        \li position
        \li This property holds the logical position of the first handle.

            The position is expressed as a fraction of the control's size, in the range
            \c {0.0 - 1.0}. For visualizing a slider, the right-to-left aware
            \l {first.visualPosition}{visualPosition} should be used instead.
    \row
        \li pressed
        \li This property holds whether the first handle is pressed by either touch,
            mouse, or keys.
    \row
        \li hovered
        \li This property holds whether the first handle is hovered.
            This property was introduced in QtQuick.Controls 2.1.
    \row
        \li implicitHandleWidth
        \li This property holds the implicit width of the first handle.
            This property was introduced in QtQuick.Controls 2.5.
    \row
        \li implicitHandleHeight
        \li This property holds the implicit height of the first handle.
            This property was introduced in QtQuick.Controls 2.5.
    \endtable

    \sa first.moved(), first.increase(), first.decrease()
*/
QQuickRangeSliderNode *QQuickRangeSlider::first() const
{
    Q_D(const QQuickRangeSlider);
    return d->first;
}

/*!
    \qmlsignal void QtQuick.Controls::RangeSlider::first.moved()
    \qmlsignal void QtQuick.Controls::RangeSlider::second.moved()
    \since QtQuick.Controls 2.5

    This signal is emitted when either the first or second handle has been
    interactively moved by the user by either touch, mouse, or keys.

    \sa first, second
*/

/*!
    \qmlpropertygroup QtQuick.Controls::RangeSlider::second
    \qmlproperty real QtQuick.Controls::RangeSlider::second.value
    \qmlproperty real QtQuick.Controls::RangeSlider::second.position
    \qmlproperty real QtQuick.Controls::RangeSlider::second.visualPosition
    \qmlproperty Item QtQuick.Controls::RangeSlider::second.handle
    \qmlproperty bool QtQuick.Controls::RangeSlider::second.pressed
    \qmlproperty bool QtQuick.Controls::RangeSlider::second.hovered
    \qmlproperty real QtQuick.Controls::RangeSlider::second.implicitHandleWidth
    \qmlproperty real QtQuick.Controls::RangeSlider::second.implicitHandleHeight

    \table
    \header
        \li Property
        \li Description
    \row
        \li value
        \li This property holds the value of the second handle in the range
            \c from - \c to.

            If \l from is greater than \l to, the value of the first handle
            must be greater than the second, and vice versa.

            The default value is \c 0.0.
    \row
        \li handle
        \li This property holds the second handle item.
    \row
        \li visualPosition
        \li This property holds the visual position of the second handle.

            The position is expressed as a fraction of the control's size, in the range
            \c {0.0 - 1.0}. When the control is \l {Control::mirrored}{mirrored}, the
            value is equal to \c {1.0 - position}. This makes the value suitable for
            visualizing the slider, taking right-to-left support into account.
    \row
        \li position
        \li This property holds the logical position of the second handle.

            The position is expressed as a fraction of the control's size, in the range
            \c {0.0 - 1.0}. For visualizing a slider, the right-to-left aware
            \l {second.visualPosition}{visualPosition} should be used instead.
    \row
        \li pressed
        \li This property holds whether the second handle is pressed by either touch,
            mouse, or keys.
    \row
        \li hovered
        \li This property holds whether the second handle is hovered.
            This property was introduced in QtQuick.Controls 2.1.
    \row
        \li implicitHandleWidth
        \li This property holds the implicit width of the second handle.
            This property was introduced in QtQuick.Controls 2.5.
    \row
        \li implicitHandleHeight
        \li This property holds the implicit height of the second handle.
            This property was introduced in QtQuick.Controls 2.5.
    \endtable

    \sa second.moved(), second.increase(), second.decrease()
*/
QQuickRangeSliderNode *QQuickRangeSlider::second() const
{
    Q_D(const QQuickRangeSlider);
    return d->second;
}

/*!
    \qmlproperty real QtQuick.Controls::RangeSlider::stepSize

    This property holds the step size. The default value is \c 0.0.

    \sa snapMode, first.increase(), first.decrease()
*/
qreal QQuickRangeSlider::stepSize() const
{
    Q_D(const QQuickRangeSlider);
    return d->stepSize;
}

void QQuickRangeSlider::setStepSize(qreal step)
{
    Q_D(QQuickRangeSlider);
    if (qFuzzyCompare(d->stepSize, step))
        return;

    d->stepSize = step;
    emit stepSizeChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::RangeSlider::snapMode

    This property holds the snap mode.

    The snap mode determines how the slider handles behave with
    regards to the \l stepSize.

    Possible values:
    \value RangeSlider.NoSnap The slider does not snap (default).
    \value RangeSlider.SnapAlways The slider snaps while the handle is dragged.
    \value RangeSlider.SnapOnRelease The slider does not snap while being dragged, but only after the handle is released.

    For visual explanations of the various modes, see the
    \l {Slider::}{snapMode} documentation of \l Slider.

    \sa stepSize
*/
QQuickRangeSlider::SnapMode QQuickRangeSlider::snapMode() const
{
    Q_D(const QQuickRangeSlider);
    return d->snapMode;
}

void QQuickRangeSlider::setSnapMode(SnapMode mode)
{
    Q_D(QQuickRangeSlider);
    if (d->snapMode == mode)
        return;

    d->snapMode = mode;
    emit snapModeChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::RangeSlider::orientation

    This property holds the orientation.

    Possible values:
    \value Qt.Horizontal Horizontal (default)
    \value Qt.Vertical Vertical

    \sa horizontal, vertical
*/
Qt::Orientation QQuickRangeSlider::orientation() const
{
    Q_D(const QQuickRangeSlider);
    return d->orientation;
}

void QQuickRangeSlider::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickRangeSlider);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    emit orientationChanged();
}

/*!
    \qmlmethod void QtQuick.Controls::RangeSlider::setValues(real firstValue, real secondValue)

    Sets \l first.value and \l second.value with the given arguments.

    If \a to is larger than \a from and \a firstValue is larger than
    \a secondValue, \a firstValue will be clamped to \a secondValue.

    If \a from is larger than \a to and \a secondValue is larger than
    \a firstValue, \a secondValue will be clamped to \a firstValue.

    This function may be necessary to set the first and second values
    after the control has been completed, as there is a circular
    dependency between firstValue and secondValue which can cause
    assigned values to be clamped to each other.

    \sa stepSize
*/
void QQuickRangeSlider::setValues(qreal firstValue, qreal secondValue)
{
    Q_D(QQuickRangeSlider);
    // Restrict the values to be within to and from.
    const qreal smaller = qMin(d->to, d->from);
    const qreal larger = qMax(d->to, d->from);
    firstValue = qBound(smaller, firstValue, larger);
    secondValue = qBound(smaller, secondValue, larger);

    if (d->from > d->to) {
        // If the from and to values are reversed, the secondValue
        // might be less than the first value, which is not allowed.
        if (secondValue > firstValue)
            secondValue = firstValue;
    } else {
        // Otherwise, clamp first to second if it's too large.
        if (firstValue > secondValue)
            firstValue = secondValue;
    }

    // Then set both values. If they didn't change, no change signal will be emitted.
    QQuickRangeSliderNodePrivate *firstPrivate = QQuickRangeSliderNodePrivate::get(d->first);
    if (firstValue != firstPrivate->value) {
        firstPrivate->value = firstValue;
        emit d->first->valueChanged();
    }

    QQuickRangeSliderNodePrivate *secondPrivate = QQuickRangeSliderNodePrivate::get(d->second);
    if (secondValue != secondPrivate->value) {
        secondPrivate->value = secondValue;
        emit d->second->valueChanged();
    }

    // After we've set both values, then we can update the positions.
    // If we don't do this last, the positions may be incorrect.
    firstPrivate->updatePosition(true);
    secondPrivate->updatePosition();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty bool QtQuick.Controls::RangeSlider::live

    This property holds whether the slider provides live updates for the \l first.value
    and \l second.value properties while the respective handles are dragged.

    The default value is \c true.

    \sa first.value, second.value
*/
bool QQuickRangeSlider::live() const
{
    Q_D(const QQuickRangeSlider);
    return d->live;
}

void QQuickRangeSlider::setLive(bool live)
{
    Q_D(QQuickRangeSlider);
    if (d->live == live)
        return;

    d->live = live;
    emit liveChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::RangeSlider::horizontal
    \readonly

    This property holds whether the slider is horizontal.

    \sa orientation
*/
bool QQuickRangeSlider::isHorizontal() const
{
    Q_D(const QQuickRangeSlider);
    return d->orientation == Qt::Horizontal;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::RangeSlider::vertical
    \readonly

    This property holds whether the slider is vertical.

    \sa orientation
*/
bool QQuickRangeSlider::isVertical() const
{
    Q_D(const QQuickRangeSlider);
    return d->orientation == Qt::Vertical;
}

void QQuickRangeSlider::focusInEvent(QFocusEvent *event)
{
    Q_D(QQuickRangeSlider);
    QQuickControl::focusInEvent(event);

    // The active focus ends up to RangeSlider when using forceActiveFocus()
    // or QML KeyNavigation. We must forward the focus to one of the handles,
    // because RangeSlider handles key events for the focused handle. If
    // neither handle has active focus, RangeSlider doesn't do anything.
    QQuickItem *handle = nextItemInFocusChain();
    // QQuickItem::nextItemInFocusChain() only works as desired with
    // Qt::TabFocusAllControls. otherwise pick the first handle
    if (!handle || handle == this)
        handle = d->first->handle();
    if (handle)
        handle->forceActiveFocus(event->reason());
}

void QQuickRangeSlider::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickRangeSlider);
    QQuickControl::keyPressEvent(event);

    QQuickRangeSliderNode *focusNode = d->first->handle()->hasActiveFocus()
        ? d->first : (d->second->handle()->hasActiveFocus() ? d->second : nullptr);
    if (!focusNode)
        return;

    const qreal oldValue = focusNode->value();
    if (d->orientation == Qt::Horizontal) {
        if (event->key() == Qt::Key_Left) {
            focusNode->setPressed(true);
            if (isMirrored())
                focusNode->increase();
            else
                focusNode->decrease();
            event->accept();
        } else if (event->key() == Qt::Key_Right) {
            focusNode->setPressed(true);
            if (isMirrored())
                focusNode->decrease();
            else
                focusNode->increase();
            event->accept();
        }
    } else {
        if (event->key() == Qt::Key_Up) {
            focusNode->setPressed(true);
            focusNode->increase();
            event->accept();
        } else if (event->key() == Qt::Key_Down) {
            focusNode->setPressed(true);
            focusNode->decrease();
            event->accept();
        }
    }
    if (!qFuzzyCompare(focusNode->value(), oldValue))
        emit focusNode->moved();
}

void QQuickRangeSlider::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QQuickRangeSlider);
    QQuickControl::hoverEnterEvent(event);
    d->updateHover(event->posF());
}

void QQuickRangeSlider::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QQuickRangeSlider);
    QQuickControl::hoverMoveEvent(event);
    d->updateHover(event->posF());
}

void QQuickRangeSlider::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QQuickRangeSlider);
    QQuickControl::hoverLeaveEvent(event);
    d->first->setHovered(false);
    d->second->setHovered(false);
}

void QQuickRangeSlider::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickRangeSlider);
    QQuickControl::keyReleaseEvent(event);
    d->first->setPressed(false);
    d->second->setPressed(false);
}

void QQuickRangeSlider::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickRangeSlider);
    QQuickControl::mousePressEvent(event);
    d->handleMove(event->localPos());
    setKeepMouseGrab(true);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickRangeSlider::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickRangeSlider);
    switch (event->type()) {
    case QEvent::TouchUpdate:
        for (const QTouchEvent::TouchPoint &point : event->touchPoints()) {
            if (!d->acceptTouch(point))
                continue;

            switch (point.state()) {
            case Qt::TouchPointPressed:
                d->handlePress(point.pos());
                break;
            case Qt::TouchPointMoved:
                if (!keepTouchGrab()) {
                    if (d->orientation == Qt::Horizontal)
                        setKeepTouchGrab(QQuickWindowPrivate::dragOverThreshold(point.pos().x() - point.startPos().x(), Qt::XAxis, &point, qRound(d->touchDragThreshold)));
                    else
                        setKeepTouchGrab(QQuickWindowPrivate::dragOverThreshold(point.pos().y() - point.startPos().y(), Qt::YAxis, &point, qRound(d->touchDragThreshold)));
                }
                if (keepTouchGrab())
                    d->handleMove(point.pos());
                break;
            case Qt::TouchPointReleased:
                d->handleRelease(point.pos());
                break;
            default:
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

void QQuickRangeSlider::mirrorChange()
{
    Q_D(QQuickRangeSlider);
    QQuickControl::mirrorChange();
    emit d->first->visualPositionChanged();
    emit d->second->visualPositionChanged();
}

void QQuickRangeSlider::classBegin()
{
    Q_D(QQuickRangeSlider);
    QQuickControl::classBegin();

    QQmlContext *context = qmlContext(this);
    if (context) {
        QQmlEngine::setContextForObject(d->first, context);
        QQmlEngine::setContextForObject(d->second, context);
    }
}

void QQuickRangeSlider::componentComplete()
{
    Q_D(QQuickRangeSlider);
    QQuickRangeSliderNodePrivate *firstPrivate = QQuickRangeSliderNodePrivate::get(d->first);
    QQuickRangeSliderNodePrivate *secondPrivate = QQuickRangeSliderNodePrivate::get(d->second);
    firstPrivate->executeHandle(true);
    secondPrivate->executeHandle(true);

    QQuickControl::componentComplete();

    if (firstPrivate->isPendingValue || secondPrivate->isPendingValue
        || !qFuzzyCompare(d->from, defaultFrom) || !qFuzzyCompare(d->to, defaultTo)) {
        // Properties were set while we were loading. To avoid clamping issues that occur when setting the
        // values of first and second overriding values set by the user, set them all at once at the end.
        // Another reason that we must set these values here is that the from and to values might have made the old range invalid.
        setValues(firstPrivate->isPendingValue ? firstPrivate->pendingValue : firstPrivate->value,
                      secondPrivate->isPendingValue ? secondPrivate->pendingValue : secondPrivate->value);

        firstPrivate->pendingValue = 0;
        firstPrivate->isPendingValue = false;
        secondPrivate->pendingValue = 0;
        secondPrivate->isPendingValue = false;
    } else {
        // If there was no pending data, we must still update the positions,
        // as first.setValue()/second.setValue() won't be called as part of default construction.
        // Don't need to ignore the second position when updating the first position here,
        // as our default values are guaranteed to be valid.
        firstPrivate->updatePosition();
        secondPrivate->updatePosition();
    }
}

/*!
    \qmlmethod void QtQuick.Controls::RangeSlider::first.increase()

    Increases the value of the handle by stepSize, or \c 0.1 if stepSize is not defined.

    \sa first
*/

/*!
    \qmlmethod void QtQuick.Controls::RangeSlider::first.decrease()

    Decreases the value of the handle by stepSize, or \c 0.1 if stepSize is not defined.

    \sa first
*/

/*!
    \qmlmethod void QtQuick.Controls::RangeSlider::second.increase()

    Increases the value of the handle by stepSize, or \c 0.1 if stepSize is not defined.

    \sa second
*/

/*!
    \qmlmethod void QtQuick.Controls::RangeSlider::second.decrease()

    Decreases the value of the handle by stepSize, or \c 0.1 if stepSize is not defined.

    \sa second
*/

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickRangeSlider::accessibleRole() const
{
    return QAccessible::Slider;
}
#endif

QT_END_NAMESPACE
