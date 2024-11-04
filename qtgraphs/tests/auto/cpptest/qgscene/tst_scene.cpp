// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtGraphs/Q3DBars>
#include <QtGraphs/Q3DScene>

#include "cpptestutil.h"

class tst_scene : public QObject
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
    void invalidProperties();

    void subViews();

private:
    Q3DScene *m_scene;
};

void tst_scene::initTestCase() {}

void tst_scene::cleanupTestCase() {}

void tst_scene::init()
{
    m_scene = new Q3DScene();
}

void tst_scene::cleanup()
{
    delete m_scene;
}

void tst_scene::construct()
{
    Q3DScene *scene = new Q3DScene();
    QVERIFY(scene);
    delete scene;
}

void tst_scene::initialProperties()
{
    QVERIFY(m_scene);

    QCOMPARE(m_scene->devicePixelRatio(), 1.0f);
    QCOMPARE(m_scene->graphPositionQuery(), m_scene->invalidSelectionPoint());
    QCOMPARE(m_scene->primarySubViewport(), QRect(0, 0, 0, 0));
    QCOMPARE(m_scene->secondarySubViewport(), QRect(0, 0, 0, 0));
    QCOMPARE(m_scene->isSecondarySubviewOnTop(), true);
    QCOMPARE(m_scene->selectionQueryPosition(), m_scene->invalidSelectionPoint());
    QCOMPARE(m_scene->isSlicingActive(), false);
    QCOMPARE(m_scene->viewport(), QRect(0, 0, 0, 0));
}

void tst_scene::initializeProperties()
{
    QVERIFY(m_scene);

    m_scene->setDevicePixelRatio(2.0f);
    m_scene->setGraphPositionQuery(QPoint(0, 0));
    m_scene->setPrimarySubViewport(QRect(0, 0, 50, 50));
    m_scene->setSecondarySubViewport(QRect(50, 50, 100, 100));
    m_scene->setSecondarySubviewOnTop(false);
    m_scene->setSlicingActive(true);
    m_scene->setSelectionQueryPosition(QPoint(0, 0));

    QCOMPARE(m_scene->devicePixelRatio(), 2.0f);
    // TODO: When doing signal checks, add tests to check that queries return something (asynchronously)
    QCOMPARE(m_scene->graphPositionQuery(), QPoint(0, 0));
    QCOMPARE(m_scene->primarySubViewport(), QRect(0, 0, 50, 50));
    QCOMPARE(m_scene->secondarySubViewport(), QRect(50, 50, 100, 100));
    QCOMPARE(m_scene->isSecondarySubviewOnTop(), false);
    // TODO: When doing signal checks, add tests to check that queries return something (asynchronously)
    QCOMPARE(m_scene->selectionQueryPosition(), QPoint(0, 0));
    QCOMPARE(m_scene->isSlicingActive(), true);
    QCOMPARE(m_scene->viewport(), QRect(0, 0, 150, 150));

    m_scene->setPrimarySubViewport(QRect());
    m_scene->setSecondarySubViewport(QRect());

    QCOMPARE(m_scene->primarySubViewport(), QRect(0, 0, 30, 30));
    QCOMPARE(m_scene->secondarySubViewport(), QRect(0, 0, 150, 150));
}

void tst_scene::invalidProperties()
{
    m_scene->setPrimarySubViewport(QRect(0, 0, -50, -50));
    m_scene->setSecondarySubViewport(QRect(-50, -50, -100, -100));
    QCOMPARE(m_scene->primarySubViewport(), QRect(0, 0, 0, 0));
    QCOMPARE(m_scene->secondarySubViewport(), QRect(0, 0, 0, 0));
}

void tst_scene::subViews()
{
    if (!CpptestUtil::isOpenGLSupported())
        QSKIP("OpenGL not supported on this platform");

    Q3DBars graph;
    graph.setMinimumSize(QSize(200, 200));

    Q3DScene *scene = graph.scene();
    graph.show();

    QCoreApplication::processEvents();

    QTRY_COMPARE(scene->viewport(), QRect(0, 0, 200, 200));
    QCOMPARE(scene->primarySubViewport(), QRect(0, 0, 200, 200));
    QCOMPARE(scene->secondarySubViewport(), QRect(0, 0, 0, 0));

    QCOMPARE(scene->isSecondarySubviewOnTop(), true);
    QCOMPARE(scene->isPointInPrimarySubView(QPoint(100, 100)), true);
    QCOMPARE(scene->isPointInPrimarySubView(QPoint(201, 201)), false);
    QCOMPARE(scene->isPointInSecondarySubView(QPoint(100, 100)), false);

    scene->setSlicingActive(true);

    QCOMPARE(scene->isSecondarySubviewOnTop(), false);
    QCOMPARE(scene->primarySubViewport(), QRect(0, 0, 40, 40));
    QCOMPARE(scene->secondarySubViewport(), QRect(0, 0, 200, 200));
    QCOMPARE(scene->isPointInPrimarySubView(QPoint(100, 100)), false);
    QCOMPARE(scene->isPointInPrimarySubView(QPoint(30, 30)), true);
    QCOMPARE(scene->isPointInSecondarySubView(QPoint(100, 100)), true);
    QCOMPARE(scene->isPointInSecondarySubView(QPoint(30, 30)), false);

    scene->setSecondarySubviewOnTop(true);

    QCOMPARE(scene->isSecondarySubviewOnTop(), true);
    QCOMPARE(scene->primarySubViewport(), QRect(0, 0, 40, 40));
    QCOMPARE(scene->secondarySubViewport(), QRect(0, 0, 200, 200));
    QCOMPARE(scene->isPointInPrimarySubView(QPoint(100, 100)), false);
    QCOMPARE(scene->isPointInPrimarySubView(QPoint(30, 30)), false);
    QCOMPARE(scene->isPointInSecondarySubView(QPoint(100, 100)), true);
    QCOMPARE(scene->isPointInSecondarySubView(QPoint(30, 30)), true);
}

QTEST_MAIN(tst_scene)
#include "tst_scene.moc"
