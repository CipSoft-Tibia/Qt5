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

#include <qtest.h>
#include <QtTest/QtTest>
#include "../../../auto/particles/shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>

class tst_emission : public QObject
{
    Q_OBJECT
public:
    tst_emission();

private slots:
    void test_basic();
    void test_basic_data();
};

tst_emission::tst_emission()
{
}

void tst_emission::test_basic_data()
{
    QTest::addColumn<int> ("dt");
    QTest::newRow("16ms") << 16;
    QTest::newRow("32ms") << 32;
    QTest::newRow("100ms") << 100;
    QTest::newRow("500ms") << 500;
    QTest::newRow("1000ms") << 1000;
    QTest::newRow("10000ms") << 10000;
}

void tst_emission::test_basic()
{
    QFETCH(int, dt);
    QQuickView* view = createView(QCoreApplication::applicationDirPath() + "/data/basic.qml");
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    //Pretend we're running, but we manually advance the simulation
    system->m_running = true;
    system->m_animation = 0;
    system->reset();

    int curTime = 1;
    system->updateCurrentTime(curTime);//Fixed point and get init out of the way.

    while (curTime < 500){//Minimum time needed to get enough alive
        QBENCHMARK {
            curTime += dt;
            system->updateCurrentTime(curTime);
        }
    }

    int stillAlive = 0;
    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 1000, 10));//Small simulation variance is permissible.
    for (QQuickParticleData *d : qAsConst(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        if (d->stillAlive(system))
            stillAlive++;
        QCOMPARE(d->x, 0.f);
        QCOMPARE(d->y, 0.f);
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    QVERIFY(extremelyFuzzyCompare(stillAlive, 1000, 10));//Small simulation variance is permissible.
    delete view;
}

QTEST_MAIN(tst_emission);

#include "tst_emission.moc"
