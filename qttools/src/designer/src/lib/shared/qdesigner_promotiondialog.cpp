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

#include "qdesigner_promotiondialog_p.h"
#include "promotionmodel_p.h"
#include "iconloader_p.h"
#include "widgetdatabase_p.h"
#include "signalslotdialog_p.h"

#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstractformwindow.h>
#include <QtDesigner/abstractpromotioninterface.h>
#include <QtDesigner/abstractwidgetdatabase.h>
#include <QtDesigner/abstractintegration.h>
#include <abstractdialoggui_p.h>

#include <QtCore/qtimer.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qformlayout.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qtreeview.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qpushbutton.h>
#include <QtCore/qitemselectionmodel.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qcheckbox.h>
#include <QtGui/qvalidator.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayoutitem.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qaction.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {
    // PromotionParameters
    struct PromotionParameters {
        QString m_baseClass;
        QString m_className;
        QString m_includeFile;
    };

    //  NewPromotedClassPanel
    NewPromotedClassPanel::NewPromotedClassPanel(const QStringList &baseClasses,
                                                   int selectedBaseClass,
                                                   QWidget *parent) :
        QGroupBox(parent),
        m_baseClassCombo(new  QComboBox),
        m_classNameEdit(new QLineEdit),
        m_includeFileEdit(new QLineEdit),
        m_globalIncludeCheckBox(new QCheckBox),
        m_addButton(new QPushButton(tr("Add")))
    {
        setTitle(tr("New Promoted Class"));
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
        QHBoxLayout *hboxLayout = new QHBoxLayout(this);

        m_classNameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^[_a-zA-Z:][:_a-zA-Z0-9]*$")), m_classNameEdit));
        connect(m_classNameEdit,   &QLineEdit::textChanged,
                this, &NewPromotedClassPanel::slotNameChanged);
        connect(m_includeFileEdit, &QLineEdit::textChanged,
                this, &NewPromotedClassPanel::slotIncludeFileChanged);

        m_baseClassCombo->setEditable(false);
        m_baseClassCombo->addItems(baseClasses);
        if (selectedBaseClass != -1)
            m_baseClassCombo->setCurrentIndex(selectedBaseClass);

        // Grid
        QFormLayout *formLayout = new QFormLayout();
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow); // Mac
        formLayout->addRow(tr("Base class name:"),     m_baseClassCombo);
        formLayout->addRow(tr("Promoted class name:"), m_classNameEdit);
        formLayout->addRow(tr("Header file:"),         m_includeFileEdit);
        formLayout->addRow(tr("Global include"),       m_globalIncludeCheckBox);
        hboxLayout->addLayout(formLayout);
        hboxLayout->addItem(new QSpacerItem(15, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
        // Button box
        QVBoxLayout *buttonLayout = new QVBoxLayout();

        m_addButton->setAutoDefault(false);
        connect(m_addButton, &QAbstractButton::clicked, this, &NewPromotedClassPanel::slotAdd);
        m_addButton->setEnabled(false);
        buttonLayout->addWidget(m_addButton);

        QPushButton *resetButton = new QPushButton(tr("Reset"));
        resetButton->setAutoDefault(false);
        connect(resetButton, &QAbstractButton::clicked, this, &NewPromotedClassPanel::slotReset);

        buttonLayout->addWidget(resetButton);
        buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding));
        hboxLayout->addLayout(buttonLayout);

        enableButtons();
    }

    void NewPromotedClassPanel::slotAdd() {
        bool ok = false;
        emit newPromotedClass(promotionParameters(), &ok);
        if (ok)
            slotReset();
    }

    void NewPromotedClassPanel::slotReset() {
        const QString empty;
        m_classNameEdit->setText(empty);
        m_includeFileEdit->setText(empty);
        m_globalIncludeCheckBox->setCheckState(Qt::Unchecked);
    }

    void NewPromotedClassPanel::grabFocus() {
        m_classNameEdit->setFocus(Qt::OtherFocusReason);
    }

    void NewPromotedClassPanel::slotNameChanged(const QString &className) {
        // Suggest a name
        if (!className.isEmpty()) {
            const QChar dot(QLatin1Char('.'));
            QString suggestedHeader = m_promotedHeaderLowerCase ?
                                      className.toLower() : className;
            suggestedHeader.replace(QStringLiteral("::"), QString(QLatin1Char('_')));
            if (!m_promotedHeaderSuffix.startsWith(dot))
                suggestedHeader += dot;
            suggestedHeader += m_promotedHeaderSuffix;

            const bool blocked = m_includeFileEdit->blockSignals(true);
            m_includeFileEdit->setText(suggestedHeader);
            m_includeFileEdit->blockSignals(blocked);
        }
        enableButtons();
    }

    void NewPromotedClassPanel::slotIncludeFileChanged(const QString &){
        enableButtons();
    }

    void NewPromotedClassPanel::enableButtons() {
        const bool enabled = !m_classNameEdit->text().isEmpty() && !m_includeFileEdit->text().isEmpty();
        m_addButton->setEnabled(enabled);
        m_addButton->setDefault(enabled);
    }

    PromotionParameters NewPromotedClassPanel::promotionParameters() const {
         PromotionParameters rc;
         rc.m_baseClass = m_baseClassCombo->currentText();
         rc.m_className = m_classNameEdit->text();
         rc.m_includeFile = buildIncludeFile(m_includeFileEdit->text(),
                                             m_globalIncludeCheckBox->checkState() == Qt::Checked ? IncludeGlobal : IncludeLocal);
         return rc;
     }

    void NewPromotedClassPanel::chooseBaseClass(const QString &baseClass) {
        const int index = m_baseClassCombo->findText (baseClass);
        if (index != -1)
            m_baseClassCombo->setCurrentIndex (index);
    }

    // --------------- QDesignerPromotionDialog
    QDesignerPromotionDialog::QDesignerPromotionDialog(QDesignerFormEditorInterface *core,
                                                       QWidget *parent,
                                                       const QString &promotableWidgetClassName,
                                                       QString *promoteTo) :
        QDialog(parent),
        m_mode(promotableWidgetClassName.isEmpty() || promoteTo == 0 ? ModeEdit : ModeEditChooseClass),
        m_promotableWidgetClassName(promotableWidgetClassName),
        m_core(core),
        m_promoteTo(promoteTo),
        m_promotion(core->promotion()),
        m_model(new PromotionModel(core)),
        m_treeView(new QTreeView),
        m_buttonBox(0),
        m_removeButton(new QPushButton(createIconSet(QString::fromUtf8("minus.png")), QString()))
    {
        m_buttonBox = createButtonBox();
        setModal(true);
        setWindowTitle(tr("Promoted Widgets"));
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QVBoxLayout *vboxLayout = new QVBoxLayout(this);

        // tree view group
        QGroupBox *treeViewGroup = new QGroupBox();
        treeViewGroup->setTitle(tr("Promoted Classes"));
        QVBoxLayout *treeViewVBoxLayout = new QVBoxLayout(treeViewGroup);
        // tree view
        m_treeView->setModel (m_model);
        m_treeView->setMinimumWidth(450);
        m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &QDesignerPromotionDialog::slotSelectionChanged);

        connect(m_treeView, &QWidget::customContextMenuRequested,
                this, &QDesignerPromotionDialog::slotTreeViewContextMenu);

        QHeaderView *headerView = m_treeView->header();
        headerView->setSectionResizeMode(QHeaderView::ResizeToContents);
        treeViewVBoxLayout->addWidget(m_treeView);
        // remove button
        QHBoxLayout *hboxLayout = new QHBoxLayout();
        hboxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));

        m_removeButton->setAutoDefault(false);
        connect(m_removeButton, &QAbstractButton::clicked, this, &QDesignerPromotionDialog::slotRemove);
        m_removeButton->setEnabled(false);
        hboxLayout->addWidget(m_removeButton);
        treeViewVBoxLayout->addLayout(hboxLayout);
        vboxLayout->addWidget(treeViewGroup);
        // Create new panel: Try to be smart and preselect a base class. Default to QFrame
        const QStringList &baseClassNameList = baseClassNames(m_promotion);
        int preselectedBaseClass = -1;
        if (m_mode == ModeEditChooseClass) {
            preselectedBaseClass = baseClassNameList.indexOf(m_promotableWidgetClassName);
        }
        if (preselectedBaseClass == -1)
            preselectedBaseClass = baseClassNameList.indexOf(QStringLiteral("QFrame"));

        NewPromotedClassPanel *newPromotedClassPanel = new NewPromotedClassPanel(baseClassNameList, preselectedBaseClass);
        newPromotedClassPanel->setPromotedHeaderSuffix(core->integration()->headerSuffix());
        newPromotedClassPanel->setPromotedHeaderLowerCase(core->integration()->isHeaderLowercase());
        connect(newPromotedClassPanel, &NewPromotedClassPanel::newPromotedClass,
                this, &QDesignerPromotionDialog::slotNewPromotedClass);
        connect(this, &QDesignerPromotionDialog::selectedBaseClassChanged,
                newPromotedClassPanel, &NewPromotedClassPanel::chooseBaseClass);
        vboxLayout->addWidget(newPromotedClassPanel);
        // button box
        vboxLayout->addWidget(m_buttonBox);
        // connect model
        connect(m_model, &PromotionModel::includeFileChanged,
                this, &QDesignerPromotionDialog::slotIncludeFileChanged);

        connect(m_model, &PromotionModel::classNameChanged,
                this, &QDesignerPromotionDialog::slotClassNameChanged);

        // focus
        if (m_mode == ModeEditChooseClass)
            newPromotedClassPanel->grabFocus();

        slotUpdateFromWidgetDatabase();
    }

    QDialogButtonBox *QDesignerPromotionDialog::createButtonBox() {
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Close);

        connect(buttonBox, &QDialogButtonBox::accepted,
                this, &QDesignerPromotionDialog::slotAcceptPromoteTo);
        buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Promote"));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        return buttonBox;
    }

    void QDesignerPromotionDialog::slotUpdateFromWidgetDatabase() {
        m_model->updateFromWidgetDatabase();
        m_treeView->expandAll();
        m_removeButton->setEnabled(false);
    }

    void QDesignerPromotionDialog::delayedUpdateFromWidgetDatabase() {
        QTimer::singleShot(0, this, &QDesignerPromotionDialog::slotUpdateFromWidgetDatabase);
    }

    const QStringList &QDesignerPromotionDialog::baseClassNames(const QDesignerPromotionInterface *promotion) {
        typedef QList<QDesignerWidgetDataBaseItemInterface *> WidgetDataBaseItemList;
        static QStringList rc;
        if (rc.empty()) {
            // Convert the item list into a string list.
            const WidgetDataBaseItemList dbItems =  promotion->promotionBaseClasses();
            const WidgetDataBaseItemList::const_iterator cend =  dbItems.constEnd();
            for (WidgetDataBaseItemList::const_iterator it = dbItems.constBegin() ; it != cend; ++it) {
                rc.push_back( (*it)->name());
            }
        }
        return rc;
    }

    void  QDesignerPromotionDialog::slotAcceptPromoteTo() {
        Q_ASSERT(m_mode == ModeEditChooseClass);
        unsigned flags;
        // Ok pressed: Promote to selected class
        if (QDesignerWidgetDataBaseItemInterface *dbItem = databaseItemAt(m_treeView->selectionModel()->selection(), flags)) {
            if (flags & CanPromote) {
                *m_promoteTo = dbItem ->name();
                accept();
            }
        }
    }

    void QDesignerPromotionDialog::slotRemove() {
        unsigned flags;
        QDesignerWidgetDataBaseItemInterface *dbItem = databaseItemAt(m_treeView->selectionModel()->selection(), flags);
        if (!dbItem || (flags & Referenced))
            return;

        QString errorMessage;
        if (m_promotion->removePromotedClass(dbItem->name(), &errorMessage)) {
            slotUpdateFromWidgetDatabase();
        } else {
            displayError(errorMessage);
        }
    }

    void QDesignerPromotionDialog::slotSelectionChanged(const QItemSelection &selected, const QItemSelection &) {
        // Enable deleting non-referenced items
        unsigned flags;
        const QDesignerWidgetDataBaseItemInterface *dbItem = databaseItemAt(selected, flags);
        m_removeButton->setEnabled(dbItem && !(flags & Referenced));
        // In choose mode, can we promote to the class?
        if (m_mode == ModeEditChooseClass) {
            const bool enablePromoted = flags & CanPromote;
            m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enablePromoted);
            m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(enablePromoted);
        }
        // different base?
        if (dbItem) {
            const QString baseClass = dbItem->extends();
            if (baseClass !=  m_lastSelectedBaseClass) {
                m_lastSelectedBaseClass = baseClass;
                emit selectedBaseClassChanged(m_lastSelectedBaseClass);
            }
        }
    }

    QDesignerWidgetDataBaseItemInterface *QDesignerPromotionDialog::databaseItemAt(const QItemSelection &selected, unsigned &flags) const {
        flags = 0;
        const QModelIndexList indexes = selected.indexes();
        if (indexes.empty())
            return 0;
        const PromotionModel::ModelData data = m_model->modelData(indexes.constFirst());
        QDesignerWidgetDataBaseItemInterface *dbItem = data.promotedItem;

        if (dbItem) {
            if (data.referenced)
                flags |= Referenced;
            // In choose mode, can we promote to the class?
            if (m_mode == ModeEditChooseClass &&  dbItem && dbItem->isPromoted() && dbItem->extends() ==  m_promotableWidgetClassName)
                flags |= CanPromote;

        }
        return dbItem;
    }

    void QDesignerPromotionDialog::slotNewPromotedClass(const PromotionParameters &p, bool *ok) {
        QString  errorMessage;
        *ok = m_promotion->addPromotedClass(p.m_baseClass, p.m_className, p.m_includeFile, &errorMessage);
        if (*ok) {
            // update and select
            slotUpdateFromWidgetDatabase();
            const QModelIndex newClassIndex = m_model->indexOfClass(p.m_className);
            if (newClassIndex.isValid()) {
                m_treeView->selectionModel()->select(newClassIndex, QItemSelectionModel::SelectCurrent|QItemSelectionModel::Rows);
            }
        } else {
            displayError(errorMessage);
        }
    }

    void QDesignerPromotionDialog::slotIncludeFileChanged(QDesignerWidgetDataBaseItemInterface *dbItem, const QString &includeFile) {
        if (includeFile.isEmpty()) {
            delayedUpdateFromWidgetDatabase();
            return;
        }

        if (dbItem->includeFile() == includeFile)
            return;

        QString errorMessage;
        if (!m_promotion->setPromotedClassIncludeFile(dbItem->name(), includeFile, &errorMessage)) {
            displayError(errorMessage);
            delayedUpdateFromWidgetDatabase();
        }
    }

    void  QDesignerPromotionDialog::slotClassNameChanged(QDesignerWidgetDataBaseItemInterface *dbItem, const QString &newName) {
        if (newName.isEmpty()) {
            delayedUpdateFromWidgetDatabase();
            return;
        }
        const QString oldName = dbItem->name();
        if (newName == oldName)
            return;

        QString errorMessage;
        if (!m_promotion->changePromotedClassName(oldName , newName, &errorMessage)) {
            displayError(errorMessage);
            delayedUpdateFromWidgetDatabase();
        }
    }

    void QDesignerPromotionDialog::slotTreeViewContextMenu(const QPoint &pos) {
        unsigned flags;
        const QDesignerWidgetDataBaseItemInterface *dbItem = databaseItemAt(m_treeView->selectionModel()->selection(), flags);
        if (!dbItem)
            return;

        QMenu menu;
        QAction *signalSlotAction = menu.addAction(tr("Change signals/slots..."));
        connect(signalSlotAction, &QAction::triggered,
                this, &QDesignerPromotionDialog::slotEditSignalsSlots);

        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
    }

    void  QDesignerPromotionDialog::slotEditSignalsSlots() {
        unsigned flags;
        const QDesignerWidgetDataBaseItemInterface *dbItem = databaseItemAt(m_treeView->selectionModel()->selection(), flags);
        if (!dbItem)
            return;

        SignalSlotDialog::editPromotedClass(m_core, dbItem->name(), this);
    }

    void QDesignerPromotionDialog::displayError(const QString &message) {
        m_core->dialogGui()->message(this, QDesignerDialogGuiInterface::PromotionErrorMessage, QMessageBox::Warning,
                                     tr("%1 - Error").arg(windowTitle()), message,  QMessageBox::Close);
    }
} // namespace qdesigner_internal

QT_END_NAMESPACE
