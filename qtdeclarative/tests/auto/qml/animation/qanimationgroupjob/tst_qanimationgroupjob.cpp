// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>

#include <QtQml/private/qanimationgroupjob_p.h>
#include <QtQml/private/qsequentialanimationgroupjob_p.h>
#include <QtQml/private/qparallelanimationgroupjob_p.h>

Q_DECLARE_METATYPE(QAbstractAnimationJob::State)

class tst_QAnimationGroupJob : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void construction();
    void emptyGroup();
    void setCurrentTime();
    void addChildTwice();
};

void tst_QAnimationGroupJob::initTestCase()
{
    qRegisterMetaType<QAbstractAnimationJob::State>("QAbstractAnimationJob::State");
}

void tst_QAnimationGroupJob::construction()
{
    QSequentialAnimationGroupJob animationgroup;
}

class TestableGenericAnimation : public QAbstractAnimationJob
{
public:
    TestableGenericAnimation(int duration = 250) : m_duration(duration) {}
    int duration() const override { return m_duration; }

private:
    int m_duration;
};

class UncontrolledAnimation : public QObject, public QAbstractAnimationJob
{
    Q_OBJECT
public:
    UncontrolledAnimation() { }

    int duration() const override { return -1; /* not time driven */ }

protected:
    void timerEvent(QTimerEvent *event) override
    {
        if (event->timerId() == id)
            stop();
    }

    void updateRunning(bool running)
    {
        if (running) {
            id = startTimer(500);
        } else {
            killTimer(id);
            id = 0;
        }
    }

private:
    int id = 0;
};

class StateChangeListener: public QAnimationJobChangeListener
{
public:
    void animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State newState, QAbstractAnimationJob::State) override
    {
        states << newState;
    }

    int count()
    {
        return states.size();
    }

    QList<QAbstractAnimationJob::State> states;
};

void tst_QAnimationGroupJob::emptyGroup()
{
    QSequentialAnimationGroupJob group;
    StateChangeListener groupStateChangedSpy;
    group.addAnimationChangeListener(&groupStateChangedSpy, QAbstractAnimationJob::StateChange);

    QCOMPARE(group.state(), QAnimationGroupJob::Stopped);
    group.start();

    QCOMPARE(groupStateChangedSpy.count(), 2);

    QCOMPARE(groupStateChangedSpy.states.at(0), QAnimationGroupJob::Running);
    QCOMPARE(groupStateChangedSpy.states.at(1), QAnimationGroupJob::Stopped);

    QCOMPARE(group.state(), QAnimationGroupJob::Stopped);

    QTest::ignoreMessage(QtWarningMsg, "QAbstractAnimationJob::pause: Cannot pause a stopped animation");
    group.pause();

    QCOMPARE(groupStateChangedSpy.count(), 2);
    QCOMPARE(group.state(), QAnimationGroupJob::Stopped);

    group.start();

    QCOMPARE(groupStateChangedSpy.states.at(2),
             QAnimationGroupJob::Running);
    QCOMPARE(groupStateChangedSpy.states.at(3),
             QAnimationGroupJob::Stopped);

    QCOMPARE(group.state(), QAnimationGroupJob::Stopped);

    group.stop();

    QCOMPARE(groupStateChangedSpy.count(), 4);
    QCOMPARE(group.state(), QAnimationGroupJob::Stopped);
}

void tst_QAnimationGroupJob::setCurrentTime()
{
    // was originally sequence operating on same object/property
    QSequentialAnimationGroupJob *sequence = new QSequentialAnimationGroupJob();
    QAbstractAnimationJob *a1_s_o1 = new TestableGenericAnimation;
    QAbstractAnimationJob *a2_s_o1 = new TestableGenericAnimation;
    QAbstractAnimationJob *a3_s_o1 = new TestableGenericAnimation;
    a2_s_o1->setLoopCount(3);
    sequence->appendAnimation(a1_s_o1);
    sequence->appendAnimation(a2_s_o1);
    sequence->appendAnimation(a3_s_o1);

    // was originally sequence operating on different object/properties
    QAnimationGroupJob *sequence2 = new QSequentialAnimationGroupJob();
    QAbstractAnimationJob *a1_s_o2 = new TestableGenericAnimation;
    QAbstractAnimationJob *a1_s_o3 = new TestableGenericAnimation;
    sequence2->appendAnimation(a1_s_o2);
    sequence2->appendAnimation(a1_s_o3);

    // was originally parallel operating on different object/properties
    QAnimationGroupJob *parallel = new QParallelAnimationGroupJob();
    QAbstractAnimationJob *a1_p_o1 = new TestableGenericAnimation;
    QAbstractAnimationJob *a1_p_o2 = new TestableGenericAnimation;
    QAbstractAnimationJob *a1_p_o3 = new TestableGenericAnimation;
    a1_p_o2->setLoopCount(3);
    parallel->appendAnimation(a1_p_o1);
    parallel->appendAnimation(a1_p_o2);
    parallel->appendAnimation(a1_p_o3);

    QAbstractAnimationJob *notTimeDriven = new UncontrolledAnimation;
    QCOMPARE(notTimeDriven->totalDuration(), -1);

    QAbstractAnimationJob *loopsForever = new TestableGenericAnimation;
    loopsForever->setLoopCount(-1);
    QCOMPARE(loopsForever->totalDuration(), -1);

    QParallelAnimationGroupJob group;
    group.appendAnimation(sequence);
    group.appendAnimation(sequence2);
    group.appendAnimation(parallel);
    group.appendAnimation(notTimeDriven);
    group.appendAnimation(loopsForever);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroupJob::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(parallel->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(a1_p_o1->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(a1_p_o2->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(a1_p_o3->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroupJob::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroupJob::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 1);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);
    QCOMPARE(a1_p_o1->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 1);
    QCOMPARE(notTimeDriven->currentLoopTime(), 1);
    QCOMPARE(loopsForever->currentLoopTime(), 1);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(sequence->currentLoopTime(), 250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 0);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoop(), 1);
    QCOMPARE(sequence->currentAnimation(), a2_s_o1);

    // Current time = 251
    group.setCurrentTime(251);
    QCOMPARE(group.currentLoopTime(), 251);
    QCOMPARE(sequence->currentLoopTime(), 251);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 251);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 1);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 251);
    QCOMPARE(loopsForever->currentLoopTime(), 1);
    QCOMPARE(sequence->currentAnimation(), a2_s_o1);
}

void tst_QAnimationGroupJob::addChildTwice()
{
    QAbstractAnimationJob *subGroup;
    QAbstractAnimationJob *subGroup2;
    auto *parent = new QSequentialAnimationGroupJob();

    subGroup = new QAbstractAnimationJob;
    parent->appendAnimation(subGroup);
    parent->appendAnimation(subGroup);
    QVERIFY(!parent->children()->isEmpty());
    QCOMPARE(parent->children()->count(), 1);
    QVERIFY(!parent->children()->next(parent->children()->first()));
    QVERIFY(!parent->children()->prev(parent->children()->last()));

    parent->clear();

    QCOMPARE(parent->currentAnimation(), nullptr);
    QVERIFY(parent->children()->isEmpty());

    // adding the same item twice to a group will remove the item from its current position
    // and append it to the end
    subGroup = new QAbstractAnimationJob;
    parent->appendAnimation(subGroup);
    subGroup2 = new QAbstractAnimationJob;
    parent->appendAnimation(subGroup2);

    QCOMPARE(parent->children()->first(), subGroup);
    QCOMPARE(parent->children()->last(), subGroup2);

    parent->appendAnimation(subGroup);

    QCOMPARE(parent->children()->first(), subGroup2);
    QCOMPARE(parent->children()->last(), subGroup);

    delete parent;
}

QTEST_MAIN(tst_QAnimationGroupJob)
#include "tst_qanimationgroupjob.moc"
