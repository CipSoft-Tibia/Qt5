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

#include "combobox_taskmenu.h"
#include "listwidgeteditor.h"
#include "qdesigner_utils_p.h"
#include <qdesigner_command_p.h>

#include <QtDesigner/abstractformwindow.h>

#include <QtWidgets/qaction.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qfontcombobox.h>
#include <QtWidgets/qstyleoption.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

using namespace qdesigner_internal;

ComboBoxTaskMenu::ComboBoxTaskMenu(QComboBox *button, QObject *parent)
    : QDesignerTaskMenu(button, parent),
      m_comboBox(button)
{
    m_editItemsAction = new QAction(this);
    m_editItemsAction->setText(tr("Edit Items..."));
    connect(m_editItemsAction, &QAction::triggered, this, &ComboBoxTaskMenu::editItems);
    m_taskActions.append(m_editItemsAction);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    m_taskActions.append(sep);
}

ComboBoxTaskMenu::~ComboBoxTaskMenu() = default;

QAction *ComboBoxTaskMenu::preferredEditAction() const
{
    return m_editItemsAction;
}

QList<QAction*> ComboBoxTaskMenu::taskActions() const
{
    return m_taskActions + QDesignerTaskMenu::taskActions();
}

void ComboBoxTaskMenu::editItems()
{
    m_formWindow = QDesignerFormWindowInterface::findFormWindow(m_comboBox);
    if (m_formWindow.isNull())
        return;

    Q_ASSERT(m_comboBox != 0);

    ListWidgetEditor dlg(m_formWindow, m_comboBox->window());
    ListContents oldItems = dlg.fillContentsFromComboBox(m_comboBox);
    if (dlg.exec() == QDialog::Accepted) {
        ListContents items = dlg.contents();
        if (items != oldItems) {
            ChangeListContentsCommand *cmd = new ChangeListContentsCommand(m_formWindow);
            cmd->init(m_comboBox, oldItems, items);
            cmd->setText(tr("Change Combobox Contents"));
            m_formWindow->commandHistory()->push(cmd);
        }
    }
}

ComboBoxTaskMenuFactory::ComboBoxTaskMenuFactory(const QString &iid, QExtensionManager *extensionManager) :
    ExtensionFactory<QDesignerTaskMenuExtension, QComboBox, ComboBoxTaskMenu>(iid, extensionManager)
{
}

QComboBox *ComboBoxTaskMenuFactory::checkObject(QObject *qObject) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(qObject);
    if (!combo)
        return 0;
    if (qobject_cast<QFontComboBox*>(combo))
        return 0;
    return combo;
}

void ComboBoxTaskMenu::updateSelection()
{
    if (m_editor)
        m_editor->deleteLater();
}

QT_END_NAMESPACE
