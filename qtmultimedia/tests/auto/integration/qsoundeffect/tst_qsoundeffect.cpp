// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QtCore/qlocale.h>
#include <qaudiodevice.h>
#include <qaudio.h>
#include "qsoundeffect.h"
#include "qmediadevices.h"

class tst_QSoundEffect : public QObject
{
    Q_OBJECT
public:
    tst_QSoundEffect(QObject* parent=nullptr) : QObject(parent) {}

public slots:
    void init();
    void cleanup();

private slots:
    void initTestCase();
    void testSource();
    void testLooping();
    void testVolume();
    void testMuting();

    void testPlaying();
    void testStatus();

    void testDestroyWhilePlaying();
    void testDestroyWhileRestartPlaying();

    void testSetSourceWhileLoading();
    void testSetSourceWhilePlaying();
    void testSupportedMimeTypes_data();
    void testSupportedMimeTypes();
    void testCorruptFile();

private:
    QSoundEffect* sound;
    QUrl url; // test.wav: pcm_s16le, 48000 Hz, stereo, s16
    QUrl url2; // test_tone.wav: pcm_s16le, 44100 Hz, mono
    QUrl urlCorrupted; // test_corrupted.wav: corrupted
};

void tst_QSoundEffect::init()
{
    sound->stop();
    sound->setSource(QUrl());
    sound->setLoopCount(1);
    sound->setVolume(1.0);
    sound->setMuted(false);
}

void tst_QSoundEffect::cleanup()
{
}

void tst_QSoundEffect::initTestCase()
{
    // Only perform tests if audio device exists
    QStringList mimeTypes = sound->supportedMimeTypes();
    if (mimeTypes.empty())
        QSKIP("No audio devices available");

    QString testFileName = QStringLiteral("test.wav");
    QString fullPath = QFINDTESTDATA(testFileName);
    QVERIFY2(!fullPath.isEmpty(), qPrintable(QStringLiteral("Unable to locate ") + testFileName));
    url = QUrl::fromLocalFile(fullPath);

    testFileName = QStringLiteral("test_tone.wav");
    fullPath = QFINDTESTDATA(testFileName);
    QVERIFY2(!fullPath.isEmpty(), qPrintable(QStringLiteral("Unable to locate ") + testFileName));
    url2 = QUrl::fromLocalFile(fullPath);

    testFileName = QStringLiteral("test_corrupted.wav");
    fullPath = QFINDTESTDATA(testFileName);
    QVERIFY2(!fullPath.isEmpty(), qPrintable(QStringLiteral("Unable to locate ") + testFileName));
    urlCorrupted = QUrl::fromLocalFile(fullPath);

    sound = new QSoundEffect(this);

    QVERIFY(sound->source().isEmpty());
    QVERIFY(sound->loopCount() == 1);
    QVERIFY(sound->volume() == 1);
    QVERIFY(sound->isMuted() == false);
}

void tst_QSoundEffect::testSource()
{
    QSignalSpy readSignal(sound, SIGNAL(sourceChanged()));

    sound->setSource(url);
    sound->setVolume(0.1f);

    QCOMPARE(sound->source(),url);
    QCOMPARE(readSignal.size(),1);

    QTestEventLoop::instance().enterLoop(1);
    sound->play();
    QTRY_COMPARE(sound->isPlaying(), false);
    QCOMPARE(sound->loopsRemaining(), 0);
}

void tst_QSoundEffect::testLooping()
{
    sound->setSource(url);
    QTRY_COMPARE(sound->status(), QSoundEffect::Ready);

    QSignalSpy readSignal_Count(sound, SIGNAL(loopCountChanged()));
    QSignalSpy readSignal_Remaining(sound, SIGNAL(loopsRemainingChanged()));

    sound->setLoopCount(3);
    sound->setVolume(0.1f);
    QCOMPARE(sound->loopCount(), 3);
    QCOMPARE(readSignal_Count.size(), 1);
    QCOMPARE(sound->loopsRemaining(), 0);
    QCOMPARE(readSignal_Remaining.size(), 0);

    sound->play();
    QVERIFY(readSignal_Remaining.size() > 0);

    // test.wav is about 200ms, wait until it has finished playing 3 times
    QTestEventLoop::instance().enterLoop(3);

    QTRY_COMPARE(sound->loopsRemaining(), 0);
    QVERIFY(readSignal_Remaining.size() == 4);
    QTRY_VERIFY(!sound->isPlaying());

    // QTBUG-36643 (setting the loop count while playing should work)
    {
        readSignal_Count.clear();
        readSignal_Remaining.clear();

        sound->setLoopCount(10);
        QCOMPARE(sound->loopCount(), 10);
        QCOMPARE(readSignal_Count.size(), 1);
        QCOMPARE(sound->loopsRemaining(), 0);
        QCOMPARE(readSignal_Remaining.size(), 0);

        sound->play();
        QVERIFY(readSignal_Remaining.size() > 0);

        // wait for the sound to be played several times
        QTRY_VERIFY(sound->loopsRemaining() <= 7);
        QVERIFY(readSignal_Remaining.size() >= 3);
        readSignal_Count.clear();
        readSignal_Remaining.clear();

        // change the loop count while playing
        sound->setLoopCount(3);
        QCOMPARE(sound->loopCount(), 3);
        QCOMPARE(readSignal_Count.size(), 1);
        QCOMPARE(sound->loopsRemaining(), 3);
        QCOMPARE(readSignal_Remaining.size(), 1);

        // wait for all the loops to be completed
        QTRY_COMPARE(sound->loopsRemaining(), 0);
        QTRY_VERIFY(readSignal_Remaining.size() == 4);
        QTRY_VERIFY(!sound->isPlaying());
    }

    {
        readSignal_Count.clear();
        readSignal_Remaining.clear();

        sound->setLoopCount(QSoundEffect::Infinite);
        QCOMPARE(sound->loopCount(), int(QSoundEffect::Infinite));
        QCOMPARE(readSignal_Count.size(), 1);
        QCOMPARE(sound->loopsRemaining(), 0);
        QCOMPARE(readSignal_Remaining.size(), 0);

        sound->play();
        QTRY_COMPARE(sound->loopsRemaining(), int(QSoundEffect::Infinite));
        QCOMPARE(readSignal_Remaining.size(), 1);

        QTest::qWait(500);
        QVERIFY(sound->isPlaying());
        readSignal_Count.clear();
        readSignal_Remaining.clear();

        // Setting the loop count to 0 should play it one last time
        sound->setLoopCount(0);
        QCOMPARE(sound->loopCount(), 1);
        QCOMPARE(readSignal_Count.size(), 1);
        QCOMPARE(sound->loopsRemaining(), 1);
        QCOMPARE(readSignal_Remaining.size(), 1);

        QTRY_COMPARE(sound->loopsRemaining(), 0);
        QTRY_VERIFY(readSignal_Remaining.size() >= 2);
        QTRY_VERIFY(!sound->isPlaying());
    }
}

void tst_QSoundEffect::testVolume()
{
    QSignalSpy readSignal(sound, SIGNAL(volumeChanged()));

    sound->setVolume(0.5);
    QCOMPARE(sound->volume(),0.5);

    QTRY_COMPARE(readSignal.size(),1);
}

void tst_QSoundEffect::testMuting()
{
    QSignalSpy readSignal(sound, SIGNAL(mutedChanged()));

    sound->setMuted(true);
    QCOMPARE(sound->isMuted(),true);

    QTRY_COMPARE(readSignal.size(),1);
}

void tst_QSoundEffect::testPlaying()
{
    sound->setLoopCount(QSoundEffect::Infinite);
    sound->setVolume(0.1f);
    //valid source
    sound->setSource(url);
    QTestEventLoop::instance().enterLoop(1);
    sound->play();
    QTestEventLoop::instance().enterLoop(1);
    QTRY_COMPARE(sound->isPlaying(), true);
    sound->stop();

    //empty source
    sound->setSource(QUrl());
    QTestEventLoop::instance().enterLoop(1);
    sound->play();
    QTestEventLoop::instance().enterLoop(1);
    QTRY_COMPARE(sound->isPlaying(), false);

    //invalid source
    sound->setSource(QUrl((QLatin1String("invalid source"))));
    QTestEventLoop::instance().enterLoop(1);
    sound->play();
    QTestEventLoop::instance().enterLoop(1);
    QTRY_COMPARE(sound->isPlaying(), false);

    sound->setLoopCount(1); // TODO: What if one of the tests fail?
}

void tst_QSoundEffect::testStatus()
{
    sound->setSource(QUrl());
    QCOMPARE(sound->status(), QSoundEffect::Null);

    //valid source
    sound->setSource(url);

    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(sound->status(), QSoundEffect::Ready);

    //empty source
    sound->setSource(QUrl());
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(sound->status(), QSoundEffect::Null);

    //invalid source
    sound->setLoopCount(QSoundEffect::Infinite);

    sound->setSource(QUrl(QLatin1String("invalid source")));
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(sound->status(), QSoundEffect::Error);
}

void tst_QSoundEffect::testDestroyWhilePlaying()
{
    QSoundEffect *instance = new QSoundEffect();
    instance->setSource(url);
    instance->setVolume(0.1f);
    QTestEventLoop::instance().enterLoop(1);
    instance->play();
    QTest::qWait(100);
    delete instance;
    QTestEventLoop::instance().enterLoop(1);
}

void tst_QSoundEffect::testDestroyWhileRestartPlaying()
{
    QSoundEffect *instance = new QSoundEffect();
    instance->setSource(url);
    instance->setVolume(0.1f);
    QTestEventLoop::instance().enterLoop(1);
    instance->play();
    QTRY_COMPARE(instance->isPlaying(), false);
    //restart playing
    instance->play();
    delete instance;
    QTestEventLoop::instance().enterLoop(1);
}

void tst_QSoundEffect::testSetSourceWhileLoading()
{
    for (int i = 0; i < 2; i++) {
        sound->setSource(url);
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Ready);
        sound->setSource(url); // set same source again
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Ready);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready); // make sure it switches to ready state
        sound->play();
        QVERIFY(sound->isPlaying());

        sound->setSource(QUrl());
        QCOMPARE(sound->status(), QSoundEffect::Null);

        sound->setSource(url2);
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Ready);
        sound->setSource(url); // set different source
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Ready);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        sound->play();
        QVERIFY(sound->isPlaying());
        sound->stop();

        sound->setSource(QUrl());
        QCOMPARE(sound->status(), QSoundEffect::Null);
    }
}

void tst_QSoundEffect::testSetSourceWhilePlaying()
{
    for (int i = 0; i < 2; i++) {
        sound->setSource(url);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        sound->play();
        QVERIFY(sound->isPlaying());
        sound->setSource(url); // set same source again
        QCOMPARE(sound->status(), QSoundEffect::Ready);
        QVERIFY(sound->isPlaying()); // playback doesn't stop, URL is the same
        sound->play();
        QVERIFY(sound->isPlaying());

        sound->setSource(QUrl());
        QCOMPARE(sound->status(), QSoundEffect::Null);

        sound->setSource(url2);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        sound->play();
        QVERIFY(sound->isPlaying());
        sound->setSource(url); // set different source
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        QVERIFY(!sound->isPlaying()); // playback stops, URL is different
        sound->play();
        QVERIFY(sound->isPlaying());
        sound->stop();

        sound->setSource(QUrl());
        QCOMPARE(sound->status(), QSoundEffect::Null);
    }
}

void tst_QSoundEffect::testSupportedMimeTypes_data()
{
    // Verify also passing of audio device info as parameter
    QTest::addColumn<QSoundEffect*>("instance");
    QTest::newRow("without QAudioDevice") << sound;
    QAudioDevice deviceInfo(QMediaDevices::defaultAudioOutput());
    QTest::newRow("with QAudioDevice")    << new QSoundEffect(deviceInfo, this);
}

void tst_QSoundEffect::testSupportedMimeTypes()
{
    QFETCH(QSoundEffect*, instance);
    QStringList mimeTypes = instance->supportedMimeTypes();
    QVERIFY(!mimeTypes.empty());
    QVERIFY(mimeTypes.indexOf(QLatin1String("audio/wav")) != -1 ||
            mimeTypes.indexOf(QLatin1String("audio/x-wav")) != -1 ||
            mimeTypes.indexOf(QLatin1String("audio/wave")) != -1 ||
            mimeTypes.indexOf(QLatin1String("audio/x-pn-wav")) != -1);
}

void tst_QSoundEffect::testCorruptFile()
{
    for (int i = 0; i < 10; i++) {
        QSignalSpy statusSpy(sound, SIGNAL(statusChanged()));
        sound->setSource(urlCorrupted);
        QVERIFY(!sound->isPlaying());
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Error);
        QTRY_COMPARE(sound->status(), QSoundEffect::Error);
        QCOMPARE(statusSpy.size(), 2);
        sound->play();
        QVERIFY(!sound->isPlaying());

        sound->setSource(url);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        sound->play();
        QVERIFY(sound->isPlaying());
    }
}

QTEST_MAIN(tst_QSoundEffect)

#include "tst_qsoundeffect.moc"
