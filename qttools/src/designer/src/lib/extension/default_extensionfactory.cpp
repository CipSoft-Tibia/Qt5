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

#include "default_extensionfactory.h"
#include "qextensionmanager.h"
#include <qpointer.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class QExtensionFactory

    \brief The QExtensionFactory class allows you to create a factory
    that is able to make instances of custom extensions in Qt
    Designer.

    \inmodule QtDesigner

    In \QD the extensions are not created until they are required. For
    that reason, when implementing a custom extension, you must also
    create a QExtensionFactory, i.e. a class that is able to make an
    instance of your extension, and register it using \QD's \l
    {QExtensionManager}{extension manager}.

    The QExtensionManager class provides extension management
    facilities for Qt Designer. When an extension is required, Qt
    Designer's \l {QExtensionManager}{extension manager} will run
    through all its registered factories calling
    QExtensionFactory::createExtension() for each until the first one
    that is able to create a requested extension for the selected
    object, is found. This factory will then make an instance of the
    extension.

    There are four available types of extensions in Qt Designer:
    QDesignerContainerExtension , QDesignerMemberSheetExtension,
    QDesignerPropertySheetExtension and QDesignerTaskMenuExtension. Qt
    Designer's behavior is the same whether the requested extension is
    associated with a multi page container, a member sheet, a property
    sheet or a task menu.

    You can either create a new QExtensionFactory and reimplement the
    QExtensionFactory::createExtension() function. For example:

    \snippet lib/tools_designer_src_lib_extension_default_extensionfactory.cpp 0

    Or you can use an existing factory, expanding the
    QExtensionFactory::createExtension() function to make the factory
    able to create your extension as well. For example:

    \snippet lib/tools_designer_src_lib_extension_default_extensionfactory.cpp 1

    For a complete example using the QExtensionFactory class, see the
    \l {taskmenuextension}{Task Menu Extension example}. The
    example shows how to create a custom widget plugin for Qt
    Designer, and how to to use the QDesignerTaskMenuExtension class
    to add custom items to Qt Designer's task menu.

    \sa QExtensionManager, QAbstractExtensionFactory
*/

/*!
    Constructs an extension factory with the given \a parent.
*/
QExtensionFactory::QExtensionFactory(QExtensionManager *parent)
    : QObject(parent)
{
}

/*!
    Returns the extension specified by \a iid for the given \a object.

    \sa createExtension()
*/

QObject *QExtensionFactory::extension(QObject *object, const QString &iid) const
{
    if (!object)
        return 0;
    const IdObjectKey key = qMakePair(iid, object);

    ExtensionMap::iterator it = m_extensions.find(key);
    if (it == m_extensions.end()) {
        if (QObject *ext = createExtension(object, iid, const_cast<QExtensionFactory*>(this))) {
            connect(ext, &QObject::destroyed, this, &QExtensionFactory::objectDestroyed);
            it = m_extensions.insert(key, ext);
        }
    }

    if (!m_extended.contains(object)) {
        connect(object, &QObject::destroyed, this, &QExtensionFactory::objectDestroyed);
        m_extended.insert(object, true);
    }

    if (it == m_extensions.end())
        return 0;

    return it.value();
}

void QExtensionFactory::objectDestroyed(QObject *object)
{
    for (auto it = m_extensions.begin(); it != m_extensions.end(); ) {
        if (it.key().second == object || object == it.value())
            it = m_extensions.erase(it);
        else
            ++it;
    }

    m_extended.remove(object);
}

/*!
    Creates an extension specified by \a iid for the given \a object.
    The extension object is created as a child of the specified \a
    parent.

    \sa extension()
*/
QObject *QExtensionFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    Q_UNUSED(object);
    Q_UNUSED(iid);
    Q_UNUSED(parent);

    return 0;
}

/*!
    Returns the extension manager for the extension factory.
*/
QExtensionManager *QExtensionFactory::extensionManager() const
{
    return static_cast<QExtensionManager *>(parent());
}

QT_END_NAMESPACE
