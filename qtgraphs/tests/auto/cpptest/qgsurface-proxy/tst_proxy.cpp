// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtGraphs/QSurfaceDataProxy>

class tst_proxy: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void construct();

    void initialProperties();
    void initializeProperties();
    void initialRow();

private:
    QSurfaceDataProxy *m_proxy;
};

void tst_proxy::initTestCase()
{
}

void tst_proxy::cleanupTestCase()
{
}

void tst_proxy::init()
{
    m_proxy = new QSurfaceDataProxy();
}

void tst_proxy::cleanup()
{
    delete m_proxy;
}

void tst_proxy::construct()
{
    QSurfaceDataProxy *proxy = new QSurfaceDataProxy();
    QVERIFY(proxy);
    delete proxy;
}

void tst_proxy::initialProperties()
{
    QVERIFY(m_proxy);

    QCOMPARE(m_proxy->columnCount(), 0);
    QCOMPARE(m_proxy->rowCount(), 0);
    QVERIFY(!m_proxy->series());

    QCOMPARE(m_proxy->type(), QAbstractDataProxy::DataType::Surface);
}

void tst_proxy::initializeProperties()
{
    QVERIFY(m_proxy);

    QSurfaceDataArray data;
    QSurfaceDataRow dataRow1;
    QSurfaceDataRow dataRow2;
    dataRow1 << QSurfaceDataItem(0.0f, 0.1f, 0.5f) << QSurfaceDataItem(1.0f, 0.5f, 0.5f);
    dataRow2 << QSurfaceDataItem(0.0f, 1.8f, 1.0f) << QSurfaceDataItem(1.0f, 1.2f, 1.0f);
    data << dataRow1 << dataRow2;

    m_proxy->resetArray(data);

    QCOMPARE(m_proxy->columnCount(), 2);
    QCOMPARE(m_proxy->rowCount(), 2);
}

void tst_proxy::initialRow()
{
    QSurfaceDataProxy proxy;
    QSurfaceDataRow row{QSurfaceDataItem{QVector3D{0, 0, 0}},
                        QSurfaceDataItem{QVector3D{1, 1, 1}}};
    proxy.addRow(QSurfaceDataRow(row));
    proxy.addRow(QSurfaceDataRow(row));
}

QTEST_MAIN(tst_proxy)
#include "tst_proxy.moc"
