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

#include "qquickcheckbox_p.h"
#include "qquickabstractbutton_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype CheckBox
    \inherits AbstractButton
    \instantiates QQuickCheckBox
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-buttons
    \brief Check button that can be toggled on or off.

    \image qtquickcontrols2-checkbox.gif

    CheckBox presents an option button that can be toggled on (checked) or
    off (unchecked). Check boxes are typically used to select one or more
    options from a set of options. For larger sets of options, such as those
    in a list, consider using \l CheckDelegate instead.

    CheckBox inherits its API from \l AbstractButton. For instance, the
    state of the checkbox can be set with the \l {AbstractButton::}{checked} property.

    In addition to the checked and unchecked states, there is a third state:
    partially checked. The partially checked state can be enabled using the
    \l tristate property. This state indicates that the regular checked/unchecked
    state can not be determined; generally because of other states that affect
    the checkbox. This state is useful when several child nodes are selected
    in a treeview, for example.

    \code
    ColumnLayout {
        CheckBox {
            checked: true
            text: qsTr("First")
        }
        CheckBox {
            text: qsTr("Second")
        }
        CheckBox {
            checked: true
            text: qsTr("Third")
        }
    }
    \endcode

    Hierarchical checkbox groups can be managed with a non-exclusive
    \l ButtonGroup.

    \image qtquickcontrols2-checkbox-group.png

    The following example illustrates how the combined check state of
    children can be bound to the check state of the parent checkbox:

    \snippet qtquickcontrols2-checkbox-group.qml 1

    \sa {Customizing CheckBox}, ButtonGroup, {Button Controls}
*/

class QQuickCheckBoxPrivate : public QQuickAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickCheckBox)

public:
    void setNextCheckState(const QJSValue &callback);

    bool tristate = false;
    Qt::CheckState checkState = Qt::Unchecked;
    QJSValue nextCheckState;
};

void QQuickCheckBoxPrivate::setNextCheckState(const QJSValue &callback)
{
    Q_Q(QQuickCheckBox);
    nextCheckState = callback;
    emit q->nextCheckStateChanged();
}

QQuickCheckBox::QQuickCheckBox(QQuickItem *parent)
    : QQuickAbstractButton(*(new QQuickCheckBoxPrivate), parent)
{
    setCheckable(true);
}

/*!
    \qmlproperty bool QtQuick.Controls::CheckBox::tristate

    This property holds whether the checkbox is a tri-state checkbox.

    In the animation below, the first checkbox is tri-state:

    \image qtquickcontrols2-checkbox-tristate.gif

    The default is \c false, i.e., the checkbox has only two states.
*/
bool QQuickCheckBox::isTristate() const
{
    Q_D(const QQuickCheckBox);
    return d->tristate;
}

void QQuickCheckBox::setTristate(bool tristate)
{
    Q_D(QQuickCheckBox);
    if (d->tristate == tristate)
        return;

    d->tristate = tristate;
    emit tristateChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::CheckBox::checkState

    This property holds the check state of the checkbox.

    Available states:
    \value Qt.Unchecked The checkbox is unchecked.
    \value Qt.PartiallyChecked The checkbox is partially checked. This state is only used when \l tristate is enabled.
    \value Qt.Checked The checkbox is checked.

    \sa tristate, {AbstractButton::checked}{checked}
*/
Qt::CheckState QQuickCheckBox::checkState() const
{
    Q_D(const QQuickCheckBox);
    return d->checkState;
}

void QQuickCheckBox::setCheckState(Qt::CheckState state)
{
    Q_D(QQuickCheckBox);
    if (d->checkState == state)
        return;

    bool wasChecked = isChecked();
    d->checked = state == Qt::Checked;
    d->checkState = state;
    emit checkStateChanged();
    if (d->checked != wasChecked)
        emit checkedChanged();
}

QFont QQuickCheckBox::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::CheckBox);
}

QPalette QQuickCheckBox::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::CheckBox);
}

void QQuickCheckBox::buttonChange(ButtonChange change)
{
    if (change == ButtonCheckedChange)
        setCheckState(isChecked() ? Qt::Checked : Qt::Unchecked);
    else
        QQuickAbstractButton::buttonChange(change);
}

/*!
    \since QtQuick.Controls 2.4 (Qt 5.11)
    \qmlproperty function QtQuick.Controls::CheckBox::nextCheckState

    This property holds a callback function that is called to determine
    the next check state whenever the checkbox is interactively toggled
    by the user via touch, mouse, or keyboard.

    By default, a normal checkbox cycles between \c Qt.Unchecked and
    \c Qt.Checked states, and a tri-state checkbox cycles between
    \c Qt.Unchecked, \c Qt.PartiallyChecked, and \c Qt.Checked states.

    The \c nextCheckState callback function can override the default behavior.
    The following example implements a tri-state checkbox that can present
    a partially checked state depending on external conditions, but never
    cycles to the partially checked state when interactively toggled by
    the user.

    \code
    CheckBox {
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
void QQuickCheckBox::nextCheckState()
{
    Q_D(QQuickCheckBox);
    if (d->nextCheckState.isCallable())
        setCheckState(static_cast<Qt::CheckState>(d->nextCheckState.call().toInt()));
    else if (d->tristate)
        setCheckState(static_cast<Qt::CheckState>((d->checkState + 1) % 3));
    else
        QQuickAbstractButton::nextCheckState();
}

QT_END_NAMESPACE

#include "moc_qquickcheckbox_p.cpp"
