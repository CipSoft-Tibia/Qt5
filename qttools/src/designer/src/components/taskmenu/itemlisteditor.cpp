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

#include "itemlisteditor.h"
#include <abstractformbuilder.h>
#include <iconloader_p.h>
#include <formwindowbase_p.h>
#include <designerpropertymanager.h>

#include <QtDesigner/abstractformwindow.h>

#include <qttreepropertybrowser.h>

#include <QtWidgets/qsplitter.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class ItemPropertyBrowser : public QtTreePropertyBrowser
{
public:
    ItemPropertyBrowser()
    {
        setResizeMode(Interactive);
        //: Sample string to determinate the width for the first column of the list item property browser
        const QString widthSampleString = QCoreApplication::translate("ItemPropertyBrowser", "XX Icon Selected off");
        m_width = fontMetrics().width(widthSampleString);
        setSplitterPosition(m_width);
        m_width += fontMetrics().width(QStringLiteral("/this/is/some/random/path"));
    }

    QSize sizeHint() const override
    {
        return QSize(m_width, 1);
    }

private:
    int m_width;
};

////////////////// Item editor ///////////////
AbstractItemEditor::AbstractItemEditor(QDesignerFormWindowInterface *form, QWidget *parent)
    : QWidget(parent),
      m_iconCache(qobject_cast<FormWindowBase *>(form)->iconCache()),
      m_updatingBrowser(false)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_propertyManager = new DesignerPropertyManager(form->core(), this);
    m_editorFactory = new DesignerEditorFactory(form->core(), this);
    m_editorFactory->setSpacing(0);
    m_propertyBrowser = new ItemPropertyBrowser;
    m_propertyBrowser->setFactoryForManager(static_cast<QtVariantPropertyManager *>(m_propertyManager),
                                            m_editorFactory);

    connect(m_editorFactory, &DesignerEditorFactory::resetProperty,
            this, &AbstractItemEditor::resetProperty);
    connect(m_propertyManager, &DesignerPropertyManager::valueChanged,
            this, &AbstractItemEditor::propertyChanged);
    connect(iconCache(), &DesignerIconCache::reloaded, this, &AbstractItemEditor::cacheReloaded);
}

AbstractItemEditor::~AbstractItemEditor()
{
    m_propertyBrowser->unsetFactoryForManager(m_propertyManager);
}

static const char * const itemFlagNames[] = {
    QT_TRANSLATE_NOOP("AbstractItemEditor", "Selectable"),
    QT_TRANSLATE_NOOP("AbstractItemEditor", "Editable"),
    QT_TRANSLATE_NOOP("AbstractItemEditor", "DragEnabled"),
    QT_TRANSLATE_NOOP("AbstractItemEditor", "DropEnabled"),
    QT_TRANSLATE_NOOP("AbstractItemEditor", "UserCheckable"),
    QT_TRANSLATE_NOOP("AbstractItemEditor", "Enabled"),
    QT_TRANSLATE_NOOP("AbstractItemEditor", "Tristate"),
    0
};

static const char * const checkStateNames[] = {
    QT_TRANSLATE_NOOP("AbstractItemEditor", "Unchecked"),
    QT_TRANSLATE_NOOP("AbstractItemEditor", "PartiallyChecked"),
    QT_TRANSLATE_NOOP("AbstractItemEditor", "Checked"),
    0
};

static QStringList c2qStringList(const char * const in[])
{
    QStringList out;
    for (int i = 0; in[i]; i++)
        out << AbstractItemEditor::tr(in[i]);
    return out;
}

void AbstractItemEditor::setupProperties(PropertyDefinition *propList)
{
    for (int i = 0; propList[i].name; i++) {
        int type = propList[i].typeFunc ? propList[i].typeFunc() : propList[i].type;
        int role = propList[i].role;
        QtVariantProperty *prop = m_propertyManager->addProperty(type, QLatin1String(propList[i].name));
        Q_ASSERT(prop);
        if (role == Qt::ToolTipPropertyRole || role == Qt::WhatsThisPropertyRole)
            prop->setAttribute(QStringLiteral("validationMode"), ValidationRichText);
        else if (role == Qt::DisplayPropertyRole)
            prop->setAttribute(QStringLiteral("validationMode"), ValidationMultiLine);
        else if (role == Qt::StatusTipPropertyRole)
            prop->setAttribute(QStringLiteral("validationMode"), ValidationSingleLine);
        else if (role == ItemFlagsShadowRole)
            prop->setAttribute(QStringLiteral("flagNames"), c2qStringList(itemFlagNames));
        else if (role == Qt::CheckStateRole)
            prop->setAttribute(QStringLiteral("enumNames"), c2qStringList(checkStateNames));
        prop->setAttribute(QStringLiteral("resettable"), true);
        m_properties.append(prop);
        m_rootProperties.append(prop);
        m_propertyToRole.insert(prop, role);
    }
}

void AbstractItemEditor::setupObject(QWidget *object)
{
    m_propertyManager->setObject(object);
    QDesignerFormWindowInterface *formWindow = QDesignerFormWindowInterface::findFormWindow(object);
    FormWindowBase *fwb = qobject_cast<FormWindowBase *>(formWindow);
    m_editorFactory->setFormWindowBase(fwb);
}

void AbstractItemEditor::setupEditor(QWidget *object, PropertyDefinition *propList)
{
    setupProperties(propList);
    setupObject(object);
}

void AbstractItemEditor::propertyChanged(QtProperty *property)
{
    if (m_updatingBrowser)
        return;


    BoolBlocker block(m_updatingBrowser);
    QtVariantProperty *prop = m_propertyManager->variantProperty(property);
    int role;
    if ((role = m_propertyToRole.value(prop, -1)) == -1)
        // Subproperty
        return;

    if ((role == ItemFlagsShadowRole && prop->value().toInt() == defaultItemFlags())
        || (role == Qt::DecorationPropertyRole && !qvariant_cast<PropertySheetIconValue>(prop->value()).mask())
        || (role == Qt::FontRole && !qvariant_cast<QFont>(prop->value()).resolve())) {
        prop->setModified(false);
        setItemData(role, QVariant());
    } else {
        prop->setModified(true);
        setItemData(role, prop->value());
    }

    switch (role) {
    case Qt::DecorationPropertyRole:
        setItemData(Qt::DecorationRole, QVariant::fromValue(iconCache()->icon(qvariant_cast<PropertySheetIconValue>(prop->value()))));
        break;
    case Qt::DisplayPropertyRole:
        setItemData(Qt::EditRole, QVariant::fromValue(qvariant_cast<PropertySheetStringValue>(prop->value()).value()));
        break;
    case Qt::ToolTipPropertyRole:
        setItemData(Qt::ToolTipRole, QVariant::fromValue(qvariant_cast<PropertySheetStringValue>(prop->value()).value()));
        break;
    case Qt::StatusTipPropertyRole:
        setItemData(Qt::StatusTipRole, QVariant::fromValue(qvariant_cast<PropertySheetStringValue>(prop->value()).value()));
        break;
    case Qt::WhatsThisPropertyRole:
        setItemData(Qt::WhatsThisRole, QVariant::fromValue(qvariant_cast<PropertySheetStringValue>(prop->value()).value()));
        break;
    default:
        break;
    }

    prop->setValue(getItemData(role));
}

void AbstractItemEditor::resetProperty(QtProperty *property)
{
    if (m_propertyManager->resetFontSubProperty(property))
        return;

    if (m_propertyManager->resetIconSubProperty(property))
        return;

    BoolBlocker block(m_updatingBrowser);

    QtVariantProperty *prop = m_propertyManager->variantProperty(property);
    int role = m_propertyToRole.value(prop);
    if (role == ItemFlagsShadowRole)
        prop->setValue(QVariant::fromValue(defaultItemFlags()));
    else
        prop->setValue(QVariant(prop->valueType(), nullptr));
    prop->setModified(false);

    setItemData(role, QVariant());
    if (role == Qt::DecorationPropertyRole)
        setItemData(Qt::DecorationRole, QVariant::fromValue(QIcon()));
    if (role == Qt::DisplayPropertyRole)
        setItemData(Qt::EditRole, QVariant::fromValue(QString()));
    if (role == Qt::ToolTipPropertyRole)
        setItemData(Qt::ToolTipRole, QVariant::fromValue(QString()));
    if (role == Qt::StatusTipPropertyRole)
        setItemData(Qt::StatusTipRole, QVariant::fromValue(QString()));
    if (role == Qt::WhatsThisPropertyRole)
        setItemData(Qt::WhatsThisRole, QVariant::fromValue(QString()));
}

void AbstractItemEditor::cacheReloaded()
{
    BoolBlocker block(m_updatingBrowser);
    m_propertyManager->reloadResourceProperties();
}

void AbstractItemEditor::updateBrowser()
{
    BoolBlocker block(m_updatingBrowser);
    for (QtVariantProperty *prop : qAsConst(m_properties)) {
        int role = m_propertyToRole.value(prop);
        QVariant val = getItemData(role);
        if (!val.isValid()) {
            if (role == ItemFlagsShadowRole)
                val = QVariant::fromValue(defaultItemFlags());
            else
                val = QVariant(int(prop->value().userType()), nullptr);
            prop->setModified(false);
        } else {
            prop->setModified(true);
        }
        prop->setValue(val);
    }

    if (m_propertyBrowser->topLevelItems().isEmpty()) {
        for (QtVariantProperty *prop : qAsConst(m_rootProperties))
            m_propertyBrowser->addProperty(prop);
    }
}

void AbstractItemEditor::injectPropertyBrowser(QWidget *parent, QWidget *widget)
{
    // It is impossible to design a splitter with just one widget, so we do it by hand.
    m_propertySplitter = new QSplitter;
    m_propertySplitter->addWidget(widget);
    m_propertySplitter->addWidget(m_propertyBrowser);
    m_propertySplitter->setStretchFactor(0, 1);
    m_propertySplitter->setStretchFactor(1, 0);
    parent->layout()->addWidget(m_propertySplitter);
}

////////////////// List editor ///////////////
ItemListEditor::ItemListEditor(QDesignerFormWindowInterface *form, QWidget *parent)
    : AbstractItemEditor(form, parent),
      m_updating(false)
{
    ui.setupUi(this);

    injectPropertyBrowser(this, ui.widget);
    connect(ui.showPropertiesButton, &QAbstractButton::clicked,
            this, &ItemListEditor::togglePropertyBrowser);
    setPropertyBrowserVisible(false);

    QIcon upIcon = createIconSet(QString::fromUtf8("up.png"));
    QIcon downIcon = createIconSet(QString::fromUtf8("down.png"));
    QIcon minusIcon = createIconSet(QString::fromUtf8("minus.png"));
    QIcon plusIcon = createIconSet(QString::fromUtf8("plus.png"));
    ui.moveListItemUpButton->setIcon(upIcon);
    ui.moveListItemDownButton->setIcon(downIcon);
    ui.newListItemButton->setIcon(plusIcon);
    ui.deleteListItemButton->setIcon(minusIcon);

    connect(iconCache(), &DesignerIconCache::reloaded, this, &AbstractItemEditor::cacheReloaded);
}

void ItemListEditor::setupEditor(QWidget *object, PropertyDefinition *propList)
{
    AbstractItemEditor::setupEditor(object, propList);

    if (ui.listWidget->count() > 0)
        ui.listWidget->setCurrentRow(0);
    else
        updateEditor();
}

void ItemListEditor::setCurrentIndex(int idx)
{
    m_updating = true;
    ui.listWidget->setCurrentRow(idx);
    m_updating = false;
}

void ItemListEditor::on_newListItemButton_clicked()
{
    int row = ui.listWidget->currentRow() + 1;

    QListWidgetItem *item = new QListWidgetItem(m_newItemText);
    item->setData(Qt::DisplayPropertyRole, QVariant::fromValue(PropertySheetStringValue(m_newItemText)));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    if (row < ui.listWidget->count())
        ui.listWidget->insertItem(row, item);
    else
        ui.listWidget->addItem(item);
    emit itemInserted(row);

    ui.listWidget->setCurrentItem(item);
    ui.listWidget->editItem(item);
}

void ItemListEditor::on_deleteListItemButton_clicked()
{
    int row = ui.listWidget->currentRow();

    if (row != -1) {
        delete ui.listWidget->takeItem(row);
        emit itemDeleted(row);
    }

    if (row == ui.listWidget->count())
        row--;
    if (row < 0)
        updateEditor();
    else
        ui.listWidget->setCurrentRow(row);
}

void ItemListEditor::on_moveListItemUpButton_clicked()
{
    int row = ui.listWidget->currentRow();
    if (row <= 0)
        return; // nothing to do

    ui.listWidget->insertItem(row - 1, ui.listWidget->takeItem(row));
    ui.listWidget->setCurrentRow(row - 1);
    emit itemMovedUp(row);
}

void ItemListEditor::on_moveListItemDownButton_clicked()
{
    int row = ui.listWidget->currentRow();
    if (row == -1 || row == ui.listWidget->count() - 1)
        return; // nothing to do

    ui.listWidget->insertItem(row + 1, ui.listWidget->takeItem(row));
    ui.listWidget->setCurrentRow(row + 1);
    emit itemMovedDown(row);
}

void ItemListEditor::on_listWidget_currentRowChanged()
{
    updateEditor();
    if (!m_updating)
        emit indexChanged(ui.listWidget->currentRow());
}

void ItemListEditor::on_listWidget_itemChanged(QListWidgetItem *item)
{
    if (m_updatingBrowser)
        return;

    PropertySheetStringValue val = qvariant_cast<PropertySheetStringValue>(item->data(Qt::DisplayPropertyRole));
    val.setValue(item->text());
    BoolBlocker block(m_updatingBrowser);
    item->setData(Qt::DisplayPropertyRole, QVariant::fromValue(val));

    // The checkState could change, too, but if this signal is connected,
    // checkState is not in the list anyway, as we are editing a header item.
    emit itemChanged(ui.listWidget->currentRow(), Qt::DisplayPropertyRole,
                     QVariant::fromValue(val));
    updateBrowser();
}

void ItemListEditor::togglePropertyBrowser()
{
    setPropertyBrowserVisible(!m_propertyBrowser->isVisible());
}

void ItemListEditor::setPropertyBrowserVisible(bool v)
{
    ui.showPropertiesButton->setText(v ? tr("Properties &>>") : tr("Properties &<<"));
    m_propertyBrowser->setVisible(v);
}

void ItemListEditor::setItemData(int role, const QVariant &v)
{
    QListWidgetItem *item = ui.listWidget->currentItem();
    bool reLayout = false;
    if ((role == Qt::EditRole && (v.toString().count(QLatin1Char('\n')) != item->data(role).toString().count(QLatin1Char('\n'))))
        || role == Qt::FontRole)
            reLayout = true;
    QVariant newValue = v;
    if (role == Qt::FontRole && newValue.type() == QVariant::Font) {
        QFont oldFont = ui.listWidget->font();
        QFont newFont = qvariant_cast<QFont>(newValue).resolve(oldFont);
        newValue = QVariant::fromValue(newFont);
        item->setData(role, QVariant()); // force the right font with the current resolve mask is set (item view bug)
    }
    item->setData(role, newValue);
    if (reLayout)
        ui.listWidget->doItemsLayout();
    emit itemChanged(ui.listWidget->currentRow(), role, newValue);
}

QVariant ItemListEditor::getItemData(int role) const
{
    return ui.listWidget->currentItem()->data(role);
}

int ItemListEditor::defaultItemFlags() const
{
    static const int flags = QListWidgetItem().flags();
    return flags;
}

void ItemListEditor::cacheReloaded()
{
    reloadIconResources(iconCache(), ui.listWidget);
}

void ItemListEditor::updateEditor()
{
    bool currentItemEnabled = false;

    bool moveRowUpEnabled = false;
    bool moveRowDownEnabled = false;

    QListWidgetItem *item = ui.listWidget->currentItem();
    if (item) {
        currentItemEnabled = true;
        int currentRow = ui.listWidget->currentRow();
        if (currentRow > 0)
            moveRowUpEnabled = true;
        if (currentRow < ui.listWidget->count() - 1)
            moveRowDownEnabled = true;
    }

    ui.moveListItemUpButton->setEnabled(moveRowUpEnabled);
    ui.moveListItemDownButton->setEnabled(moveRowDownEnabled);
    ui.deleteListItemButton->setEnabled(currentItemEnabled);

    if (item)
        updateBrowser();
    else
        m_propertyBrowser->clear();
}
} // namespace qdesigner_internal

QT_END_NAMESPACE
