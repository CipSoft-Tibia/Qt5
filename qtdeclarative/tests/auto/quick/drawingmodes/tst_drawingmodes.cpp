// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qsgnode.h>
#include <QtQuick/qsggeometry.h>
#include <QtQuick/qsgflatcolormaterial.h>
#include <QtGui/qscreen.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_drawingmodes : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_drawingmodes();

    bool hasPixelAround(const QImage &fb, int centerX, int centerY);
    QImage runTest(const QString &fileName)
    {
        QQuickView view(&outerWindow);
        view.setResizeMode(QQuickView::SizeViewToRootObject);
        view.setSource(testFileUrl(fileName));
        view.setVisible(true);
        bool exposed = QTest::qWaitForWindowExposed(&view);
        return exposed ? view.grabWindow() : QImage();
    }

    //It is important for platforms that only are able to show fullscreen windows
    //to have a container for the window that is painted on.
    QQuickWindow outerWindow;
    const QRgb black;
    const QRgb red;

private slots:
    void points();
    void lines();
    void lineStrip();
    void lineLoop();
    void triangles();
    void triangleStrip();
    void triangleFan();

private:
    bool isRunningOnRhi() const;
};

class DrawingModeItem : public QQuickItem
{
    Q_OBJECT
public:
    static QSGGeometry::DrawingMode drawingMode;

    DrawingModeItem() : first(QSGGeometry::defaultAttributes_Point2D(), 5),
        second(QSGGeometry::defaultAttributes_Point2D(), 5)
    {
        setFlag(ItemHasContents, true);
        material.setColor(Qt::red);
    }

protected:
    QSGGeometry first;
    QSGGeometry second;
    QSGFlatColorMaterial material;

     QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override
    {
        if (!node) {
            QRect bounds(0, 0, 200, 200);
            first.setDrawingMode(drawingMode);
            second.setDrawingMode(drawingMode);

            QSGGeometry::Point2D *v = first.vertexDataAsPoint2D();
            v[0].set(bounds.width() * 2 / 8, bounds.height() / 2);
            v[1].set(bounds.width() / 8, bounds.height() / 4);
            v[2].set(bounds.width() * 3 / 8, bounds.height() / 4);
            v[3].set(bounds.width() * 3 / 8, bounds.height() * 3 / 4);
            v[4].set(bounds.width() / 8, bounds.height() * 3 / 4);

            v = second.vertexDataAsPoint2D();
            v[0].set(bounds.width() * 6 / 8, bounds.height() / 2);
            v[1].set(bounds.width() * 5 / 8, bounds.height() / 4);
            v[2].set(bounds.width() * 7 / 8, bounds.height() / 4);
            v[3].set(bounds.width() * 7 / 8, bounds.height() * 3 / 4);
            v[4].set(bounds.width() * 5 / 8, bounds.height() * 3 / 4);

            node = new QSGNode;
            QSGGeometryNode *child = new QSGGeometryNode;
            child->setGeometry(&first);
            child->setMaterial(&material);
            node->appendChildNode(child);
            child = new QSGGeometryNode;
            child->setGeometry(&second);
            child->setMaterial(&material);
            node->appendChildNode(child);
        }
        return node;
    }
};

QSGGeometry::DrawingMode DrawingModeItem::drawingMode;

bool tst_drawingmodes::hasPixelAround(const QImage &fb, int centerX, int centerY) {
    for (int x = centerX - 2; x <= centerX + 2; ++x) {
        for (int y = centerY - 2; y <= centerY + 2; ++y) {
            if (fb.pixel(x, y) == red)
                return true;
        }
    }
    return false;
}

tst_drawingmodes::tst_drawingmodes()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
    , black(qRgb(0, 0, 0))
    , red(qRgb(0xff, 0, 0))
{
    qmlRegisterType<DrawingModeItem>("Test", 1, 0, "DrawingModeItem");
    outerWindow.showNormal();
    outerWindow.setGeometry(0,0,400,400);
}

void tst_drawingmodes::points()
{
    DrawingModeItem::drawingMode = QSGGeometry::DrawPoints;
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    QSKIP("Skipping points test due to unexpected failures in M1 CI VM");
#endif

    QImage fb = runTest("DrawingModes.qml");

    QVERIFY(hasPixelAround(fb, 50, 100));
    QVERIFY(hasPixelAround(fb, 25, 50));
    QVERIFY(hasPixelAround(fb, 75, 50));
    QVERIFY(hasPixelAround(fb, 75, 150));
    QVERIFY(hasPixelAround(fb, 25, 150));

    QVERIFY(hasPixelAround(fb, 150, 100));
    QVERIFY(hasPixelAround(fb, 125, 50));
    QVERIFY(hasPixelAround(fb, 175, 50));
    QVERIFY(hasPixelAround(fb, 175, 150));
    QVERIFY(hasPixelAround(fb, 125, 150));

    QVERIFY(!hasPixelAround(fb, 135, 70));
    QVERIFY(!hasPixelAround(fb, 175, 100));
    QVERIFY(!hasPixelAround(fb, 110, 140));
    QVERIFY(!hasPixelAround(fb, 50, 50));
    QVERIFY(!hasPixelAround(fb, 50, 150));
    QVERIFY(!hasPixelAround(fb, 25, 100));
    QVERIFY(!hasPixelAround(fb, 75, 100));
    QVERIFY(!hasPixelAround(fb, 125, 100));
    QVERIFY(!hasPixelAround(fb, 150, 50));
    QVERIFY(!hasPixelAround(fb, 150, 150));
    QVERIFY(!hasPixelAround(fb, 135, 130));
    QVERIFY(!hasPixelAround(fb, 35, 130));
}

void tst_drawingmodes::lines()
{
    DrawingModeItem::drawingMode = QSGGeometry::DrawLines;
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    QImage fb = runTest("DrawingModes.qml");

    QCOMPARE(fb.width(), 200);
    QCOMPARE(fb.height(), 200);

    QVERIFY(hasPixelAround(fb, 135, 70));
    QVERIFY(hasPixelAround(fb, 175, 100));
    QVERIFY(!hasPixelAround(fb, 110, 140));
    QVERIFY(!hasPixelAround(fb, 50, 50));
    QVERIFY(!hasPixelAround(fb, 50, 150));

    QVERIFY(hasPixelAround(fb, 35, 70));
    QVERIFY(hasPixelAround(fb, 75, 100));
    QVERIFY(!hasPixelAround(fb, 25, 100));
    QVERIFY(!hasPixelAround(fb, 125, 100));
    QVERIFY(!hasPixelAround(fb, 150, 50));
    QVERIFY(!hasPixelAround(fb, 150, 150));
    QVERIFY(!hasPixelAround(fb, 135, 130));
    QVERIFY(!hasPixelAround(fb, 35, 130));
}

void tst_drawingmodes::lineStrip()
{
    DrawingModeItem::drawingMode = QSGGeometry::DrawLineStrip;
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    QImage fb = runTest("DrawingModes.qml");

    QCOMPARE(fb.width(), 200);
    QCOMPARE(fb.height(), 200);

    QVERIFY(hasPixelAround(fb, 135, 70));
    QVERIFY(hasPixelAround(fb, 150, 50));
    QVERIFY(hasPixelAround(fb, 175, 100));
    QVERIFY(hasPixelAround(fb, 150, 150));

    QVERIFY(hasPixelAround(fb, 35, 70));
    QVERIFY(hasPixelAround(fb, 50, 50));
    QVERIFY(hasPixelAround(fb, 75, 100));
    QVERIFY(hasPixelAround(fb, 50, 150));

    QVERIFY(!hasPixelAround(fb, 110, 140)); // bad line not there => line strip unbatched

    QVERIFY(!hasPixelAround(fb, 25, 100));
    QVERIFY(!hasPixelAround(fb, 125, 100));
    QVERIFY(!hasPixelAround(fb, 135, 130));
    QVERIFY(!hasPixelAround(fb, 35, 130));
}

void tst_drawingmodes::lineLoop()
{
    DrawingModeItem::drawingMode = QSGGeometry::DrawLineLoop;
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    if (isRunningOnRhi())
        QSKIP("Line loops are not supported by some modern graphics APIs - skipping test");

    QImage fb = runTest("DrawingModes.qml");

    QCOMPARE(fb.width(), 200);
    QCOMPARE(fb.height(), 200);

    QVERIFY(hasPixelAround(fb, 135, 70));
    QVERIFY(hasPixelAround(fb, 135, 130));
    QVERIFY(hasPixelAround(fb, 150, 50));
    QVERIFY(hasPixelAround(fb, 175, 100));
    QVERIFY(hasPixelAround(fb, 150, 150));

    QVERIFY(hasPixelAround(fb, 35, 70));
    QVERIFY(hasPixelAround(fb, 35, 130));
    QVERIFY(hasPixelAround(fb, 50, 50));
    QVERIFY(hasPixelAround(fb, 75, 100));
    QVERIFY(hasPixelAround(fb, 50, 150));

    QVERIFY(!hasPixelAround(fb, 110, 140)); // bad line not there => line loop unbatched

    QVERIFY(!hasPixelAround(fb, 25, 100));
    QVERIFY(!hasPixelAround(fb, 125, 100));
}

void tst_drawingmodes::triangles()
{
    DrawingModeItem::drawingMode = QSGGeometry::DrawTriangles;
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    QImage fb = runTest("DrawingModes.qml");

    QCOMPARE(fb.width(), 200);
    QCOMPARE(fb.height(), 200);

    QVERIFY(hasPixelAround(fb, 150, 75));
    QVERIFY(!hasPixelAround(fb, 162, 100));
    QVERIFY(!hasPixelAround(fb, 150, 125));
    QVERIFY(!hasPixelAround(fb, 137, 100));

    QVERIFY(!hasPixelAround(fb, 100, 125));

    QVERIFY(hasPixelAround(fb, 50, 75));
    QVERIFY(!hasPixelAround(fb, 62, 100));
    QVERIFY(!hasPixelAround(fb, 50, 125));
    QVERIFY(!hasPixelAround(fb, 37, 100));
}


void tst_drawingmodes::triangleStrip()
{
    DrawingModeItem::drawingMode = QSGGeometry::DrawTriangleStrip;
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    QImage fb = runTest("DrawingModes.qml");

    QCOMPARE(fb.width(), 200);
    QCOMPARE(fb.height(), 200);

    QVERIFY(hasPixelAround(fb, 150, 75));
    QVERIFY(hasPixelAround(fb, 162, 100));
    QVERIFY(hasPixelAround(fb, 150, 125));
    QVERIFY(!hasPixelAround(fb, 137, 100));

    QVERIFY(!hasPixelAround(fb, 100, 125)); // batching avoids extra triangle by duplicating vertices.

    QVERIFY(hasPixelAround(fb, 50, 75));
    QVERIFY(hasPixelAround(fb, 62, 100));
    QVERIFY(hasPixelAround(fb, 50, 125));
    QVERIFY(!hasPixelAround(fb, 37, 100));
}

void tst_drawingmodes::triangleFan()
{
    DrawingModeItem::drawingMode = QSGGeometry::DrawTriangleFan;
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    if (isRunningOnRhi())
        QSKIP("Triangle fans are not supported by some modern graphics APIs - skipping test");

    QImage fb = runTest("DrawingModes.qml");

    QCOMPARE(fb.width(), 200);
    QCOMPARE(fb.height(), 200);

    QVERIFY(hasPixelAround(fb, 150, 75));
    QVERIFY(hasPixelAround(fb, 162, 100));
    QVERIFY(hasPixelAround(fb, 150, 125));
    QVERIFY(!hasPixelAround(fb, 137, 100));

    QVERIFY(!hasPixelAround(fb, 100, 125)); // no extra triangle; triangle fan is not batched

    QVERIFY(hasPixelAround(fb, 50, 75));
    QVERIFY(hasPixelAround(fb, 62, 100));
    QVERIFY(hasPixelAround(fb, 50, 125));
    QVERIFY(!hasPixelAround(fb, 37, 100));
}

bool tst_drawingmodes::isRunningOnRhi() const
{
    static bool retval = false;
    static bool decided = false;
    if (!decided) {
        decided = true;
        QQuickView dummy;
        dummy.show();
        if (QTest::qWaitForWindowExposed(&dummy)) {
            QSGRendererInterface::GraphicsApi api = dummy.rendererInterface()->graphicsApi();
            retval = QSGRendererInterface::isApiRhiBased(api);
        }
        dummy.hide();
    }
    return retval;
}


QTEST_MAIN(tst_drawingmodes)

#include "tst_drawingmodes.moc"
