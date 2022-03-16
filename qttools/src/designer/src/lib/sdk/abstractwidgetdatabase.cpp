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

#include "abstractwidgetdatabase.h"
#include <QtCore/qdebug.h>
#include <qalgorithms.h>

QT_BEGIN_NAMESPACE

namespace {
    enum { debugWidgetDataBase =  0 };
}

/*!
    \class QDesignerWidgetDataBaseInterface
    \brief The QDesignerWidgetDataBaseInterface class provides an interface that is used to
    access and modify Qt Designer's widget database.
    \inmodule QtDesigner
    \internal
*/

/*!
    Constructs an interface to the widget database with the given \a parent.
*/
QDesignerWidgetDataBaseInterface::QDesignerWidgetDataBaseInterface(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the interface to the widget database.
*/
QDesignerWidgetDataBaseInterface::~QDesignerWidgetDataBaseInterface()
{
    qDeleteAll(m_items);
}

/*!

*/
int QDesignerWidgetDataBaseInterface::count() const
{
    return m_items.count();
}

/*!
*/
QDesignerWidgetDataBaseItemInterface *QDesignerWidgetDataBaseInterface::item(int index) const
{
    return index != -1 ? m_items.at(index) : 0;
}

/*!
*/
int QDesignerWidgetDataBaseInterface::indexOf(QDesignerWidgetDataBaseItemInterface *item) const
{
    return m_items.indexOf(item);
}

/*!
*/
void QDesignerWidgetDataBaseInterface::insert(int index, QDesignerWidgetDataBaseItemInterface *item)
{
    if (debugWidgetDataBase)
        qDebug() << "insert at " << index << ' ' << item->name() << " derived from " << item->extends();

    m_items.insert(index, item);
}

/*!
*/
void QDesignerWidgetDataBaseInterface::append(QDesignerWidgetDataBaseItemInterface *item)
{
    if (debugWidgetDataBase)
        qDebug() << "append " << item->name() << " derived from " << item->extends();
    m_items.append(item);
}

/*!
*/
QDesignerFormEditorInterface *QDesignerWidgetDataBaseInterface::core() const
{
    return 0;
}

/*!
*/
int QDesignerWidgetDataBaseInterface::indexOfClassName(const QString &name, bool) const
{
    const int itemCount = count();
    for (int i=0; i<itemCount; ++i) {
        const QDesignerWidgetDataBaseItemInterface *entry = item(i);
        if (entry->name() == name)
            return i;
    }

    return -1;
}

/*!
*/
int QDesignerWidgetDataBaseInterface::indexOfObject(QObject *object, bool) const
{
    if (!object)
        return -1;

    const QString className = QString::fromUtf8(object->metaObject()->className());
    return indexOfClassName(className);
}

/*!
*/
bool QDesignerWidgetDataBaseInterface::isContainer(QObject *object, bool resolveName) const
{
    if (const QDesignerWidgetDataBaseItemInterface *i = item(indexOfObject(object, resolveName)))
        return i->isContainer();
    return false;
}

/*!
*/
bool QDesignerWidgetDataBaseInterface::isCustom(QObject *object, bool resolveName) const
{
    if (const QDesignerWidgetDataBaseItemInterface *i = item(indexOfObject(object, resolveName)))
        return i->isCustom();
    return false;
}

/*!
    \fn void QDesignerWidgetDataBaseInterface::changed()

    This signal is emitted ...
*/


// Doc: No implementation - an abstract class

/*!
    \class QDesignerWidgetDataBaseItemInterface
    \brief The QDesignerWidgetDataBaseItemInterface class provides an interface that is used to
    access individual items in Qt Designer's widget database.
    \inmodule QtDesigner
    \internal

    This class enables individual items in the widget database to be accessed and modified.
    Changes to the widget database itself are made through the QDesignerWidgetDataBaseInterface
    class.
*/

/*!
    \fn virtual QDesignerWidgetDataBaseItemInterface::~QDesignerWidgetDataBaseItemInterface()

    Destroys the interface.
*/

/*!
    \fn virtual QString QDesignerWidgetDataBaseItemInterface::name() const = 0

    Returns the name of the widget.
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setName(const QString &name) = 0
*/

/*!
    \fn virtual QString QDesignerWidgetDataBaseItemInterface::group() const = 0

    Returns the name of the group that the widget belongs to.
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setGroup(const QString &group) = 0
*/

/*!
    \fn virtual QString QDesignerWidgetDataBaseItemInterface::toolTip() const = 0

    Returns the tool tip to be used by the widget.
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setToolTip(const QString &toolTip) = 0
*/

/*!
    \fn virtual QString QDesignerWidgetDataBaseItemInterface::whatsThis() const = 0

    Returns the "What's This?" help for the widget.
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setWhatsThis(const QString &whatsThis) = 0
*/

/*!
    \fn virtual QString QDesignerWidgetDataBaseItemInterface::includeFile() const = 0

    Returns the name of the include file that the widget needs when being built from source.
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setIncludeFile(const QString &includeFile) = 0
*/

/*!
    \fn virtual QIcon QDesignerWidgetDataBaseItemInterface::icon() const = 0

    Returns the icon used to represent the item.
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setIcon(const QIcon &icon) = 0
*/

/*!
    \fn virtual bool QDesignerWidgetDataBaseItemInterface::isCompat() const = 0

    Returns true if this type of widget is provided for compatibility purposes (e.g. Qt3Support
    widgets); otherwise returns false.

    \sa setCompat()
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setCompat(bool compat) = 0

    If \a compat is true, the widget is handled as a compatibility widget; otherwise it is
    handled normally by \QD.

    \sa isCompat()
*/

/*!
    \fn virtual bool QDesignerWidgetDataBaseItemInterface::isContainer() const = 0

    Returns true if this widget is intended to be used to hold other widgets; otherwise returns
    false.

    \sa setContainer()
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setContainer(bool container) = 0

    If \a container is true, the widget can be used to hold other widgets in \QD; otherwise
    \QD will refuse to let the user place other widgets inside it.

    \sa isContainer()
*/

/*!
    \fn virtual bool QDesignerWidgetDataBaseItemInterface::isCustom() const = 0

    Returns true if the widget is a custom widget; otherwise return false if it is a standard
    Qt widget.

    \sa setCustom()
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setCustom(bool custom) = 0

    If \a custom is true, the widget is handled specially by \QD; otherwise it is handled as
    a standard Qt widget.

    \sa isCustom()
*/

/*!
    \fn virtual QString QDesignerWidgetDataBaseItemInterface::pluginPath() const = 0

    Returns the path to use for the widget plugin.
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setPluginPath(const QString &path) = 0
*/

/*!
    \fn virtual bool QDesignerWidgetDataBaseItemInterface::isPromoted() const = 0

    Returns true if the widget is promoted; otherwise returns false.

    Promoted widgets are those that represent custom widgets, but which are represented in
    \QD by either standard Qt widgets or readily-available custom widgets.

    \sa setPromoted()
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setPromoted(bool promoted) = 0

    If \a promoted is true, the widget is handled as a promoted widget by \QD and will use
    a placeholder widget to represent it; otherwise it is handled as a standard widget.

    \sa isPromoted()
*/

/*!
    \fn virtual QString QDesignerWidgetDataBaseItemInterface::extends() const = 0

    Returns the name of the widget that the item extends.
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setExtends(const QString &s) = 0
*/

/*!
    \fn virtual void QDesignerWidgetDataBaseItemInterface::setDefaultPropertyValues(const QList<QVariant> &list) = 0

    Sets the default property values for the widget to the given \a list.
*/

/*!
    \fn virtual QList<QVariant> QDesignerWidgetDataBaseItemInterface::defaultPropertyValues() const = 0

    Returns a list of default values to be used as properties for the item.
*/

QT_END_NAMESPACE
