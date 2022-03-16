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

#include "widgetboxcategorylistview.h"

#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstractwidgetdatabase.h>

#include <QtXml/qdom.h>

#include <QtGui/qicon.h>
#include <QtGui/qvalidator.h>
#include <QtWidgets/qlistview.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qitemdelegate.h>
#include <QtCore/qsortfilterproxymodel.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qlist.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qregularexpression.h>

static const char *widgetElementC = "widget";
static const char *nameAttributeC = "name";
static const char *uiOpeningTagC = "<ui>";
static const char *uiClosingTagC = "</ui>";

QT_BEGIN_NAMESPACE

enum { FilterRole = Qt::UserRole + 11 };

static QString domToString(const QDomElement &elt)
{
    QString result;
    QTextStream stream(&result, QIODevice::WriteOnly);
    elt.save(stream, 2);
    stream.flush();
    return result;
}

static QDomDocument stringToDom(const QString &xml)
{
    QDomDocument result;
    result.setContent(xml);
    return result;
}

namespace qdesigner_internal {

// Entry of the model list

struct WidgetBoxCategoryEntry {
    WidgetBoxCategoryEntry() = default;
    explicit WidgetBoxCategoryEntry(const QDesignerWidgetBoxInterface::Widget &widget,
                                    const QString &filter,
                                    const QIcon &icon,
                                    bool editable);

    QDesignerWidgetBoxInterface::Widget widget;
    QString toolTip;
    QString whatsThis;
    QString filter;
    QIcon icon;
    bool editable{false};
};

WidgetBoxCategoryEntry::WidgetBoxCategoryEntry(const QDesignerWidgetBoxInterface::Widget &w,
                                               const QString &filterIn,
                                               const QIcon &i, bool e) :
    widget(w),
    filter(filterIn),
    icon(i),
    editable(e)
{
}

/* WidgetBoxCategoryModel, representing a list of category entries. Uses a
 * QAbstractListModel since the behaviour depends on the view mode of the list
 * view, it does not return text in the case of IconMode. */

class WidgetBoxCategoryModel : public QAbstractListModel {
public:
    explicit WidgetBoxCategoryModel(QDesignerFormEditorInterface *core, QObject *parent = 0);

    // QAbstractListModel
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags (const QModelIndex & index ) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // The model returns no text in icon mode, so, it also needs to know it
    QListView::ViewMode viewMode() const;
    void setViewMode(QListView::ViewMode vm);

    void addWidget(const QDesignerWidgetBoxInterface::Widget &widget, const QIcon &icon, bool editable);

    QDesignerWidgetBoxInterface::Widget widgetAt(const QModelIndex & index) const;
    QDesignerWidgetBoxInterface::Widget widgetAt(int row) const;

    int indexOfWidget(const QString &name);

    QDesignerWidgetBoxInterface::Category category() const;
    bool removeCustomWidgets();

private:
    typedef QList<WidgetBoxCategoryEntry> WidgetBoxCategoryEntrys;

    QDesignerFormEditorInterface *m_core;
    WidgetBoxCategoryEntrys m_items;
    QListView::ViewMode m_viewMode;
};

WidgetBoxCategoryModel::WidgetBoxCategoryModel(QDesignerFormEditorInterface *core, QObject *parent) :
    QAbstractListModel(parent),
    m_core(core),
    m_viewMode(QListView::ListMode)
{
}

QListView::ViewMode WidgetBoxCategoryModel::viewMode() const
{
    return m_viewMode;
}

void WidgetBoxCategoryModel::setViewMode(QListView::ViewMode vm)
{
    if (m_viewMode == vm)
        return;
    const bool empty = m_items.isEmpty();
    if (!empty)
        beginResetModel();
    m_viewMode = vm;
    if (!empty)
        endResetModel();
}

int WidgetBoxCategoryModel::indexOfWidget(const QString &name)
{
    const int count = m_items.size();
    for (int  i = 0; i < count; i++)
        if (m_items.at(i).widget.name() == name)
            return i;
    return -1;
}

QDesignerWidgetBoxInterface::Category WidgetBoxCategoryModel::category() const
{
    QDesignerWidgetBoxInterface::Category rc;
    const WidgetBoxCategoryEntrys::const_iterator cend = m_items.constEnd();
    for (WidgetBoxCategoryEntrys::const_iterator it = m_items.constBegin(); it != cend; ++it)
        rc.addWidget(it->widget);
    return rc;
}

bool WidgetBoxCategoryModel::removeCustomWidgets()
{
    // Typically, we are a whole category of custom widgets, so, remove all
    // and do reset.
    bool changed = false;
    for (WidgetBoxCategoryEntrys::iterator it = m_items.begin(); it != m_items.end(); )
        if (it->widget.type() == QDesignerWidgetBoxInterface::Widget::Custom) {
            if (!changed)
                beginResetModel();
            it = m_items.erase(it);
            changed = true;
        } else {
            ++it;
        }
    if (changed)
        endResetModel();
    return changed;
}

void WidgetBoxCategoryModel::addWidget(const QDesignerWidgetBoxInterface::Widget &widget, const QIcon &icon,bool editable)
{
    // build item. Filter on name + class name if it is different and not a layout.
    QString filter = widget.name();
    if (!filter.contains(QStringLiteral("Layout"))) {
        static const QRegularExpression classNameRegExp(QStringLiteral("<widget +class *= *\"([^\"]+)\""));
        Q_ASSERT(classNameRegExp.isValid());
        const QRegularExpressionMatch match = classNameRegExp.match(widget.domXml());
        if (match.hasMatch()) {
            const QString className = match.captured(1);
            if (!filter.contains(className))
                filter += className;
        }
    }
    WidgetBoxCategoryEntry item(widget, filter, icon, editable);
    const QDesignerWidgetDataBaseInterface *db = m_core->widgetDataBase();
    const int dbIndex = db->indexOfClassName(widget.name());
    if (dbIndex != -1) {
        const QDesignerWidgetDataBaseItemInterface *dbItem = db->item(dbIndex);
        const QString toolTip = dbItem->toolTip();
        if (!toolTip.isEmpty())
            item.toolTip = toolTip;
        const QString whatsThis = dbItem->whatsThis();
        if (!whatsThis.isEmpty())
            item.whatsThis = whatsThis;
    }
    // insert
    const int row = m_items.size();
    beginInsertRows(QModelIndex(), row, row);
    m_items.push_back(item);
    endInsertRows();
}

QVariant WidgetBoxCategoryModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >=  m_items.size())
        return QVariant();

    const WidgetBoxCategoryEntry &item = m_items.at(row);
    switch (role) {
    case Qt::DisplayRole:
        // No text in icon mode
        return QVariant(m_viewMode == QListView::ListMode ? item.widget.name() : QString());
    case Qt::DecorationRole:
        return QVariant(item.icon);
    case Qt::EditRole:
        return QVariant(item.widget.name());
    case Qt::ToolTipRole: {
        if (m_viewMode == QListView::ListMode)
            return QVariant(item.toolTip);
        // Icon mode tooltip should contain the  class name
        QString tt =  item.widget.name();
        if (!item.toolTip.isEmpty()) {
            tt += QLatin1Char('\n');
            tt += item.toolTip;
        }
        return QVariant(tt);

    }
    case Qt::WhatsThisRole:
        return QVariant(item.whatsThis);
    case FilterRole:
        return item.filter;
    }
    return QVariant();
}

bool WidgetBoxCategoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const int row = index.row();
    if (role != Qt::EditRole || row < 0 || row >=  m_items.size() || value.type() != QVariant::String)
        return false;
    // Set name and adapt Xml
    WidgetBoxCategoryEntry &item = m_items[row];
    const QString newName = value.toString();
    item.widget.setName(newName);

    const QDomDocument doc = stringToDom(WidgetBoxCategoryListView::widgetDomXml(item.widget));
    QDomElement widget_elt = doc.firstChildElement(QLatin1String(widgetElementC));
    if (!widget_elt.isNull()) {
        widget_elt.setAttribute(QLatin1String(nameAttributeC), newName);
        item.widget.setDomXml(domToString(widget_elt));
    }
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags WidgetBoxCategoryModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = Qt::ItemIsEnabled;
    const int row = index.row();
    if (row >= 0 && row <  m_items.size())
        if (m_items.at(row).editable) {
            rc |= Qt::ItemIsSelectable;
            // Can change name in list mode only
            if (m_viewMode == QListView::ListMode)
                rc |= Qt::ItemIsEditable;
        }
    return rc;
}

int WidgetBoxCategoryModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_items.size();
}

bool WidgetBoxCategoryModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (row < 0 || count < 1)
        return false;
    const int size = m_items.size();
    const int last =  row + count - 1;
    if (row >= size || last >= size)
        return false;
    beginRemoveRows(parent, row, last);
    for (int r = last; r >= row; r--)
         m_items.removeAt(r);
    endRemoveRows();
    return true;
}

QDesignerWidgetBoxInterface::Widget WidgetBoxCategoryModel::widgetAt(const QModelIndex & index) const
{
    return widgetAt(index.row());
}

QDesignerWidgetBoxInterface::Widget WidgetBoxCategoryModel::widgetAt(int row) const
{
    if (row < 0 || row >=  m_items.size())
        return QDesignerWidgetBoxInterface::Widget();
    return m_items.at(row).widget;
}

/* WidgetSubBoxItemDelegate, ensures a valid name using a regexp validator */

class WidgetBoxCategoryEntryDelegate : public QItemDelegate
{
public:
    explicit WidgetBoxCategoryEntryDelegate(QWidget *parent = 0) : QItemDelegate(parent) {}
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
};

QWidget *WidgetBoxCategoryEntryDelegate::createEditor(QWidget *parent,
                                                const QStyleOptionViewItem &option,
                                                const QModelIndex &index) const
{
    QWidget *result = QItemDelegate::createEditor(parent, option, index);
    if (QLineEdit *line_edit = qobject_cast<QLineEdit*>(result)) {
        static const QRegularExpression re(QStringLiteral("^[_a-zA-Z][_a-zA-Z0-9]*$"));
        Q_ASSERT(re.isValid());
        line_edit->setValidator(new QRegularExpressionValidator(re, line_edit));
    }
    return result;
}

// ----------------------  WidgetBoxCategoryListView

WidgetBoxCategoryListView::WidgetBoxCategoryListView(QDesignerFormEditorInterface *core, QWidget *parent) :
    QListView(parent),
    m_proxyModel(new QSortFilterProxyModel(this)),
    m_model(new WidgetBoxCategoryModel(core, this))
{
    setFocusPolicy(Qt::NoFocus);
    setFrameShape(QFrame::NoFrame);
    setIconSize(QSize(22, 22));
    setSpacing(1);
    setTextElideMode(Qt::ElideMiddle);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setResizeMode(QListView::Adjust);
    setUniformItemSizes(true);

    setItemDelegate(new WidgetBoxCategoryEntryDelegate(this));

    connect(this, &QListView::pressed, this,
            &WidgetBoxCategoryListView::slotPressed);
    setEditTriggers(QAbstractItemView::AnyKeyPressed);

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterRole(FilterRole);
    setModel(m_proxyModel);
    connect(m_model, &QAbstractItemModel::dataChanged,
            this, &WidgetBoxCategoryListView::scratchPadChanged);
}

void WidgetBoxCategoryListView::setViewMode(ViewMode vm)
{
    QListView::setViewMode(vm);
    m_model->setViewMode(vm);
}

void WidgetBoxCategoryListView::setCurrentItem(AccessMode am, int row)
{
    const QModelIndex index = am == FilteredAccess ?
        m_proxyModel->index(row, 0) :
        m_proxyModel->mapFromSource(m_model->index(row, 0));

    if (index.isValid())
        setCurrentIndex(index);
}

void WidgetBoxCategoryListView::slotPressed(const QModelIndex &index)
{
    const QDesignerWidgetBoxInterface::Widget wgt = m_model->widgetAt(m_proxyModel->mapToSource(index));
    if (wgt.isNull())
        return;
    emit pressed(wgt.name(), widgetDomXml(wgt), QCursor::pos());
}

void WidgetBoxCategoryListView::removeCurrentItem()
{
    const QModelIndex index = currentIndex();
    if (!index.isValid() || !m_proxyModel->removeRow(index.row()))
        return;

    // We check the unfiltered item count here, we don't want to get removed if the
    // filtered view is empty
    if (m_model->rowCount()) {
        emit itemRemoved();
    } else {
        emit lastItemRemoved();
    }
}

void WidgetBoxCategoryListView::editCurrentItem()
{
    const QModelIndex index = currentIndex();
    if (index.isValid())
        edit(index);
}

int WidgetBoxCategoryListView::count(AccessMode am) const
{
    return am == FilteredAccess ? m_proxyModel->rowCount() : m_model->rowCount();
}

int WidgetBoxCategoryListView::mapRowToSource(int filterRow) const
{
    const QModelIndex filterIndex = m_proxyModel->index(filterRow, 0);
    return m_proxyModel->mapToSource(filterIndex).row();
}

QDesignerWidgetBoxInterface::Widget WidgetBoxCategoryListView::widgetAt(AccessMode am, const QModelIndex & index) const
{
    const QModelIndex unfilteredIndex = am == FilteredAccess ? m_proxyModel->mapToSource(index) : index;
    return m_model->widgetAt(unfilteredIndex);
}

QDesignerWidgetBoxInterface::Widget WidgetBoxCategoryListView::widgetAt(AccessMode am, int row) const
{
    return m_model->widgetAt(am == UnfilteredAccess ? row : mapRowToSource(row));
}

void WidgetBoxCategoryListView::removeRow(AccessMode am, int row)
{
    m_model->removeRow(am == UnfilteredAccess ? row : mapRowToSource(row));
}

bool WidgetBoxCategoryListView::containsWidget(const QString &name)
{
    return m_model->indexOfWidget(name) != -1;
}

void WidgetBoxCategoryListView::addWidget(const QDesignerWidgetBoxInterface::Widget &widget, const QIcon &icon, bool editable)
{
    m_model->addWidget(widget, icon, editable);
}

QString WidgetBoxCategoryListView::widgetDomXml(const QDesignerWidgetBoxInterface::Widget &widget)
{
    QString domXml = widget.domXml();

    if (domXml.isEmpty()) {
        domXml = QLatin1String(uiOpeningTagC);
        domXml += QStringLiteral("<widget class=\"");
        domXml += widget.name();
        domXml += QStringLiteral("\"/>");
        domXml += QLatin1String(uiClosingTagC);
    }
    return domXml;
}

void WidgetBoxCategoryListView::filter(const QRegExp &re)
{
    m_proxyModel->setFilterRegExp(re);
}

QDesignerWidgetBoxInterface::Category WidgetBoxCategoryListView::category() const
{
    return m_model->category();
}

bool WidgetBoxCategoryListView::removeCustomWidgets()
{
    return m_model->removeCustomWidgets();
}
}  // namespace qdesigner_internal

QT_END_NAMESPACE
