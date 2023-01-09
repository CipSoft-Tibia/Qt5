/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <private/qquickmultipointtoucharea_p.h>
#include <private/qquickflickable_p.h>
#include <private/qquickmousearea_p.h>
#include <private/qquickwindow_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtQuick/qquickview.h>
#include <QtGui/QScreen>
#include "../../shared/util.h"
#include "../shared/viewtestutil.h"

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

class tst_QQuickMultiPointTouchArea : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickMultiPointTouchArea() { }

private slots:
    void initTestCase() {
        QQmlDataTest::initTestCase();
        if (!device) {
            device = new QTouchDevice;
            device->setType(QTouchDevice::TouchScreen);
            QWindowSystemInterface::registerTouchDevice(device);
        }
    }
    void cleanupTestCase() {}

    void properties();
    void signalTest();
    void release();
    void reuse();
    void nonOverlapping();
    void nested();
    void inFlickable();
    void inFlickable2();
    void inFlickableWithPressDelay();
    void inMouseArea();
    void mouseAsTouchpoint();
    void invisible();
    void transformedTouchArea_data();
    void transformedTouchArea();
    void mouseInteraction();
    void mouseInteraction_data();
    void mouseGestureStarted_data();
    void mouseGestureStarted();
    void cancel();
    void stationaryTouchWithChangingPressure();

private:
    QQuickView *createAndShowView(const QString &file);
    QTouchDevice *device = nullptr;
};

void tst_QQuickMultiPointTouchArea::properties()
{
    QScopedPointer<QQuickView> window(createAndShowView("properties.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window->rootObject());
    QVERIFY(area != nullptr);

    QCOMPARE(area->minimumTouchPoints(), 2);
    QCOMPARE(area->maximumTouchPoints(), 4);

    QQmlListReference ref(area, "touchPoints");
    QCOMPARE(ref.count(), 4);
}

void tst_QQuickMultiPointTouchArea::signalTest()
{
    QScopedPointer<QQuickView> window(createAndShowView("signalTest.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window->rootObject());
    QVERIFY(area != nullptr);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,100);
    QPoint p4(80,100);
    QPoint p5(100,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(window.data(), device);

    sequence.press(0, p1).press(1, p2).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(area->property("touchPointPressCount").toInt(), 2);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 2);
    QMetaObject::invokeMethod(area, "clearCounts");

    sequence.stationary(0).stationary(1).press(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(area->property("touchPointPressCount").toInt(), 1);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    sequence.move(0, p1).move(1, p2).stationary(2).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 2);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    p3 += QPoint(10,10);
    sequence.release(0, p1).release(1, p2)
            .move(2, p3).press(3, p4).press(4, p5).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(area->property("touchPointPressCount").toInt(), 2);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 1);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 2);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    sequence.release(2, p3).release(3, p4).release(4, p5).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 3);
    QCOMPARE(area->property("touchCount").toInt(), 0);
    QCOMPARE(area->property("touchUpdatedHandled").toBool(), true);
    QMetaObject::invokeMethod(area, "clearCounts");
}

void tst_QQuickMultiPointTouchArea::release()
{
    QScopedPointer<QQuickView> window(createAndShowView("basic.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickTouchPoint *point1 = window->rootObject()->findChild<QQuickTouchPoint*>("point1");

    QCOMPARE(point1->pressed(), false);

    QPoint p1(20,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(window.data(), device);

    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressed(), true);

    p1 += QPoint(0,10);

    sequence.move(0, p1).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point1->x(), qreal(20)); QCOMPARE(point1->y(), qreal(110));

    p1 += QPoint(4,10);

    sequence.release(0, p1).commit();
    QQuickTouchUtils::flush(window.data());

    //test that a release without a prior move to the release position successfully updates the point's position
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point1->x(), qreal(24)); QCOMPARE(point1->y(), qreal(120));
}

void tst_QQuickMultiPointTouchArea::reuse()
{
    QScopedPointer<QQuickView> window(createAndShowView("basic.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickTouchPoint *point1 = window->rootObject()->findChild<QQuickTouchPoint*>("point1");
    QQuickTouchPoint *point2 = window->rootObject()->findChild<QQuickTouchPoint*>("point2");
    QQuickTouchPoint *point3 = window->rootObject()->findChild<QQuickTouchPoint*>("point3");

    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,100);
    QPoint p4(80,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(window.data(), device);

    sequence.press(0, p1).press(1, p2).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), false);

    sequence.release(0, p1).stationary(1).press(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    //we shouldn't reuse point 1 yet
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), true);

    //back to base state (no touches)
    sequence.release(1, p2).release(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), false);
    QCOMPARE(point3->pressed(), false);

    sequence.press(0, p1).press(1, p2).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), false);

    sequence.release(0, p1).stationary(1).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), false);

    sequence.press(4, p4).stationary(1).commit();
    QQuickTouchUtils::flush(window.data());

    //the new touch point should reuse point 1
    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), false);

    QCOMPARE(point1->x(), qreal(80)); QCOMPARE(point1->y(), qreal(100));
}

void tst_QQuickMultiPointTouchArea::nonOverlapping()
{
    QScopedPointer<QQuickView> window(createAndShowView("nonOverlapping.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickTouchPoint *point11 = window->rootObject()->findChild<QQuickTouchPoint*>("point11");
    QQuickTouchPoint *point12 = window->rootObject()->findChild<QQuickTouchPoint*>("point12");
    QQuickTouchPoint *point21 = window->rootObject()->findChild<QQuickTouchPoint*>("point21");
    QQuickTouchPoint *point22 = window->rootObject()->findChild<QQuickTouchPoint*>("point22");
    QQuickTouchPoint *point23 = window->rootObject()->findChild<QQuickTouchPoint*>("point23");

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,180);
    QPoint p4(80,180);
    QPoint p5(100,180);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(window.data(), device);

    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    sequence.stationary(0).press(1, p2).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(100));
    QCOMPARE(point12->x(), qreal(40)); QCOMPARE(point12->y(), qreal(100));

    p1 += QPoint(0,10);
    p2 += QPoint(5,0);
    sequence.move(0, p1).move(1, p2).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));

    sequence.stationary(0).stationary(1).press(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    sequence.stationary(0).stationary(1).stationary(2).press(3, p4).press(4, p5).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(60)); QCOMPARE(point21->y(), qreal(20));
    QCOMPARE(point22->x(), qreal(80)); QCOMPARE(point22->y(), qreal(20));
    QCOMPARE(point23->x(), qreal(100)); QCOMPARE(point23->y(), qreal(20));

    p1 += QPoint(4,10);
    p2 += QPoint(17,17);
    p3 += QPoint(3,0);
    p4 += QPoint(1,-1);
    p5 += QPoint(-7,10);
    sequence.move(0, p1).move(1, p2).move(2, p3).move(3, p4).move(4, p5).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    QCOMPARE(point11->x(), qreal(24)); QCOMPARE(point11->y(), qreal(120));
    QCOMPARE(point12->x(), qreal(62)); QCOMPARE(point12->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(63)); QCOMPARE(point21->y(), qreal(20));
    QCOMPARE(point22->x(), qreal(81)); QCOMPARE(point22->y(), qreal(19));
    QCOMPARE(point23->x(), qreal(93)); QCOMPARE(point23->y(), qreal(30));

    sequence.release(0, p1).release(1, p2).release(2, p3).release(3, p4).release(4, p5).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);
}

void tst_QQuickMultiPointTouchArea::nested()
{
    QScopedPointer<QQuickView> window(createAndShowView("nested.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickTouchPoint *point11 = window->rootObject()->findChild<QQuickTouchPoint*>("point11");
    QQuickTouchPoint *point12 = window->rootObject()->findChild<QQuickTouchPoint*>("point12");
    QQuickTouchPoint *point21 = window->rootObject()->findChild<QQuickTouchPoint*>("point21");
    QQuickTouchPoint *point22 = window->rootObject()->findChild<QQuickTouchPoint*>("point22");
    QQuickTouchPoint *point23 = window->rootObject()->findChild<QQuickTouchPoint*>("point23");

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,180);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(window.data(), device);

    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    sequence.stationary(0).press(1, p2).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(100));
    QCOMPARE(point12->x(), qreal(40)); QCOMPARE(point12->y(), qreal(100));

    p1 += QPoint(0,10);
    p2 += QPoint(5,0);
    sequence.move(0, p1).move(1, p2).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));

    sequence.stationary(0).stationary(1).press(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    //point11 should be same as point21, point12 same as point22
    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(20)); QCOMPARE(point21->y(), qreal(110));
    QCOMPARE(point22->x(), qreal(45)); QCOMPARE(point22->y(), qreal(100));
    QCOMPARE(point23->x(), qreal(60)); QCOMPARE(point23->y(), qreal(180));

    sequence.stationary(0).stationary(1).stationary(2).press(3, QPoint(80,180)).press(4, QPoint(100,180)).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    //new touch points should be ignored (have no impact on our existing touch points)
    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(20)); QCOMPARE(point21->y(), qreal(110));
    QCOMPARE(point22->x(), qreal(45)); QCOMPARE(point22->y(), qreal(100));
    QCOMPARE(point23->x(), qreal(60)); QCOMPARE(point23->y(), qreal(180));

    sequence.stationary(0).stationary(1).stationary(2).release(3, QPoint(80,180)).release(4, QPoint(100,180)).commit();

    p1 += QPoint(4,10);
    p2 += QPoint(17,17);
    p3 += QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point23->x(), qreal(63)); QCOMPARE(point23->y(), qreal(180));

    p1 += QPoint(4,10);
    p2 += QPoint(17,17);
    p3 += QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    //first two remain the same (touches now grabbed by inner touch area)
    QCOMPARE(point11->x(), qreal(24)); QCOMPARE(point11->y(), qreal(120));
    QCOMPARE(point12->x(), qreal(62)); QCOMPARE(point12->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(28)); QCOMPARE(point21->y(), qreal(130));
    QCOMPARE(point22->x(), qreal(79)); QCOMPARE(point22->y(), qreal(134));
    QCOMPARE(point23->x(), qreal(66)); QCOMPARE(point23->y(), qreal(180));

    sequence.release(0, p1).release(1, p2).release(2, p3).commit();

    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    sequence.release(0, p1).commit();
    QQuickTouchUtils::flush(window.data());

    //test with grabbing turned off
    window->rootObject()->setProperty("grabInnerArea", false);

    sequence.press(0, p1).press(1, p2).press(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    p1 -= QPoint(4,10);
    p2 -= QPoint(17,17);
    p3 -= QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point23->x(), qreal(63)); QCOMPARE(point23->y(), qreal(180));

    p1 -= QPoint(4,10);
    p2 -= QPoint(17,17);
    p3 -= QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    //all change (touches not grabbed by inner touch area)
    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(20)); QCOMPARE(point21->y(), qreal(110));
    QCOMPARE(point22->x(), qreal(45)); QCOMPARE(point22->y(), qreal(100));
    QCOMPARE(point23->x(), qreal(60)); QCOMPARE(point23->y(), qreal(180));

    sequence.release(0, p1).release(1, p2).release(2, p3).commit();
    QQuickTouchUtils::flush(window.data());
}

void tst_QQuickMultiPointTouchArea::inFlickable()
{
    QScopedPointer<QQuickView> window(createAndShowView("inFlickable.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(window->rootObject());
    QVERIFY(flickable != nullptr);

    QQuickMultiPointTouchArea *mpta = window->rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta != nullptr);

    QQuickTouchPoint *point11 = window->rootObject()->findChild<QQuickTouchPoint*>("point1");
    QQuickTouchPoint *point12 = window->rootObject()->findChild<QQuickTouchPoint*>("point2");

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);

    //moving one point vertically
    QTest::touchEvent(window.data(), device).press(0, p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());

    QVERIFY(flickable->contentY() < 0);
    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);

    QTest::touchEvent(window.data(), device).release(0, p1);
    QQuickTouchUtils::flush(window.data());

    QTRY_VERIFY(!flickable->isMoving());

    //moving two points vertically
    p1 = QPoint(20,100);
    QTest::touchEvent(window.data(), device).press(0, p1).press(1, p2);
    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, p1);
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(flickable->property("cancelCount").toInt(), 0);
    QCOMPARE(flickable->property("touchCount").toInt(), 2);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    QVERIFY(flickable->contentY() < 0);
    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(flickable->property("cancelCount").toInt(), 2);
    QCOMPARE(flickable->property("touchCount").toInt(), 0);

    QTest::touchEvent(window.data(), device).release(0, p1).release(1, p2);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, p1);
    QQuickTouchUtils::flush(window.data());

    QTRY_VERIFY(!flickable->isMoving());

    //moving two points horizontally, then one point vertically
    p1 = QPoint(20,100);
    p2 = QPoint(40,100);
    QTest::touchEvent(window.data(), device).press(0, p1).press(1, p2);
    QQuickTouchUtils::flush(window.data());
    // ensure that mouse events do not fall through to the Flickable
    mpta->setMaximumTouchPoints(3);
    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, p1);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);

    p1 += QPoint(15,0); p2 += QPoint(15,0);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(15,0); p2 += QPoint(15,0);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(15,0); p2 += QPoint(15,0);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(15,0); p2 += QPoint(15,0);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1).move(1, p2);
    QTest::mouseMove(window.data(), p1);
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(flickable->contentY(), qreal(0));
    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);

    QTest::touchEvent(window.data(), device).release(0, p1).release(1, p2);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, p1);
    QQuickTouchUtils::flush(window.data());
}

// test that dragging out of a Flickable containing a MPTA doesn't harm Flickable's state.
void tst_QQuickMultiPointTouchArea::inFlickable2()
{
    QScopedPointer<QQuickView> window(createAndShowView("inFlickable2.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable != nullptr);

    QQuickTouchPoint *point11 = window->rootObject()->findChild<QQuickTouchPoint*>("point1");
    QVERIFY(point11);

    QCOMPARE(point11->pressed(), false);

    QPoint p1(50,100);

    // move point horizontally, out of Flickable area
    QTest::touchEvent(window.data(), device).press(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, p1);

    p1 += QPoint(15,0);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTest::mouseMove(window.data(), p1);

    p1 += QPoint(15,0);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTest::mouseMove(window.data(), p1);

    p1 += QPoint(15,0);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTest::mouseMove(window.data(), p1);

    p1 += QPoint(15,0);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTest::mouseMove(window.data(), p1);

    QVERIFY(!flickable->isMoving());
    QVERIFY(point11->pressed());

    QTest::touchEvent(window.data(), device).release(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, p1);
    QTest::qWait(50);

    QTRY_VERIFY(!flickable->isMoving());

    // Check that we can still move the Flickable
    p1 = QPoint(50,100);
    QTest::touchEvent(window.data(), device).press(0, p1);
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point11->pressed(), true);

    p1 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());

    p1 += QPoint(0,15);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());

    QVERIFY(flickable->contentY() < 0);
    QVERIFY(flickable->isMoving());
    QCOMPARE(point11->pressed(), false);

    QTest::touchEvent(window.data(), device).release(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTest::qWait(50);

    QTRY_VERIFY(!flickable->isMoving());
}

void tst_QQuickMultiPointTouchArea::inFlickableWithPressDelay() // QTBUG-78818
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> window(createAndShowView("inFlickable.qml"));
    QVERIFY(window->rootObject() != nullptr);
    QQuickWindowPrivate *windowPriv = QQuickWindowPrivate::get(window.data());

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(window->rootObject());
    QVERIFY(flickable != nullptr);
    flickable->setPressDelay(50);

    QQuickMultiPointTouchArea *mpta = window->rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta != nullptr);
    mpta->setMinimumTouchPoints(1);
    QQuickTouchPoint *point11 = window->rootObject()->findChild<QQuickTouchPoint*>("point1");
    QPoint p1(20,100);

    // press: Flickable prevents delivery of TouchBegin, but sends mouse press instead, after the delay.
    // MPTA handles the mouse press, and its first declared touchpoint is pressed.
    QTest::touchEvent(window.data(), device).press(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTRY_COMPARE(point11->pressed(), true);
    auto pointerEvent = windowPriv->pointerEventInstance(QQuickPointerDevice::touchDevices().at(0));
    QCOMPARE(pointerEvent->point(0)->exclusiveGrabber(), mpta);

    // release: MPTA receives TouchEnd (which is asymmetric with mouse press); does NOT emit canceled.
    QTest::touchEvent(window.data(), device).release(0, p1);
    QQuickTouchUtils::flush(window.data());
    QCOMPARE(flickable->property("cancelCount").toInt(), 0);

    // press again
    QTest::touchEvent(window.data(), device).press(0, p1);
    QQuickTouchUtils::flush(window.data());
    QTRY_COMPARE(point11->pressed(), true); // wait until pressDelay exceeded
    QCOMPARE(pointerEvent->point(0)->exclusiveGrabber(), mpta);

    // drag past the threshold: Flickable takes over the grab, MPTA gets touchUngrab and is no longer pressed
    int i = 0;
    for (; i < 10 && window->mouseGrabberItem() != flickable; ++i) {
        p1 += QPoint(0,dragThreshold);
        QTest::touchEvent(window.data(), device).move(0, p1);
        QQuickTouchUtils::flush(window.data());
    }
    QCOMPARE(window->mouseGrabberItem(), flickable);
    qCDebug(lcTests, "Flickable stole grab from MPTA after %d moves", i);
    QCOMPARE(pointerEvent->point(0)->exclusiveGrabber(), flickable);
    QCOMPARE(point11->pressed(), false);
    QVERIFY(flickable->property("cancelCount").toInt() > 0); // actually 2 because 2 touchPoints are declared... but only one was really cancelled

    // drag a little more and the Flickable moves
    p1 += QPoint(0,1);
    QTest::touchEvent(window.data(), device).move(0, p1);
    QQuickTouchUtils::flush(window.data());
    QVERIFY(flickable->contentY() < 0);
    QVERIFY(flickable->isMoving());

    QTest::touchEvent(window.data(), device).release(0, p1);
    QQuickTouchUtils::flush(window.data());

    QTRY_VERIFY(!flickable->isMoving());
}

// QTBUG-31047
void tst_QQuickMultiPointTouchArea::inMouseArea()
{
    QScopedPointer<QQuickView> window(createAndShowView("inMouseArea.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea *>(window->rootObject());
    QVERIFY(mouseArea != nullptr);

    QQuickMultiPointTouchArea *mpta = window->rootObject()->findChild<QQuickMultiPointTouchArea*>("mpta");
    QVERIFY(mpta != nullptr);

    QPoint innerPoint(40,100);
    QPoint outerPoint(10,100);

    QTest::touchEvent(window.data(), device).press(0, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QTest::touchEvent(window.data(), device).release(0, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, outerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, outerPoint);
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::touchEvent(window.data(), device).press(0, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QTest::touchEvent(window.data(), device).release(0, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());

    QTest::touchEvent(window.data(), device).press(0, outerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());
    QTest::touchEvent(window.data(), device).release(0, outerPoint);
    QVERIFY(!mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());

    // Right click should pass through
    QTest::mousePress(window.data(), Qt::RightButton, Qt::NoModifier, innerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());
    QTest::mouseRelease(window.data(), Qt::RightButton, Qt::NoModifier, innerPoint);

    mpta->setProperty("mouseEnabled", false);

    QTest::touchEvent(window.data(), device).press(0, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());
    QTest::touchEvent(window.data(), device).release(0, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, outerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, outerPoint);
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());
    QVERIFY(mouseArea->property("pressed").toBool());
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::touchEvent(window.data(), device).press(0, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QTest::touchEvent(window.data(), device).release(0, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());

    QTest::touchEvent(window.data(), device).press(0, outerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());
    QTest::touchEvent(window.data(), device).release(0, outerPoint);
    QVERIFY(!mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());
}

void tst_QQuickMultiPointTouchArea::mouseAsTouchpoint()
{
    QScopedPointer<QQuickView> window(createAndShowView("dualGestures.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickMultiPointTouchArea *dualmpta = window->rootObject()->findChild<QQuickMultiPointTouchArea*>("dualTouchArea");
    QVERIFY(dualmpta != nullptr);

    QQuickItem *touch1rect = window->rootObject()->findChild<QQuickItem*>("touch1rect");
    QQuickItem *touch2rect = window->rootObject()->findChild<QQuickItem*>("touch2rect");
    QQuickItem *touch3rect = window->rootObject()->findChild<QQuickItem*>("touch3rect");
    QQuickItem *touch4rect = window->rootObject()->findChild<QQuickItem*>("touch4rect");
    QQuickItem *touch5rect = window->rootObject()->findChild<QQuickItem*>("touch5rect");

    {
        QPoint touch1(40,10);
        QPoint touch2(40,100);
        QPoint touch3(10,10);

        // Touch both, release one, manipulate other touchpoint with mouse
        QTest::touchEvent(window.data(), device).press(1, touch1);
        QQuickTouchUtils::flush(window.data());
        QTest::touchEvent(window.data(), device).move(1, touch1).press(2, touch2);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
        QTest::touchEvent(window.data(), device).release(1, touch1).move(2, touch2);
        touch1.setY(20);
        QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, touch1);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
        QTest::touchEvent(window.data(), device).release(2, touch2);
        QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, touch1);
        QQuickTouchUtils::flush(window.data());

        // Start with mouse, move it, touch second point, move it
        QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, touch1);
        touch1.setX(60);
        QTest::mouseMove(window.data(), touch1);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        touch2.setX(60);
        QTest::touchEvent(window.data(), device).press(3, touch2);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
        touch2.setY(150);
        QTest::touchEvent(window.data(), device).move(3, touch2);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());

        // Touch third point - nothing happens
        QTest::touchEvent(window.data(), device).press(4, touch3);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());

        // Release all
        QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, touch1);
        QTest::touchEvent(window.data(), device).release(3, touch2);
        QQuickTouchUtils::flush(window.data());
        QTest::touchEvent(window.data(), device).release(4, touch3);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
    }

    // Mouse and touch on left, touch 3 points on right
    {
        QPoint mouse1(40,10);
        QPoint touch1(10,10);
        QPoint touch2(340,10);
        QPoint touch3(340,100);
        QPoint touch4(540,10);

        QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, mouse1);
        QCOMPARE(touch1rect->property("x").toInt(), mouse1.x());
        QCOMPARE(touch1rect->property("y").toInt(), mouse1.y());
        QTest::touchEvent(window.data(), device).press(1, touch1);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), mouse1.x());
        QCOMPARE(touch1rect->property("y").toInt(), mouse1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch1.y());

        QTest::touchEvent(window.data(), device).press(2, touch2).press(3, touch3).press(4, touch4);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), mouse1.x());
        QCOMPARE(touch1rect->property("y").toInt(), mouse1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch3rect->property("x").toInt(), touch2.x() - 320);
        QCOMPARE(touch3rect->property("y").toInt(), touch2.y());
        QCOMPARE(touch4rect->property("x").toInt(), touch3.x() - 320);
        QCOMPARE(touch4rect->property("y").toInt(), touch3.y());
        QCOMPARE(touch5rect->property("x").toInt(), touch4.x() - 320);
        QCOMPARE(touch5rect->property("y").toInt(), touch4.y());

        // Release all
        QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, mouse1);
        QTest::touchEvent(window.data(), device).release(1, touch1).release(2, touch2).release(3, touch3).release(4, touch4);
        QQuickTouchUtils::flush(window.data());
    }

    dualmpta->setProperty("mouseEnabled", false);
    {
        QPoint mouse1(40,10);
        QPoint touch1(10,10);
        QPoint touch2(100,10);

        touch1rect->setX(10);
        touch1rect->setY(10);
        touch2rect->setX(20);
        touch2rect->setY(10);

        // Start with mouse, move it, touch a point, move it, touch another.
        // Mouse is ignored, both touch points are heeded.
        QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, mouse1);
        mouse1.setX(60);
        QTest::mouseMove(window.data(), mouse1);
        QCOMPARE(touch1rect->property("x").toInt(), 10);
        QCOMPARE(touch1rect->property("y").toInt(), 10);

        QTest::touchEvent(window.data(), device).press(1, touch1);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        touch1.setY(150);
        QTest::touchEvent(window.data(), device).move(1, touch1);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QTest::touchEvent(window.data(), device).press(2, touch2);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());

        // Release all
        QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, mouse1);
        QTest::touchEvent(window.data(), device).release(1, touch1);
        QQuickTouchUtils::flush(window.data());
        QTest::touchEvent(window.data(), device).release(2, touch2);
        QQuickTouchUtils::flush(window.data());
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
    }
}

// QTBUG-23327
void tst_QQuickMultiPointTouchArea::invisible()
{
    QScopedPointer<QQuickView> window(createAndShowView("signalTest.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window->rootObject());
    QVERIFY(area != nullptr);

    area->setVisible(false);

    QPoint p1(20,100);
    QPoint p2(40,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(window.data(), device);

    sequence.press(0, p1).press(1, p2).commit();

    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 0);
}

void tst_QQuickMultiPointTouchArea::transformedTouchArea_data()
{
    QTest::addColumn<QPoint>("p1");
    QTest::addColumn<QPoint>("p2");
    QTest::addColumn<QPoint>("p3");
    QTest::addColumn<int>("total1");
    QTest::addColumn<int>("total2");
    QTest::addColumn<int>("total3");

    QTest::newRow("1st point inside")
        << QPoint(140, 200) << QPoint(260, 260) << QPoint(0, 140) << 1 << 1 << 1;

    QTest::newRow("2nd point inside")
        << QPoint(260, 260) << QPoint(200, 200) << QPoint(0, 0) << 0 << 1 << 1;

    QTest::newRow("3rd point inside")
        << QPoint(140, 260) << QPoint(260, 140) << QPoint(200, 140) << 0 << 0 << 1;
    QTest::newRow("all points inside")
        << QPoint(200, 140) << QPoint(200, 260) << QPoint(140, 200) << 1 << 2 << 3;

    QTest::newRow("all points outside")
        << QPoint(140, 140) << QPoint(260, 260) << QPoint(260, 140) << 0 << 0 << 0;

    QTest::newRow("1st and 2nd points inside")
        << QPoint(200, 260) << QPoint(200, 140) << QPoint(140, 140) << 1 << 2 << 2;

    QTest::newRow("1st and 3rd points inside")
        << QPoint(200, 200) << QPoint(0, 0) << QPoint(200, 260) << 1 << 1 << 2;
}

void tst_QQuickMultiPointTouchArea::transformedTouchArea()
{
    QFETCH(QPoint, p1);
    QFETCH(QPoint, p2);
    QFETCH(QPoint, p3);
    QFETCH(int, total1);
    QFETCH(int, total2);
    QFETCH(int, total3);


    QScopedPointer<QQuickView> view(createAndShowView("transformedMultiPointTouchArea.qml"));
    QVERIFY(view->rootObject() != nullptr);

    QQuickMultiPointTouchArea *area = view->rootObject()->findChild<QQuickMultiPointTouchArea *>("touchArea");
    QVERIFY(area != nullptr);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(view.data(), device);

    sequence.press(0, p1).commit();
    QCOMPARE(area->property("pointCount").toInt(), total1);

    sequence.stationary(0).press(1, p2).commit();
    QCOMPARE(area->property("pointCount").toInt(), total2);

    sequence.stationary(0).stationary(1).press(2, p3).commit();
    QCOMPARE(area->property("pointCount").toInt(), total3);
}

QQuickView *tst_QQuickMultiPointTouchArea::createAndShowView(const QString &file)
{
    QScopedPointer<QQuickView> window(new QQuickView(nullptr));
    window->setSource(testFileUrl(file));
    if (window->status() != QQuickView::Ready)
        return nullptr;
    const QRect screenGeometry = window->screen()->availableGeometry();
    const QSize size = window->size();
    const QPoint offset = QPoint(size.width() / 2, size.height() / 2);
    window->setFramePosition(screenGeometry.center() - offset);
    window->show();
    if (!QTest::qWaitForWindowExposed(window.data()))
        return nullptr;
    return window.take();
}

void tst_QQuickMultiPointTouchArea::mouseInteraction_data()
{
    QTest::addColumn<int>("buttons");
    QTest::addColumn<int>("accept");

    QTest::newRow("left") << (int) Qt::LeftButton << 1;
    QTest::newRow("right") << (int) Qt::RightButton << 0;
    QTest::newRow("middle") << (int) Qt::MiddleButton << 0;
}

void tst_QQuickMultiPointTouchArea::mouseInteraction()
{
    QFETCH(int, buttons);
    QFETCH(int, accept);

    QScopedPointer<QQuickView> view(createAndShowView("mouse.qml"));
    QVERIFY(view->rootObject() != nullptr);

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(view->rootObject());
    QVERIFY(area != nullptr);
    QQuickTouchPoint *point1 = view->rootObject()->findChild<QQuickTouchPoint*>("point1");
    QCOMPARE(point1->pressed(), false);

    QCOMPARE(area->property("touchCount").toInt(), 0);
    QPoint p1 = QPoint(100, 100);
    QTest::mousePress(view.data(), (Qt::MouseButton) buttons, Qt::NoModifier, p1);
    QCOMPARE(area->property("touchCount").toInt(), accept);
    QCOMPARE(point1->pressed(), accept != 0);
    p1 += QPoint(10, 10);
    QTest::mouseMove(view.data(), p1);
    QCOMPARE(point1->pressed(), accept != 0);
    QCOMPARE(area->property("touchCount").toInt(), accept);
    p1 += QPoint(10, 10);
    QTest::mouseMove(view.data(), p1);
    QCOMPARE(point1->pressed(), accept != 0);
    QCOMPARE(area->property("touchCount").toInt(), accept);
    QTest::mouseRelease(view.data(), (Qt::MouseButton) buttons);
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(area->property("touchCount").toInt(), 0);
}

void tst_QQuickMultiPointTouchArea::mouseGestureStarted_data()
{
    QTest::addColumn<bool>("grabGesture");
    QTest::addColumn<int>("distanceFromOrigin");

    QTest::newRow("near origin, don't grab") << false << 4;
    QTest::newRow("near origin, grab") << true << 4;
    QTest::newRow("away from origin, don't grab") << false << 100;
    QTest::newRow("away from origin, grab") << true << 100;
}

void tst_QQuickMultiPointTouchArea::mouseGestureStarted() // QTBUG-70258
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QFETCH(bool, grabGesture);
    QFETCH(int, distanceFromOrigin);

    QScopedPointer<QQuickView> view(createAndShowView("mouse.qml"));
    QVERIFY(view->rootObject() != nullptr);

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(view->rootObject());
    QVERIFY(area);
    area->setProperty("grabGesture", grabGesture);
    QQuickTouchPoint *point1 = view->rootObject()->findChild<QQuickTouchPoint*>("point1");
    QCOMPARE(point1->pressed(), false);
    QSignalSpy gestureStartedSpy(area, SIGNAL(gestureStarted(QQuickGrabGestureEvent *)));

    QPoint p1 = QPoint(distanceFromOrigin, distanceFromOrigin);
    QTest::mousePress(view.data(), Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(gestureStartedSpy.count(), 0);

    p1 += QPoint(dragThreshold, dragThreshold);
    QTest::mouseMove(view.data(), p1);
    QCOMPARE(gestureStartedSpy.count(), 0);

    p1 += QPoint(1, 1);
    QTest::mouseMove(view.data(), p1);
    QTRY_COMPARE(gestureStartedSpy.count(), 1);
    QTRY_COMPARE(area->property("gestureStartedX").toInt(), distanceFromOrigin);
    QCOMPARE(area->property("gestureStartedY").toInt(), distanceFromOrigin);

    p1 += QPoint(10, 10);
    QTest::mouseMove(view.data(), p1);
    // if nobody called gesteure->grab(), gestureStarted will keep happening
    QTRY_COMPARE(gestureStartedSpy.count(), grabGesture ? 1 : 2);
    QCOMPARE(area->property("gestureStartedX").toInt(), distanceFromOrigin);
    QCOMPARE(area->property("gestureStartedY").toInt(), distanceFromOrigin);

    QTest::mouseRelease(view.data(), Qt::LeftButton);
}

void tst_QQuickMultiPointTouchArea::cancel()
{
    QScopedPointer<QQuickView> window(createAndShowView("cancel.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window->rootObject());
    QTest::QTouchEventSequence sequence = QTest::touchEvent(window.data(), device);
    QQuickTouchPoint *point1 = area->findChild<QQuickTouchPoint*>("point1");

    QPoint p1(20,100);
    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(window.data());
    QCOMPARE(point1->pressed(), true);
    QCOMPARE(area->property("touchPointPressCount").toInt(), 1);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchPointCancelCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 1);
    QMetaObject::invokeMethod(area, "clearCounts");

    area->setVisible(false);
    // we should get a onCancel signal
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchPointCancelCount").toInt(), 1);
    QCOMPARE(area->property("touchCount").toInt(), 0);
    QMetaObject::invokeMethod(area, "clearCounts");
    area->setVisible(true);


    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(window.data());
    QCOMPARE(point1->pressed(), true);
    QCOMPARE(area->property("touchPointPressCount").toInt(), 1);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchPointCancelCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 1);
    QMetaObject::invokeMethod(area, "clearCounts");

    area->setEnabled(false);
    // we should get a onCancel signal
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchPointCancelCount").toInt(), 1);
    QCOMPARE(area->property("touchCount").toInt(), 0);
    QMetaObject::invokeMethod(area, "clearCounts");

}

void tst_QQuickMultiPointTouchArea::stationaryTouchWithChangingPressure() // QTBUG-77142
{
    QScopedPointer<QQuickView> window(createAndShowView("basic.qml"));
    QVERIFY(window->rootObject() != nullptr);

    QQuickTouchPoint *point1 = window->rootObject()->findChild<QQuickTouchPoint*>("point1");
    QCOMPARE(point1->pressed(), false);

    QPoint p1(20,100);
    QTouchEvent::TouchPoint tp1(1);

    tp1.setScreenPos(window->mapToGlobal(p1));
    tp1.setState(Qt::TouchPointPressed);
    tp1.setPressure(0.5);
    qt_handleTouchEvent(window.data(), device, {tp1});
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point1->pressure(), 0.5);

    tp1.setState(Qt::TouchPointStationary);
    tp1.setPressure(0.6);
    qt_handleTouchEvent(window.data(), device, {tp1});
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressure(), 0.6);

    tp1.setState(Qt::TouchPointReleased);
    tp1.setPressure(0);
    qt_handleTouchEvent(window.data(), device, {tp1});
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point1->pressure(), 0);
}


QTEST_MAIN(tst_QQuickMultiPointTouchArea)

#include "tst_qquickmultipointtoucharea.moc"
