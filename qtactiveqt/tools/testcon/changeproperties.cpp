// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changeproperties.h"

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/qt_windows.h>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QRegularExpression>
#include <QtAxContainer/QAxWidget>

QT_BEGIN_NAMESPACE

ChangeProperties::ChangeProperties(QWidget *parent)
: QDialog(parent), activex(nullptr)
{
    setupUi(this);

    listProperties->setColumnCount(3);
    listProperties->headerItem()->setText(0, QLatin1String("Name"));
    listProperties->headerItem()->setText(1, QLatin1String("Type"));
    listProperties->headerItem()->setText(2, QLatin1String("Value"));

    listEditRequests->setColumnCount(1);
    listEditRequests->headerItem()->setText(0, QLatin1String("Name"));
}

void ChangeProperties::setControl(QAxWidget *ax)
{
    activex = ax;
    updateProperties();
}

void ChangeProperties::on_listProperties_currentItemChanged(QTreeWidgetItem *current)
{
    editValue->setEnabled(current != nullptr);
    buttonSet->setEnabled(current != nullptr);
    valueLabel->setEnabled(current != nullptr);

    if (!current)
        return;

    editValue->setText(current->text(2));
    QString prop = current->text(0);
    valueLabel->setText(prop + QLatin1String(" ="));

    const QMetaObject *mo = activex->metaObject();
    const QMetaProperty property = mo->property(mo->indexOfProperty(prop.toLatin1()));

    valueLabel->setEnabled(property.isWritable());
    editValue->setEnabled(property.isWritable());
    buttonSet->setEnabled(property.isWritable());
}

void ChangeProperties::on_buttonSet_clicked()
{
    QTreeWidgetItem *item = listProperties->currentItem();
    if (!item)
        return;

    QString prop = item->text(0);
    QVariant value = activex->property(prop.toLatin1());
    int type = value.metaType().id();
    if (!value.isValid()) {
        const QMetaObject *mo = activex->metaObject();
        const QMetaProperty property = mo->property(mo->indexOfProperty(prop.toLatin1()));
        type = QMetaType::fromName(property.typeName()).id();
    }
    switch (type) {
    case QMetaType::QColor:
        {
            const QColor col = QColor::fromString(editValue->text());
            if (col.isValid()) {
                value = QVariant::fromValue(col);
            } else {
                QMessageBox::warning(this, tr("Can't parse input"),
                                           tr("Failed to create a color from %1\n"
                                              "The string has to be a valid color name (e.g. 'red')\n"
                                              "or a RGB triple of format '#rrggbb'."
                                              ).arg(editValue->text()));
            }
        }
        break;
    case QMetaType::QFont:
        {
            QFont fnt;
            if (fnt.fromString(editValue->text())) {
                value = QVariant::fromValue(fnt);
            } else {
                QMessageBox::warning(this, tr("Can't parse input"),
                                           tr("Failed to create a font from %1\n"
                                              "The string has to have a format family,<point size> or\n"
                                              "family,pointsize,stylehint,weight,italic,underline,strikeout,fixedpitch,rawmode."
                                              ).arg(editValue->text()));
            }
        }
        break;
    case QMetaType::QPixmap:
        {
            QString fileName = editValue->text();
            if (fileName.isEmpty())
                fileName = QFileDialog::getOpenFileName(this);
            QPixmap pm(fileName);
            if (pm.isNull())
                return;

            value = QVariant::fromValue(pm);
        }
        break;
    case QMetaType::Bool:
        {
            const QString txt = editValue->text();
            value = QVariant(txt != QLatin1String("0") && txt.compare(QLatin1String("false"), Qt::CaseInsensitive));
        }
        break;
    case QMetaType::QVariantList:
        {
            QStringList txtList = editValue->text().split(QRegularExpression(QLatin1String("[,;]")));
            QVariantList varList;
            for (qsizetype i = 0; i < txtList.size(); ++i) {
                QVariant svar(txtList.at(i));
                QString str = svar.toString();
                str = str.trimmed();
                bool ok;
                int n = str.toInt(&ok);
                if (ok) {
                    varList << n;
                    continue;
                }
                double d = str.toDouble(&ok);
                if (ok) {
                    varList << d;
                    continue;
                }
                varList << str;
            }
            value = varList;
        }
        break;

    default:
        value = editValue->text();
        break;
    }

    Q_ASSERT(activex->setProperty(prop.toLatin1(), value));
    updateProperties();
    listProperties->setCurrentItem(listProperties->findItems(prop, Qt::MatchExactly).at(0));
}

void ChangeProperties::on_listEditRequests_itemChanged(QTreeWidgetItem *item)
{
    if (!item)
        return;

    QString property = item->text(0);
    activex->setPropertyWritable(property.toLatin1(), item->checkState(0) == Qt::Checked);
}


void ChangeProperties::updateProperties()
{
    bool hasControl = activex && !activex->isNull();
    tabWidget->setEnabled(hasControl);

    listProperties->clear();
    listEditRequests->clear();
    if (hasControl) {
        const QMetaObject *mo = activex->metaObject();
        const int numprops = mo->propertyCount();
        for (int i = mo->propertyOffset(); i < numprops; ++i) {
            const QMetaProperty property = mo->property(i);
            QTreeWidgetItem *item = new QTreeWidgetItem(listProperties);
            item->setText(0, QString::fromLatin1(property.name()));
            item->setText(1, QString::fromLatin1(property.typeName()));
            if (!property.isDesignable()) {
                item->setForeground(0, Qt::gray);
                item->setForeground(1, Qt::gray);
                item->setForeground(2, Qt::gray);
            }
            QVariant var = activex->property(property.name());

            switch (var.metaType().id()) {
            case QMetaType::QColor:
                {
                    QColor col = qvariant_cast<QColor>(var);
                    item->setText(2, col.name());
                }
                break;
            case QMetaType::QFont:
                {
                    QFont fnt = qvariant_cast<QFont>(var);
                    item->setText(2, fnt.toString());
                }
                break;
            case QMetaType::Bool:
                {
                    item->setText(2, var.toBool() ? QLatin1String("true") : QLatin1String("false"));
                }
                break;
            case QMetaType::QPixmap:
                {
                    QPixmap pm = qvariant_cast<QPixmap>(var);
                    item->setIcon(2, pm);
                }
                break;
            case QMetaType::QVariantList:
                {
                    const auto varList = var.toList();
                    QStringList strList;
                    for (const auto &var : varList)
                        strList << var.toString();
                    item->setText(2, strList.join(QLatin1String(", ")));
                }
                break;
            case QMetaType::Int:
                if (property.isEnumType()) {
                    const QMetaEnum enumerator = mo->enumerator(mo->indexOfEnumerator(property.typeName()));
                    item->setText(2, QString::fromLatin1(enumerator.valueToKey(var.toInt())));
                    break;
                }
                //FALLTHROUGH
            default:
                item->setText(2, var.toString());
                break;
            }

            bool requesting = false;
#if 0
            {
                void *argv[] = { &requesting };
                activex->qt_metacall(QMetaObject::Call(0x10000000) /*RequestingEdit*/, i, argv);
            }
#endif
            if (requesting) {
                QTreeWidgetItem *check = new QTreeWidgetItem(listEditRequests);
                check->setText(0, QString::fromLatin1(property.name()));
                check->setCheckState(0, activex->propertyWritable(property.name()) ? Qt::Checked : Qt::Unchecked);
            }
        }
        listProperties->setCurrentItem(listProperties->topLevelItem(0));
    } else {
        editValue->clear();
    }
}

QT_END_NAMESPACE
