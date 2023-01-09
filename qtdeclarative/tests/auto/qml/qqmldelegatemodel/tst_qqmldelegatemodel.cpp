﻿/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/qtest.h>
#include <QtCore/QConcatenateTablesProxyModel>
#include <QtGui/QStandardItemModel>
#include <QtQml/qqmlcomponent.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>

#include "../../shared/util.h"

class tst_QQmlDelegateModel : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlDelegateModel();

private slots:
    void valueWithoutCallingObjectFirst_data();
    void valueWithoutCallingObjectFirst();
    void filterOnGroup_removeWhenCompleted();
    void qtbug_86017();
    void contextAccessedByHandler();
    void redrawUponColumnChange();
};

class AbstractItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    AbstractItemModel()
    {
        for (int i = 0; i < 3; ++i)
            mValues.append(QString::fromLatin1("Item %1").arg(i));
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return QModelIndex();

        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override
    {
        return QModelIndex();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0;

        return mValues.count();
    }

    int columnCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;

        return 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole)
            return QVariant();

        return mValues.at(index.row());
    }

private:
    QVector<QString> mValues;
};

tst_QQmlDelegateModel::tst_QQmlDelegateModel()
{
    qmlRegisterType<AbstractItemModel>("Test", 1, 0, "AbstractItemModel");
}

void tst_QQmlDelegateModel::valueWithoutCallingObjectFirst_data()
{
    QTest::addColumn<QUrl>("qmlFileUrl");
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("role");
    QTest::addColumn<QVariant>("expectedValue");

    QTest::addRow("integer") << testFileUrl("integerModel.qml")
        << 50 << QString::fromLatin1("modelData") << QVariant(50);
    QTest::addRow("ListModel") << testFileUrl("listModel.qml")
        << 1 << QString::fromLatin1("name") << QVariant(QLatin1String("Item 1"));
    QTest::addRow("QAbstractItemModel") << testFileUrl("abstractItemModel.qml")
        << 1 << QString::fromLatin1("display") << QVariant(QLatin1String("Item 1"));
}

// Tests that it's possible to call variantValue() without creating
// costly delegate items first via object().
void tst_QQmlDelegateModel::valueWithoutCallingObjectFirst()
{
    QFETCH(const QUrl, qmlFileUrl);
    QFETCH(const int, index);
    QFETCH(const QString, role);
    QFETCH(const QVariant, expectedValue);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(qmlFileUrl);
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QQmlDelegateModel *model = qobject_cast<QQmlDelegateModel*>(root.data());
    QVERIFY(model);
    QCOMPARE(model->variantValue(index, role), expectedValue);
}

void tst_QQmlDelegateModel::filterOnGroup_removeWhenCompleted()
{
    QQuickView view(testFileUrl("removeFromGroup.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QQuickItem *root = view.rootObject();
    QVERIFY(root);
    QQmlDelegateModel *model = root->findChild<QQmlDelegateModel*>();
    QVERIFY(model);
    QTest::qWaitFor([=]{ return model->count() == 2; } );

void tst_QQmlDelegateModel::qtbug_86017()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("qtbug_86017.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QTRY_VERIFY(component.isReady());
    QQmlDelegateModel *model = qobject_cast<QQmlDelegateModel*>(root.data());

    QVERIFY(model);
    QCOMPARE(model->count(), 2);
    QCOMPARE(model->filterGroup(), "selected");
}

void tst_QQmlDelegateModel::filterOnGroup_removeWhenCompleted()
{
    QQuickView view(testFileUrl("removeFromGroup.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QQuickItem *root = view.rootObject();
    QVERIFY(root);
    QQmlDelegateModel *model = root->findChild<QQmlDelegateModel*>();
    QVERIFY(model);
    QVERIFY(QTest::qWaitFor([=]{ return model->count() == 2; }));
}

void tst_QQmlDelegateModel::contextAccessedByHandler()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("contextAccessedByHandler.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(root->property("works").toBool());
}

void tst_QQmlDelegateModel::redrawUponColumnChange()
{
    QStandardItemModel m1;
    m1.appendRow({
            new QStandardItem("Banana"),
            new QStandardItem("Coconut"),
    });

    QQuickView view(testFileUrl("redrawUponColumnChange.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QQuickItem *root = view.rootObject();
    root->setProperty("model", QVariant::fromValue<QObject *>(&m1));

    QObject *item = root->property("currentItem").value<QObject *>();
    QVERIFY(item);
    QCOMPARE(item->property("text").toString(), "Banana");

    QVERIFY(root);
    m1.removeColumn(0);

    QCOMPARE(item->property("text").toString(), "Coconut");
}

QTEST_MAIN(tst_QQmlDelegateModel)

#include "tst_qqmldelegatemodel.moc"
