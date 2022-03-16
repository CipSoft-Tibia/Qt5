/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "invokemethod.h"

#include <qt_windows.h>
#include <ActiveQt/ActiveQt>

QT_BEGIN_NAMESPACE

InvokeMethod::InvokeMethod(QWidget *parent)
: QDialog(parent), activex(0)
{
    setupUi(this);

    listParameters->setColumnCount(3);
    listParameters->headerItem()->setText(0, tr("Parameter"));
    listParameters->headerItem()->setText(1, tr("Type"));
    listParameters->headerItem()->setText(2, tr("Value"));
}

void InvokeMethod::setControl(QAxBase *ax)
{
    activex = ax;
    bool hasControl = activex && !activex->isNull();
    labelMethods->setEnabled(hasControl);
    comboMethods->setEnabled(hasControl);
    buttonInvoke->setEnabled(hasControl);
    boxParameters->setEnabled(hasControl);

    comboMethods->clear();
    listParameters->clear();

    if (!hasControl) {
        editValue->clear();
        return;
    }

    const QMetaObject *mo = activex->metaObject();
    if (mo->methodCount()) {
        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
            const QMetaMethod method = mo->method(i);
            if (method.methodType() == QMetaMethod::Slot)
                comboMethods->addItem(QString::fromLatin1(method.methodSignature()));
        }
        comboMethods->model()->sort(0);

        on_comboMethods_activated(comboMethods->currentText());
    }
}

void InvokeMethod::on_buttonInvoke_clicked()
{
    if (!activex)
        return;

    on_buttonSet_clicked();
    QString method = comboMethods->currentText();
    QList<QVariant> vars;

    int itemCount = listParameters->topLevelItemCount();
    for (int i = 0; i < itemCount; ++i) {
        QTreeWidgetItem *parameter = listParameters->topLevelItem(i);
        vars << parameter->text(2);
    }
    QVariant result = activex->dynamicCall(method.toLatin1(), vars);

    int v = 0;
    for (int i = 0; i < itemCount; ++i) {
        QTreeWidgetItem *parameter = listParameters->topLevelItem(i);
        parameter->setText(2, vars[v++].toString());
    }

    editReturn->setText(QString::fromLatin1(result.typeName())
                        + QLatin1Char(' ') + result.toString());
}

void InvokeMethod::on_comboMethods_activated(const QString &method)
{
    if (!activex)
        return;
    listParameters->clear();

    const QMetaObject *mo = activex->metaObject();
    const QMetaMethod slot = mo->method(mo->indexOfSlot(method.toLatin1()));
    QString signature = QString::fromLatin1(slot.methodSignature());
    signature.remove(0, signature.indexOf(QLatin1Char('(')) + 1);
    signature.truncate(signature.length()-1);

    QList<QByteArray> pnames = slot.parameterNames();
    QList<QByteArray> ptypes = slot.parameterTypes();

    for (int p = 0; p < ptypes.count(); ++p) {
        QString ptype(QString::fromLatin1(ptypes.at(p)));
        if (ptype.isEmpty())
            continue;
        QString pname(QString::fromLatin1(pnames.at(p).constData()));
        if (pname.isEmpty())
            pname = QString::fromLatin1("<unnamed %1>").arg(p);
        QTreeWidgetItem *item = new QTreeWidgetItem(listParameters);
        item->setText(0, pname);
        item->setText(1, ptype);
    }

    if (listParameters->topLevelItemCount())
        listParameters->setCurrentItem(listParameters->topLevelItem(0));
    editReturn->setText(QString::fromLatin1(slot.typeName()));
}

void InvokeMethod::on_listParameters_currentItemChanged(QTreeWidgetItem *item)
{
    if (!activex)
        return;
    editValue->setEnabled(item != 0);
    buttonSet->setEnabled(item != 0);
    if (!item)
        return;
    editValue->setText(item->text(2));
}

void InvokeMethod::on_buttonSet_clicked()
{
    if (!activex)
        return;
    QTreeWidgetItem *item = listParameters->currentItem();
    if (!item)
        return;
    item->setText(2, editValue->text());
}

QT_END_NAMESPACE
