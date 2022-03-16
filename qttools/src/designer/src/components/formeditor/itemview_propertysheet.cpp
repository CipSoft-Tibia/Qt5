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

#include "itemview_propertysheet.h"

#include <QtDesigner/abstractformeditor.h>

#include <QtWidgets/qabstractitemview.h>
#include <QtWidgets/qheaderview.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

struct Property {
    Property() = default;
    Property(QDesignerPropertySheetExtension *sheet, int id)
        : m_sheet(sheet), m_id(id) {}

    QDesignerPropertySheetExtension *m_sheet{nullptr};
    int m_id{-1};
};

typedef QMap<int, Property> FakePropertyMap;

struct ItemViewPropertySheetPrivate {
    ItemViewPropertySheetPrivate(QDesignerFormEditorInterface *core,
                                 QHeaderView *horizontalHeader,
                                 QHeaderView *verticalHeader);

    inline QStringList realPropertyNames();
    inline QString fakePropertyName(const QString &prefix, const QString &realName);

    // Maps index of fake property to index of real property in respective sheet
    FakePropertyMap m_propertyIdMap;

    // Maps name of fake property to name of real property
    QHash<QString, QString> m_propertyNameMap;

    QHash<QHeaderView *, QDesignerPropertySheetExtension *> m_propertySheet;
    QStringList m_realPropertyNames;
};

// Name of the fake group
static const char *headerGroup = "Header";

// Name of the real properties
static const char *visibleProperty = "visible";
static const char *cascadingSectionResizesProperty = "cascadingSectionResizes";
static const char *defaultSectionSizeProperty = "defaultSectionSize";
static const char *highlightSectionsProperty = "highlightSections";
static const char *minimumSectionSizeProperty = "minimumSectionSize";
static const char *showSortIndicatorProperty = "showSortIndicator";
static const char *stretchLastSectionProperty = "stretchLastSection";
} // namespace qdesigner_internal

using namespace qdesigner_internal;


/***************** ItemViewPropertySheetPrivate *********************/

ItemViewPropertySheetPrivate::ItemViewPropertySheetPrivate(QDesignerFormEditorInterface *core,
                                                           QHeaderView *horizontalHeader,
                                                           QHeaderView *verticalHeader)
{
    if (horizontalHeader)
        m_propertySheet.insert(horizontalHeader,
                               qt_extension<QDesignerPropertySheetExtension*>
                               (core->extensionManager(), horizontalHeader));
    if (verticalHeader)
        m_propertySheet.insert(verticalHeader,
                               qt_extension<QDesignerPropertySheetExtension*>
                               (core->extensionManager(), verticalHeader));
}

QStringList ItemViewPropertySheetPrivate::realPropertyNames()
{
    if (m_realPropertyNames.isEmpty())
        m_realPropertyNames
            << QLatin1String(visibleProperty)
            << QLatin1String(cascadingSectionResizesProperty)
            << QLatin1String(defaultSectionSizeProperty)
            << QLatin1String(highlightSectionsProperty)
            << QLatin1String(minimumSectionSizeProperty)
            << QLatin1String(showSortIndicatorProperty)
            << QLatin1String(stretchLastSectionProperty);
    return m_realPropertyNames;
}

QString ItemViewPropertySheetPrivate::fakePropertyName(const QString &prefix,
                                                       const QString &realName)
{
    // prefix = "header", realPropertyName = "isVisible" returns "headerIsVisible"
    QString fakeName = prefix + realName.at(0).toUpper() + realName.mid(1);
    m_propertyNameMap.insert(fakeName, realName);
    return fakeName;
}

/***************** ItemViewPropertySheet *********************/

/*!
  \class qdesigner_internal::ItemViewPropertySheet

  \brief
    Adds header fake properties to QTreeView and QTableView objects

    QHeaderView objects are currently not shown in the object inspector.
    This class adds some fake properties to the property sheet
    of QTreeView and QTableView objects that nevertheless allow the manipulation
    of the headers attached to the item view object.

    Currently the defaultAlignment property is not shown because the property sheet
    would only show integers, instead of the Qt::Alignment enumeration.

    The fake properties here need special handling in QDesignerResource, uiloader and uic.
  */

ItemViewPropertySheet::ItemViewPropertySheet(QTreeView *treeViewObject, QObject *parent)
        : QDesignerPropertySheet(treeViewObject, parent),
        d(new ItemViewPropertySheetPrivate(core(), treeViewObject->header(), 0))
{
    initHeaderProperties(treeViewObject->header(), QStringLiteral("header"));
}

ItemViewPropertySheet::ItemViewPropertySheet(QTableView *tableViewObject, QObject *parent)
        : QDesignerPropertySheet(tableViewObject, parent),
        d(new ItemViewPropertySheetPrivate(core(),
                                           tableViewObject->horizontalHeader(),
                                           tableViewObject->verticalHeader()))
{
    initHeaderProperties(tableViewObject->horizontalHeader(), QStringLiteral("horizontalHeader"));
    initHeaderProperties(tableViewObject->verticalHeader(), QStringLiteral("verticalHeader"));
}

ItemViewPropertySheet::~ItemViewPropertySheet()
{
    delete d;
}

void ItemViewPropertySheet::initHeaderProperties(QHeaderView *hv, const QString &prefix)
{
    QDesignerPropertySheetExtension *headerSheet = d->m_propertySheet.value(hv);
    Q_ASSERT(headerSheet);
    const QString headerGroupS = QLatin1String(headerGroup);
    const QStringList &realPropertyNames = d->realPropertyNames();
    for (const QString &realPropertyName : realPropertyNames) {
        const int headerIndex = headerSheet->indexOf(realPropertyName);
        Q_ASSERT(headerIndex != -1);
        const QVariant defaultValue = realPropertyName == QLatin1String(visibleProperty) ?
                                      QVariant(true) : headerSheet->property(headerIndex);
        const QString fakePropertyName = d->fakePropertyName(prefix, realPropertyName);
        const int fakeIndex = createFakeProperty(fakePropertyName, defaultValue);
        d->m_propertyIdMap.insert(fakeIndex, Property(headerSheet, headerIndex));
        setAttribute(fakeIndex, true);
        setPropertyGroup(fakeIndex, headerGroupS);
    }
}

/*!
  Returns the mapping of fake property names to real property names
  */
QHash<QString,QString> ItemViewPropertySheet::propertyNameMap() const
{
    return d->m_propertyNameMap;
}

QVariant ItemViewPropertySheet::property(int index) const
{
    const FakePropertyMap::const_iterator it = d->m_propertyIdMap.constFind(index);
    if (it != d->m_propertyIdMap.constEnd())
        return it.value().m_sheet->property(it.value().m_id);
    return QDesignerPropertySheet::property(index);
}

void ItemViewPropertySheet::setProperty(int index, const QVariant &value)
{
    const FakePropertyMap::iterator it = d->m_propertyIdMap.find(index);
    if (it != d->m_propertyIdMap.end()) {
        it.value().m_sheet->setProperty(it.value().m_id, value);
    } else {
        QDesignerPropertySheet::setProperty(index, value);
    }
}

void ItemViewPropertySheet::setChanged(int index, bool changed)
{
    const FakePropertyMap::iterator it = d->m_propertyIdMap.find(index);
    if (it != d->m_propertyIdMap.end()) {
        it.value().m_sheet->setChanged(it.value().m_id, changed);
    } else {
        QDesignerPropertySheet::setChanged(index, changed);
    }
}

bool ItemViewPropertySheet::isChanged(int index) const
{
    const FakePropertyMap::const_iterator it = d->m_propertyIdMap.constFind(index);
    if (it != d->m_propertyIdMap.constEnd())
        return it.value().m_sheet->isChanged(it.value().m_id);
    return QDesignerPropertySheet::isChanged(index);
}

bool ItemViewPropertySheet::hasReset(int index) const
{
    const FakePropertyMap::const_iterator it = d->m_propertyIdMap.constFind(index);
    if (it != d->m_propertyIdMap.constEnd())
        return it.value().m_sheet->hasReset(it.value().m_id);
    return QDesignerPropertySheet::hasReset(index);
}

bool ItemViewPropertySheet::reset(int index)
{
    const FakePropertyMap::iterator it = d->m_propertyIdMap.find(index);
    if (it != d->m_propertyIdMap.end()) {
       QDesignerPropertySheetExtension *headerSheet = it.value().m_sheet;
       const int headerIndex = it.value().m_id;
       const bool resetRC = headerSheet->reset(headerIndex);
       // Resetting for "visible" might fail and the stored default
       // of the Widget database is "false" due to the widget not being
       // visible at the time it was determined. Reset to "true" manually.
       if (!resetRC && headerSheet->propertyName(headerIndex) == QLatin1String(visibleProperty)) {
           headerSheet->setProperty(headerIndex, QVariant(true));
           headerSheet->setChanged(headerIndex, false);
           return true;
       }
       return resetRC;
    }
    return QDesignerPropertySheet::reset(index);
}

QT_END_NAMESPACE
