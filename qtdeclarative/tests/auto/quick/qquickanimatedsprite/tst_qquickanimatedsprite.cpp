// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest/QtTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTest/QtQuickTest>
#include <private/qabstractanimation_p.h>
#include <private/qquickanimatedsprite_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickwindow_p.h>
#include <QtCore/qscopedpointer.h>
#include <QtGui/qpainter.h>
#include <QtGui/qoffscreensurface.h>
#include <QtQml/qqmlproperty.h>

class tst_qquickanimatedsprite : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickanimatedsprite() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_properties();
    void test_runningChangedSignal();
    void test_startStop();
    void test_frameChangedSignal();
    void test_largeAnimation_data();
    void test_largeAnimation();
    void test_reparenting();
    void test_changeSourceToSmallerImgKeepingBigFrameSize();
    void test_infiniteLoops();
    void test_implicitSize();
    void test_finishBehavior();
};

void tst_qquickanimatedsprite::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickanimatedsprite::test_properties()
{
    QScopedPointer<QQuickView> window(new QQuickView);

    window->setSource(testFileUrl("basic.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QVERIFY(window->rootObject());
    QQuickAnimatedSprite* sprite = window->rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QVERIFY(sprite);

    QTRY_VERIFY(sprite->running());
    QVERIFY(!sprite->paused());
    QVERIFY(sprite->interpolate());
    QCOMPARE(sprite->loops(), 30);

    QSignalSpy finishedSpy(sprite, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());

    sprite->setRunning(false);
    QVERIFY(!sprite->running());
    // The finished() signal shouldn't be emitted when running is manually set to false.
    QCOMPARE(finishedSpy.size(), 0);
    sprite->setInterpolate(false);
    QVERIFY(!sprite->interpolate());
}

void tst_qquickanimatedsprite::test_runningChangedSignal()
{
    QScopedPointer<QQuickView> window(new QQuickView);

    window->setSource(testFileUrl("runningChange.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QVERIFY(window->rootObject());
    QQuickAnimatedSprite* sprite = window->rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QVERIFY(sprite);

    QVERIFY(!sprite->running());

    QSignalSpy runningChangedSpy(sprite, SIGNAL(runningChanged(bool)));
    QSignalSpy finishedSpy(sprite, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());

    sprite->setRunning(true);
    QTRY_COMPARE(runningChangedSpy.size(), 1);
    QCOMPARE(finishedSpy.size(), 0);
    QTRY_VERIFY(!sprite->running());
    QTRY_COMPARE(runningChangedSpy.size(), 2);
    QCOMPARE(finishedSpy.size(), 1);
}

void tst_qquickanimatedsprite::test_startStop()
{
    QScopedPointer<QQuickView> window(new QQuickView);

    window->setSource(testFileUrl("runningChange.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QVERIFY(window->rootObject());
    QQuickAnimatedSprite* sprite = window->rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QVERIFY(sprite);

    QVERIFY(!sprite->running());

    QSignalSpy runningChangedSpy(sprite, SIGNAL(runningChanged(bool)));
    QSignalSpy finishedSpy(sprite, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());

    sprite->start();
    QVERIFY(sprite->running());
    QTRY_COMPARE(runningChangedSpy.size(), 1);
    QCOMPARE(finishedSpy.size(), 0);
    sprite->stop();
    QVERIFY(!sprite->running());
    QTRY_COMPARE(runningChangedSpy.size(), 2);
    QCOMPARE(finishedSpy.size(), 0);

    sprite->setCurrentFrame(2);
    sprite->start();
    QVERIFY(sprite->running());
    QCOMPARE(sprite->currentFrame(), 0);
    QTRY_VERIFY(sprite->currentFrame() > 0);
    sprite->stop();
    QVERIFY(!sprite->running());
}

template <typename T>
static bool isWithinRange(T min, T value, T max)
{
    Q_ASSERT(min < max);
    return min <= value && value <= max;
}

void tst_qquickanimatedsprite::test_frameChangedSignal()
{
    QScopedPointer<QQuickView> window(new QQuickView);

    window->setSource(testFileUrl("frameChange.qml"));
    window->show();

    QVERIFY(window->rootObject());
    QQuickAnimatedSprite* sprite = window->rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QSignalSpy frameChangedSpy(sprite, SIGNAL(currentFrameChanged(int)));
    QVERIFY(sprite);
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QVERIFY(!sprite->running());
    QVERIFY(!sprite->paused());
    QCOMPARE(sprite->loops(), 3);
    QCOMPARE(sprite->frameCount(), 6);
    QCOMPARE(frameChangedSpy.size(), 0);

    frameChangedSpy.clear();
    sprite->setRunning(true);
    QTRY_VERIFY(!sprite->running());
    QCOMPARE(frameChangedSpy.size(), 3*6 + 1);

    int prevFrame = -1;
    int loopCounter = 0;
    int maxFrame = 0;
    while (!frameChangedSpy.isEmpty()) {
        QList<QVariant> args = frameChangedSpy.takeFirst();
        int frame = args.first().toInt();
        if (frame < prevFrame) {
            ++loopCounter;
        } else {
            QVERIFY(frame > prevFrame);
        }
        maxFrame = qMax(frame, maxFrame);
        prevFrame = frame;
    }
    QCOMPARE(loopCounter, 3);
}

void tst_qquickanimatedsprite::test_largeAnimation_data()
{
    QTest::addColumn<bool>("frameSync");

    QTest::newRow("frameSync") << true;
    QTest::newRow("no_frameSync") << false;

}

class AnimationImageProvider : public QQuickImageProvider
{
public:
    AnimationImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &/*id*/, QSize *size, const QSize &requestedSize) override
    {
        if (requestedSize.isValid())
            qWarning() << "requestPixmap called with requestedSize of" << requestedSize;
        // 40 frames.
        const int nFrames = 40; // 40 is good for texture max width of 4096, 64 is good for 16384

        const int frameWidth = 512;
        const int frameHeight = 64;

        const QSize pixSize(frameWidth, nFrames * frameHeight);
        QPixmap pixmap(pixSize);
        pixmap.fill();

        for (int i = 0; i < nFrames; ++i) {
            QImage frame(frameWidth, frameHeight, QImage::Format_ARGB32_Premultiplied);
            frame.fill(Qt::white);
            QPainter p1(&frame);
            p1.setRenderHint(QPainter::Antialiasing, true);
            QRect r(0,0, frameWidth, frameHeight);
            p1.setBrush(QBrush(Qt::red, Qt::SolidPattern));
            p1.drawPie(r, i*360*16/nFrames, 90*16);
            p1.drawText(r, QString::number(i));
            p1.end();

            QPainter p2(&pixmap);
            p2.drawImage(0, i * frameHeight, frame);
        }

        if (size)
            *size = pixSize;
        return pixmap;
    }
};

void tst_qquickanimatedsprite::test_largeAnimation()
{
    QFETCH(bool, frameSync);

    QScopedPointer<QQuickView> window(new QQuickView);
    window->engine()->addImageProvider(QLatin1String("test"), new AnimationImageProvider);
    window->setSource(testFileUrl("largeAnimation.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QVERIFY(window->rootObject());
    QQuickAnimatedSprite* sprite = window->rootObject()->findChild<QQuickAnimatedSprite*>("sprite");

    QVERIFY(sprite);

    QVERIFY(!sprite->running());
    QVERIFY(!sprite->paused());
    QCOMPARE(sprite->loops(), 3);
    QCOMPARE(sprite->frameCount(), 40);
    sprite->setProperty("frameSync", frameSync);
    if (!frameSync)
        sprite->setProperty("frameDuration", 30);

    QSignalSpy frameChangedSpy(sprite, SIGNAL(currentFrameChanged(int)));
    sprite->setRunning(true);
    QTRY_VERIFY_WITH_TIMEOUT(!sprite->running(), 100000 /* make sure we wait until its done*/ );
    if (frameSync)
        QVERIFY(isWithinRange(3*40, int(frameChangedSpy.size()), 3*40 + 1));
    int prevFrame = -1;
    int loopCounter = 0;
    int maxFrame = 0;
    while (!frameChangedSpy.isEmpty()) {
        QList<QVariant> args = frameChangedSpy.takeFirst();
        int frame = args.first().toInt();
        if (frame < prevFrame) {
            ++loopCounter;
        } else {
            QVERIFY(frame > prevFrame);
        }
        maxFrame = qMax(frame, maxFrame);
        prevFrame = frame;
    }
    int maxTextureSize = QQuickWindowPrivate::get(window.data())->context->maxTextureSize();
    maxTextureSize /= 512;
    QVERIFY(maxFrame > maxTextureSize); // make sure we go beyond the texture width limitation
    QCOMPARE(loopCounter, sprite->loops());
}

void tst_qquickanimatedsprite::test_reparenting()
{
    QQuickView window;
    window.setSource(testFileUrl("basic.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.rootObject());
    QQuickAnimatedSprite* sprite = window.rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QVERIFY(sprite);

    QTRY_VERIFY(sprite->running());
    sprite->setParentItem(nullptr);

    sprite->setParentItem(window.rootObject());
    // don't crash (QTBUG-51162)
    sprite->polish();
    QVERIFY(QQuickTest::qIsPolishScheduled(sprite));
    QVERIFY(QQuickTest::qWaitForPolish(sprite));
}

class KillerThread : public QThread
{
    Q_OBJECT
protected:
    void run() override {
        QMutexLocker lock(&abortMutex);
        if (!aborted)
            abortWaitCondition.wait(&abortMutex, 3000);

        if (!aborted)
            qFatal("Either the GUI or the render thread is stuck in an infinite loop.");
    }

public:
    void abort()
    {
        QMutexLocker lock(&abortMutex);
        aborted = true;
        abortWaitCondition.wakeAll();
    }

private:
    QMutex abortMutex;
    QWaitCondition abortWaitCondition;
    bool aborted = false;
};

// Regression test for QTBUG-53937
void tst_qquickanimatedsprite::test_changeSourceToSmallerImgKeepingBigFrameSize()
{
    QQuickView window;
    window.setSource(testFileUrl("sourceSwitch.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.rootObject());
    QQuickAnimatedSprite* sprite = qobject_cast<QQuickAnimatedSprite*>(window.rootObject());
    QVERIFY(sprite);

    QQmlProperty big(sprite, "big");
    big.write(QVariant::fromValue(false));

    QScopedPointer<KillerThread> killer(new KillerThread);
    killer->start(); // will kill us in case the GUI or render thread enters an infinite loop

    QTest::qWait(50); // let it draw with the new source.

    // If we reach this point it's because we didn't hit QTBUG-53937

    killer->abort();
    killer->wait();
}

void tst_qquickanimatedsprite::test_implicitSize()
{
    QQuickView window;
    window.setSource(testFileUrl("basic.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject());

    QQuickAnimatedSprite* sprite = window.rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QVERIFY(sprite);
    QCOMPARE(sprite->frameWidth(), 31);
    QCOMPARE(sprite->frameHeight(), 30);
    QCOMPARE(sprite->implicitWidth(), 31);
    QCOMPARE(sprite->implicitHeight(), 30);

    // Ensure that implicitWidth matches frameWidth.
    QSignalSpy frameWidthChangedSpy(sprite, SIGNAL(frameWidthChanged(int)));
    QVERIFY(frameWidthChangedSpy.isValid());

    QSignalSpy frameImplicitWidthChangedSpy(sprite, SIGNAL(implicitWidthChanged()));
    QVERIFY(frameImplicitWidthChangedSpy.isValid());

    sprite->setFrameWidth(20);
    QCOMPARE(frameWidthChangedSpy.size(), 1);
    QCOMPARE(frameImplicitWidthChangedSpy.size(), 1);

    // Ensure that implicitHeight matches frameHeight.
    QSignalSpy frameHeightChangedSpy(sprite, SIGNAL(frameHeightChanged(int)));
    QVERIFY(frameHeightChangedSpy.isValid());

    QSignalSpy frameImplicitHeightChangedSpy(sprite, SIGNAL(implicitHeightChanged()));
    QVERIFY(frameImplicitHeightChangedSpy.isValid());

    sprite->setFrameHeight(20);
    QCOMPARE(frameHeightChangedSpy.size(), 1);
    QCOMPARE(frameImplicitHeightChangedSpy.size(), 1);
}

void tst_qquickanimatedsprite::test_infiniteLoops()
{
    QQuickView window;
    window.setSource(testFileUrl("infiniteLoops.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject());

    QQuickAnimatedSprite* sprite = qobject_cast<QQuickAnimatedSprite*>(window.rootObject());
    QVERIFY(sprite);

    QTRY_VERIFY(sprite->running());

    QSignalSpy finishedSpy(sprite, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());

    // The finished() signal shouldn't be emitted for infinite animations.
    const int previousFrame = sprite->currentFrame();
    QTRY_VERIFY(sprite->currentFrame() != previousFrame);
    QCOMPARE(finishedSpy.size(), 0);
}

void tst_qquickanimatedsprite::test_finishBehavior()
{
    QQuickView window;
    window.setSource(testFileUrl("finishBehavior.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject());

    QQuickAnimatedSprite* sprite = window.rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QVERIFY(sprite);

    QTRY_VERIFY(sprite->running());

    // correctly stops at last frame
    QSignalSpy finishedSpy(sprite, SIGNAL(finished()));
    QVERIFY(finishedSpy.wait(2000));
    QCOMPARE(sprite->running(), false);
    QCOMPARE(sprite->currentFrame(), 5);

    // correctly starts a second time
    sprite->start();
    QTRY_VERIFY(sprite->running());
    QTRY_COMPARE(sprite->currentFrame(), 5);
}

QTEST_MAIN(tst_qquickanimatedsprite)

#include "tst_qquickanimatedsprite.moc"
