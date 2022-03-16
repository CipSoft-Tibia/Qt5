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

#include "qaxwidgettaskmenu.h"
#include "qdesigneraxwidget.h"
#include "qaxwidgetpropertysheet.h"

#include <QtDesigner/abstractformwindow.h>
#include <QtDesigner/abstractformwindowcursor.h>
#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/qextensionmanager.h>

#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qundostack.h>
#include <QtWidgets/qaction.h>
#include <QtCore/quuid.h>
#include <ActiveQt/qaxselect.h>

#include <qt_windows.h>
#include <olectl.h>
#include <qaxtypes.h>

QT_BEGIN_NAMESPACE

/* SetControlCommand: An undo commands that sets a control bypassing
   Designer's property system which cannot handle the changing
   of the 'control' property's index and other cached information
   when modifying it. */

class SetControlCommand : public QUndoCommand
{
public:
    SetControlCommand(QDesignerAxWidget *ax, QDesignerFormWindowInterface *core, const QString &newClsid = QString());

    virtual void redo() {  apply(m_newClsid); }
    virtual void undo() {  apply(m_oldClsid);  }

private:
    bool apply(const QString &clsid);

    QDesignerAxWidget *m_axWidget;
    QDesignerFormWindowInterface *m_formWindow;
    QString m_oldClsid;
    QString m_newClsid;
};

SetControlCommand::SetControlCommand(QDesignerAxWidget *ax, QDesignerFormWindowInterface *fw, const QString &newClsid) :
    m_axWidget(ax),
    m_formWindow(fw),
    m_oldClsid(ax->control()),
    m_newClsid(newClsid)
{
    if (m_newClsid.isEmpty())
        setText(QDesignerAxWidget::tr("Reset control"));
    else
        setText(QDesignerAxWidget::tr("Set control"));
}

bool SetControlCommand::apply(const QString &clsid)
{
    if (m_oldClsid == m_newClsid)
        return true;

    QObject *ext = m_formWindow->core()->extensionManager()->extension(m_axWidget, Q_TYPEID(QDesignerPropertySheetExtension));
    QAxWidgetPropertySheet *sheet = qobject_cast<QAxWidgetPropertySheet*>(ext);
    if (!sheet)
        return false;

    const bool hasClsid = !clsid.isEmpty();
    const int index = sheet->indexOf(QLatin1String(QAxWidgetPropertySheet::controlPropertyName));
    if (hasClsid)
        sheet->setProperty(index, clsid);
    else
        sheet->reset(index);
    return true;
}

// -------------------- QAxWidgetTaskMenu
QAxWidgetTaskMenu::QAxWidgetTaskMenu(QDesignerAxWidget *object, QObject *parent) :
    QObject(parent),
    m_axwidget(object),
    m_setAction(new QAction(tr("Set Control"), this)),
    m_resetAction(new QAction(tr("Reset Control"), this))
{
    connect(m_setAction, &QAction::triggered, this, &QAxWidgetTaskMenu::setActiveXControl);
    connect(m_resetAction, &QAction::triggered, this, &QAxWidgetTaskMenu::resetActiveXControl);
    m_taskActions.push_back(m_setAction);
    m_taskActions.push_back(m_resetAction);
}

QAxWidgetTaskMenu::~QAxWidgetTaskMenu()
{
}

QList<QAction*> QAxWidgetTaskMenu::taskActions() const
{
    const bool loaded = m_axwidget->loaded();
    m_setAction->setEnabled(!loaded);
    m_resetAction->setEnabled(loaded);
    return m_taskActions;
}

void QAxWidgetTaskMenu::resetActiveXControl()
{
    QDesignerFormWindowInterface *formWin = QDesignerFormWindowInterface::findFormWindow(m_axwidget);
    Q_ASSERT(formWin != 0);
    formWin->commandHistory()->push(new SetControlCommand(m_axwidget, formWin));
}

void QAxWidgetTaskMenu::setActiveXControl()
{
    QAxSelect *dialog = new QAxSelect(m_axwidget->topLevelWidget());
    if (dialog->exec())    {
        QUuid clsid = dialog->clsid();
        QString key;

        IClassFactory2 *cf2 = 0;
        CoGetClassObject(clsid, CLSCTX_SERVER, 0, IID_IClassFactory2, (void**)&cf2);

        if (cf2)  {
            BSTR bKey;
            HRESULT hres = cf2->RequestLicKey(0, &bKey);
            if (hres == CLASS_E_NOTLICENSED) {
                QMessageBox::warning(m_axwidget->topLevelWidget(), tr("Licensed Control"),
                                     tr("The control requires a design-time license"));
                clsid = QUuid();
            } else {
                key = QString::fromWCharArray(bKey);
            }

            cf2->Release();
        }

        if (!clsid.isNull())  {
            QDesignerFormWindowInterface *formWin = QDesignerFormWindowInterface::findFormWindow(m_axwidget);

            Q_ASSERT(formWin != 0);
            QString value = clsid.toString();
            if (!key.isEmpty()) {
                value += QLatin1Char(':');
                value += key;
            }
            formWin->commandHistory()->push(new SetControlCommand(m_axwidget, formWin, value));
        }
    }
    delete dialog;
}

QT_END_NAMESPACE
