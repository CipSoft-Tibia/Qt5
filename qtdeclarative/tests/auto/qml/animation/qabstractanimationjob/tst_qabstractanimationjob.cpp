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


#include <QtQml/private/qabstractanimationjob_p.h>
#include <QtQml/private/qanimationgroupjob_p.h>
#include <QtTest>

class tst_QAbstractAnimationJob : public QObject
{
  Q_OBJECT
private slots:
    void construction();
    void destruction();
    void currentLoop();
    void currentLoopTime();
    void currentTime();
    void direction();
    void group();
    void loopCount();
    void state();
    void totalDuration();
    void avoidJumpAtStart();
    void avoidJumpAtStartWithStop();
    void avoidJumpAtStartWithRunning();
};

class TestableQAbstractAnimation : public QAbstractAnimationJob
{
public:
    TestableQAbstractAnimation() {}
    virtual ~TestableQAbstractAnimation() {};

    int duration() const { return m_duration; }
    virtual void updateCurrentTime(int) {}

    void setDuration(int duration) { m_duration = duration; }
private:
    int m_duration = 10;
};

class DummyQAnimationGroup : public QAnimationGroupJob
{
public:
    int duration() const { return 10; }
    virtual void updateCurrentTime(int) {}
};

void tst_QAbstractAnimationJob::construction()
{
    TestableQAbstractAnimation anim;
}

void tst_QAbstractAnimationJob::destruction()
{
    TestableQAbstractAnimation *anim = new TestableQAbstractAnimation;
    delete anim;
}

void tst_QAbstractAnimationJob::currentLoop()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentLoop(), 0);
}

void tst_QAbstractAnimationJob::currentLoopTime()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentLoopTime(), 0);
}

void tst_QAbstractAnimationJob::currentTime()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentTime(), 0);
    anim.setCurrentTime(10);
    QCOMPARE(anim.currentTime(), 10);
}

void tst_QAbstractAnimationJob::direction()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.direction(), QAbstractAnimationJob::Forward);
    anim.setDirection(QAbstractAnimationJob::Backward);
    QCOMPARE(anim.direction(), QAbstractAnimationJob::Backward);
    anim.setDirection(QAbstractAnimationJob::Forward);
    QCOMPARE(anim.direction(), QAbstractAnimationJob::Forward);
}

void tst_QAbstractAnimationJob::group()
{
    TestableQAbstractAnimation *anim = new TestableQAbstractAnimation;
    DummyQAnimationGroup group;
    group.appendAnimation(anim);
    QCOMPARE(anim->group(), &group);
}

void tst_QAbstractAnimationJob::loopCount()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.loopCount(), 1);
    anim.setLoopCount(10);
    QCOMPARE(anim.loopCount(), 10);
}

void tst_QAbstractAnimationJob::state()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.state(), QAbstractAnimationJob::Stopped);
}

void tst_QAbstractAnimationJob::totalDuration()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.duration(), 10);
    anim.setLoopCount(5);
    QCOMPARE(anim.totalDuration(), 50);
}

void tst_QAbstractAnimationJob::avoidJumpAtStart()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(1000);

    /*
        the timer shouldn't actually start until we hit the event loop,
        so the sleep should have no effect
    */
    anim.start();
    QTest::qSleep(300);
    QCoreApplication::processEvents();
    QVERIFY(anim.currentTime() < 50);
}

void tst_QAbstractAnimationJob::avoidJumpAtStartWithStop()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(1000);

    TestableQAbstractAnimation anim2;
    anim2.setDuration(1000);

    TestableQAbstractAnimation anim3;
    anim3.setDuration(1000);

    anim.start();
    QTest::qWait(300);
    anim.stop();

    /*
        same test as avoidJumpAtStart, but after there is a
        running animation that is stopped
    */
    anim2.start();
    QTest::qSleep(300);
    anim3.start();
    QCoreApplication::processEvents();
    QVERIFY(anim2.currentTime() < 50);
    QVERIFY(anim3.currentTime() < 50);
}

void tst_QAbstractAnimationJob::avoidJumpAtStartWithRunning()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(2000);

    TestableQAbstractAnimation anim2;
    anim2.setDuration(1000);

    TestableQAbstractAnimation anim3;
    anim3.setDuration(1000);

    anim.start();
    QTest::qWait(300);  //make sure timer has started

    /*
        same test as avoidJumpAtStart, but with an
        existing running animation
    */
    anim2.start();
    QTest::qSleep(300); //force large delta for next tick
    anim3.start();
    QCoreApplication::processEvents();
    QVERIFY(anim2.currentTime() < 50);
    QVERIFY(anim3.currentTime() < 50);
}


QTEST_MAIN(tst_QAbstractAnimationJob)

#include "tst_qabstractanimationjob.moc"
