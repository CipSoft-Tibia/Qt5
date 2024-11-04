// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

/* WARNING: this source-code is reused by another test.

   As Qt built with GUI support may use a different backend for its event loops
   and other timer-related matters, it is important to test it in that form, as
   well as in its GUI-less form. So this source file is reused by a build config
   in the GUI module. Similarly, testing with and without glib is supported,
   where relevant (see DISABLE_GLIB below).
*/
#ifdef QT_GUI_LIB
// When compiled as tests/auto/gui/kernel/qguitimer/'s source-code:
#  include <QtGui/QGuiApplication>
#else
// When compiled as tests/auto/corelib/kernel/qtimer/'s source-code:
#  include <QtCore/QCoreApplication>
#endif

#include <QtCore/private/qglobal_p.h>
#include <QTest>
#include <QSignalSpy>
#include <QtTest/private/qpropertytesthelper_p.h>

#include <qtimer.h>
#include <qthread.h>
#include <qelapsedtimer.h>
#include <qproperty.h>

#if defined Q_OS_UNIX
#include <unistd.h>
#endif

#ifdef DISABLE_GLIB
static bool glibDisabled = []() {
    qputenv("QT_NO_GLIB", "1");
    return true;
}();
#endif

using namespace std::chrono_literals;

class tst_QTimer : public QObject
{
    Q_OBJECT
public:
    static void initMain();

private slots:
    void cleanupTestCase();
    void zeroTimer();
    void singleShotTimeout();
    void timeout();
    void singleShotNormalizes_data();
    void singleShotNormalizes();
    void sequentialTimers_data();
    void sequentialTimers();
    void singleShotSequentialTimers_data();
    void singleShotSequentialTimers();
    void remainingTime();
    void remainingTimeInitial_data();
    void remainingTimeInitial();
    void remainingTimeDuringActivation_data();
    void remainingTimeDuringActivation();
    void basic_chrono();
    void livelock_data();
    void livelock();
    void timerInfiniteRecursion_data();
    void timerInfiniteRecursion();
    void recurringTimer_data();
    void recurringTimer();
    void deleteLaterOnQTimer(); // long name, don't want to shadow QObject::deleteLater()
    void moveToThread();
    void restartedTimerFiresTooSoon();
    void timerFiresOnlyOncePerProcessEvents_data();
    void timerFiresOnlyOncePerProcessEvents();
    void timerIdPersistsAfterThreadExit();
    void cancelLongTimer();
    void singleShotStaticFunctionZeroTimeout();
    void recurseOnTimeoutAndStopTimer();
    void singleShotToFunctors();
    void singleShot_chrono();
    void singleShot_static();
    void crossThreadSingleShotToFunctor_data();
    void crossThreadSingleShotToFunctor();
    void timerOrder();
    void timerOrder_data();
    void timerOrderBackgroundThread();
    void timerOrderBackgroundThread_data() { timerOrder_data(); }

    void dontBlockEvents();
    void postedEventsShouldNotStarveTimers();
    void callOnTimeout();

    void bindToTimer();
    void bindTimer();
    void automatedBindingTests();

    void negativeInterval();
};

void tst_QTimer::zeroTimer()
{
    QTimer timer;
    QVERIFY(!timer.isSingleShot());
    timer.setInterval(0);
    timer.setSingleShot(true);
    QVERIFY(timer.isSingleShot());

    QSignalSpy timeoutSpy(&timer, &QTimer::timeout);
    timer.start();

    // Pass timeout to work round glib issue, see QTBUG-84291.
    QCoreApplication::processEvents(QEventLoop::AllEvents, INT_MAX);

    QCOMPARE(timeoutSpy.size(), 1);
}

void tst_QTimer::singleShotTimeout()
{
    QTimer timer;
    QVERIFY(!timer.isSingleShot());
    timer.setSingleShot(true);
    QVERIFY(timer.isSingleShot());

    QSignalSpy timeoutSpy(&timer, &QTimer::timeout);
    timer.start(100);

    QVERIFY(timeoutSpy.wait(500));
    QCOMPARE(timeoutSpy.size(), 1);
    QTest::qWait(500);
    QCOMPARE(timeoutSpy.size(), 1);
}

#define TIMEOUT_TIMEOUT 200

void tst_QTimer::timeout()
{
    QTimer timer;
    QSignalSpy timeoutSpy(&timer, &QTimer::timeout);
    timer.start(100);

    QCOMPARE(timeoutSpy.size(), 0);

    QTRY_VERIFY_WITH_TIMEOUT(timeoutSpy.size() > 0, TIMEOUT_TIMEOUT);
    int oldCount = timeoutSpy.size();

    QTRY_VERIFY_WITH_TIMEOUT(timeoutSpy.size() > oldCount, TIMEOUT_TIMEOUT);
}

void tst_QTimer::singleShotNormalizes_data()
{
    QTest::addColumn<QByteArray>("slotName");

    QTest::newRow("normalized") << QByteArray(SLOT(exitLoop()));

    QTest::newRow("space-before") << QByteArray(SLOT( exitLoop()));
    QTest::newRow("space-after") << QByteArray(SLOT(exitLoop ()));
    QTest::newRow("space-around") << QByteArray(SLOT( exitLoop ()));
    QTest::newRow("spaces-before") << QByteArray(SLOT(  exitLoop()));
    QTest::newRow("spaces-after") << QByteArray(SLOT(exitLoop  ()));
    QTest::newRow("spaces-around") << QByteArray(SLOT(  exitLoop  ()));

    QTest::newRow("space-in-parens") << QByteArray(SLOT(exitLoop( )));
    QTest::newRow("spaces-in-parens") << QByteArray(SLOT(exitLoop(  )));
    QTest::newRow("space-after-parens") << QByteArray(SLOT(exitLoop() ));
    QTest::newRow("spaces-after-parens") << QByteArray(SLOT(exitLoop()  ));
}

void tst_QTimer::singleShotNormalizes()
{
    static constexpr auto TestTimeout = 250ms;
    QFETCH(QByteArray, slotName);
    QEventLoop loop;

    // control test: regular connection
    {
        QTimer timer;
        QVERIFY(QObject::connect(&timer, SIGNAL(timeout()), &QTestEventLoop::instance(), slotName));
        timer.setSingleShot(true);
        timer.start(1);
        QTestEventLoop::instance().enterLoop(TestTimeout);
        QVERIFY(!QTestEventLoop::instance().timeout());
    }

    // non-zero time
    QTimer::singleShot(1, &QTestEventLoop::instance(), slotName);
    QTestEventLoop::instance().enterLoop(TestTimeout);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QTimer::singleShot(1ms, &QTestEventLoop::instance(), slotName);
    QTestEventLoop::instance().enterLoop(TestTimeout);
    QVERIFY(!QTestEventLoop::instance().timeout());

    // zero time
    QTimer::singleShot(0, &QTestEventLoop::instance(), slotName);
    QTestEventLoop::instance().enterLoop(TestTimeout);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QTimer::singleShot(0ms, &QTestEventLoop::instance(), slotName);
    QTestEventLoop::instance().enterLoop(TestTimeout);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_QTimer::sequentialTimers_data()
{
#ifdef Q_OS_WIN
    QSKIP("The API used by QEventDispatcherWin32 doesn't respect the order");
#endif
    QTest::addColumn<QList<int>>("timeouts");
    auto addRow = [](const QList<int> &l) {
        QByteArray name;
        int last = -1;
        for (int i = 0; i < l.size(); ++i) {
            Q_ASSERT_X(l[i] >= last, "tst_QTimer", "input list must be sorted");
            name += QByteArray::number(l[i]) + ',';
        }
        name.chop(1);
        QTest::addRow("%s", name.constData()) << l;
    };
    // PreciseTimers
    addRow({0, 0, 0, 0, 0, 0});
    addRow({0, 1, 2});
    addRow({1, 1, 1, 2, 2, 2, 2});
    addRow({1, 2, 3});
    addRow({19, 19, 19});
    // CoarseTimer for setInterval
    addRow({20, 20, 20, 20, 20});
    addRow({25, 25, 25, 25, 25, 25, 50});
}

void tst_QTimer::sequentialTimers()
{
    QFETCH(const QList<int>, timeouts);
    QByteArray result, expected;
    std::vector<std::unique_ptr<QTimer>> timers;
    expected.resize(timeouts.size());
    result.reserve(timeouts.size());
    timers.reserve(timeouts.size());
    for (int i = 0; i < timeouts.size(); ++i) {
        auto timer = std::make_unique<QTimer>();
        timer->setSingleShot(true);
        timer->setInterval(timeouts[i]);

        char c = 'A' + i;
        expected[i] = c;
        QObject::connect(timer.get(), &QTimer::timeout, this, [&result, c = c]() {
            result.append(c);
        });
        timers.push_back(std::move(timer));
    }

    // start the timers
    for (auto &timer : timers)
        timer->start();

    QTestEventLoop::instance().enterLoopMSecs(timeouts.last() * 2 + 10);

    QCOMPARE(result, expected);
}

void tst_QTimer::singleShotSequentialTimers_data()
{
    sequentialTimers_data();
}

void tst_QTimer::singleShotSequentialTimers()
{
    QFETCH(const QList<int>, timeouts);
    QByteArray result, expected;
    expected.resize(timeouts.size());
    result.reserve(timeouts.size());
    for (int i = 0; i < timeouts.size(); ++i) {
        char c = 'A' + i;
        expected[i] = c;
        QTimer::singleShot(timeouts[i], this, [&result, c = c]() {
            result.append(c);
        });
    }

    QTestEventLoop::instance().enterLoopMSecs(timeouts.last() * 2 + 10);

    QCOMPARE(result, expected);
}

void tst_QTimer::remainingTime()
{
    QTimer tested;
    tested.setTimerType(Qt::PreciseTimer);

    QTimer tester;
    tester.setTimerType(Qt::PreciseTimer);
    tester.setSingleShot(true);

    const int testedInterval = 200;
    const int testerInterval = 50;
    const int expectedRemainingTime = testedInterval - testerInterval;

    int testIteration = 0;
    const int desiredTestCount = 2;

    auto connection = QObject::connect(&tested, &QTimer::timeout, [&tester]() {
        // We let tested (which isn't a single-shot) run repeatedly, to verify
        // it *does* repeat, and check that the single-shot tester, starting
        // at the same time, does finish first each time, by about the right duration.
        tester.start(); // Start tester again.
    });

    QObject::connect(&tester, &QTimer::timeout, [&]() {
        const int remainingTime = tested.remainingTime();
        // We expect that remainingTime is at most 150 and not overdue.
        const bool remainingTimeInRange = remainingTime > 0
                                       && remainingTime <= expectedRemainingTime;
        if (remainingTimeInRange)
            ++testIteration;
        else
            testIteration = desiredTestCount; // We are going to fail on QVERIFY2()
                                              // below, so we don't want to iterate
                                              // anymore and quickly exit the QTRY_...()
                                              // with this failure.
        if (testIteration == desiredTestCount)
            QObject::disconnect(connection); // Last iteration, don't start tester again.
        QVERIFY2(remainingTimeInRange, qPrintable("Remaining time "
                 + QByteArray::number(remainingTime) + "ms outside expected range (0ms, "
                 + QByteArray::number(expectedRemainingTime) + "ms]"));
    });

    tested.start(testedInterval);
    tester.start(testerInterval); // Start tester for the 1st time.

    // Test it desiredTestCount times, give it reasonable amount of time
    // (twice as much as needed).
    QTRY_COMPARE_WITH_TIMEOUT(testIteration, desiredTestCount,
                              testedInterval * desiredTestCount * 2);
}

void tst_QTimer::remainingTimeInitial_data()
{
    QTest::addColumn<int>("startTimeMs");
    QTest::addColumn<Qt::TimerType>("timerType");

    QTest::addRow("precise time 0ms") << 0 << Qt::PreciseTimer;
    QTest::addRow("precise time 1ms") << 1 << Qt::PreciseTimer;
    QTest::addRow("precise time 10ms") << 10 << Qt::PreciseTimer;

    QTest::addRow("coarse time 0ms") << 0 << Qt::CoarseTimer;
    QTest::addRow("coarse time 1ms") << 1 << Qt::CoarseTimer;
    QTest::addRow("coarse time 10ms") << 10 << Qt::CoarseTimer;
}

void tst_QTimer::remainingTimeInitial()
{
    QFETCH(int, startTimeMs);
    QFETCH(Qt::TimerType, timerType);

    QTimer timer;
    QCOMPARE(timer.timerType(), Qt::CoarseTimer);
    timer.setTimerType(timerType);
    QCOMPARE(timer.timerType(), timerType);
    timer.start(startTimeMs);

    const int rt = timer.remainingTime();
    QVERIFY2(rt >= 0 && rt <= startTimeMs, qPrintable(QString::number(rt)));
}

void tst_QTimer::remainingTimeDuringActivation_data()
{
    QTest::addColumn<bool>("singleShot");
    QTest::newRow("repeating") << false;
    QTest::newRow("single-shot") << true;
}

void tst_QTimer::remainingTimeDuringActivation()
{
    QFETCH(bool, singleShot);

    QTimer timer;
    timer.setSingleShot(singleShot);

    int remainingTime = 0; // not the expected value in either case
    connect(&timer, &QTimer::timeout,
            [&]() {
                remainingTime = timer.remainingTime();
            });
    QSignalSpy timeoutSpy(&timer, &QTimer::timeout);
    const int timeout = 20; // 20 ms is short enough and should not round down to 0 in any timer mode
    timer.start(timeout);

    QVERIFY(timeoutSpy.wait());
    if (singleShot)
        QCOMPARE(remainingTime, -1);     // timer not running
    else
        QVERIFY2(remainingTime <= timeout && remainingTime > 0,
                 qPrintable(QString::number(remainingTime)));

    if (!singleShot) {
        // do it again - see QTBUG-46940
        remainingTime = -1;
        QVERIFY(timeoutSpy.wait());
        QVERIFY2(remainingTime <= timeout && remainingTime > 0,
                 qPrintable(QString::number(remainingTime)));
    }
}

namespace {

    template <typename T>
    std::chrono::milliseconds to_ms(T t)
    { return std::chrono::duration_cast<std::chrono::milliseconds>(t); }

} // unnamed namespace

void tst_QTimer::basic_chrono()
{
    // duplicates zeroTimer, singleShotTimeout, interval and remainingTime
    using namespace std::chrono;
    QTimer timer;
    QSignalSpy timeoutSpy(&timer, &QTimer::timeout);
    timer.setInterval(to_ms(nanoseconds(0)));
    timer.start();
    QCOMPARE(timer.intervalAsDuration().count(), milliseconds::rep(0));
    QCOMPARE(timer.remainingTimeAsDuration().count(), milliseconds::rep(0));

    QCoreApplication::processEvents();

    QCOMPARE(timeoutSpy.size(), 1);

    timeoutSpy.clear();
    timer.start(milliseconds(100));
    QCOMPARE(timeoutSpy.size(), 0);

    QVERIFY(timeoutSpy.wait(TIMEOUT_TIMEOUT));
    QVERIFY(timeoutSpy.size() > 0);
    int oldCount = timeoutSpy.size();

    QVERIFY(timeoutSpy.wait(TIMEOUT_TIMEOUT));
    QVERIFY(timeoutSpy.size() > oldCount);

    timeoutSpy.clear();
    timer.start(to_ms(microseconds(200000)));
    QCOMPARE(timer.intervalAsDuration().count(), milliseconds::rep(200));
    QTest::qWait(50);
    QCOMPARE(timeoutSpy.size(), 0);

    milliseconds rt = timer.remainingTimeAsDuration();
    QVERIFY2(rt.count() >= 50 && rt.count() <= 200, qPrintable(QString::number(rt.count())));

    timeoutSpy.clear();
    timer.setSingleShot(true);
    timer.start(milliseconds(100));
    QVERIFY(timeoutSpy.wait(TIMEOUT_TIMEOUT));
    QCOMPARE(timeoutSpy.size(), 1);
    QTest::qWait(500);
    QCOMPARE(timeoutSpy.size(), 1);
}

void tst_QTimer::livelock_data()
{
    QTest::addColumn<int>("interval");
    QTest::newRow("zero timer") << 0;
    QTest::newRow("non-zero timer") << 1;
    QTest::newRow("longer than sleep") << 20;
}

/*!
 *
 * DO NOT "FIX" THIS TEST!  it is written like this for a reason, do
 * not *change it without first dicussing it with its maintainers.
 *
*/
class LiveLockTester : public QObject
{
public:
    LiveLockTester(int i)
        : interval(i),
          timeoutsForFirst(0), timeoutsForExtra(0), timeoutsForSecond(0),
          postEventAtRightTime(false)
    {
        firstTimerId = startTimer(interval);
        extraTimerId = startTimer(interval + 80);
        secondTimerId = -1; // started later
    }

    bool event(QEvent *e) override
    {
        if (e->type() == 4002) {
            // got the posted event
            if (timeoutsForFirst == 1 && timeoutsForSecond == 0)
                postEventAtRightTime = true;
            return true;
        }
        return QObject::event(e);
    }

    void timerEvent(QTimerEvent *te) override
    {
        if (te->timerId() == firstTimerId) {
            if (++timeoutsForFirst == 1) {
                killTimer(extraTimerId);
                extraTimerId = -1;
                QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>(4002)));
                secondTimerId = startTimer(interval);
            }
        } else if (te->timerId() == secondTimerId) {
            ++timeoutsForSecond;
        } else if (te->timerId() == extraTimerId) {
            ++timeoutsForExtra;
        }

        // sleep for 2ms
        QTest::qSleep(2);
        killTimer(te->timerId());
    }

    const int interval;
    int firstTimerId;
    int secondTimerId;
    int extraTimerId;
    int timeoutsForFirst;
    int timeoutsForExtra;
    int timeoutsForSecond;
    bool postEventAtRightTime;
};

void tst_QTimer::livelock()
{
    /*
      New timers created in timer event handlers should not be sent
      until the next iteration of the eventloop.  Note: this test
      depends on the fact that we send posted events before timer
      events (since new posted events are not sent until the next
      iteration of the eventloop either).
    */
    QFETCH(int, interval);
    LiveLockTester tester(interval);
    QTest::qWait(180); // we have to use wait here, since we're testing timers with a non-zero timeout
    QTRY_COMPARE(tester.timeoutsForFirst, 1);
    QCOMPARE(tester.timeoutsForExtra, 0);
    QTRY_COMPARE(tester.timeoutsForSecond, 1);
    QVERIFY(tester.postEventAtRightTime);
}

class TimerInfiniteRecursionObject : public QObject
{
public:
    bool inTimerEvent;
    bool timerEventRecursed;
    int interval;

    TimerInfiniteRecursionObject(int interval)
        : inTimerEvent(false), timerEventRecursed(false), interval(interval)
    { }

    void timerEvent(QTimerEvent *timerEvent) override
    {
        timerEventRecursed = inTimerEvent;
        if (timerEventRecursed) {
            // bug detected!
            return;
        }

        inTimerEvent = true;

        QEventLoop eventLoop;
        QTimer::singleShot(qMax(100, interval * 2), &eventLoop, SLOT(quit()));
        eventLoop.exec();

        inTimerEvent = false;

        killTimer(timerEvent->timerId());
    }
};

void tst_QTimer::timerInfiniteRecursion_data()
{
    QTest::addColumn<int>("interval");
    QTest::newRow("zero timer") << 0;
    QTest::newRow("non-zero timer") << 1;
    QTest::newRow("10ms timer") << 10;
    QTest::newRow("11ms timer") << 11;
    QTest::newRow("100ms timer") << 100;
    QTest::newRow("1s timer") << 1000;
}


void tst_QTimer::timerInfiniteRecursion()
{
    QFETCH(int, interval);
    TimerInfiniteRecursionObject object(interval);
    (void) object.startTimer(interval);

    QEventLoop eventLoop;
    QTimer::singleShot(qMax(100, interval * 2), &eventLoop, SLOT(quit()));
    eventLoop.exec();

    QVERIFY(!object.timerEventRecursed);
}

class RecurringTimerObject : public QObject
{
Q_OBJECT
public:
    int times;
    int target;
    bool recurse;

    RecurringTimerObject(int target)
        : times(0), target(target), recurse(false)
    { }

    void timerEvent(QTimerEvent *timerEvent) override
    {
        if (++times == target) {
            killTimer(timerEvent->timerId());
            emit done();
        } if (recurse) {
            QEventLoop eventLoop;
            QTimer::singleShot(100, &eventLoop, SLOT(quit()));
            eventLoop.exec();
        }
    }

signals:
    void done();
};

void tst_QTimer::recurringTimer_data()
{
    QTest::addColumn<int>("interval");
    QTest::addColumn<bool>("recurse");
    // make sure that eventloop recursion doesn't affect timer recurrence
    QTest::newRow("zero timer, don't recurse") << 0 << false;
    QTest::newRow("zero timer, recurse") << 0 << true;
    QTest::newRow("non-zero timer, don't recurse") << 1 << false;
    QTest::newRow("non-zero timer, recurse") << 1 << true;
}

void tst_QTimer::recurringTimer()
{
    const int target = 5;
    QFETCH(int, interval);
    QFETCH(bool, recurse);

    RecurringTimerObject object(target);
    object.recurse = recurse;
    QSignalSpy doneSpy(&object, &RecurringTimerObject::done);

    (void) object.startTimer(interval);
    QVERIFY(doneSpy.wait());

    QCOMPARE(object.times, target);
}

void tst_QTimer::deleteLaterOnQTimer()
{
    QTimer *timer = new QTimer;
    connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));
    QSignalSpy destroyedSpy(timer, &QObject::destroyed);
    timer->setInterval(1);
    timer->setSingleShot(true);
    timer->start();
    QPointer<QTimer> pointer = timer;
    QVERIFY(destroyedSpy.wait());
    QVERIFY(pointer.isNull());
}

#define MOVETOTHREAD_TIMEOUT 200
#define MOVETOTHREAD_WAIT 300

void tst_QTimer::moveToThread()
{
#if defined(Q_OS_WIN32)
    QSKIP("Does not work reliably on Windows :(");
#elif defined(Q_OS_MACOS)
    QSKIP("Does not work reliably on macOS 10.12+ (QTBUG-59679)");
#endif
    QTimer ti1;
    QTimer ti2;
    ti1.setSingleShot(true);
    ti1.start(MOVETOTHREAD_TIMEOUT);
    ti2.start(MOVETOTHREAD_TIMEOUT);
    QVERIFY((ti1.timerId() & 0xffffff) != (ti2.timerId() & 0xffffff));
    QThread tr;
    ti1.moveToThread(&tr);
    connect(&ti1,SIGNAL(timeout()), &tr, SLOT(quit()));
    tr.start();
    QTimer ti3;
    ti3.start(MOVETOTHREAD_TIMEOUT);
    QVERIFY((ti3.timerId() & 0xffffff) != (ti2.timerId() & 0xffffff));
    QVERIFY((ti3.timerId() & 0xffffff) != (ti1.timerId() & 0xffffff));
    QTest::qWait(MOVETOTHREAD_WAIT);
    QVERIFY(tr.wait());
    ti2.stop();
    QTimer ti4;
    ti4.start(MOVETOTHREAD_TIMEOUT);
    ti3.stop();
    ti2.start(MOVETOTHREAD_TIMEOUT);
    ti3.start(MOVETOTHREAD_TIMEOUT);
    QVERIFY((ti4.timerId() & 0xffffff) != (ti2.timerId() & 0xffffff));
    QVERIFY((ti3.timerId() & 0xffffff) != (ti2.timerId() & 0xffffff));
    QVERIFY((ti3.timerId() & 0xffffff) != (ti1.timerId() & 0xffffff));
}

class RestartedTimerFiresTooSoonObject : public QObject
{
    Q_OBJECT

public:
    QBasicTimer m_timer;

    int m_interval;
    QElapsedTimer m_elapsedTimer;
    QEventLoop eventLoop;

    inline RestartedTimerFiresTooSoonObject()
        : QObject(), m_interval(0)
    { }

    void timerFired()
    {
        static int interval = 1000;

        m_interval = interval;
        m_elapsedTimer.start();
        m_timer.start(interval, this);

        // alternate between single-shot and 1 sec
        interval = interval ? 0 : 1000;
    }

    void timerEvent(QTimerEvent* ev) override
    {
        if (ev->timerId() != m_timer.timerId())
            return;

        m_timer.stop();

        int elapsed = m_elapsedTimer.elapsed();

        if (elapsed < m_interval / 2) {
            // severely too early!
            m_timer.stop();
            eventLoop.exit(-1);
            return;
        }

        timerFired();

        // don't do this forever
        static int count = 0;
        if (count++ > 20) {
            m_timer.stop();
            eventLoop.quit();
            return;
        }
    }
};

void tst_QTimer::restartedTimerFiresTooSoon()
{
    RestartedTimerFiresTooSoonObject object;
    object.timerFired();
    QCOMPARE(object.eventLoop.exec(), 0);
}

class LongLastingSlotClass : public QObject
{
    Q_OBJECT

public:
    LongLastingSlotClass(QTimer *timer) : count(0), timer(timer) {}

public slots:
    void longLastingSlot()
    {
        // Don't use QTimer for this, because we are testing it.
        QElapsedTimer control;
        control.start();
        while (control.elapsed() < 200) {
            for (int c = 0; c < 100000; c++) {} // Mindless looping.
        }
        if (++count >= 2) {
            timer->stop();
        }
    }

public:
    int count;
    QTimer *timer;
};

void tst_QTimer::timerFiresOnlyOncePerProcessEvents_data()
{
    QTest::addColumn<int>("interval");
    QTest::newRow("zero timer") << 0;
    QTest::newRow("non-zero timer") << 10;
}

void tst_QTimer::timerFiresOnlyOncePerProcessEvents()
{
    QFETCH(int, interval);

    QTimer t;
    LongLastingSlotClass longSlot(&t);
    t.start(interval);
    connect(&t, SIGNAL(timeout()), &longSlot, SLOT(longLastingSlot()));
    // Loop because there may be other events pending.
    while (longSlot.count == 0) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }

    QCOMPARE(longSlot.count, 1);
}

class TimerIdPersistsAfterThreadExitThread : public QThread
{
public:
    QTimer *timer;
    int timerId, returnValue;

    TimerIdPersistsAfterThreadExitThread()
        : QThread(), timer(0), timerId(-1), returnValue(-1)
    { }
    ~TimerIdPersistsAfterThreadExitThread()
    {
        delete timer;
    }

    void run() override
    {
        QEventLoop eventLoop;
        timer = new QTimer;
        connect(timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
        timer->start(100);
        timerId = timer->timerId();
        returnValue = eventLoop.exec();
    }
};

void tst_QTimer::timerIdPersistsAfterThreadExit()
{
    TimerIdPersistsAfterThreadExitThread thread;
    thread.start();
    QVERIFY(thread.wait(30000));
    QCOMPARE(thread.returnValue, 0);

    // even though the thread has exited, and the event dispatcher destroyed, the timer is still
    // "active", meaning the timer id should NOT be reused (i.e. the event dispatcher should not
    // have unregistered it)
    int timerId = thread.startTimer(100);
    QVERIFY((timerId & 0xffffff) != (thread.timerId & 0xffffff));
}

void tst_QTimer::cancelLongTimer()
{
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(1000 * 60 * 60); //set timer for 1 hour
    QCoreApplication::processEvents();
    QVERIFY(timer.isActive()); //if the timer completes immediately with an error, then this will fail
    timer.stop();
    QVERIFY(!timer.isActive());
}

class TimeoutCounter : public QObject
{
    Q_OBJECT
public slots:
    void timeout() { ++count; };
public:
    int count = 0;
};

void tst_QTimer::singleShotStaticFunctionZeroTimeout()
{
    {
        TimeoutCounter counter;

        QTimer::singleShot(0, &counter, SLOT(timeout()));
        QTRY_COMPARE(counter.count, 1);
        QTest::qWait(500);
        QCOMPARE(counter.count, 1);
    }

    {
        TimeoutCounter counter;

        QTimer::singleShot(0, &counter, &TimeoutCounter::timeout);
        QTRY_COMPARE(counter.count, 1);
        QTest::qWait(500);
        QCOMPARE(counter.count, 1);
    }
}

class RecursOnTimeoutAndStopTimerTimer : public QObject
{
    Q_OBJECT

public:
    QTimer *one;
    QTimer *two;

public slots:
    void onetrigger()
    {
        QCoreApplication::processEvents();
    }

    void twotrigger()
    {
        one->stop();
    }
};

void tst_QTimer::recurseOnTimeoutAndStopTimer()
{
    QEventLoop eventLoop;
    QTimer::singleShot(1000, &eventLoop, SLOT(quit()));

    RecursOnTimeoutAndStopTimerTimer t;
    t.one = new QTimer(&t);
    t.two = new QTimer(&t);

    QObject::connect(t.one, SIGNAL(timeout()), &t, SLOT(onetrigger()));
    QObject::connect(t.two, SIGNAL(timeout()), &t, SLOT(twotrigger()));

    t.two->setSingleShot(true);

    t.one->start();
    t.two->start();

    (void) eventLoop.exec();

    QVERIFY(!t.one->isActive());
    QVERIFY(!t.two->isActive());
}

struct CountedStruct
{
    CountedStruct(int *count, QThread *t = nullptr) : count(count), thread(t) { }
    ~CountedStruct() { }
    void operator()() const { ++(*count); if (thread) QCOMPARE(QThread::currentThread(), thread); }

    int *count;
    QThread *thread;
};

static QScopedPointer<QEventLoop> _e;
static QThread *_t = nullptr;

class StaticEventLoop
{
public:
    static void quitEventLoop()
    {
        quitEventLoop_noexcept();
    }

    static void quitEventLoop_noexcept() noexcept
    {
        QVERIFY(!_e.isNull());
        _e->quit();
        if (_t)
            QCOMPARE(QThread::currentThread(), _t);
    }
};

void tst_QTimer::singleShotToFunctors()
{
    int count = 0;
    _e.reset(new QEventLoop);
    QEventLoop e;

    QTimer::singleShot(0, CountedStruct(&count));
    QCoreApplication::processEvents();
    QCOMPARE(count, 1);

    QTimer::singleShot(0, &StaticEventLoop::quitEventLoop);
    QCOMPARE(_e->exec(), 0);

    QTimer::singleShot(0, &StaticEventLoop::quitEventLoop_noexcept);
    QCOMPARE(_e->exec(), 0);

    QThread t1;
    QObject c1;
    c1.moveToThread(&t1);

    QObject::connect(&t1, SIGNAL(started()), &e, SLOT(quit()));
    t1.start();
    QCOMPARE(e.exec(), 0);

    QTimer::singleShot(0, &c1, CountedStruct(&count, &t1));
    QTRY_COMPARE(count, 2);

    t1.quit();
    t1.wait();

    _t = new QThread;
    QObject c2;
    c2.moveToThread(_t);

    QObject::connect(_t, SIGNAL(started()), &e, SLOT(quit()));
    _t->start();
    QCOMPARE(e.exec(), 0);

    QTimer::singleShot(0, &c2, &StaticEventLoop::quitEventLoop);
    QCOMPARE(_e->exec(), 0);

    _t->quit();
    _t->wait();
    _t->deleteLater();
    _t = nullptr;

    {
        QObject c3;
        QTimer::singleShot(500, &c3, CountedStruct(&count));
    }
    QTest::qWait(800); // Wait until the singleshot timer would have timed out
    QCOMPARE(count, 2);

    QTimer::singleShot(0, [&count] { ++count; });
    QTRY_COMPARE(count, 3);

    QObject context;
    QThread thread;

    context.moveToThread(&thread);
    QObject::connect(&thread, SIGNAL(started()), &e, SLOT(quit()));
    thread.start();
    QCOMPARE(e.exec(), 0);

    QTimer::singleShot(0, &context, [&count, &thread] { ++count; QCOMPARE(QThread::currentThread(), &thread); });
    QTRY_COMPARE(count, 4);

    thread.quit();
    thread.wait();

    struct MoveOnly : CountedStruct {
        Q_DISABLE_COPY(MoveOnly)
        MoveOnly(MoveOnly &&o) : CountedStruct(std::move(o)) {};
        MoveOnly(int *c) : CountedStruct(c) {}
    };
    QTimer::singleShot(0, MoveOnly(&count));
    QTRY_COMPARE(count, 5);

    _e.reset();
    _t = nullptr;
}

void tst_QTimer::singleShot_chrono()
{
    // duplicates singleShotStaticFunctionZeroTimeout and singleShotToFunctors
    using namespace std::chrono;
    {
        TimeoutCounter counter;

        QTimer::singleShot(hours(0), &counter, SLOT(timeout()));
        QTRY_COMPARE(counter.count, 1);
        QTest::qWait(500);
        QCOMPARE(counter.count, 1);
    }

    {
        TimeoutCounter counter;

        QTimer::singleShot(hours(0), &counter, &TimeoutCounter::timeout);
        QTRY_COMPARE(counter.count, 1);
        QTest::qWait(500);
        QCOMPARE(counter.count, 1);
    }

    int count = 0;
    QTimer::singleShot(to_ms(microseconds(0)), CountedStruct(&count));
    QTRY_COMPARE(count, 1);

    _e.reset(new QEventLoop);
    QTimer::singleShot(0, &StaticEventLoop::quitEventLoop);
    QCOMPARE(_e->exec(), 0);

    QObject c3;
    QTimer::singleShot(milliseconds(500), &c3, CountedStruct(&count));
    QTRY_COMPARE(count, 2);

    QTimer::singleShot(0, [&count] { ++count; });
    QTRY_COMPARE(count, 3);

    _e.reset();
}

class DontBlockEvents : public QObject
{
    Q_OBJECT
public:
    DontBlockEvents();
    void timerEvent(QTimerEvent*) override;

    int count;
    int total;
    QBasicTimer m_timer;

public slots:
    void paintEvent();

};

DontBlockEvents::DontBlockEvents()
{
    count = 0;
    total = 0;

    // need a few unrelated timers running to reproduce the bug.
    (new QTimer(this))->start(2000);
    (new QTimer(this))->start(2500);
    (new QTimer(this))->start(3000);
    (new QTimer(this))->start(5000);
    (new QTimer(this))->start(1000);
    (new QTimer(this))->start(2000);

    m_timer.start(1, this);
}

void DontBlockEvents::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_timer.timerId()) {
        QMetaObject::invokeMethod(this, "paintEvent", Qt::QueuedConnection);
        m_timer.start(0, this);
        count++;
        QCOMPARE(count, 1);
        total++;
    }
}

void DontBlockEvents::paintEvent()
{
    count--;
    QCOMPARE(count, 0);
}

// This is a regression test for QTBUG-13633, where a timer with a zero
// timeout that was restarted by the event handler could starve other timers.
void tst_QTimer::dontBlockEvents()
{
    DontBlockEvents t;
    QTest::qWait(60);
    QTRY_VERIFY(t.total > 2);
}

class SlotRepeater : public QObject {
    Q_OBJECT
public:
    SlotRepeater() {}

public slots:
    void repeatThisSlot()
    {
        QMetaObject::invokeMethod(this, "repeatThisSlot", Qt::QueuedConnection);
    }
};

void tst_QTimer::postedEventsShouldNotStarveTimers()
{
    QTimer timer;
    timer.setInterval(0);
    timer.setSingleShot(false);
    QSignalSpy timeoutSpy(&timer, &QTimer::timeout);
    timer.start();
    SlotRepeater slotRepeater;
    slotRepeater.repeatThisSlot();
    QTRY_VERIFY_WITH_TIMEOUT(timeoutSpy.size() > 5, 100);
}

struct DummyFunctor {
    static QThread *callThread;
    void operator()() {
        callThread = QThread::currentThread();
        callThread->quit();
    }
};
QThread *DummyFunctor::callThread = nullptr;

void tst_QTimer::crossThreadSingleShotToFunctor_data()
{
    QTest::addColumn<int>("timeout");

    QTest::addRow("zero-timer") << 0;
    QTest::addRow("1ms") << 1;
}

void tst_QTimer::crossThreadSingleShotToFunctor()
{
    QFETCH(int, timeout);
    // We're also testing for crashes here, so the test simply running to
    // completion is part of the success
    DummyFunctor::callThread = nullptr;

    QThread t;
    std::unique_ptr<QObject> o(new QObject());
    o->moveToThread(&t);

    QTimer::singleShot(timeout, o.get(), DummyFunctor());

    // wait enough time for the timer to have timed out before the timer
    // could be start in the receiver's thread.
    QTest::qWait(10 + timeout * 10);
    t.start();
    t.wait();
    QCOMPARE(DummyFunctor::callThread, &t);

    // continue with a stress test - the calling thread is busy, the
    // timer should still fire and no crashes.
    DummyFunctor::callThread = nullptr;
    t.start();
    for (int i = 0; i < 10000; i++)
        QTimer::singleShot(timeout, o.get(), DummyFunctor());

    t.wait();
    o.reset();

    QCOMPARE(DummyFunctor::callThread, &t);
}

void tst_QTimer::callOnTimeout()
{
    QTimer timer;
    QSignalSpy timeoutSpy(&timer, &QTimer::timeout);
    timer.setInterval(0);
    timer.start();

    auto context = new QObject();

    int count = 0;
    timer.callOnTimeout([&count] { count++; });
    QMetaObject::Connection connection = timer.callOnTimeout(context, [&count] { count++; });
    timer.callOnTimeout(&timer, &QTimer::stop);


    QTest::qWait(100);
    QCOMPARE(count, 2);
    QCOMPARE(timeoutSpy.size(), 1);

    // Test that connection is bound to context lifetime
    QVERIFY(connection);
    delete context;
    QVERIFY(!connection);
}

void tst_QTimer::bindToTimer()
{
    QTimer timer;

    // singleShot property
    QProperty<bool> singleShot;
    singleShot.setBinding(timer.bindableSingleShot().makeBinding());
    QCOMPARE(timer.isSingleShot(), singleShot);

    timer.setSingleShot(true);
    QVERIFY(singleShot);
    timer.setSingleShot(false);
    QVERIFY(!singleShot);

    // interval property
    QProperty<int> interval;
    interval.setBinding([&](){ return timer.interval(); });
    QCOMPARE(timer.interval(), interval);

    timer.setInterval(10);
    QCOMPARE(interval, 10);
    timer.setInterval(100);
    QCOMPARE(interval, 100);

    // timerType property
    QProperty<Qt::TimerType> timerType;
    timerType.setBinding(timer.bindableTimerType().makeBinding());
    QCOMPARE(timer.timerType(), timerType);

    timer.setTimerType(Qt::PreciseTimer);
    QCOMPARE(timerType, Qt::PreciseTimer);

    timer.setTimerType(Qt::VeryCoarseTimer);
    QCOMPARE(timerType, Qt::VeryCoarseTimer);

    // active property
    QProperty<bool> active;
    active.setBinding([&](){ return timer.isActive(); });
    QCOMPARE(active, timer.isActive());

    timer.start(1000);
    QVERIFY(active);

    timer.stop();
    QVERIFY(!active);

    auto ignoreMsg = [] {
        QTest::ignoreMessage(QtWarningMsg,
                             "QObject::startTimer: Timers cannot have negative intervals");
    };

    // also test that using negative interval updates the binding correctly
    timer.start(100);
    QVERIFY(active);
    ignoreMsg();
    timer.setInterval(-100);
    QVERIFY(!active);
    timer.start(100);
    QVERIFY(active);
    ignoreMsg();
    timer.start(-100);
    QVERIFY(!active);
}

void tst_QTimer::bindTimer()
{
    QTimer timer;

    // singleShot property
    QVERIFY(!timer.isSingleShot());

    QProperty<bool> singleShot;
    timer.bindableSingleShot().setBinding(Qt::makePropertyBinding(singleShot));

    singleShot = true;
    QVERIFY(timer.isSingleShot());
    singleShot = false;
    QVERIFY(!timer.isSingleShot());

    // interval property
    QCOMPARE(timer.interval(), 0);

    QProperty<int> interval;
    timer.bindableInterval().setBinding(Qt::makePropertyBinding(interval));

    interval = 10;
    QCOMPARE(timer.interval(), 10);
    interval = 100;
    QCOMPARE(timer.interval(), 100);
    timer.setInterval(50);
    QCOMPARE(timer.interval(), 50);
    interval = 30;
    QCOMPARE(timer.interval(), 50);

    // timerType property
    QCOMPARE(timer.timerType(), Qt::CoarseTimer);

    QProperty<Qt::TimerType> timerType;
    timer.bindableTimerType().setBinding(Qt::makePropertyBinding(timerType));

    timerType = Qt::PreciseTimer;
    QCOMPARE(timer.timerType(), Qt::PreciseTimer);
    timerType = Qt::VeryCoarseTimer;
    QCOMPARE(timer.timerType(), Qt::VeryCoarseTimer);
}

void tst_QTimer::automatedBindingTests()
{
    QTimer timer;

    QVERIFY(!timer.isSingleShot());
    QTestPrivate::testReadWritePropertyBasics(timer, true, false, "singleShot");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QTimer::singleShot");
        return;
    }

    QCOMPARE_NE(timer.interval(), 10);
    QTestPrivate::testReadWritePropertyBasics(timer, 10, 20, "interval");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QTimer::interval");
        return;
    }

    QCOMPARE_NE(timer.timerType(), Qt::PreciseTimer);
    QTestPrivate::testReadWritePropertyBasics(timer, Qt::PreciseTimer, Qt::CoarseTimer,
                                              "timerType");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QTimer::timerType");
        return;
    }

    timer.start(1000);
    QVERIFY(timer.isActive());
    QTestPrivate::testReadOnlyPropertyBasics(timer, true, false, "active",
                                             [&timer]() { timer.stop(); });
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QTimer::active");
        return;
    }
}

void tst_QTimer::negativeInterval()
{
    auto ignoreMsg = [] {
        QTest::ignoreMessage(QtWarningMsg,
                             "QObject::startTimer: Timers cannot have negative intervals");
    };

    QTimer timer;

    // Starting with a negative interval does not change active state.
    ignoreMsg();
    timer.start(-100ms);
    QVERIFY(!timer.isActive());

    // Updating the interval to a negative value stops the timer and changes
    // the active state.
    timer.start(100ms);
    QVERIFY(timer.isActive());
    ignoreMsg();
    timer.setInterval(-100);
    QVERIFY(!timer.isActive());

    // Starting with a negative interval when already started leads to stop
    // and inactive state.
    timer.start(100);
    QVERIFY(timer.isActive());
    ignoreMsg();
    timer.start(-100ms);
    QVERIFY(!timer.isActive());
}

class OrderHelper : public QObject
{
    Q_OBJECT
public:
    enum CallType
    {
        String,
        PMF,
        Functor,
        FunctorNoCtx
    };
    Q_ENUM(CallType)
    QList<CallType> calls;

    void triggerCall(CallType callType)
    {
        switch (callType)
        {
        case String:
            QTimer::singleShot(0, this, SLOT(stringSlot()));
            break;
        case PMF:
            QTimer::singleShot(0, this, &OrderHelper::pmfSlot);
            break;
        case Functor:
            QTimer::singleShot(0, this, [this]() { functorSlot(); });
            break;
        case FunctorNoCtx:
            QTimer::singleShot(0, [this]() { functorNoCtxSlot(); });
            break;
        }
    }

public slots:
    void stringSlot() { calls << String; }
    void pmfSlot() { calls << PMF; }
    void functorSlot() { calls << Functor; }
    void functorNoCtxSlot() { calls << FunctorNoCtx; }
};

Q_DECLARE_METATYPE(OrderHelper::CallType)

void tst_QTimer::timerOrder()
{
    QFETCH(QList<OrderHelper::CallType>, calls);

    OrderHelper helper;

    for (const auto call : calls)
        helper.triggerCall(call);

    QTRY_COMPARE(helper.calls, calls);
}

void tst_QTimer::timerOrder_data()
{
    QTest::addColumn<QList<OrderHelper::CallType>>("calls");

    QList<OrderHelper::CallType> calls = {
        OrderHelper::String, OrderHelper::PMF,
        OrderHelper::Functor, OrderHelper::FunctorNoCtx
    };
    std::sort(calls.begin(), calls.end());

    int permutation = 0;
    do {
        QTest::addRow("permutation=%d", permutation) << calls;
        ++permutation;
    } while (std::next_permutation(calls.begin(), calls.end()));
}

void tst_QTimer::timerOrderBackgroundThread()
{
    auto *thread = QThread::create([this]() { timerOrder(); });
    thread->start();
    QVERIFY(thread->wait());
    delete thread;
}

struct StaticSingleShotUser
{
    StaticSingleShotUser()
    {
        for (auto call : calls())
            helper.triggerCall(call);
    }
    OrderHelper helper;

    static QList<OrderHelper::CallType> calls()
    {
        return {OrderHelper::String, OrderHelper::PMF,
                OrderHelper::Functor, OrderHelper::FunctorNoCtx};
    }
};

// NOTE: to prevent any static initialization order fiasco, we implement
//       initMain() to instantiate staticSingleShotUser before qApp

static StaticSingleShotUser *s_staticSingleShotUser = nullptr;

void tst_QTimer::initMain()
{
    s_staticSingleShotUser = new StaticSingleShotUser;
}

void tst_QTimer::cleanupTestCase()
{
    delete s_staticSingleShotUser;
}

void tst_QTimer::singleShot_static()
{
    QCoreApplication::processEvents();
    QCOMPARE(s_staticSingleShotUser->helper.calls, s_staticSingleShotUser->calls());
}

QTEST_MAIN(tst_QTimer)

#include "tst_qtimer.moc"
