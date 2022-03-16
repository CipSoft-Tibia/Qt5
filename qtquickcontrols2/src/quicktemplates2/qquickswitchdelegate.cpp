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

#include "qquickswitchdelegate_p.h"

#include "qquickitemdelegate_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype SwitchDelegate
    \inherits ItemDelegate
    \instantiates QQuickSwitchDelegate
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-delegates
    \brief Item delegate with a switch indicator that can be toggled on or off.

    \image qtquickcontrols2-switchdelegate.gif

    SwitchDelegate presents an item delegate that can be toggled on (checked) or
    off (unchecked). Switch delegates are typically used to select one or more
    options from a set of options. For smaller sets of options, or for options
    that need to be uniquely identifiable, consider using \l Switch instead.

    SwitchDelegate inherits its API from \l ItemDelegate, which is inherited
    from \l AbstractButton. For instance, you can set \l {AbstractButton::text}{text},
    and react to \l {AbstractButton::clicked}{clicks} using the \l AbstractButton
    API. The state of the switch delegate can be set with the
    \l {AbstractButton::}{checked} property.

    \code
    ListView {
        model: ["Option 1", "Option 2", "Option 3"]
        delegate: SwitchDelegate {
            text: modelData
        }
    }
    \endcode

    \sa {Customizing SwitchDelegate}, {Delegate Controls}
*/

class QQuickSwitchDelegatePrivate : public QQuickItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QQuickSwitchDelegate)

public:
    qreal positionAt(const QPointF &point) const;

    bool canDrag(const QPointF &movePoint) const;
    void handleMove(const QPointF &point) override;
    void handleRelease(const QPointF &point) override;

    qreal position = 0;
};

qreal QQuickSwitchDelegatePrivate::positionAt(const QPointF &point) const
{
    Q_Q(const QQuickSwitchDelegate);
    qreal pos = 0.0;
    if (indicator)
        pos = indicator->mapFromItem(q, point).x() / indicator->width();
    if (q->isMirrored())
        return 1.0 - pos;
    return pos;
}

bool QQuickSwitchDelegatePrivate::canDrag(const QPointF &movePoint) const
{
    // don't start dragging the handle unless the initial press was at the indicator,
    // or the drag has reached the indicator area. this prevents unnatural jumps when
    // dragging far outside the indicator.
    const qreal pressPos = positionAt(pressPoint);
    const qreal movePos = positionAt(movePoint);
    return (pressPos >= 0.0 && pressPos <= 1.0) || (movePos >= 0.0 && movePos <= 1.0);
}

void QQuickSwitchDelegatePrivate::handleMove(const QPointF &point)
{
    Q_Q(QQuickSwitchDelegate);
    QQuickItemDelegatePrivate::handleMove(point);
    if (q->keepMouseGrab() || q->keepTouchGrab())
        q->setPosition(positionAt(point));
}

void QQuickSwitchDelegatePrivate::handleRelease(const QPointF &point)
{
    Q_Q(QQuickSwitchDelegate);
    QQuickItemDelegatePrivate::handleRelease(point);
    q->setKeepMouseGrab(false);
    q->setKeepTouchGrab(false);
}

QQuickSwitchDelegate::QQuickSwitchDelegate(QQuickItem *parent)
    : QQuickItemDelegate(*(new QQuickSwitchDelegatePrivate), parent)
{
    Q_D(QQuickSwitchDelegate);
    d->keepPressed = true;
    setCheckable(true);
}

/*!
    \qmlproperty real QtQuick.Controls::SwitchDelegate::position
    \readonly

    \input includes/qquickswitch.qdocinc position
*/
qreal QQuickSwitchDelegate::position() const
{
    Q_D(const QQuickSwitchDelegate);
    return d->position;
}

void QQuickSwitchDelegate::setPosition(qreal position)
{
    Q_D(QQuickSwitchDelegate);
    position = qBound<qreal>(0.0, position, 1.0);
    if (qFuzzyCompare(d->position, position))
        return;

    d->position = position;
    emit positionChanged();
    emit visualPositionChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::SwitchDelegate::visualPosition
    \readonly

    \input includes/qquickswitch.qdocinc visualPosition
*/
qreal QQuickSwitchDelegate::visualPosition() const
{
    Q_D(const QQuickSwitchDelegate);
    if (isMirrored())
        return 1.0 - d->position;
    return d->position;
}

void QQuickSwitchDelegate::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickSwitchDelegate);
    if (!keepMouseGrab()) {
        const QPointF movePoint = event->localPos();
        if (d->canDrag(movePoint))
            setKeepMouseGrab(QQuickWindowPrivate::dragOverThreshold(movePoint.x() - d->pressPoint.x(), Qt::XAxis, event));
    }
    QQuickItemDelegate::mouseMoveEvent(event);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickSwitchDelegate::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickSwitchDelegate);
    if (!keepTouchGrab() && event->type() == QEvent::TouchUpdate) {
        for (const QTouchEvent::TouchPoint &point : event->touchPoints()) {
            if (point.id() != d->touchId || point.state() != Qt::TouchPointMoved)
                continue;
            if (d->canDrag(point.pos()))
                setKeepTouchGrab(QQuickWindowPrivate::dragOverThreshold(point.pos().x() - d->pressPoint.x(), Qt::XAxis, &point));
        }
    }
    QQuickItemDelegate::touchEvent(event);
}
#endif

QFont QQuickSwitchDelegate::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ListView);
}

QPalette QQuickSwitchDelegate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::ListView);
}

void QQuickSwitchDelegate::mirrorChange()
{
    QQuickItemDelegate::mirrorChange();
    emit visualPositionChanged();
}

void QQuickSwitchDelegate::nextCheckState()
{
    Q_D(QQuickSwitchDelegate);
    if (keepMouseGrab() || keepTouchGrab()) {
        d->toggle(d->position > 0.5);
        // the checked state might not change => force a position update to
        // avoid that the handle is left somewhere in the middle (QTBUG-57944)
        setPosition(d->checked ? 1.0 : 0.0);
    } else {
        QQuickItemDelegate::nextCheckState();
    }
}

void QQuickSwitchDelegate::buttonChange(ButtonChange change)
{
    Q_D(QQuickSwitchDelegate);
    if (change == ButtonCheckedChange)
        setPosition(d->checked ? 1.0 : 0.0);
    else
        QQuickAbstractButton::buttonChange(change);
}

QT_END_NAMESPACE
