// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtGui/QWindow>
#include <QtGui/QCursor>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE
namespace QTestPrivate {
extern Q_TESTLIB_EXPORT Qt::MouseButtons qtestMouseButtons; // from qtestcase.cpp
}
QT_END_NAMESPACE

class tst_Mouse : public QObject
{
    Q_OBJECT

private slots:
    void timestampBetweenTestFunction_data();
    void timestampBetweenTestFunction();
    void stateHandlingPart1_data();
    void stateHandlingPart1();
    void stateHandlingPart2();
    void deterministicEvents_data();
    void deterministicEvents();
    void doubleClick();
};

class MouseWindow : public QWindow
{
public:
    Qt::MouseButtons stateInMouseMove = Qt::NoButton;
    int moveCount = 0;
    int pressCount = 0;
    int doubleClickCount = 0;
    ulong lastTimeStamp = 0;

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        pressCount++;
        processEvent(e);
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        moveCount++;
        stateInMouseMove = e->buttons();
        processEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        processEvent(e);
    }

    void mouseDoubleClickEvent(QMouseEvent *e) override
    {
        doubleClickCount++;
        processEvent(e);
    }

    void processEvent(QMouseEvent *e)
    {
        lastTimeStamp = e->timestamp();
    }

};

static ulong lastTimeStampInPreviousTestFunction = 0;

void tst_Mouse::timestampBetweenTestFunction_data()
{
    QTest::addColumn<bool>("hoverLast");
    QTest::addColumn<bool>("pressAndRelease");
    QTest::newRow("press, release") << true << false;
    QTest::newRow("press, release, hover") << true << true;
    QTest::newRow("hover") << false << true;
    QTest::newRow("hover #2") << false << true;
    QTest::newRow("press, release #2") << true << false;
    QTest::newRow("press, release, hover #2") << true << true;
}

void tst_Mouse::timestampBetweenTestFunction()
{
    QFETCH(bool, hoverLast);
    QFETCH(bool, pressAndRelease);

    MouseWindow w;
    w.show();
    w.setGeometry(100, 100, 200, 200);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QPoint point(10, 10);
    QCOMPARE(w.pressCount, 0);
    if (pressAndRelease) {
        QTest::mousePress(&w, Qt::LeftButton, { }, point);
        QVERIFY(w.lastTimeStamp - lastTimeStampInPreviousTestFunction > 500);   // Should be at least 500 ms timestamp between each test case
        QCOMPARE(w.pressCount, 1);
        QTest::mouseRelease(&w, Qt::LeftButton, { }, point);
    }
    QCOMPARE(w.doubleClickCount, 0);
    if (hoverLast) {
        static int xMove = 0;
        xMove += 5;     // Just make sure we generate different hover coordinates
        point.rx() += xMove;
        QTest::mouseMove(&w, point);     // a hover move. This doesn't generate a timestamp delay of 500 ms
    }
    lastTimeStampInPreviousTestFunction = w.lastTimeStamp;
}

void tst_Mouse::stateHandlingPart1_data()
{
    QTest::addColumn<bool>("dummy");
    QTest::newRow("dummy-1") << true;
    QTest::newRow("dummy-2") << true;
}

void tst_Mouse::stateHandlingPart1()
{
    QFETCH(bool, dummy);
    Q_UNUSED(dummy);

    QWindow w;
    w.setFlags(w.flags() | Qt::FramelessWindowHint); // ### FIXME: QTBUG-63542
    w.show();
    w.setGeometry(100, 100, 200, 200);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QPoint point(10, 10);
    QPoint step(1, 1);

    // verify that we have a clean state after the previous data set
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::NoButton);

    QTest::mousePress(&w, Qt::LeftButton, { }, point);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::LeftButton);
    QTest::mousePress(&w, Qt::RightButton, { }, point);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::LeftButton | Qt::RightButton);
    QTest::mouseMove(&w, point += step);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::LeftButton | Qt::RightButton);
    QTest::mouseRelease(&w, Qt::LeftButton, { }, point);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::RightButton);
    QTest::mouseMove(&w, point += step);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::RightButton);
    // test invalid input - left button was already released
    QTest::mouseRelease(&w, Qt::LeftButton, { }, point += point);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::RightButton);
    // test invalid input - right button is already pressed
    QTest::mousePress(&w, Qt::RightButton, { }, point);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::RightButton);
    // now continue with valid input
    QTest::mouseRelease(&w, Qt::RightButton, { }, point += point);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::NoButton);
    QTest::mouseMove(&w, point += step);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::NoButton);

    // exit this test function with some button in a pressed state
    QTest::mousePress(&w, Qt::LeftButton, { }, point);
    QTest::mousePress(&w, Qt::RightButton, { }, point);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::LeftButton | Qt::RightButton);
}

void tst_Mouse::stateHandlingPart2()
{
    MouseWindow w;
    w.setFlags(w.flags() | Qt::FramelessWindowHint); // ### FIXME: QTBUG-63542
    w.show();
    w.setGeometry(100, 100, 200, 200);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // verify that we have a clean state after stateHandlingPart1()
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::NoButton);

#if !QT_CONFIG(cursor)
    QSKIP("This part of the test requires the QCursor API");
#else
    // The windowing system's view on a current button state might be different
    // from the qtestlib's mouse button state. This test verifies that the mouse
    // events generated by the system are adjusted to reflect qtestlib's view
    // on the current button state.
    // SKIP: not convinced yet that there is a valid use case for this.

    QSKIP("Not implemented beyond this point!");

    QPoint point(40, 40);
    QTest::mousePress(&w, Qt::LeftButton, { }, point);
    QTest::mousePress(&w, Qt::RightButton, { }, point);
    QCOMPARE(QTestPrivate::qtestMouseButtons, Qt::LeftButton | Qt::RightButton);
    w.moveCount = 0;
    // The windowing system will send mouse events with no buttons set
    QPoint moveToPoint = w.mapToGlobal(point + QPoint(1, 1));
    if (QCursor::pos() == moveToPoint)
        moveToPoint += QPoint(1, 1);
    QCursor::setPos(moveToPoint);
    QTRY_COMPARE(w.moveCount, 1);
    // Verify that qtestlib adjusted the button state
    QCOMPARE(w.stateInMouseMove, Qt::LeftButton | Qt::RightButton);
#endif
}

void tst_Mouse::deterministicEvents_data()
{
    QTest::addColumn<bool>("firstRun");
    QTest::newRow("first-run-true") << true;
    QTest::newRow("first-run-false") << false;
}

void tst_Mouse::deterministicEvents()
{
    /* QGuiApplication uses QGuiApplicationPrivate::lastCursorPosition to
    determine if it needs to generate an additional mouse move event for
    mouse press/release. Verify that this property is reset to it's default
    value, ensuring deterministic event generation behavior. Not resetting
    this value might affect event generation for subsequent tests runs (in
    unlikely case where a subsquent test does a mouse press in a pos that is
    equal to QGuiApplicationPrivate::lastCursorPosition, not causing mouse
    move to be generated.
    NOTE: running this test alone as in "./mouse deterministicEvents:first-run-false"
    won't test what this test is designed to test. */

    QSKIP("Not implemented!");

    /* It is undecided how and at what scope we want to handle reseting
    lastCursorPosition, or perhaps Qt should not be generating mouse move
    events as documented in QGuiApplicationPrivate::processMouseEvent(),
    then the problem would go away - ### Qt6 ? */

    QVERIFY(qIsInf(QGuiApplicationPrivate::lastCursorPosition.x()));
    QVERIFY(qIsInf(QGuiApplicationPrivate::lastCursorPosition.y()));

    QFETCH(bool, firstRun);

    MouseWindow w;
    w.setFlags(w.flags() | Qt::FramelessWindowHint); // ### FIXME: QTBUG-63542
    w.show();
    w.setGeometry(100, 100, 200, 200);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QCOMPARE(w.pressCount, 0);
    QCOMPARE(w.moveCount, 0);
    static QPoint m_cachedLastCursorPosition;
    if (firstRun) {
        QTest::mousePress(&w, Qt::LeftButton, { }, QPoint(40, 40));
        m_cachedLastCursorPosition = QGuiApplicationPrivate::lastCursorPosition.toPoint();
    } else {
        QPoint point = w.mapFromGlobal(m_cachedLastCursorPosition);
        QTest::mousePress(&w, Qt::LeftButton, { }, point);
    }
    QCOMPARE(w.pressCount, 1);
    QCOMPARE(w.moveCount, 1);
}

void tst_Mouse::doubleClick()
{
    MouseWindow w;
    w.show();
    w.setGeometry(100, 100, 200, 200);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // click
    QPoint point(10, 10);
    QCOMPARE(w.pressCount, 0);
    QTest::mousePress(&w, Qt::LeftButton, { }, point);
    QCOMPARE(w.pressCount, 1);
    // give a delay of 10ms
    auto ts = w.lastTimeStamp;
    QTest::mouseRelease(&w, Qt::LeftButton, { }, point, 10);
    QCOMPARE(w.lastTimeStamp, ts + 10);
    QCOMPARE(w.doubleClickCount, 0);

    // click again within a short time to generate double-click
    QTest::mousePress(&w, Qt::LeftButton, { }, point, 10);
    QCOMPARE(w.pressCount, 2);
    QCOMPARE(w.lastTimeStamp, ts + 20);
    // this time, let some virtual time elapse, because we're going to test double-click again afterwards
    QTest::mouseRelease(&w, Qt::LeftButton, { }, point);
    QCOMPARE_GT(w.lastTimeStamp, ts + 20);
    QCOMPARE(w.doubleClickCount, 1);

    // use the mouseClick function to generate another double-click
    ts = w.lastTimeStamp;
    QTest::mouseClick(&w, Qt::LeftButton, {}, point, 10);
    QCOMPARE_GE(w.lastTimeStamp, ts + 500); // because the last release had a default delay
    QTest::mouseClick(&w, Qt::LeftButton, {}, point);
    QCOMPARE(w.doubleClickCount, 2);

    // use the mouseDClick function to generate another double-click
    ts = w.lastTimeStamp;
    QTest::mouseDClick(&w, Qt::LeftButton, {}, point);
    QCOMPARE_GE(w.lastTimeStamp, ts + 500); // because the last release had a default delay
    QCOMPARE(w.doubleClickCount, 3);

    // use the mouseClick function with default delay to avoid double-click
    ts = w.lastTimeStamp;
    QTest::mouseClick(&w, Qt::LeftButton, {}, point);
    QCOMPARE_GE(w.lastTimeStamp, ts + 500); // because the last release had a default delay
    QTest::mouseClick(&w, Qt::LeftButton, {}, point);
    QCOMPARE(w.doubleClickCount, 3);
}

QTEST_MAIN(tst_Mouse)
#include "tst_mouse.moc"
