/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "abstractpropertyeditor.h"

QT_BEGIN_NAMESPACE

/*!
    \class QDesignerPropertyEditorInterface

    \brief The QDesignerPropertyEditorInterface class allows you to
    query and manipulate the current state of Qt Designer's property
    editor.

    \inmodule QtDesigner

    QDesignerPropertyEditorInterface contains a collection of
    functions that is typically used to query the property editor for
    its current state, and several slots manipulating it's state. The
    interface also provide a signal, propertyChanged(), which is
    emitted whenever a property changes in the property editor. The
    signal's arguments are the property that changed and its new
    value.

    For example, when implementing a custom widget plugin, you can
    connect the signal to a custom slot:

    \snippet lib/tools_designer_src_lib_sdk_abstractpropertyeditor.cpp 0

    Then the custom slot can check if the new value is within the
    range we want when a specified property, belonging to a particular
    widget, changes:

    \snippet lib/tools_designer_src_lib_sdk_abstractpropertyeditor.cpp 1

    The QDesignerPropertyEditorInterface class is not intended to be
    instantiated directly. You can retrieve an interface to \QD's
    property editor using the
    QDesignerFormEditorInterface::propertyEditor() function. A pointer
    to \QD's current QDesignerFormEditorInterface object (\c
    formEditor in the examples above) is provided by the
    QDesignerCustomWidgetInterface::initialize() function's
    parameter. When implementing a custom widget plugin, you must
    subclass the QDesignerCustomWidgetInterface to expose your plugin
    to \QD.

    The functions accessing the property editor are the core()
    function that you can use to retrieve an interface to the form
    editor, the currentPropertyName() function that returns the name
    of the currently selected property in the property editor, the
    object() function that returns the currently selected object in
    \QD's workspace, and the isReadOnly() function that returns true
    if the property editor is write proteced (otherwise false).

    The slots manipulating the property editor's state are the
    setObject() slot that you can use to change the currently selected
    object in \QD's workspace, the setPropertyValue() slot that
    changes the value of a given property and the setReadOnly() slot
    that control the write protection of the property editor.

    \sa QDesignerFormEditorInterface
*/

/*!
    Constructs a property editor interface with the given \a parent and
    the specified window \a flags.
*/
QDesignerPropertyEditorInterface::QDesignerPropertyEditorInterface(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
{
}

/*!
    Destroys the property editor interface.
*/
QDesignerPropertyEditorInterface::~QDesignerPropertyEditorInterface()
{
}

/*!
    Returns a pointer to \QD's current QDesignerFormEditorInterface
    object.
*/
QDesignerFormEditorInterface *QDesignerPropertyEditorInterface::core() const
{
    return 0;
}

/*!
    \fn bool QDesignerPropertyEditorInterface::isReadOnly() const

    Returns true if the property editor is write protected; otherwise
    false.

    \sa setReadOnly()
*/

/*!
    \fn QObject *QDesignerPropertyEditorInterface::object() const

    Returns the currently selected object in \QD's workspace.

    \sa setObject()
*/

/*!
    \fn QString QDesignerPropertyEditorInterface::currentPropertyName() const

    Returns the name of the currently selected property in the
    property editor.

    \sa setPropertyValue()
*/

/*!
    \fn void QDesignerPropertyEditorInterface::propertyChanged(const QString &name, const QVariant &value)

    This signal is emitted whenever a property changes in the property
    editor. The property that changed and its new value are specified
    by \a name and \a value respectively.

    \sa setPropertyValue()
*/

/*!
    \fn void QDesignerPropertyEditorInterface::setObject(QObject *object)

    Changes the currently selected object in \QD's workspace, to \a
    object.

    \sa object()
*/

/*!
    \fn void QDesignerPropertyEditorInterface::setPropertyValue(const QString &name, const QVariant &value, bool changed = true)

    Sets the value of the property specified by \a name to \a
    value.

    In addition, the property is marked as \a changed in the property
    editor, i.e. its value is different from the default value.

    \sa currentPropertyName(), propertyChanged()
*/

/*!
    \fn void QDesignerPropertyEditorInterface::setReadOnly(bool readOnly)

    If \a readOnly is true, the property editor is made write
    protected; otherwise the write protection is removed.

    \sa isReadOnly()
*/

QT_END_NAMESPACE
