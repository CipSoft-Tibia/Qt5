// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcheckdelegate_p.h"
#include "qquickitemdelegate_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype CheckDelegate
    \inherits ItemDelegate
//!     \instantiates QQuickCheckDelegate
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-delegates
    \brief Item delegate with a check indicator that can be toggled on or off.

    \image qtquickcontrols-checkdelegate.gif

    CheckDelegate presents an item delegate that can be toggled on (checked) or
    off (unchecked). Check delegates are typically used to select one or more
    options from a set of options in a list. For smaller sets of options, or
    for options that need to be uniquely identifiable, consider using
    \l CheckBox instead.

    CheckDelegate inherits its API from \l ItemDelegate, which is inherited
    from AbstractButton. For instance, you can set \l {AbstractButton::text}{text},
    and react to \l {AbstractButton::clicked}{clicks} using the AbstractButton
    API. The state of the check delegate can be set with the
    \l {AbstractButton::}{checked} property.

    In addition to the checked and unchecked states, there is a third state:
    partially checked. The partially checked state can be enabled using the
    \l tristate property. This state indicates that the regular checked/unchecked
    state can not be determined; generally because of other states that affect
    the check delegate. This state is useful when several child nodes are selected
    in a treeview, for example.

    \code
    ListView {
        model: ["Option 1", "Option 2", "Option 3"]
        delegate: CheckDelegate {
            text: modelData
        }
    }
    \endcode

    \sa {Customizing CheckDelegate}, {Delegate Controls}, CheckBox
*/

class QQuickCheckDelegatePrivate : public QQuickItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QQuickCheckDelegate)

public:
    void setNextCheckState(const QJSValue &callback);

    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::ListView); }

    bool tristate = false;
    Qt::CheckState checkState = Qt::Unchecked;
    QJSValue nextCheckState;
};

void QQuickCheckDelegatePrivate::setNextCheckState(const QJSValue &callback)
{
    Q_Q(QQuickCheckDelegate);
    nextCheckState = callback;
    emit q->nextCheckStateChanged();
}

QQuickCheckDelegate::QQuickCheckDelegate(QQuickItem *parent)
    : QQuickItemDelegate(*(new QQuickCheckDelegatePrivate), parent)
{
    setCheckable(true);
}

/*!
    \qmlproperty bool QtQuick.Controls::CheckDelegate::tristate

    This property determines whether the check delegate has three states.

    In the animation below, the first checkdelegate is tri-state:

    \image qtquickcontrols-checkdelegate-tristate.gif

    The default is \c false, i.e., the delegate has only two states.
*/
bool QQuickCheckDelegate::isTristate() const
{
    Q_D(const QQuickCheckDelegate);
    return d->tristate;
}

void QQuickCheckDelegate::setTristate(bool tristate)
{
    Q_D(QQuickCheckDelegate);
    if (d->tristate == tristate)
        return;

    d->tristate = tristate;
    emit tristateChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::CheckDelegate::checkState

    This property determines the check state of the check delegate.

    Available states:
    \value Qt.Unchecked The delegate is unchecked.
    \value Qt.PartiallyChecked The delegate is partially checked. This state is only used when \l tristate is enabled.
    \value Qt.Checked The delegate is checked.

    \sa tristate, {AbstractButton::checked}{checked}
*/
Qt::CheckState QQuickCheckDelegate::checkState() const
{
    Q_D(const QQuickCheckDelegate);
    return d->checkState;
}

void QQuickCheckDelegate::setCheckState(Qt::CheckState state)
{
    Q_D(QQuickCheckDelegate);
    if (d->checkState == state)
        return;

    bool wasChecked = isChecked();
    d->checked = state == Qt::Checked;
    d->checkState = state;
    emit checkStateChanged();
    if (d->checked != wasChecked)
        emit checkedChanged();
}

QFont QQuickCheckDelegate::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ListView);
}

void QQuickCheckDelegate::buttonChange(ButtonChange change)
{
    if (change == ButtonCheckedChange)
        setCheckState(isChecked() ? Qt::Checked : Qt::Unchecked);
    else
        QQuickAbstractButton::buttonChange(change);
}

/*!
    \since QtQuick.Controls 2.4 (Qt 5.11)
    \qmlproperty function QtQuick.Controls::CheckDelegate::nextCheckState

    This property holds a callback function that is called to determine
    the next check state whenever the check delegate is interactively toggled
    by the user via touch, mouse, or keyboard.

    By default, a normal check delegate cycles between \c Qt.Unchecked and
    \c Qt.Checked states, and a tri-state check delegate cycles between
    \c Qt.Unchecked, \c Qt.PartiallyChecked, and \c Qt.Checked states.

    The \c nextCheckState callback function can override the default behavior.
    The following example implements a tri-state check delegate that can present
    a partially checked state depending on external conditions, but never
    cycles to the partially checked state when interactively toggled by
    the user.

    \code
    CheckDelegate {
        tristate: true
        checkState: allChildrenChecked ? Qt.Checked :
                       anyChildChecked ? Qt.PartiallyChecked : Qt.Unchecked

        nextCheckState: function() {
            if (checkState === Qt.Checked)
                return Qt.Unchecked
            else
                return Qt.Checked
        }
    }
    \endcode
*/
void QQuickCheckDelegate::nextCheckState()
{
    Q_D(QQuickCheckDelegate);
    if (d->nextCheckState.isCallable())
        setCheckState(static_cast<Qt::CheckState>(d->nextCheckState.call().toInt()));
    else if (d->tristate)
        setCheckState(static_cast<Qt::CheckState>((d->checkState + 1) % 3));
    else
        QQuickItemDelegate::nextCheckState();
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickCheckDelegate::accessibleRole() const
{
    return QAccessible::CheckBox;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickcheckdelegate_p.cpp"
