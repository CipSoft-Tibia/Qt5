// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickprogressbar_p.h"
#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ProgressBar
    \inherits Control
//!     \instantiates QQuickProgressBar
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-indicators
    \brief Indicates the progress of an operation.

    \image qtquickcontrols-progressbar.gif

    ProgressBar indicates the progress of an operation. The value should be updated
    regularly. The range is defined by \l from and \l to, which both can contain any value.

    \code
    ProgressBar {
        value: 0.5
    }
    \endcode

    ProgressBar also supports a special \l indeterminate mode, which is useful,
    for example, when unable to determine the size of the item being downloaded,
    or if the download progress gets interrupted due to a network disconnection.

    \image qtquickcontrols-progressbar-indeterminate.gif

    \code
    ProgressBar {
        indeterminate: true
    }
    \endcode

    The indeterminate mode is similar to a \l BusyIndicator. Both can be used
    to indicate background activity. The main difference is visual, and that
    ProgressBar can also present a concrete amount of progress (when it can be
    determined). Due to the visual difference, indeterminate progress bars and
    busy indicators fit different places in user interfaces. Typical places for
    an indeterminate progress bar:
    \list
    \li at the bottom of a \l ToolBar
    \li inline within the content of a \l Page
    \li in an \l ItemDelegate to show the progress of a particular item
    \endlist

    \sa {Customizing ProgressBar}, BusyIndicator, {Indicator Controls}
*/

class QQuickProgressBarPrivate : public QQuickControlPrivate
{
public:
    qreal from = 0;
    qreal to = 1;
    qreal value = 0;
    bool indeterminate = false;
};

QQuickProgressBar::QQuickProgressBar(QQuickItem *parent)
    : QQuickControl(*(new QQuickProgressBarPrivate), parent)
{
}

/*!
    \qmlproperty real QtQuick.Controls::ProgressBar::from

    This property holds the starting value for the progress. The default value is \c 0.0.

    \sa to, value
*/
qreal QQuickProgressBar::from() const
{
    Q_D(const QQuickProgressBar);
    return d->from;
}

void QQuickProgressBar::setFrom(qreal from)
{
    Q_D(QQuickProgressBar);
    if (qFuzzyCompare(d->from, from))
        return;

    d->from = from;
    emit fromChanged();
    emit positionChanged();
    emit visualPositionChanged();
    if (isComponentComplete())
        setValue(d->value);
}

/*!
    \qmlproperty real QtQuick.Controls::ProgressBar::to

    This property holds the end value for the progress. The default value is \c 1.0.

    \sa from, value
*/
qreal QQuickProgressBar::to() const
{
    Q_D(const QQuickProgressBar);
    return d->to;
}

void QQuickProgressBar::setTo(qreal to)
{
    Q_D(QQuickProgressBar);
    if (qFuzzyCompare(d->to, to))
        return;

    d->to = to;
    emit toChanged();
    emit positionChanged();
    emit visualPositionChanged();
    if (isComponentComplete())
        setValue(d->value);
}

/*!
    \qmlproperty real QtQuick.Controls::ProgressBar::value

    This property holds the progress value. The default value is \c 0.0.

    \sa from, to, position
*/
qreal QQuickProgressBar::value() const
{
    Q_D(const QQuickProgressBar);
    return d->value;
}

void QQuickProgressBar::setValue(qreal value)
{
    Q_D(QQuickProgressBar);
    if (isComponentComplete())
        value = d->from > d->to ? qBound(d->to, value, d->from) : qBound(d->from, value, d->to);

    if (qFuzzyCompare(d->value, value))
        return;

    d->value = value;
    emit valueChanged();
    emit positionChanged();
    emit visualPositionChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::ProgressBar::position
    \readonly

    This property holds the logical position of the progress.

    The position is expressed as a fraction of the value, in the range
    \c {0.0 - 1.0}. For visualizing the progress, the right-to-left
    aware \l visualPosition should be used instead.

    \sa value, visualPosition
*/
qreal QQuickProgressBar::position() const
{
    Q_D(const QQuickProgressBar);
    if (qFuzzyCompare(d->from, d->to))
        return 0;
    return (d->value - d->from) / (d->to - d->from);
}

/*!
    \qmlproperty real QtQuick.Controls::ProgressBar::visualPosition
    \readonly

    This property holds the visual position of the progress.

    The position is expressed as a fraction of the value, in the range \c {0.0 - 1.0}.
    When the control is \l {Control::mirrored}{mirrored}, \c visuaPosition is equal
    to \c {1.0 - position}. This makes \c visualPosition suitable for visualizing
    the progress, taking right-to-left support into account.

    \sa position, value
*/
qreal QQuickProgressBar::visualPosition() const
{
    if (isMirrored())
        return 1.0 - position();
    return position();
}

/*!
    \qmlproperty bool QtQuick.Controls::ProgressBar::indeterminate

    This property holds whether the progress bar is in indeterminate mode.
    A progress bar in indeterminate mode displays that an operation is in progress, but it
    doesn't show how much progress has been made.

    \image qtquickcontrols-progressbar-indeterminate.gif
*/
bool QQuickProgressBar::isIndeterminate() const
{
    Q_D(const QQuickProgressBar);
    return d->indeterminate;
}

void QQuickProgressBar::setIndeterminate(bool indeterminate)
{
    Q_D(QQuickProgressBar);
    if (d->indeterminate == indeterminate)
        return;

    d->indeterminate = indeterminate;
    emit indeterminateChanged();
}

void QQuickProgressBar::mirrorChange()
{
    QQuickControl::mirrorChange();
    if (!qFuzzyCompare(position(), qreal(0.5)))
        emit visualPositionChanged();
}

void QQuickProgressBar::componentComplete()
{
    Q_D(QQuickProgressBar);
    QQuickControl::componentComplete();
    setValue(d->value);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickProgressBar::accessibleRole() const
{
    return QAccessible::ProgressBar;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickprogressbar_p.cpp"
