// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QDebug>
#include "qmediaplayer.h"
#include "mediaplayerstate.h"
#include "fake.h"
#include "fixture.h"
#include "server.h"
#include <qmediametadata.h>
#include <qaudiobuffer.h>
#include <qvideosink.h>
#include <qvideoframe.h>
#include <qaudiooutput.h>
#include <qprocess.h>
#include <private/qglobal_p.h>
#ifdef QT_FEATURE_network
#include <qtcpserver.h>
#endif
#include <qmediatimerange.h>
#include <private/qplatformvideosink_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickloader_p.h>

#include "../shared/mediafileselector.h"
#include <QtMultimedia/private/qtmultimedia-config_p.h>
#include "private/qquickvideooutput_p.h"

#include <array>

QT_USE_NAMESPACE

namespace {
static qreal colorDifference(QRgb first, QRgb second)
{
    const auto diffVector = QVector3D(qRed(first), qGreen(first), qBlue(first))
            - QVector3D(qRed(second), qGreen(second), qBlue(second));
    static const auto normalizationFactor = 1. / (255 * qSqrt(3.));
    return diffVector.length() * normalizationFactor;
}

template <typename It>
It findSimilarColor(It it, It end, QRgb color)
{
    return std::min_element(it, end, [color](QRgb first, QRgb second) {
        return colorDifference(first, color) < colorDifference(second, color);
    });
}

template <typename Colors>
size_t findSimilarColorIndex(const Colors &colors, QRgb color)
{
    return std::distance(std::begin(colors),
                         findSimilarColor(std::begin(colors), std::end(colors), color));
}
}

/*
 This is the backend conformance test.

 Since it relies on platform media framework and sound hardware
 it may be less stable.
*/

class tst_QMediaPlayerBackend : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();
    void init() { m_fixture = std::make_unique<Fixture>(); }
    void cleanup() { m_fixture = nullptr; }

private slots:
    void testMediaFilesAreSupported();
    void destructor_cancelsPreviousSetSource_whenServerDoesNotRespond();
    void destructor_emitsOnlyQObjectDestroyedSignal_whenPlayerIsRunning();

    void getters_returnExpectedValues_whenCalledWithDefaultConstructedPlayer_data() const;
    void getters_returnExpectedValues_whenCalledWithDefaultConstructedPlayer() const;

    void setSource_emitsSourceChanged_whenCalledWithInvalidFile();
    void setSource_emitsError_whenCalledWithInvalidFile();
    void setSource_emitsMediaStatusChange_whenCalledWithInvalidFile();
    void setSource_doesNotEmitPlaybackStateChange_whenCalledWithInvalidFile();
    void setSource_setsSourceMediaStatusAndError_whenCalledWithInvalidFile();
    void setSource_silentlyCancelsPreviousCall_whenServerDoesNotRespond();
    void setSource_changesSourceAndMediaStatus_whenCalledWithValidFile();
    void setSource_updatesExpectedAttributes_whenMediaHasLoaded();
    void setSource_stopsAndEntersErrorState_whenPlayerWasPlaying();
    void setSource_loadsAudioTrack_whenCalledWithValidWavFile();
    void setSource_resetsState_whenCalledWithEmptyUrl();
    void setSource_loadsNewMedia_whenPreviousMediaWasFullyLoaded();
    void setSource_loadsCorrectTracks_whenLoadingMediaInSequence();
    void setSource_remainsInStoppedState_whenPlayerWasStopped();
    void setSource_entersStoppedState_whenPlayerWasPlaying();

    void setSourceAndPlay_setCorrectVideoSize_whenVideoHasNonStandardPixelAspectRatio_data();
    void setSourceAndPlay_setCorrectVideoSize_whenVideoHasNonStandardPixelAspectRatio();

    void pause_doesNotChangePlayerState_whenInvalidFileLoaded();
    void pause_doesNothing_whenMediaIsNotLoaded();
    void pause_entersPauseState_whenPlayerWasPlaying();

    void play_resetsErrorState_whenCalledWithInvalidFile();
    void play_resumesPlaying_whenValidMediaIsProvidedAfterInvalidMedia();
    void play_doesNothing_whenMediaIsNotLoaded();
    void play_setsPlaybackStateAndMediaStatus_whenValidFileIsLoaded();
    void play_startsPlaybackAndChangesPosition_whenValidFileIsLoaded();
    void play_doesNotEnterMediaLoadingState_whenResumingPlayingAfterStop();
    void playAndSetSource_emitsExpectedSignalsAndStopsPlayback_whenSetSourceWasCalledWithEmptyUrl();
    void play_createsFramesWithExpectedContentAndIncreasingFrameTime_whenPlayingRtspMediaStream();
    void play_waitsForLastFrameEnd_whenPlayingVideoWithLongFrames();

    void stop_entersStoppedState_whenPlayerWasPaused();
    void stop_setsPositionToZero_afterPlayingToEndOfMedia();

    void playbackRate_returnsOne_byDefault();
    void setPlaybackRate_changesPlaybackRateAndEmitsSignal_data();
    void setPlaybackRate_changesPlaybackRateAndEmitsSignal();

    void setVolume_changesVolume_whenVolumeIsInRange();
    void setVolume_clampsToRange_whenVolumeIsOutsideRange();
    void setVolume_doesNotChangeMutedState();

    void setMuted_changesMutedState_whenMutedStateChanged();
    void setMuted_doesNotChangeVolume();

    void processEOS();
    void deleteLaterAtEOS();

    void volumeAcrossFiles_data();
    void volumeAcrossFiles();
    void initialVolume();
    void seekPauseSeek();
    void seekInStoppedState();
    void subsequentPlayback();
    void surfaceTest();
    void metadata();
    void metadata_returnsMetadataWithThumbnail_whenMediaHasThumbnail_data();
    void metadata_returnsMetadataWithThumbnail_whenMediaHasThumbnail();
    void playerStateAtEOS();
    void playFromBuffer();
    void audioVideoAvailable();
    void isSeekable();
    void positionAfterSeek();
    void videoDimensions();
    void position();
    void multipleMediaPlayback();
    void multiplePlaybackRateChangingStressTest();
    void multipleSeekStressTest();
    void setPlaybackRate_changesActualRateAndFramesRenderingTime_data();
    void setPlaybackRate_changesActualRateAndFramesRenderingTime();
    void durationDetectionIssues_data();
    void durationDetectionIssues();
    void finiteLoops();
    void infiniteLoops();
    void seekOnLoops();
    void changeLoopsOnTheFly();
    void changeVideoOutputNoFramesLost();
    void cleanSinkAndNoMoreFramesAfterStop();
    void lazyLoadVideo();
    void videoSinkSignals();
    void nonAsciiFileName();
    void setMedia_setsVideoSinkSize_beforePlaying();
    void play_playsRotatedVideoOutput_whenVideoFileHasOrientationMetadata_data();
    void play_playsRotatedVideoOutput_whenVideoFileHasOrientationMetadata();

private:
    QUrl selectVideoFile(const QStringList &mediaCandidates);

    bool canCreateRtspStream() const;
    std::unique_ptr<QProcess> createRtspStreamProcess(QString fileName, QString outputUrl);
    void detectVlcCommand();

    //one second local wav file
    MaybeUrl m_localWavFile = QUnexpect{};
    MaybeUrl m_localWavFile2 = QUnexpect{};
    MaybeUrl m_localVideoFile = QUnexpect{};
    MaybeUrl m_localVideoFile2 = QUnexpect{};
    MaybeUrl m_videoDimensionTestFile = QUnexpect{};
    MaybeUrl m_localCompressedSoundFile = QUnexpect{};
    MaybeUrl m_localFileWithMetadata = QUnexpect{};
    MaybeUrl m_localVideoFile3ColorsWithSound = QUnexpect{};
    MaybeUrl m_videoFileWithJpegThumbnail = QUnexpect{};
    MaybeUrl m_videoFileWithPngThumbnail = QUnexpect{};
    MaybeUrl m_oneRedFrameVideo = QUnexpect{};
    MaybeUrl m_192x108_PAR_2_3_Video = QUnexpect{};
    MaybeUrl m_192x108_PAR_3_2_Video = QUnexpect{};
    MaybeUrl m_colorMatrixVideo = QUnexpect{};
    MaybeUrl m_colorMatrix90degClockwiseVideo = QUnexpect{};
    MaybeUrl m_colorMatrix180degClockwiseVideo = QUnexpect{};
    MaybeUrl m_colorMatrix270degClockwiseVideo = QUnexpect{};

    MediaFileSelector m_mediaSelector;

    const std::array<QRgb, 3> m_video3Colors = { { 0xFF0000, 0x00FF00, 0x0000FF } };
    QString m_vlcCommand;

    std::unique_ptr<Fixture> m_fixture;
};


static bool commandExists(const QString &command)
{

#if defined(Q_OS_WINDOWS)
    static constexpr QChar separator = ';';
#else
    static constexpr QChar separator = ':';
#endif
    static const QStringList pathDirs = qEnvironmentVariable("PATH").split(separator);
    return std::any_of(pathDirs.cbegin(), pathDirs.cend(), [&command](const QString &dir) {
        QString fullPath = QDir(dir).filePath(command);
        return QFile::exists(fullPath);
    });
}

static std::unique_ptr<QTemporaryFile> copyResourceToTemporaryFile(QString resource,
                                                                   QString filePattern)
{
    QFile resourceFile(resource);
    if (!resourceFile.open(QIODeviceBase::ReadOnly))
        return nullptr;

    auto temporaryFile = std::make_unique<QTemporaryFile>(filePattern);
    if (!temporaryFile->open())
        return nullptr;

    QByteArray bytes = resourceFile.readAll();
    QDataStream stream(temporaryFile.get());
    stream.writeRawData(bytes.data(), bytes.length());

    temporaryFile->close();

    return temporaryFile;
}

void tst_QMediaPlayerBackend::detectVlcCommand()
{
    m_vlcCommand = qEnvironmentVariable("QT_VLC_COMMAND");

    if (!m_vlcCommand.isEmpty())
        return;

#if defined(Q_OS_WINDOWS)
    m_vlcCommand = "vlc.exe";
#else
    m_vlcCommand = "vlc";
#endif
    if (commandExists(m_vlcCommand))
        return;

    m_vlcCommand.clear();

#if defined(Q_OS_MACOS)
    m_vlcCommand = "/Applications/VLC.app/Contents/MacOS/VLC";
#elif defined(Q_OS_WINDOWS)
    m_vlcCommand = "C:/Program Files/VideoLAN/VLC/vlc.exe";
#endif

    if (!QFile::exists(m_vlcCommand))
        m_vlcCommand.clear();
}

bool tst_QMediaPlayerBackend::canCreateRtspStream() const
{
    return !m_vlcCommand.isEmpty();
}

void tst_QMediaPlayerBackend::initTestCase()
{
#ifdef Q_OS_ANDROID
     QSKIP("SKIP initTestCase on CI, because of QTBUG-118571");
#endif

    QMediaPlayer player;
    if (!player.isAvailable())
        QSKIP("Media player service is not available");

    qRegisterMetaType<MaybeUrl>();

    m_localWavFile = m_mediaSelector.select("qrc:/testdata/test.wav");
    m_localWavFile2 = m_mediaSelector.select("qrc:/testdata/_test.wav");

    m_localVideoFile =
            m_mediaSelector.select("qrc:/testdata/colors.mp4", "qrc:/testdata/colors.ogv");

    m_localVideoFile3ColorsWithSound =
            m_mediaSelector.select("qrc:/testdata/3colors_with_sound_1s.mp4");

    m_videoFileWithJpegThumbnail =
            m_mediaSelector.select("qrc:/testdata/audio_video_with_jpg_thumbnail.mp4");

    m_videoFileWithPngThumbnail =
            m_mediaSelector.select("qrc:/testdata/audio_video_with_png_thumbnail.mp4");

    m_localVideoFile2 =
            m_mediaSelector.select("qrc:/testdata/BigBuckBunny.mp4", "qrc:/testdata/busMpeg4.mp4");

    m_videoDimensionTestFile = m_mediaSelector.select("qrc:/testdata/BigBuckBunny.mp4");

    m_localCompressedSoundFile =
            m_mediaSelector.select("qrc:/testdata/nokia-tune.mp3", "qrc:/testdata/nokia-tune.mkv");

    m_localFileWithMetadata = m_mediaSelector.select("qrc:/testdata/nokia-tune.mp3");

    m_oneRedFrameVideo = m_mediaSelector.select("qrc:/testdata/one_red_frame.mp4");

    m_192x108_PAR_2_3_Video = m_mediaSelector.select("qrc:/testdata/par_2_3.mp4");
    m_192x108_PAR_3_2_Video = m_mediaSelector.select("qrc:/testdata/par_3_2.mp4");

    m_colorMatrixVideo = m_mediaSelector.select("qrc:/testdata/color_matrix.mp4");
    m_colorMatrix90degClockwiseVideo =
            m_mediaSelector.select("qrc:/testdata/color_matrix_90_deg_clockwise.mp4");
    m_colorMatrix180degClockwiseVideo =
            m_mediaSelector.select("qrc:/testdata/color_matrix_180_deg_clockwise.mp4");
    m_colorMatrix270degClockwiseVideo =
            m_mediaSelector.select("qrc:/testdata/color_matrix_270_deg_clockwise.mp4");

    detectVlcCommand();
}

void tst_QMediaPlayerBackend::testMediaFilesAreSupported()
{
    const auto mediaSelectionErrors = m_mediaSelector.dumpErrors();
    if (!mediaSelectionErrors.isEmpty())
        qDebug().noquote() << "Dump media selection errors:\n" << mediaSelectionErrors;

    // TODO: probalbly, we should check errors anyway; TBD.
    QCOMPARE(m_mediaSelector.failedSelectionsCount(), 0);
}

void tst_QMediaPlayerBackend::destructor_cancelsPreviousSetSource_whenServerDoesNotRespond()
{
#ifdef QT_FEATURE_network
    UnResponsiveRtspServer server;
    QVERIFY(server.listen());

    auto player = std::make_unique<QMediaPlayer>();
    player->setSource(server.address());

    QVERIFY(server.waitForConnection());

    // Cancel connection (should be fast, but can't be reliably verified
    // in a test. For now we just verify that we don't crash.
    player = nullptr;
#else
    QSKIP("Test requires network feature");
#endif
}

void tst_QMediaPlayerBackend::destructor_emitsOnlyQObjectDestroyedSignal_whenPlayerIsRunning()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

    // Arrange
    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);
    m_fixture->player.play();
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);

    m_fixture->clearSpies();

    // Act
    m_fixture->player.~QMediaPlayer();
    new (&m_fixture->player) QMediaPlayer;

    // Assert
    QCOMPARE(m_fixture->playbackStateChanged.size(), 0);
    QCOMPARE(m_fixture->errorOccurred.size(), 0);
    QCOMPARE(m_fixture->sourceChanged.size(), 0);
    QCOMPARE(m_fixture->mediaStatusChanged.size(), 0);
    QCOMPARE(m_fixture->positionChanged.size(), 0);
    QCOMPARE(m_fixture->durationChanged.size(), 0);
    QCOMPARE(m_fixture->metadataChanged.size(), 0);
    QCOMPARE(m_fixture->volumeChanged.size(), 0);
    QCOMPARE(m_fixture->mutedChanged.size(), 0);
    QCOMPARE(m_fixture->bufferProgressChanged.size(), 0);
    QCOMPARE(m_fixture->destroyed.size(), 1);
}

void tst_QMediaPlayerBackend::
        getters_returnExpectedValues_whenCalledWithDefaultConstructedPlayer_data() const
{
    QTest::addColumn<bool>("hasAudioOutput");
    QTest::addColumn<bool>("hasVideoOutput");
    QTest::addColumn<bool>("hasVideoSink");

    QTest::newRow("noOutput") << false << false << false;
    QTest::newRow("withAudioOutput") << true << false << false;
    QTest::newRow("withVideoOutput") << false << true << false;
    QTest::newRow("withVideoSink") << false << false << true;
    QTest::newRow("withAllOutputs") << true << true << true;
}

void tst_QMediaPlayerBackend::getters_returnExpectedValues_whenCalledWithDefaultConstructedPlayer()
        const
{
    QFETCH(const bool, hasAudioOutput);
    QFETCH(const bool, hasVideoOutput);
    QFETCH(const bool, hasVideoSink);

    QAudioOutput audioOutput;
    TestVideoOutput videoOutput;

    QMediaPlayer player;

    if (hasAudioOutput)
        player.setAudioOutput(&audioOutput);

    if (hasVideoOutput)
        player.setVideoOutput(&videoOutput);

    if (hasVideoSink)
        player.setVideoSink(videoOutput.videoSink());

    MediaPlayerState expectedState = MediaPlayerState::defaultState();
    expectedState.audioOutput = hasAudioOutput ? &audioOutput : nullptr;
    expectedState.videoOutput = (hasVideoOutput && !hasVideoSink) ? &videoOutput : nullptr;
    expectedState.videoSink = (hasVideoSink || hasVideoOutput) ? videoOutput.videoSink() : nullptr;

    const MediaPlayerState actualState{ player };
    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);
}

void tst_QMediaPlayerBackend::setSource_emitsSourceChanged_whenCalledWithInvalidFile()
{
    m_fixture->player.setSource({ "Some not existing media" });
    QTRY_COMPARE_EQ(m_fixture->player.error(), QMediaPlayer::ResourceError);

    QCOMPARE_EQ(m_fixture->sourceChanged, SignalList({ { QUrl("Some not existing media") } }));
}

void tst_QMediaPlayerBackend::setSource_emitsError_whenCalledWithInvalidFile()
{
    m_fixture->player.setSource({ "Some not existing media" });
    QTRY_COMPARE_EQ(m_fixture->player.error(), QMediaPlayer::ResourceError);

    QCOMPARE_EQ(m_fixture->errorOccurred[0][0], QMediaPlayer::ResourceError);
}

void tst_QMediaPlayerBackend::setSource_emitsMediaStatusChange_whenCalledWithInvalidFile()
{
    m_fixture->player.setSource({ "Some not existing media" });
    QTRY_COMPARE_EQ(m_fixture->player.error(), QMediaPlayer::ResourceError);

    QCOMPARE_EQ(m_fixture->mediaStatusChanged,
                SignalList({ { QMediaPlayer::LoadingMedia }, { QMediaPlayer::InvalidMedia } }));
}

void tst_QMediaPlayerBackend::setSource_doesNotEmitPlaybackStateChange_whenCalledWithInvalidFile()
{
    m_fixture->player.setSource({ "Some not existing media" });
    QTRY_COMPARE_EQ(m_fixture->player.error(), QMediaPlayer::ResourceError);

    QVERIFY(m_fixture->playbackStateChanged.empty());
}

void tst_QMediaPlayerBackend::setSource_setsSourceMediaStatusAndError_whenCalledWithInvalidFile()
{
    const QUrl invalidFile{ "Some not existing media" };

    m_fixture->player.setSource(invalidFile);
    QTRY_COMPARE_EQ(m_fixture->player.error(), QMediaPlayer::ResourceError);

    MediaPlayerState expectedState = MediaPlayerState::defaultState();
    expectedState.source = invalidFile;
    expectedState.mediaStatus = QMediaPlayer::InvalidMedia;
    expectedState.error = QMediaPlayer::ResourceError;

    const MediaPlayerState actualState{ m_fixture->player };

    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);
}

void tst_QMediaPlayerBackend::setSource_silentlyCancelsPreviousCall_whenServerDoesNotRespond()
{
#ifdef QT_FEATURE_network
    CHECK_SELECTED_URL(m_localVideoFile);

    UnResponsiveRtspServer server;

    QVERIFY(server.listen());

    m_fixture->player.setSource(server.address());
    QVERIFY(server.waitForConnection());

    m_fixture->player.setSource(*m_localVideoFile);

    // Cancellation can not be reliably verified due to relatively short timeout,
    // but we can verify that the player is in the correct state.
    QTRY_COMPARE_EQ(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);

    // Cancellation is silent
    QVERIFY(m_fixture->errorOccurred.empty());

    // Media status is emitted as if only one file was loaded
    const SignalList expectedMediaStatus = { { QMediaPlayer::LoadingMedia },
                                             { QMediaPlayer::LoadedMedia } };
    QCOMPARE_EQ(m_fixture->mediaStatusChanged, expectedMediaStatus);

    // Two media source changed signals should be emitted still
    const SignalList expectedSource = { { server.address() }, { *m_localVideoFile } };
    QCOMPARE_EQ(m_fixture->sourceChanged, expectedSource);

#else
    QSKIP("Test requires network feature");
#endif
}

void tst_QMediaPlayerBackend::setSource_changesSourceAndMediaStatus_whenCalledWithValidFile()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    m_fixture->player.setSource(*m_localVideoFile);

    QCOMPARE_EQ(m_fixture->mediaStatusChanged, SignalList({ { QMediaPlayer::LoadingMedia } }));

    MediaPlayerState expectedState = MediaPlayerState::defaultState();
    expectedState.source = *m_localVideoFile;
    expectedState.mediaStatus = QMediaPlayer::LoadingMedia;

    MediaPlayerState actualState{ m_fixture->player };

    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);
}

void tst_QMediaPlayerBackend::setSource_updatesExpectedAttributes_whenMediaHasLoaded()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    m_fixture->player.setSource(*m_localVideoFile);

    QTRY_COMPARE_EQ(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);

    MediaPlayerState expectedState = MediaPlayerState::defaultState();

    // Modify all attributes that are supposed to change with this media file
    // All other state variables are verified to be unchanged.
    expectedState.source = *m_localVideoFile;
    expectedState.mediaStatus = QMediaPlayer::LoadedMedia;
    expectedState.audioTracks = std::nullopt; // Don't compare
    expectedState.videoTracks = std::nullopt; // Don't compare
    expectedState.activeAudioTrack = 0;
    expectedState.activeVideoTrack = 0;
    expectedState.duration = 15018;
    expectedState.hasAudio = true;
    expectedState.hasVideo = true;
    expectedState.isSeekable = true;
    expectedState.metaData = std::nullopt; // Don't compare

    MediaPlayerState actualState{ m_fixture->player };

    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);
}

void tst_QMediaPlayerBackend::setSource_stopsAndEntersErrorState_whenPlayerWasPlaying()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

    // Arrange
    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);
    m_fixture->player.play();
    QTRY_VERIFY(m_fixture->framesCount > 0);
    QCOMPARE(m_fixture->errorOccurred.size(), 0);

    // Act
    m_fixture->player.setSource(QUrl("Some not existing media"));

    // Assert
    const int savedFramesCount = m_fixture->framesCount;

    QCOMPARE(m_fixture->player.source(), QUrl("Some not existing media"));

    QTRY_COMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::InvalidMedia);
    QTRY_COMPARE(m_fixture->player.error(), QMediaPlayer::ResourceError);

    QVERIFY(!m_fixture->surface.videoFrame().isValid());

    QCOMPARE(m_fixture->errorOccurred.size(), 1);

    QTest::qWait(20);
    QCOMPARE(m_fixture->framesCount, savedFramesCount);
}


void tst_QMediaPlayerBackend::setSource_loadsAudioTrack_whenCalledWithValidWavFile()
{
    CHECK_SELECTED_URL(m_localWavFile);

    m_fixture->player.setSource(*m_localWavFile);

    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);

    QVERIFY(m_fixture->player.mediaStatus() != QMediaPlayer::NoMedia);
    QVERIFY(m_fixture->player.mediaStatus() != QMediaPlayer::InvalidMedia);
    QVERIFY(m_fixture->player.source() == *m_localWavFile);

    QCOMPARE(m_fixture->playbackStateChanged.size(), 0);
    QVERIFY(m_fixture->mediaStatusChanged.size() > 0);
    QCOMPARE(m_fixture->sourceChanged.size(), 1);
    QCOMPARE(m_fixture->sourceChanged.last()[0].value<QUrl>(), *m_localWavFile);

    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(m_fixture->player.hasAudio());
    QVERIFY(!m_fixture->player.hasVideo());
}

void tst_QMediaPlayerBackend::setSource_resetsState_whenCalledWithEmptyUrl()
{
    CHECK_SELECTED_URL(m_localWavFile);

    // Load valid media and start playing
    m_fixture->player.setSource(*m_localWavFile);

    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QVERIFY(m_fixture->player.position() == 0);
#ifdef Q_OS_QNX
    // QNX mm-renderer only updates the duration when 'play' is triggered
    QVERIFY(m_fixture->player.duration() == 0);
#else
    QVERIFY(m_fixture->player.duration() > 0);
#endif

    m_fixture->player.play();

    QTRY_VERIFY(m_fixture->player.position() > 0);
    QVERIFY(m_fixture->player.duration() > 0);

    // Set empty URL and verify that state is fully reset to default
    m_fixture->clearSpies();

    m_fixture->player.setSource(QUrl());

    QVERIFY(!m_fixture->mediaStatusChanged.isEmpty());
    QVERIFY(!m_fixture->sourceChanged.isEmpty());

    const MediaPlayerState expectedState = MediaPlayerState::defaultState();
    const MediaPlayerState actualState{ m_fixture->player };

    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);
}

void tst_QMediaPlayerBackend::setSource_loadsNewMedia_whenPreviousMediaWasFullyLoaded()
{
    CHECK_SELECTED_URL(m_localWavFile);
    CHECK_SELECTED_URL(m_localWavFile2);

    // Load media and wait for it to completely load
    m_fixture->player.setSource(*m_localWavFile2);
    QCOMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadingMedia);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);

    // Load another media file, play it, and wait for it to enter playing state
    m_fixture->player.setSource(*m_localWavFile);
    QCOMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadingMedia);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);
    m_fixture->player.play();
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);

    // Load first file again, and wait for it to start loading
    m_fixture->player.setSource(*m_localWavFile2);
    QCOMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadingMedia);
}

void tst_QMediaPlayerBackend::setSource_loadsCorrectTracks_whenLoadingMediaInSequence()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);
    CHECK_SELECTED_URL(m_localWavFile2);

    // Load audio/video file, play it, and verify that both tracks are loaded
    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);
    m_fixture->player.play();
    QTRY_COMPARE_EQ(m_fixture->player.playbackState(), QMediaPlayer::PlayingState);
    QVERIFY(m_fixture->surface.waitForFrame().isValid());
    QVERIFY(m_fixture->player.hasAudio());
    QVERIFY(m_fixture->player.hasVideo());

    m_fixture->clearSpies();

    // Load an audio file, and verify that only audio track is loaded
    m_fixture->player.setSource(*m_localWavFile2);

    QTRY_COMPARE_EQ(m_fixture->player.mediaStatus(), QMediaPlayer::MediaStatus::LoadedMedia);

    QCOMPARE(m_fixture->player.source(), *m_localWavFile2);
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(m_fixture->playbackStateChanged.size(), 1);
    QCOMPARE(m_fixture->errorOccurred.size(), 0);
    QVERIFY(m_fixture->player.hasAudio());
    QVERIFY(!m_fixture->player.hasVideo());
    QVERIFY(!m_fixture->surface.videoFrame().isValid());

    m_fixture->player.play();

    // Load video only file, and verify that only video track is loaded
    m_fixture->player.setSource(*m_localVideoFile2);

    QTRY_COMPARE_EQ(m_fixture->player.mediaStatus(), QMediaPlayer::MediaStatus::LoadedMedia);

    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QVERIFY(m_fixture->player.hasVideo());
    QVERIFY(!m_fixture->player.hasAudio());
    QCOMPARE(m_fixture->errorOccurred.size(), 0);
}

void tst_QMediaPlayerBackend::setSource_remainsInStoppedState_whenPlayerWasStopped()
{
    CHECK_SELECTED_URL(m_localWavFile);
    CHECK_SELECTED_URL(m_localWavFile2);

    // Arrange
    m_fixture->player.setSource(*m_localWavFile);
    m_fixture->player.play();
    QTRY_VERIFY(m_fixture->player.position() > 100);
    m_fixture->player.stop();
    m_fixture->clearSpies();

    // Act
    m_fixture->player.setSource(*m_localWavFile2);

    // Assert
    QTRY_VERIFY(m_fixture->mediaStatusChanged.size() > 0);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE_EQ(m_fixture->mediaStatusChanged,
                SignalList({ { QMediaPlayer::LoadingMedia }, { QMediaPlayer::LoadedMedia } }));
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QVERIFY(m_fixture->playbackStateChanged.empty());
}

void tst_QMediaPlayerBackend::setSource_entersStoppedState_whenPlayerWasPlaying()
{
    CHECK_SELECTED_URL(m_localWavFile);
    CHECK_SELECTED_URL(m_localWavFile2);

    // Arrange
    m_fixture->player.setSource(*m_localWavFile2);
    m_fixture->clearSpies();
    m_fixture->player.play();
    QTRY_VERIFY(m_fixture->player.position() > 100);

    // Act
    m_fixture->player.setSource(*m_localWavFile);

    // Assert
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_COMPARE(m_fixture->mediaStatusChanged,
                 SignalList({ { QMediaPlayer::LoadedMedia },
                              { QMediaPlayer::BufferingMedia },
                              { QMediaPlayer::BufferedMedia },
                              { QMediaPlayer::LoadedMedia },
                              { QMediaPlayer::LoadingMedia },
                              { QMediaPlayer::LoadedMedia } }));

    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(m_fixture->playbackStateChanged,
                 SignalList({ { QMediaPlayer::PlayingState }, { QMediaPlayer::StoppedState } }));

    QTRY_VERIFY(!m_fixture->positionChanged.empty()
                && m_fixture->positionChanged.last()[0].value<qint64>() == 0);

    QCOMPARE(m_fixture->player.position(), 0);
}

void tst_QMediaPlayerBackend::
        setSourceAndPlay_setCorrectVideoSize_whenVideoHasNonStandardPixelAspectRatio_data()
{
    QTest::addColumn<MaybeUrl>("url");
    QTest::addColumn<QSize>("expectedVideoSize");

    QTest::addRow("Horizontal expanding (par=3/2)")
            << m_192x108_PAR_3_2_Video << QSize(192 * 3 / 2, 108);
    QTest::addRow("Vertical expanding (par=2/3)")
            << m_192x108_PAR_2_3_Video << QSize(192, 108 * 3 / 2);
}

void tst_QMediaPlayerBackend::
        setSourceAndPlay_setCorrectVideoSize_whenVideoHasNonStandardPixelAspectRatio()
{
    QFETCH(MaybeUrl, url);
    QFETCH(QSize, expectedVideoSize);

    CHECK_SELECTED_URL(url);

    m_fixture->player.setSource(*url);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_fixture->player.metaData().value(QMediaMetaData::Resolution), QSize(192, 108));

    QCOMPARE(m_fixture->surface.videoSize(), expectedVideoSize);

    m_fixture->player.play();

    auto frame = m_fixture->surface.waitForFrame();
    QVERIFY(frame.isValid());
    QCOMPARE(frame.size(), expectedVideoSize);
    QCOMPARE(frame.surfaceFormat().frameSize(), expectedVideoSize);
    QCOMPARE(frame.surfaceFormat().viewport(), QRect(QPoint(), expectedVideoSize));

#ifdef Q_OS_ANDROID
    QSKIP("frame.toImage will return null image because of QTBUG-108446");
#endif

    auto image = frame.toImage();
    QCOMPARE(frame.size(), expectedVideoSize);

    // clang-format off

    // Video schema:
    //
    //           192
    // /---------------------\
    // |   White  |          |
    // |          |          |
    // |----------/          | 108
    // |              Red    |
    // |                     |
    // \---------------------/

    // clang-format on

    // check the proper scaling
    const std::vector<QRgb> colors = { 0xFFFFFF, 0xFF0000, 0xFF00, 0xFF, 0x0 };

    const auto pixelsOffset = 4;
    const auto halfSize = expectedVideoSize / 2;

    QCOMPARE(findSimilarColorIndex(colors, image.pixel(0, 0)), 0);
    QCOMPARE(findSimilarColorIndex(colors, image.pixel(halfSize.width() - pixelsOffset, 0)), 0);
    QCOMPARE(findSimilarColorIndex(colors, image.pixel(0, halfSize.height() - pixelsOffset)), 0);
    QCOMPARE(findSimilarColorIndex(colors,
                                   image.pixel(halfSize.width() - pixelsOffset,
                                               halfSize.height() - pixelsOffset)),
             0);

    QCOMPARE(findSimilarColorIndex(colors, image.pixel(halfSize.width() + pixelsOffset, 0)), 1);
    QCOMPARE(findSimilarColorIndex(colors, image.pixel(0, halfSize.height() + pixelsOffset)), 1);
    QCOMPARE(findSimilarColorIndex(colors,
                                   image.pixel(halfSize.width() + pixelsOffset,
                                               halfSize.height() + pixelsOffset)),
             1);
}

void tst_QMediaPlayerBackend::pause_doesNotChangePlayerState_whenInvalidFileLoaded()
{
    m_fixture->player.setSource({ "Some not existing media" });
    QTRY_COMPARE_EQ(m_fixture->player.error(), QMediaPlayer::ResourceError);

    const MediaPlayerState expectedState{ m_fixture->player };

    m_fixture->player.pause();

    const MediaPlayerState actualState{ m_fixture->player };

    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);
}

void tst_QMediaPlayerBackend::pause_doesNothing_whenMediaIsNotLoaded()
{
    m_fixture->player.pause();

    const MediaPlayerState expectedState = MediaPlayerState::defaultState();
    const MediaPlayerState actualState{ m_fixture->player };

    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);

    QVERIFY(m_fixture->playbackStateChanged.empty());
    QVERIFY(m_fixture->mediaStatusChanged.empty());
    QVERIFY(m_fixture->positionChanged.empty());
    QVERIFY(m_fixture->errorOccurred.empty());
}

void tst_QMediaPlayerBackend::pause_entersPauseState_whenPlayerWasPlaying()
{
    CHECK_SELECTED_URL(m_localWavFile);

    // Arrange
    m_fixture->player.setSource(*m_localWavFile);
    m_fixture->player.play();
    QTRY_VERIFY(m_fixture->player.position() > 100);
    m_fixture->clearSpies();
    const qint64 positionBeforePause = m_fixture->player.position();

    // Act
    m_fixture->player.pause();

    // Assert
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::PausedState);
    QCOMPARE_EQ(m_fixture->playbackStateChanged, SignalList({ { QMediaPlayer::PausedState } }));
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QTest::qWait(500);

    QTRY_VERIFY(qAbs(m_fixture->player.position() - positionBeforePause) < 150);
}

void tst_QMediaPlayerBackend::play_resetsErrorState_whenCalledWithInvalidFile()
{
    m_fixture->player.setSource({ "Some not existing media" });
    QTRY_COMPARE_EQ(m_fixture->player.error(), QMediaPlayer::ResourceError);

    MediaPlayerState expectedState{ m_fixture->player };

    m_fixture->player.play();

    expectedState.error = QMediaPlayer::NoError;
    COMPARE_MEDIA_PLAYER_STATE_EQ(MediaPlayerState{ m_fixture->player }, expectedState);

    QTest::qWait(150); // wait a bit and check position is not changed

    COMPARE_MEDIA_PLAYER_STATE_EQ(MediaPlayerState{ m_fixture->player }, expectedState);
    QCOMPARE(m_fixture->surface.m_totalFrames, 0);
}

void tst_QMediaPlayerBackend::play_resumesPlaying_whenValidMediaIsProvidedAfterInvalidMedia()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

    // Arrange
    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);
    m_fixture->player.play();
    QTRY_VERIFY(m_fixture->framesCount > 0);
    m_fixture->player.setSource(QUrl("Some not existing media"));
    QTRY_COMPARE(m_fixture->player.error(), QMediaPlayer::ResourceError);
    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);

    // Act
    m_fixture->player.play();

    // Assert
    QTRY_VERIFY(m_fixture->framesCount > 0);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);
    QCOMPARE_EQ(m_fixture->player.playbackState(), QMediaPlayer::PlayingState);
    QCOMPARE(m_fixture->player.error(), QMediaPlayer::NoError);
}

void tst_QMediaPlayerBackend::play_doesNothing_whenMediaIsNotLoaded()
{
    m_fixture->player.play();

    const MediaPlayerState expectedState = MediaPlayerState::defaultState();
    const MediaPlayerState actualState{ m_fixture->player };

    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);

    QVERIFY(m_fixture->playbackStateChanged.empty());
    QVERIFY(m_fixture->mediaStatusChanged.empty());
    QVERIFY(m_fixture->positionChanged.empty());
    QVERIFY(m_fixture->errorOccurred.empty());
}

void tst_QMediaPlayerBackend::play_setsPlaybackStateAndMediaStatus_whenValidFileIsLoaded()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    m_fixture->player.setSource(*m_localVideoFile);
    m_fixture->player.play();

    QTRY_COMPARE_EQ(m_fixture->player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(m_fixture->playbackStateChanged, SignalList({ { QMediaPlayer::PlayingState } }));
    QTRY_COMPARE(m_fixture->mediaStatusChanged,
                 SignalList({ { QMediaPlayer::LoadingMedia },
                              { QMediaPlayer::LoadedMedia },
                              { QMediaPlayer::BufferingMedia },
                              { QMediaPlayer::BufferedMedia } }));

    QTRY_COMPARE_GT(m_fixture->bufferProgressChanged.size(), 0);
    QTRY_COMPARE_NE(m_fixture->bufferProgressChanged.front().front(), 0.f);
    QTRY_COMPARE(m_fixture->bufferProgressChanged.back().front(), 1.f);
}

void tst_QMediaPlayerBackend::play_startsPlaybackAndChangesPosition_whenValidFileIsLoaded()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    m_fixture->player.setSource(*m_localVideoFile);
    m_fixture->player.play();

    QTRY_VERIFY(m_fixture->player.position() > 100);
    QTRY_VERIFY(!m_fixture->durationChanged.empty());
    QTRY_VERIFY(!m_fixture->positionChanged.empty());
    QTRY_VERIFY(m_fixture->positionChanged.last()[0].value<qint64>() > 100);
}

void tst_QMediaPlayerBackend::play_doesNotEnterMediaLoadingState_whenResumingPlayingAfterStop()
{
    CHECK_SELECTED_URL(m_localWavFile);

    // Arrange: go through a play->pause->stop sequence
    m_fixture->player.setSource(*m_localWavFile);
    m_fixture->player.play();
    QTRY_VERIFY(m_fixture->player.position() > 100);
    m_fixture->player.pause();
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);
    m_fixture->player.stop();
    m_fixture->clearSpies();

    // Act
    m_fixture->player.play();

    // Assert
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferingMedia);
    QCOMPARE_EQ(m_fixture->playbackStateChanged, SignalList({ { QMediaPlayer::PlayingState } }));

    // Note: Should not go through Loading again when play -> stop -> play
    QCOMPARE_EQ(m_fixture->mediaStatusChanged, SignalList({ { QMediaPlayer::BufferingMedia } }));
}

void tst_QMediaPlayerBackend::playAndSetSource_emitsExpectedSignalsAndStopsPlayback_whenSetSourceWasCalledWithEmptyUrl()
{
    CHECK_SELECTED_URL(m_localWavFile2);

    // Arrange
    m_fixture->player.setSource(*m_localWavFile2);
    m_fixture->clearSpies();

    // Act
    m_fixture->player.play();
    QTRY_VERIFY(m_fixture->player.position() > 100);
    m_fixture->player.setSource(QUrl());

    // Assert
    const MediaPlayerState expectedState = MediaPlayerState::defaultState();
    const MediaPlayerState actualState{ m_fixture->player };
    COMPARE_MEDIA_PLAYER_STATE_EQ(actualState, expectedState);

    QTRY_COMPARE_EQ(m_fixture->mediaStatusChanged,
                    SignalList({ { QMediaPlayer::LoadedMedia },
                                 { QMediaPlayer::BufferingMedia },
                                 { QMediaPlayer::BufferedMedia },
                                 { QMediaPlayer::LoadedMedia },
                                 { QMediaPlayer::NoMedia } }));

    QTRY_COMPARE_EQ(m_fixture->playbackStateChanged,
                    SignalList({ { QMediaPlayer::PlayingState }, { QMediaPlayer::StoppedState } }));

    QTRY_VERIFY(m_fixture->positionChanged.size() > 0);
    QCOMPARE(m_fixture->positionChanged.last()[0].value<qint64>(), 0);
}

void tst_QMediaPlayerBackend::
        play_createsFramesWithExpectedContentAndIncreasingFrameTime_whenPlayingRtspMediaStream()
{
    if (!canCreateRtspStream())
        QSKIP("Rtsp stream cannot be created");

    auto temporaryFile = copyResourceToTemporaryFile(":/testdata/colors.mp4", "colors.XXXXXX.mp4");
    QVERIFY(temporaryFile);

    const QString streamUrl = "rtsp://localhost:8083/stream";

    auto process = createRtspStreamProcess(temporaryFile->fileName(), streamUrl);
    QVERIFY2(process, "Cannot start rtsp process");

    auto processCloser = qScopeGuard([&process]() { process->close(); });

    TestVideoSink surface(false);
    QMediaPlayer player;

    QSignalSpy errorSpy(&player, &QMediaPlayer::errorOccurred);

    player.setVideoSink(&surface);
    // Ignore audio output to check timings accuratelly
    // player.setAudioOutput(&output);

    player.setSource(streamUrl);

    player.play();

    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PlayingState);

    const auto colors = { qRgb(0, 0, 0xFF), qRgb(0xFF, 0, 0), qRgb(0, 0xFE, 0) };
    const auto colorInterval = 5000;

    for (auto pos : { colorInterval / 2, colorInterval + 100 }) {
        qDebug() << "Waiting for position:" << pos;

        QTRY_COMPARE_GT(player.position(), pos);

        auto frame1 = surface.waitForFrame();
        QVERIFY(frame1.isValid());
        QCOMPARE(frame1.size(), QSize(213, 120));

        QCOMPARE_GT(frame1.startTime(), pos * 1000);

        auto frameTime = frame1.startTime();
        const auto coloIndex = frameTime / (colorInterval * 1000);
        QCOMPARE_LT(coloIndex, 2);

        const auto image1 = frame1.toImage();
        QVERIFY(!image1.isNull());
        QCOMPARE(findSimilarColorIndex(colors, image1.pixel(1, 1)), coloIndex);
        QCOMPARE(findSimilarColorIndex(colors, image1.pixel(100, 100)), coloIndex);

        auto frame2 = surface.waitForFrame();
        QVERIFY(frame2.isValid());
        QCOMPARE_GT(frame2.startTime(), frame1.startTime());
    }

    player.stop();

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(errorSpy.size(), 0);
}

void tst_QMediaPlayerBackend::play_waitsForLastFrameEnd_whenPlayingVideoWithLongFrames()
{
    CHECK_SELECTED_URL(m_oneRedFrameVideo);

    m_fixture->player.setSource(*m_oneRedFrameVideo);
    m_fixture->player.play();

    auto firstFrame = m_fixture->surface.waitForFrame();
    QVERIFY(firstFrame.isValid());

    QElapsedTimer timer;
    timer.start();

    auto endFrame = m_fixture->surface.waitForFrame();
    QVERIFY(!endFrame.isValid());

    const auto elapsed = timer.elapsed();

    // 1000 is expected
    QCOMPARE_GT(elapsed, 900);
    QCOMPARE_LT(elapsed, 1400);

    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(m_fixture->surface.m_totalFrames, 2);
}

void tst_QMediaPlayerBackend::stop_entersStoppedState_whenPlayerWasPaused()
{
    CHECK_SELECTED_URL(m_localWavFile);

    // Arrange
    m_fixture->player.setSource(*m_localWavFile);
    m_fixture->player.play();
    QTRY_VERIFY(m_fixture->player.position() > 100);
    m_fixture->player.pause();
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);
    m_fixture->clearSpies();

    // Act
    m_fixture->player.stop();

    // Assert
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QCOMPARE(m_fixture->playbackStateChanged, SignalList({ { QMediaPlayer::StoppedState } }));
    // it's allowed to emit statusChanged() signal async
    QTRY_COMPARE(m_fixture->mediaStatusChanged, SignalList({ { QMediaPlayer::LoadedMedia } }));
    QCOMPARE(m_fixture->bufferProgressChanged, SignalList({ { 0.f } }));

    QTRY_COMPARE(m_fixture->player.position(), qint64(0));
    QTRY_VERIFY(!m_fixture->positionChanged.empty());
    QCOMPARE(m_fixture->positionChanged.last()[0].value<qint64>(), qint64(0));
    QVERIFY(m_fixture->player.duration() > 0);
}

void tst_QMediaPlayerBackend::stop_setsPositionToZero_afterPlayingToEndOfMedia()
{
    // Arrange
    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);
    m_fixture->player.play();
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);

    // Act
    m_fixture->player.stop();

    // Assert
    QCOMPARE(m_fixture->player.position(), qint64(0));
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);

    m_fixture->player.play();
    QVERIFY(m_fixture->surface.waitForFrame().isValid());
}


void tst_QMediaPlayerBackend::playbackRate_returnsOne_byDefault()
{
    QCOMPARE_EQ(m_fixture->player.playbackRate(), static_cast<qreal>(1.0f));
}

void tst_QMediaPlayerBackend::setPlaybackRate_changesPlaybackRateAndEmitsSignal_data()
{
    QTest::addColumn<float>("initialPlaybackRate");
    QTest::addColumn<float>("targetPlaybackRate");
    QTest::addColumn<float>("expectedPlaybackRate");
    QTest::addColumn<bool>("signalExpected");

    QTest::addRow("Increase") << 1.0f << 2.0f << 2.0f << true;
    QTest::addRow("Decrease") << 1.0f << 0.5f << 0.5f << true;
    QTest::addRow("Keep") << 0.5f << 0.5f << 0.5f << false;
    QTest::addRow("DecreaseBelowZero") << 0.5f << -0.5f << 0.0f << true;
    QTest::addRow("KeepDecreasingBelowZero") << -0.5f << -0.6f << 0.0f << false;

}

void tst_QMediaPlayerBackend::setPlaybackRate_changesPlaybackRateAndEmitsSignal()
{
    QFETCH(const float, initialPlaybackRate);
    QFETCH(const float, targetPlaybackRate);
    QFETCH(const float, expectedPlaybackRate);
    QFETCH(const bool, signalExpected);

    // Arrange
    m_fixture->player.setPlaybackRate(initialPlaybackRate);
    m_fixture->clearSpies();

    // Act
    m_fixture->player.setPlaybackRate(targetPlaybackRate);

    // Assert
    if (signalExpected)
        QCOMPARE_EQ(m_fixture->playbackRateChanged, SignalList({ { expectedPlaybackRate } }));
    else
        QVERIFY(m_fixture->playbackRateChanged.empty());

    QCOMPARE_EQ(m_fixture->player.playbackRate(), expectedPlaybackRate);
}

void tst_QMediaPlayerBackend::setVolume_changesVolume_whenVolumeIsInRange()
{
    m_fixture->output.setVolume(0.0f);
    QCOMPARE_EQ(m_fixture->output.volume(), 0.0f);
    QCOMPARE(m_fixture->volumeChanged, SignalList({ { 0.0f } }));

    m_fixture->output.setVolume(0.5f);
    QCOMPARE_EQ(m_fixture->output.volume(), 0.5f);
    QCOMPARE(m_fixture->volumeChanged, SignalList({ { 0.0f }, { 0.5f } }));

    m_fixture->output.setVolume(1.0f);
    QCOMPARE_EQ(m_fixture->output.volume(), 1.0f);
    QCOMPARE(m_fixture->volumeChanged, SignalList({ { 0.0f }, { 0.5f }, { 1.0f } }));
}

void tst_QMediaPlayerBackend::setVolume_clampsToRange_whenVolumeIsOutsideRange()
{
    m_fixture->output.setVolume(-0.1f);
    QCOMPARE_EQ(m_fixture->output.volume(), 0.0f);
    QCOMPARE(m_fixture->volumeChanged, SignalList({ { 0.0f } }));

    m_fixture->output.setVolume(1.1f);
    QCOMPARE_EQ(m_fixture->output.volume(), 1.0f);
    QCOMPARE(m_fixture->volumeChanged, SignalList({ { 0.0f }, { 1.0f } }));
}

void tst_QMediaPlayerBackend::setVolume_doesNotChangeMutedState()
{
    m_fixture->output.setMuted(true);
    m_fixture->output.setVolume(0.5f);
    QVERIFY(m_fixture->output.isMuted());

    m_fixture->output.setMuted(false);
    m_fixture->output.setVolume(0.0f);
    QVERIFY(!m_fixture->output.isMuted());
}

void tst_QMediaPlayerBackend::setMuted_changesMutedState_whenMutedStateChanged()
{
    m_fixture->output.setMuted(true);
    QVERIFY(m_fixture->output.isMuted());
    QCOMPARE(m_fixture->mutedChanged, SignalList({ { true } }));

    // No new events emitted when muted state did not change
    m_fixture->output.setMuted(true);
    QCOMPARE(m_fixture->mutedChanged, SignalList({ { true } }));

    m_fixture->output.setMuted(false);
    QVERIFY(!m_fixture->output.isMuted());
    QCOMPARE(m_fixture->mutedChanged, SignalList({ { true }, { false } }));

    // No new events emitted when muted state did not change
    m_fixture->output.setMuted(false);
    QCOMPARE(m_fixture->mutedChanged, SignalList({ { true }, { false } }));
}

void tst_QMediaPlayerBackend::setMuted_doesNotChangeVolume()
{
    m_fixture->output.setVolume(0.5f);

    m_fixture->output.setMuted(true);
    QCOMPARE_EQ(m_fixture->output.volume(), 0.5f);

    m_fixture->output.setMuted(false);
    QCOMPARE_EQ(m_fixture->output.volume(), 0.5f);
}

void tst_QMediaPlayerBackend::processEOS()
{
    CHECK_SELECTED_URL(m_localWavFile);

    m_fixture->player.setSource(*m_localWavFile);

    m_fixture->player.play();
    m_fixture->player.setPosition(900);

    //wait up to 5 seconds for EOS
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);

    QVERIFY(m_fixture->mediaStatusChanged.size() > 0);
    QCOMPARE(m_fixture->mediaStatusChanged.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::EndOfMedia);
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(m_fixture->playbackStateChanged.size(), 2);
    QCOMPARE(m_fixture->playbackStateChanged.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);

    //at EOS the position stays at the end of file
    QCOMPARE(m_fixture->player.position(), m_fixture->player.duration());
    QTRY_VERIFY(m_fixture->positionChanged.size() > 0);
    QTRY_COMPARE(m_fixture->positionChanged.last()[0].value<qint64>(), m_fixture->player.duration());

    m_fixture->playbackStateChanged.clear();
    m_fixture->mediaStatusChanged.clear();
    m_fixture->positionChanged.clear();

    m_fixture->player.play();

    //position is reset to start
    QTRY_VERIFY(m_fixture->player.position() < 100);
    QTRY_VERIFY(m_fixture->positionChanged.size() > 0);
    QCOMPARE(m_fixture->positionChanged.first()[0].value<qint64>(), 0);

    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(m_fixture->playbackStateChanged.size(), 1);
    QCOMPARE(m_fixture->playbackStateChanged.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QVERIFY(m_fixture->mediaStatusChanged.size() > 0);
    QCOMPARE(m_fixture->mediaStatusChanged.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);

    m_fixture->positionChanged.clear();
    QTRY_VERIFY(m_fixture->player.position() > 100);
    QTRY_VERIFY(m_fixture->positionChanged.size() > 0 && m_fixture->positionChanged.last()[0].value<qint64>() > 100);
    m_fixture->player.setPosition(900);
    //wait up to 5 seconds for EOS
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QVERIFY(m_fixture->mediaStatusChanged.size() > 0);
    QCOMPARE(m_fixture->mediaStatusChanged.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::EndOfMedia);
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(m_fixture->playbackStateChanged.size(), 2);
    QCOMPARE(m_fixture->playbackStateChanged.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);

    QCOMPARE_GT(m_fixture->bufferProgressChanged.size(), 1);
    QCOMPARE(m_fixture->bufferProgressChanged.back().front(), 0.f);

    //position stays at the end of file
    QCOMPARE(m_fixture->player.position(), m_fixture->player.duration());
    QTRY_VERIFY(m_fixture->positionChanged.size() > 0);
    QTRY_COMPARE(m_fixture->positionChanged.last()[0].value<qint64>(), m_fixture->player.duration());

    //after setPosition EndOfMedia status should be reset to Loaded
    m_fixture->playbackStateChanged.clear();
    m_fixture->mediaStatusChanged.clear();
    m_fixture->player.setPosition(500);

    //this transition can be async, so allow backend to perform it
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);

    QCOMPARE(m_fixture->playbackStateChanged.size(), 0);
    QTRY_VERIFY(m_fixture->mediaStatusChanged.size() > 0 &&
        m_fixture->mediaStatusChanged.last()[0].value<QMediaPlayer::MediaStatus>() == QMediaPlayer::LoadedMedia);

    m_fixture->player.play();
    m_fixture->player.setPosition(900);
    //wait up to 5 seconds for EOS
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(m_fixture->player.position(), m_fixture->player.duration());

    m_fixture->playbackStateChanged.clear();
    m_fixture->mediaStatusChanged.clear();
    m_fixture->positionChanged.clear();

    // pause() should reset position to beginning and status to Buffered
    m_fixture->player.pause();

    QTRY_COMPARE(m_fixture->player.position(), 0);
    QTRY_VERIFY(m_fixture->positionChanged.size() > 0);
    QTRY_COMPARE(m_fixture->positionChanged.first()[0].value<qint64>(), 0);

    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::PausedState);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(m_fixture->playbackStateChanged.size(), 1);
    QCOMPARE(m_fixture->playbackStateChanged.last()[0].value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PausedState);
    QVERIFY(m_fixture->mediaStatusChanged.size() > 0);
    QCOMPARE(m_fixture->mediaStatusChanged.last()[0].value<QMediaPlayer::MediaStatus>(), QMediaPlayer::BufferedMedia);
}

// Helper class for tst_QMediaPlayerBackend::deleteLaterAtEOS()
class DeleteLaterAtEos : public QObject
{
    Q_OBJECT
public:
    DeleteLaterAtEos(QMediaPlayer* p) : player(p)
    {
    }

public slots:
    void play()
    {
        QVERIFY(connect(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
                        this,   SLOT(onMediaStatusChanged(QMediaPlayer::MediaStatus))));
        player->play();
    }

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status)
    {
        if (status == QMediaPlayer::EndOfMedia) {
            player->deleteLater();
            player = nullptr;
        }
    }

private:
    QMediaPlayer* player;
};

// Regression test for
// QTBUG-24927 - deleteLater() called to QMediaPlayer from its signal handler does not work as expected
void tst_QMediaPlayerBackend::deleteLaterAtEOS()
{
    CHECK_SELECTED_URL(m_localWavFile);

    QPointer<QMediaPlayer> player(new QMediaPlayer);
    QAudioOutput output;
    player->setAudioOutput(&output);
    player->setPosition(800); // don't wait as long for EOS
    DeleteLaterAtEos deleter(player);
    player->setSource(*m_localWavFile);

    // Create an event loop for verifying deleteLater behavior instead of using
    // QTRY_VERIFY or QTest::qWait. QTest::qWait makes extra effort to process
    // DeferredDelete events during the wait, which interferes with this test.
    QEventLoop loop;
    QTimer::singleShot(0, &deleter, SLOT(play()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    connect(player.data(), SIGNAL(destroyed()), &loop, SLOT(quit()));
    loop.exec();
    // Verify that the player was destroyed within the event loop.
    // This check will fail without the fix for QTBUG-24927.
    QVERIFY(player.isNull());
}

void tst_QMediaPlayerBackend::volumeAcrossFiles_data()
{
    QTest::addColumn<int>("volume");
    QTest::addColumn<bool>("muted");

    QTest::newRow("100 unmuted") << 100 << false;
    QTest::newRow("50 unmuted") << 50 << false;
    QTest::newRow("0 unmuted") << 0 << false;
    QTest::newRow("100 muted") << 100 << true;
    QTest::newRow("50 muted") << 50 << true;
    QTest::newRow("0 muted") << 0 << true;
}

void tst_QMediaPlayerBackend::volumeAcrossFiles()
{
    CHECK_SELECTED_URL(m_localWavFile);

    QFETCH(int, volume);
    QFETCH(bool, muted);
    float vol = volume/100.;

    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);

    //volume and muted should not be preserved between player instances
    QVERIFY(output.volume() > 0);
    QVERIFY(!output.isMuted());

    output.setVolume(vol);
    output.setMuted(muted);

    QTRY_COMPARE(output.volume(), vol);
    QTRY_COMPARE(output.isMuted(), muted);

    player.setSource(*m_localWavFile);
    QCOMPARE(output.volume(), vol);
    QCOMPARE(output.isMuted(), muted);

    player.pause();

    //to ensure the backend doesn't change volume/muted
    //async during file loading.

    QTRY_COMPARE(output.volume(), vol);
    QCOMPARE(output.isMuted(), muted);

    player.setSource(QUrl());
    QTRY_COMPARE(output.volume(), vol);
    QCOMPARE(output.isMuted(), muted);

    player.setSource(*m_localWavFile);
    player.pause();

    QTRY_COMPARE(output.volume(), vol);
    QCOMPARE(output.isMuted(), muted);
}

void tst_QMediaPlayerBackend::initialVolume()
{
    CHECK_SELECTED_URL(m_localWavFile);

    {
        QAudioOutput output;
        QMediaPlayer player;
        player.setAudioOutput(&output);
        output.setVolume(1);
        player.setSource(*m_localWavFile);
        QCOMPARE(output.volume(), 1);
        player.play();
        QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(output.volume(), 1);
    }

    {
        QAudioOutput output;
        QMediaPlayer player;
        player.setAudioOutput(&output);
        player.setSource(*m_localWavFile);
        QCOMPARE(output.volume(), 1);
        player.play();
        QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(output.volume(), 1);
    }
}

void tst_QMediaPlayerBackend::seekPauseSeek()
{
#ifdef Q_OS_ANDROID
    QSKIP("frame.toImage will return null image because of QTBUG-108446");
#endif
    CHECK_SELECTED_URL(m_localVideoFile);

    TestVideoSink surface(true);
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);

    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setVideoOutput(&surface);

    player.setSource(*m_localVideoFile);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(surface.m_frameList.isEmpty()); // frame must not appear until we call pause() or play()

    positionSpy.clear();
    qint64 position = 7000;
    player.setPosition(position);
    QTRY_VERIFY(!positionSpy.isEmpty());
    QTRY_COMPARE(player.position(), position);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTest::qWait(250); // wait a bit to ensure the frame is not rendered
    QVERIFY(surface.m_frameList
                    .isEmpty()); // still no frame, we must call pause() or play() to see a frame

    player.pause();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PausedState); // it might take some time for the operation to be completed
    QTRY_VERIFY(!surface.m_frameList.isEmpty()); // we must see a frame at position 7000 here

    // Make sure that the frame has a timestamp before testing - not all backends provides this
    if (!surface.m_frameList.back().isValid() || surface.m_frameList.back().startTime() < 0)
        QSKIP("No timestamp");

    {
        QVideoFrame frame = surface.m_frameList.back();
        const qint64 elapsed = (frame.startTime() / 1000) - position; // frame.startTime() is microsecond, position is milliseconds.
        QVERIFY2(qAbs(elapsed) < (qint64)500, QByteArray::number(elapsed).constData());
        QCOMPARE(frame.width(), 213);
        QCOMPARE(frame.height(), 120);

        // create QImage for QVideoFrame to verify RGB pixel colors
        QImage image = frame.toImage();
        QVERIFY(!image.isNull());
        QVERIFY(qRed(image.pixel(0, 0)) >= 230); // conversion from YUV => RGB, that's why it's not 255
        QVERIFY(qGreen(image.pixel(0, 0)) < 20);
        QVERIFY(qBlue(image.pixel(0, 0)) < 20);
    }

    surface.m_frameList.clear();
    positionSpy.clear();
    position = 12000;
    player.setPosition(position);
    QTRY_VERIFY(!positionSpy.isEmpty() && qAbs(player.position() - position) < (qint64)500);
    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QTRY_VERIFY(!surface.m_frameList.isEmpty());

    {
        QVideoFrame frame = surface.m_frameList.back();
        const qint64 elapsed = (frame.startTime() / 1000) - position;
        QVERIFY2(qAbs(elapsed) < (qint64)500, QByteArray::number(elapsed).constData());
        QCOMPARE(frame.width(), 213);
        QCOMPARE(frame.height(), 120);

        QImage image = frame.toImage();
        QVERIFY(!image.isNull());
        QVERIFY(qRed(image.pixel(0, 0)) < 20);
        QVERIFY(qGreen(image.pixel(0, 0)) >= 230);
        QVERIFY(qBlue(image.pixel(0, 0)) < 20);
    }
}

void tst_QMediaPlayerBackend::seekInStoppedState()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    QSignalSpy stateSpy(&player, SIGNAL(playbackStateChanged(QMediaPlayer::PlaybackState)));
    QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));

    player.setSource(*m_localVideoFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.position(), 0);
    QVERIFY(player.isSeekable());

    stateSpy.clear();
    positionSpy.clear();

    qint64 position = 5000;
    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(200));
    QTRY_VERIFY(positionSpy.size() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > position);

    QTest::qWait(100);
    // Check that it never played from the beginning
    QVERIFY(player.position() > position);
    for (int i = 0; i < positionSpy.size(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 200));

    // ------
    // Same tests but after play() --> stop()

    player.stop();
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_COMPARE(player.position(), 0);

    stateSpy.clear();
    positionSpy.clear();

    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(200));
    QTRY_VERIFY(positionSpy.size() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 0);

    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    positionSpy.clear();

    player.play();

    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QVERIFY(qAbs(player.position() - position) < qint64(200));

    QTest::qWait(500);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 200));
    for (int i = 0; i < positionSpy.size(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 200));

    // ------
    // Same tests but after reaching the end of the media

    player.setPosition(player.duration() - 100);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QVERIFY(qAbs(player.position() - player.duration()) < 10);

    stateSpy.clear();
    positionSpy.clear();

    player.setPosition(position);

    QTRY_VERIFY(qAbs(player.position() - position) < qint64(200));
    QTRY_VERIFY(positionSpy.size() > 0);
    QVERIFY(qAbs(positionSpy.last()[0].value<qint64>() - position) < qint64(200));

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    player.play();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::BufferedMedia);

    positionSpy.clear();
    QTRY_VERIFY(player.position() > (position - 200));

    QTest::qWait(500);
    // Check that it never played from the beginning
    QVERIFY(player.position() > (position - 200));
    for (int i = 0; i < positionSpy.size(); ++i)
        QVERIFY(positionSpy.at(i)[0].value<qint64>() > (position - 200));
}

void tst_QMediaPlayerBackend::subsequentPlayback()
{
    CHECK_SELECTED_URL(m_localCompressedSoundFile);

    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);
    player.setSource(*m_localCompressedSoundFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_VERIFY(player.isSeekable());
    player.setPosition(5000);
    player.play();

    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    // Could differ by up to 1 compressed frame length
    QVERIFY(qAbs(player.position() - player.duration()) < 100);
    QVERIFY(player.position() > 0);

    player.play();
    QTRY_COMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > 1000);
    player.pause();
    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    // make sure position does not "jump" closer to the end of the file
    QVERIFY(player.position() > 1000);
    // try to seek back to zero
    player.setPosition(0);
    QTRY_COMPARE(player.position(), qint64(0));
    player.play();
    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > 1000);
    player.pause();
    QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    QVERIFY(player.position() > 1000);
}

void tst_QMediaPlayerBackend::multipleMediaPlayback()
{
    CHECK_SELECTED_URL(m_localVideoFile);
    CHECK_SELECTED_URL(m_localVideoFile2);

    QAudioOutput output;
    TestVideoSink surface(false);
    QMediaPlayer player;

    player.setVideoOutput(&surface);
    player.setAudioOutput(&output);
    player.setSource(*m_localVideoFile);

    QCOMPARE(player.source(), *m_localVideoFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    player.setPosition(0);
    player.play();

    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > 0);
    QCOMPARE(player.source(), *m_localVideoFile);

    player.stop();

    player.setSource(*m_localVideoFile2);

    QCOMPARE(player.source(), *m_localVideoFile2);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_VERIFY(player.isSeekable());

    player.setPosition(0);
    player.play();

    QCOMPARE(player.error(), QMediaPlayer::NoError);
    QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(player.position() > 0);
    QCOMPARE(player.source(), *m_localVideoFile2);

    player.stop();

    QTRY_COMPARE(player.playbackState(), QMediaPlayer::StoppedState);
}

void tst_QMediaPlayerBackend::multiplePlaybackRateChangingStressTest()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("SKIP on macOS CI since multiple fake drawing on macOS CI platform causes UB. To be "
              "investigated.");
#endif

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    player.setSource(*m_localVideoFile3ColorsWithSound);

    player.play();

    surface.waitForFrame();

    QSignalSpy spy(&player, &QMediaPlayer::playbackStateChanged);

    constexpr qint64 expectedVideoDuration = 3000;
    constexpr int waitingInterval = 200;
    constexpr qint64 maxDuration = expectedVideoDuration + 2000;
    constexpr qint64 minDuration = expectedVideoDuration - 100;
    constexpr qint64 maxFrameDelay = 2000;

    surface.m_elapsedTimer.start();

    qint64 duration = 0;

    for (int i = 0; !spy.wait(waitingInterval); ++i) {
        duration += waitingInterval * player.playbackRate();

        player.setPlaybackRate(0.5 * (i % 4 + 1));

        QCOMPARE_LE(duration, maxDuration);

        QVERIFY2(surface.m_elapsedTimer.elapsed() < maxFrameDelay,
                 "If the delay is more than 2s, we consider the video playing is hanging.");

        /* Some debug code for windows. Use the code instead of the check above to debug the bug.
         * https://bugreports.qt.io/browse/QTBUG-105940.
         * TODO: fix hanging on windows and remove.
        if ( surface.m_elapsedTimer.elapsed() > maxFrameDelay ) {
            qDebug() << "pause/play";
            player.pause();
            player.play();
            surface.m_elapsedTimer.restart();
            spy.clear();
        }*/
    }

    duration += waitingInterval * player.playbackRate();

    QCOMPARE_GT(duration, minDuration);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).size(), 1);
    QCOMPARE(spy.at(0).at(0).value<QMediaPlayer::PlaybackState>(), QMediaPlayer::StoppedState);

    QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
}

void tst_QMediaPlayerBackend::multipleSeekStressTest()
{
#ifdef Q_OS_ANDROID
    QSKIP("frame.toImage will return null image because of QTBUG-108446");
#endif
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);

    player.setSource(*m_localVideoFile3ColorsWithSound);

    player.play();

    auto waitAndCheckFrame = [&](qint64 pos, QString checkInfo) {
        auto errorPrintingGuard = qScopeGuard([&]() {
            qDebug() << "Error:" << checkInfo;
            qDebug() << "Position:" << pos;
        });

        auto frame = surface.waitForFrame();
        QVERIFY(frame.isValid());

        const auto trackTime = pos * 1000;

        // in theory, previous frame might be received, in this case we wait for a new one that is
        // expected to be relevant
        if (frame.endTime() < trackTime || frame.startTime() > trackTime) {
            frame = surface.waitForFrame();
            QVERIFY(frame.isValid());
        }

        QCOMPARE_GE(frame.startTime(), trackTime - 200'000);
        QCOMPARE_LE(frame.endTime(), trackTime + 200'000);

        auto frameImage = frame.toImage();
        const auto actualColor = frameImage.pixel(1, 1);

        const auto actualColorIndex = findSimilarColorIndex(m_video3Colors, actualColor);

        const auto expectedColorIndex = pos / 1000;

        QCOMPARE(actualColorIndex, expectedColorIndex);

        errorPrintingGuard.dismiss();
    };

    auto seekAndCheck = [&](qint64 pos) {
        QSignalSpy positionSpy(&player, SIGNAL(positionChanged(qint64)));
        player.setPosition(pos);

        QTRY_VERIFY(positionSpy.size() >= 1);
        int setPosition = positionSpy.first().first().toInt();
        QCOMPARE_GT(setPosition, pos - 100);
        QCOMPARE_LT(setPosition, pos + 100);
    };

    constexpr qint64 posInterval = 10;

    {
        for (qint64 pos = posInterval; pos <= 2200; pos += posInterval)
            seekAndCheck(pos);

        waitAndCheckFrame(2200, "emulate fast moving of a seek slider forward");

        QCOMPARE_NE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    }

    {
        for (qint64 pos = 2100; pos >= 800; pos -= posInterval)
            seekAndCheck(pos);

        waitAndCheckFrame(800, "emulate fast moving of a seek slider backward");

        QCOMPARE_NE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(player.playbackState(), QMediaPlayer::PlayingState);
    }

    {
        player.pause();

        for (qint64 pos = 500; pos <= 1100; pos += posInterval)
            seekAndCheck(pos);

        waitAndCheckFrame(1100, "emulate fast moving of a seek slider forward on paused state");

        QCOMPARE_NE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
        QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
    }
}

void tst_QMediaPlayerBackend::setPlaybackRate_changesActualRateAndFramesRenderingTime_data()
{
    QTest::addColumn<bool>("withAudio");
    QTest::addColumn<int>("positionDeviationMs");

    QTest::newRow("Without audio") << false << 150;

    // set greater positionDeviationMs for case with audio due to possible synchronization.
    QTest::newRow("With audio") << true << 200;
}

void tst_QMediaPlayerBackend::setPlaybackRate_changesActualRateAndFramesRenderingTime()
{
    QFETCH(bool, withAudio);
    QFETCH(int, positionDeviationMs);

#ifdef Q_OS_ANDROID
    QSKIP("frame.toImage will return null image because of QTBUG-108446");
#endif
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("SKIP on macOS CI since multiple fake drawing on macOS CI platform causes UB. To be "
              "investigated: QTBUG-111744");
#endif
    m_fixture->player.setAudioOutput(
            withAudio ? &m_fixture->output
                      : nullptr); // TODO: mock audio output and check sound by frequency
    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);

    auto checkColorAndPosition = [&](qint64 expectedPosition, QString errorTag) {
        constexpr qint64 intervalTime = 1000;

        const int colorIndex = expectedPosition / intervalTime;
        const auto expectedColor = m_video3Colors[colorIndex];
        const auto actualPosition = m_fixture->player.position();

        auto frame = m_fixture->surface.videoFrame();
        auto image = frame.toImage();
        QVERIFY(!image.isNull());

        const auto actualColor = image.pixel(1, 1);

        auto errorPrintingGuard = qScopeGuard([&]() {
            qDebug() << "Error Tag:" << errorTag;
            qDebug() << "  Actual Color:" << QColor(actualColor)
                     << "  Expected Color:" << QColor(expectedColor);
            qDebug() << "  Most probable actual color index:"
                     << findSimilarColorIndex(m_video3Colors, actualColor)
                     << "Expected color index:" << colorIndex;
            qDebug() << "  Actual position:" << actualPosition;
            qDebug() << "  Frame start time:" << frame.startTime();
        });

        // TODO: investigate why frames sometimes are not delivered in time on windows
        constexpr qreal maxColorDifference = 0.18;
        QVERIFY(m_fixture->player.isPlaying());
        QCOMPARE_LE(colorDifference(actualColor, expectedColor), maxColorDifference);
        QCOMPARE_GT(actualPosition, expectedPosition - positionDeviationMs);
        QCOMPARE_LT(actualPosition, expectedPosition + positionDeviationMs);

        const auto framePosition = frame.startTime() / 1000;

        QCOMPARE_GT(framePosition, expectedPosition - positionDeviationMs);
        QCOMPARE_LT(framePosition, expectedPosition + positionDeviationMs);
        QCOMPARE_LT(qAbs(framePosition - actualPosition), positionDeviationMs);

        errorPrintingGuard.dismiss();
    };

    m_fixture->player.play();

    m_fixture->surface.waitForFrame();

    auto waitUntil = [&](qint64 targetPosition) {
        const auto position = m_fixture->player.position();

        const auto waitingIntervalMs =
                static_cast<int>((targetPosition - position) / m_fixture->player.playbackRate());

        if (targetPosition > position)
            QTest::qWait(waitingIntervalMs);

        qDebug() << "Test waiting:" << waitingIntervalMs << "ms, Position:" << position << "=>"
                 << m_fixture->player.position() << "Expected target position:" << targetPosition
                 << "playbackRate:" << m_fixture->player.playbackRate();
    };

    waitUntil(400);
    checkColorAndPosition(400, "Check default playback rate");

    m_fixture->player.setPlaybackRate(2.);

    waitUntil(1400);
    checkColorAndPosition(1400, "Check 2.0 playback rate");

    m_fixture->player.setPlaybackRate(0.5);

    waitUntil(1800);
    checkColorAndPosition(1800, "Check 0.5 playback rate");

    m_fixture->player.setPlaybackRate(0.321);

    m_fixture->player.stop();
}

void tst_QMediaPlayerBackend::surfaceTest()
{
    CHECK_SELECTED_URL(m_localVideoFile);
    // 25 fps video file

    QAudioOutput output;
    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setAudioOutput(&output);
    player.setVideoOutput(&surface);
    player.setSource(*m_localVideoFile);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
    QVERIFY2(surface.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface.m_totalFrames)));
}

void tst_QMediaPlayerBackend::metadata()
{
    CHECK_SELECTED_URL(m_localFileWithMetadata);

    m_fixture->player.setSource(*m_localFileWithMetadata);

    QTRY_VERIFY(m_fixture->metadataChanged.size() > 0);

    const QMediaMetaData metadata = m_fixture->player.metaData();
    QCOMPARE(metadata.value(QMediaMetaData::Title).toString(), QStringLiteral("Nokia Tune"));
    QCOMPARE(metadata.value(QMediaMetaData::ContributingArtist).toString(), QStringLiteral("TestArtist"));
    QCOMPARE(metadata.value(QMediaMetaData::AlbumTitle).toString(), QStringLiteral("TestAlbum"));
    QCOMPARE(metadata.value(QMediaMetaData::Duration), QVariant(7704));
    QVERIFY(!metadata.value(QMediaMetaData::ThumbnailImage).value<QImage>().isNull());
    m_fixture->clearSpies();

    m_fixture->player.setSource(QUrl());

    QCOMPARE(m_fixture->metadataChanged.size(), 1);
    QVERIFY(m_fixture->player.metaData().isEmpty());
}

void tst_QMediaPlayerBackend::metadata_returnsMetadataWithThumbnail_whenMediaHasThumbnail_data()
{
    QTest::addColumn<MaybeUrl>("mediaUrl");
    QTest::addColumn<bool>("hasThumbnail");
    QTest::addColumn<QSize>("expectedSize");
    QTest::addColumn<QColor>("expectedColor");

    QTest::addRow("jpeg thumbnail") << m_videoFileWithJpegThumbnail << true << QSize{ 20, 28 } << QColor(35, 177, 77);
    QTest::addRow("png thumbnail") << m_videoFileWithPngThumbnail << true << QSize{ 20, 28 } << QColor(35, 177, 77);
    QTest::addRow("no thumbnail") << m_localVideoFile3ColorsWithSound << false << QSize{ 0, 0 } << QColor(0, 0, 0);
}

void tst_QMediaPlayerBackend::metadata_returnsMetadataWithThumbnail_whenMediaHasThumbnail()
{
    // Arrange
    QFETCH(const MaybeUrl, mediaUrl);
    QFETCH(const bool, hasThumbnail);
    QFETCH(const QSize, expectedSize);
    QFETCH(const QColor, expectedColor);

    CHECK_SELECTED_URL(mediaUrl);

    m_fixture->player.setSource(*mediaUrl);
    QTRY_VERIFY(!m_fixture->metadataChanged.empty());

    // Act
    const QMediaMetaData metadata = m_fixture->player.metaData();
    const QImage thumbnail = metadata.value(QMediaMetaData::ThumbnailImage).value<QImage>();

    // Assert
    QCOMPARE_EQ(!thumbnail.isNull(), hasThumbnail);
    QCOMPARE_EQ(thumbnail.size(), expectedSize);

    if (hasThumbnail) {
        const QPoint center{ expectedSize.width() / 2, expectedSize.height() / 2 };
        const auto centerColor = thumbnail.pixelColor(center);

        constexpr int maxChannelDiff = 5;
        QCOMPARE_LT(std::abs(centerColor.red() - expectedColor.red()), maxChannelDiff);
        QCOMPARE_LT(std::abs(centerColor.green() - expectedColor.green()), maxChannelDiff);
        QCOMPARE_LT(std::abs(centerColor.blue() - expectedColor.blue()), maxChannelDiff);
    }
}

void tst_QMediaPlayerBackend::playerStateAtEOS()
{
    CHECK_SELECTED_URL(m_localWavFile);

    QAudioOutput output;
    QMediaPlayer player;
    player.setAudioOutput(&output);

    bool endOfMediaReceived = false;
    connect(&player, &QMediaPlayer::mediaStatusChanged,
            this, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
            endOfMediaReceived = true;
        }
    });

    player.setSource(*m_localWavFile);
    player.play();

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::EndOfMedia);
    QVERIFY(endOfMediaReceived);
}

void tst_QMediaPlayerBackend::playFromBuffer()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QFile file(m_localVideoFile->toLocalFile());
    if (!file.open(QIODevice::ReadOnly))
        QSKIP("Could not open file");
    player.setSourceDevice(&file, *m_localVideoFile);
    player.play();
    QTRY_VERIFY(player.position() >= 1000);
    QVERIFY2(surface.m_totalFrames >= 25, qPrintable(QString("Expected >= 25, got %1").arg(surface.m_totalFrames)));
}

void tst_QMediaPlayerBackend::audioVideoAvailable()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;
    QSignalSpy hasVideoSpy(&player, SIGNAL(hasVideoChanged(bool)));
    QSignalSpy hasAudioSpy(&player, SIGNAL(hasAudioChanged(bool)));
    player.setVideoOutput(&surface);
    player.setAudioOutput(&output);
    player.setSource(*m_localVideoFile);
    QTRY_VERIFY(player.hasVideo());
    QTRY_VERIFY(player.hasAudio());
    QCOMPARE(hasVideoSpy.size(), 1);
    QCOMPARE(hasAudioSpy.size(), 1);
    player.setSource(QUrl());
    QTRY_VERIFY(!player.hasVideo());
    QTRY_VERIFY(!player.hasAudio());
    QCOMPARE(hasVideoSpy.size(), 2);
    QCOMPARE(hasAudioSpy.size(), 2);
}

void tst_QMediaPlayerBackend::isSeekable()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QVERIFY(!player.isSeekable());
    player.setSource(*m_localVideoFile);
    QTRY_VERIFY(player.isSeekable());
}

void tst_QMediaPlayerBackend::positionAfterSeek()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    TestVideoSink surface(false);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QVERIFY(!player.isSeekable());
    player.setSource(*m_localVideoFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    player.pause();
    player.setPosition(500);
    QTRY_VERIFY(player.position() == 500);
    player.setPosition(700);
    QVERIFY(player.position() != 0);
    QTRY_VERIFY(player.position() == 700);
    player.play();
    QTRY_VERIFY(player.position() > 700);
    player.setPosition(200);
    QVERIFY(player.position() != 0);
    QTRY_VERIFY(player.position() < 700);
}

void tst_QMediaPlayerBackend::videoDimensions()
{
    CHECK_SELECTED_URL(m_videoDimensionTestFile);

    TestVideoSink surface(true);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QVERIFY(!player.isSeekable());
    player.setSource(*m_videoDimensionTestFile);
    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::LoadedMedia);
    player.pause();
    QTRY_COMPARE(surface.m_totalFrames, 1);
    QCOMPARE(surface.m_frameList.last().width(), 540);
    QCOMPARE(surface.videoSize().width(), 540);
    QCOMPARE(surface.m_frameList.last().height(), 320);
    QCOMPARE(surface.videoSize().height(), 320);
}

void tst_QMediaPlayerBackend::position()
{
    CHECK_SELECTED_URL(m_localVideoFile);

    TestVideoSink surface(true);
    QMediaPlayer player;
    player.setVideoOutput(&surface);
    QVERIFY(!player.isSeekable());
    player.setSource(*m_localVideoFile);
    QTRY_VERIFY(player.isSeekable());

    player.play();
    player.setPosition(1000);
    QVERIFY(player.position() > 950);
    QVERIFY(player.position() < 1050);
    QTRY_VERIFY(player.position() > 1050);

    player.pause();
    player.setPosition(500);
    QVERIFY(player.position() > 450);
    QVERIFY(player.position() < 550);
    QTest::qWait(200);
    QVERIFY(player.position() > 450);
    QVERIFY(player.position() < 550);
}

void tst_QMediaPlayerBackend::durationDetectionIssues_data()
{
    QTest::addColumn<QString>("mediaFile");
    QTest::addColumn<qint64>("expectedDuration");
    QTest::addColumn<int>("expectedVideoTrackCount");
    QTest::addColumn<qint64>("expectedVideoTrackDuration");
    QTest::addColumn<int>("expectedAudioTrackCount");
    QTest::addColumn<QVariant>("expectedAudioTrackDuration");

    // clang-format off

    QTest::newRow("stream-duration-in-metadata")
            << QString{ "qrc:/testdata/duration_issues.webm" }
            << 400ll        // Total media duration
            << 1            // Number of video tracks in file
            << 400ll        // Video stream duration
            << 0            // Number of audio tracks in file
            << QVariant{};  // Audio stream duration (unused)

    QTest::newRow("no-stream-duration-in-metadata")
            << QString{ "qrc:/testdata/nokia-tune.mkv" }
            << 7531ll       // Total media duration
            << 0            // Number of video tracks in file
            << 0ll          // Video stream duration (unused)
            << 1            // Number of audio tracks in file
            << QVariant{};  // Audio stream duration (not present on file)

    // clang-format on
}

void tst_QMediaPlayerBackend::durationDetectionIssues()
{
    QFETCH(QString, mediaFile);
    QFETCH(qint64, expectedDuration);
    QFETCH(int, expectedVideoTrackCount);
    QFETCH(qint64, expectedVideoTrackDuration);
    QFETCH(int, expectedAudioTrackCount);
    QFETCH(QVariant, expectedAudioTrackDuration);

    // ffmpeg detects stream an incorrect stream duration, so we take
    // the correct duration from the metadata

    TestVideoSink surface(false);
    QAudioOutput output;
    QMediaPlayer player;

    QSignalSpy durationSpy(&player, &QMediaPlayer::durationChanged);

    player.setVideoOutput(&surface);
    player.setAudioOutput(&output);
    player.setSource(mediaFile);

    QTRY_COMPARE_EQ(player.mediaStatus(), QMediaPlayer::LoadedMedia);

    // Duration event received
    QCOMPARE(durationSpy.size(), 1);
    QCOMPARE(durationSpy.front().front(), expectedDuration);

    // Duration property
    QCOMPARE(player.duration(), expectedDuration);
    QCOMPARE(player.metaData().value(QMediaMetaData::Duration), expectedDuration);

    // Track duration properties
    const auto videoTracks = player.videoTracks();
    QCOMPARE(videoTracks.size(), expectedVideoTrackCount);

    if (expectedVideoTrackCount != 0)
        QCOMPARE(videoTracks.front().value(QMediaMetaData::Duration), expectedVideoTrackDuration);

    const auto audioTracks = player.audioTracks();
    QCOMPARE(audioTracks.size(), expectedAudioTrackCount);

    if (expectedAudioTrackCount != 0)
        QCOMPARE(audioTracks.front().value(QMediaMetaData::Duration), expectedAudioTrackDuration);
}

struct LoopIteration {
    qint64 startPos;
    qint64 endPos;
    qint64 posCount;
};
// Creates a vector of LoopIterations, containing start- and end position
// and the number of position changes per video loop iteration.
static std::vector<LoopIteration> loopIterations(const QSignalSpy &positionSpy)
{
    std::vector<LoopIteration> result;
    // Loops through all positions emitted by QMediaPlayer::positionChanged
    for (auto &params : positionSpy) {
        const auto pos = params.front().value<qint64>();

        // Adds new LoopIteration struct to result if position is lower than previous position
        if (result.empty() || pos < result.back().endPos) {
            result.push_back(LoopIteration{pos, pos, 1});
        }
        // Updates end position of the current LoopIteration if position is higher than previous position
        else {
            result.back().posCount++;
            result.back().endPos = pos;
        }
    }
    return result;
}

void tst_QMediaPlayerBackend::finiteLoops()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("The test accidently gets crashed on macOS CI, not reproduced locally. To be "
              "investigated: QTBUG-111744");
#endif

    QCOMPARE(m_fixture->player.loops(), 1);
    m_fixture->player.setLoops(3);
    QCOMPARE(m_fixture->player.loops(), 3);

    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);
    m_fixture->player.setPlaybackRate(5);
    QCOMPARE(m_fixture->player.loops(), 3);

    m_fixture->player.play();
    m_fixture->surface.waitForFrame();

    // check pause doesn't affect looping
    {
        QTest::qWait(static_cast<int>(m_fixture->player.duration() * 3
                                      * 0.6 /*relative pos*/ / m_fixture->player.playbackRate()));
        m_fixture->player.pause();
        m_fixture->player.play();
    }

    QTRY_COMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);

    // Check for expected number of loop iterations and startPos, endPos and posCount per iteration
    std::vector<LoopIteration> iterations = loopIterations(m_fixture->positionChanged);
    QCOMPARE(iterations.size(), 3u);
    QCOMPARE_GT(iterations[0].startPos, 0);
    QCOMPARE(iterations[0].endPos, m_fixture->player.duration());
    QCOMPARE_GT(iterations[0].posCount, 10);
    QCOMPARE(iterations[1].startPos, 0);
    QCOMPARE(iterations[1].endPos, m_fixture->player.duration());
    QCOMPARE_GT(iterations[1].posCount, 10);
    QCOMPARE(iterations[2].startPos, 0);
    QCOMPARE(iterations[2].endPos, m_fixture->player.duration());
    QCOMPARE_GT(iterations[2].posCount, 10);

    QCOMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);

    // Check that loop counter is reset when playback is restarted.
    {
        m_fixture->positionChanged.clear();
        m_fixture->player.play();
        m_fixture->player.setPlaybackRate(10);
        m_fixture->surface.waitForFrame();

        QTRY_COMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
        QCOMPARE(loopIterations(m_fixture->positionChanged).size(), 3u);
        QCOMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);
    }
}

void tst_QMediaPlayerBackend::infiniteLoops()
{
    CHECK_SELECTED_URL(m_localVideoFile2);

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("The test accidently gets crashed on macOS CI, not reproduced locally. To be "
              "investigated: QTBUG-111744");
#endif

    m_fixture->player.setLoops(QMediaPlayer::Infinite);
    QCOMPARE(m_fixture->player.loops(), QMediaPlayer::Infinite);

    // select some small file
    m_fixture->player.setSource(*m_localVideoFile2);
    m_fixture->player.setPlaybackRate(20);

    m_fixture->player.play();
    m_fixture->surface.waitForFrame();

    for (int i = 0; i < 2; ++i) {
        m_fixture->positionChanged.clear();

        QTest::qWait(
                std::max(static_cast<int>(m_fixture->player.duration()
                                          / m_fixture->player.playbackRate() * 4),
                         300 /*ensure some minimum waiting time to reduce threading flakiness*/));
        QVERIFY(m_fixture->player.mediaStatus() == QMediaPlayer::BufferingMedia
                || m_fixture->player.mediaStatus() == QMediaPlayer::BufferedMedia);
        QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::PlayingState);

        const auto iterations = loopIterations(m_fixture->positionChanged);
        QVERIFY(!iterations.empty());
        QCOMPARE(iterations.front().endPos, m_fixture->player.duration());
    }

    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);

    m_fixture->player.stop(); // QMediaPlayer::stop stops whether or not looping is infinite
    QCOMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);

    QCOMPARE(m_fixture->mediaStatusChanged,
             SignalList({ { QMediaPlayer::LoadingMedia },
                          { QMediaPlayer::LoadedMedia },
                          { QMediaPlayer::BufferingMedia },
                          { QMediaPlayer::BufferedMedia },
                          { QMediaPlayer::LoadedMedia } }));
}

void tst_QMediaPlayerBackend::seekOnLoops()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("The test accidently gets crashed on macOS CI, not reproduced locally. To be "
              "investigated: QTBUG-111744");
#endif

    m_fixture->player.setLoops(3);
    m_fixture->player.setPlaybackRate(2);

    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);

    m_fixture->player.play();
    m_fixture->surface.waitForFrame();

    // seek in the 1st loop
    m_fixture->player.setPosition(m_fixture->player.duration() * 4 / 5);

    // wait for the 2nd loop and seek
    m_fixture->surface.waitForFrame();
    QTRY_VERIFY(m_fixture->player.position() < m_fixture->player.duration() / 2);
    m_fixture->player.setPosition(m_fixture->player.duration() * 8 / 9);

    // wait for the 3rd loop and seek
    m_fixture->surface.waitForFrame();
    QTRY_VERIFY(m_fixture->player.position() < m_fixture->player.duration() / 2);
    m_fixture->player.setPosition(m_fixture->player.duration() * 4 / 5);

    QTRY_COMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);

    auto iterations = loopIterations(m_fixture->positionChanged);

    QCOMPARE(iterations.size(), 3u);
    QCOMPARE_GT(iterations[0].startPos, 0);
    QCOMPARE(iterations[0].endPos, m_fixture->player.duration());
    QCOMPARE_GT(iterations[0].posCount, 2);
    QCOMPARE(iterations[1].startPos, 0);
    QCOMPARE(iterations[1].endPos, m_fixture->player.duration());
    QCOMPARE_GT(iterations[1].posCount, 2);
    QCOMPARE(iterations[2].startPos, 0);
    QCOMPARE(iterations[2].endPos, m_fixture->player.duration());
    QCOMPARE_GT(iterations[2].posCount, 2);

    QCOMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);
}

void tst_QMediaPlayerBackend::changeLoopsOnTheFly()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("The test accidently gets crashed on macOS CI, not reproduced locally. To be "
              "investigated: QTBUG-111744");
#endif

    m_fixture->player.setLoops(4);
    m_fixture->player.setPlaybackRate(5);

    m_fixture->player.setSource(*m_localVideoFile3ColorsWithSound);

    m_fixture->player.play();
    m_fixture->surface.waitForFrame();

    m_fixture->player.setPosition(m_fixture->player.duration() * 4 / 5);

    // wait for the 2nd loop
    m_fixture->surface.waitForFrame();
    QTRY_VERIFY(m_fixture->player.position() < m_fixture->player.duration() / 2);
    m_fixture->player.setPosition(m_fixture->player.duration() * 8 / 9);

    m_fixture->player.setLoops(1);

    QTRY_COMPARE(m_fixture->player.playbackState(), QMediaPlayer::StoppedState);
    QCOMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::EndOfMedia);

    auto iterations = loopIterations(m_fixture->positionChanged);
    QCOMPARE(iterations.size(), 2u);

    QCOMPARE(iterations[1].startPos, 0);
    QCOMPARE(iterations[1].endPos, m_fixture->player.duration());
    QCOMPARE_GT(iterations[1].posCount, 2);
}

void tst_QMediaPlayerBackend::changeVideoOutputNoFramesLost()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

    QVideoSink sinks[4];
    std::atomic_int framesCount[4] = {
        0,
    };
    for (int i = 0; i < 4; ++i)
        setVideoSinkAsyncFramesCounter(sinks[i], framesCount[i]);

    QMediaPlayer player;

    player.setPlaybackRate(10);

    player.setVideoOutput(&sinks[0]);
    player.setSource(*m_localVideoFile3ColorsWithSound);
    player.play();
    QTRY_VERIFY(!player.isPlaying());

    player.setPlaybackRate(4);
    player.setVideoOutput(&sinks[1]);
    player.play();

    QTRY_VERIFY(framesCount[1] >= framesCount[0] / 4);
    player.setVideoOutput(&sinks[2]);
    const int savedFrameNumber1 = framesCount[1];

    QTRY_VERIFY(framesCount[2] >= (framesCount[0] - savedFrameNumber1) / 2);
    player.setVideoOutput(&sinks[3]);
    const int savedFrameNumber2 = framesCount[2];

    QTRY_VERIFY(!player.isPlaying());

    // check if no frames sent to old sinks
    QCOMPARE(framesCount[1], savedFrameNumber1);
    QCOMPARE(framesCount[2], savedFrameNumber2);

    // no frames lost
    QCOMPARE(framesCount[1] + framesCount[2] + framesCount[3], framesCount[0]);
}

void tst_QMediaPlayerBackend::cleanSinkAndNoMoreFramesAfterStop()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

    QVideoSink sink;
    std::atomic_int framesCount = 0;
    setVideoSinkAsyncFramesCounter(sink, framesCount);
    QMediaPlayer player;

    player.setPlaybackRate(10);
    player.setVideoOutput(&sink);

    player.setSource(*m_localVideoFile3ColorsWithSound);

    // Run a few time to have more chances to detect race conditions
    for (int i = 0; i < 8; ++i) {
        player.play();
        QTRY_VERIFY(framesCount > 0);

        player.stop();

        QVERIFY(!sink.videoFrame().isValid());

        QCOMPARE_NE(framesCount, 0);
        framesCount = 0;

        QTest::qWait(30);

        // check if nothing changed after short waiting
        QCOMPARE(framesCount, 0);
    }
}

void tst_QMediaPlayerBackend::lazyLoadVideo()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(QUrl("qrc:/LazyLoad.qml"));
    QScopedPointer<QObject> root(component.create());
    QQuickItem *rootItem = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(rootItem);

    QQuickView view;
    rootItem->setParentItem(view.contentItem());
    view.resize(600, 800);
    view.show();

    QQuickLoader *loader = qobject_cast<QQuickLoader *>(rootItem->findChild<QQuickItem *>("loader"));
    QVERIFY(loader);
    QCOMPARE(QQmlProperty::read(loader, "active").toBool(), false);
    loader->setProperty("active", true);
    QCOMPARE(QQmlProperty::read(loader, "active").toBool(), true);

    QQuickItem *videoPlayer = qobject_cast<QQuickItem *>(loader->findChild<QQuickItem *>("videoPlayer"));
    QVERIFY(videoPlayer);

    QTRY_COMPARE_EQ(QQmlProperty::read(videoPlayer, "playbackState").value<QMediaPlayer::PlaybackState>(), QMediaPlayer::PlayingState);
    QCOMPARE(QQmlProperty::read(videoPlayer, "error").value<QMediaPlayer::Error>(), QMediaPlayer::NoError);

    QVideoSink *videoSink = QQmlProperty::read(videoPlayer, "videoSink").value<QVideoSink *>();
    QVERIFY(videoSink);

    QSignalSpy spy(videoSink, &QVideoSink::videoFrameChanged);
    QVERIFY(spy.wait());

    QVideoFrame frame = spy.at(0).at(0).value<QVideoFrame>();
    QVERIFY(frame.isValid());
}

void tst_QMediaPlayerBackend::videoSinkSignals()
{
    // TODO: come up with custom frames source,
    // create the test target tst_QVideoSinkBackend,
    // and move the test there

    CHECK_SELECTED_URL(m_localVideoFile2);

    QVideoSink sink;
    QMediaPlayer player;
    player.setVideoSink(&sink);

    std::atomic<int> videoFrameCounter = 0;
    std::atomic<int> videoSizeCounter = 0;

    player.setSource(*m_localVideoFile2);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::MediaStatus::LoadedMedia);

    sink.platformVideoSink()->setNativeSize({}); // reset size to be able to check the size update

    connect(&sink, &QVideoSink::videoFrameChanged, this, [&](const QVideoFrame &frame) {
        QCOMPARE(sink.videoFrame(), frame);
        QCOMPARE(sink.videoSize(), frame.size());
        ++videoFrameCounter;
    }, Qt::DirectConnection);

    connect(&sink, &QVideoSink::videoSizeChanged, this, [&]() {
        QCOMPARE(sink.videoSize(), sink.videoFrame().size());
        if (sink.videoSize().isValid()) // filter end frame
            ++videoSizeCounter;
    }, Qt::DirectConnection);

    player.play();

    QTRY_COMPARE_GE(videoFrameCounter, 2);
    QCOMPARE(videoSizeCounter, 1);
}

void tst_QMediaPlayerBackend::nonAsciiFileName()
{
    CHECK_SELECTED_URL(m_localWavFile);

    auto temporaryFile =
            copyResourceToTemporaryFile(":/testdata/test.wav", "äöüØøÆ中文.XXXXXX.wav");
    QVERIFY(temporaryFile);

    m_fixture->player.setSource(temporaryFile->fileName());
    m_fixture->player.play();

    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::BufferedMedia);

    QCOMPARE(m_fixture->errorOccurred.size(), 0);
}

void tst_QMediaPlayerBackend::setMedia_setsVideoSinkSize_beforePlaying()
{
    CHECK_SELECTED_URL(m_localVideoFile3ColorsWithSound);

    QVideoSink sink1;
    QVideoSink sink2;
    QMediaPlayer player;

    QSignalSpy spy1(&sink1, &QVideoSink::videoSizeChanged);
    QSignalSpy spy2(&sink2, &QVideoSink::videoSizeChanged);

    player.setVideoOutput(&sink1);
    QCOMPARE(sink1.videoSize(), QSize());

    player.setSource(*m_localVideoFile3ColorsWithSound);

    QTRY_COMPARE(player.mediaStatus(), QMediaPlayer::MediaStatus::LoadedMedia);

    QCOMPARE(sink1.videoSize(), QSize(684, 384));

    player.setVideoOutput(&sink2);
    QCOMPARE(sink2.videoSize(), QSize(684, 384));

    QCOMPARE(spy1.size(), 1);
    QCOMPARE(spy2.size(), 1);
}

std::unique_ptr<QProcess> tst_QMediaPlayerBackend::createRtspStreamProcess(QString fileName,
                                                                           QString outputUrl)
{
    Q_ASSERT(!m_vlcCommand.isEmpty());

    auto process = std::make_unique<QProcess>();
#if defined(Q_OS_WINDOWS)
    fileName.replace('/', '\\');
#endif

    // clang-format off
    QStringList vlcParams =
    {
        "-vvv", fileName,
        "--sout", QLatin1String("#rtp{sdp=%1}").arg(outputUrl),
        "--intf", "dummy"
    };
    // clang-format on

    process->start(m_vlcCommand, vlcParams);
    if (!process->waitForStarted())
        return nullptr;

    // rtsp stream might be with started some delay after the vlc process starts.
    // Ideally, we should wait for open connections, it requires some extra work + QNetwork dependency.
    QTest::qWait(500);

    return process;
}

void tst_QMediaPlayerBackend::play_playsRotatedVideoOutput_whenVideoFileHasOrientationMetadata_data()
{
    QTest::addColumn<MaybeUrl>("fileURL");
    QTest::addColumn<QRgb>("expectedColor");
    QTest::addColumn<QtVideo::Rotation>("expectedRotationAngle");
    QTest::addRow("without rotation") << m_colorMatrixVideo << QRgb(0xff0000) << QtVideo::Rotation::None;
    QTest::addRow("90 deg clockwise") << m_colorMatrix90degClockwiseVideo << QRgb(0x0000FF) << QtVideo::Rotation::Clockwise90;
    QTest::addRow("180 deg clockwise") << m_colorMatrix180degClockwiseVideo << QRgb(0xFFFF00) << QtVideo::Rotation::Clockwise180;
    QTest::addRow("270 deg clockwise") << m_colorMatrix270degClockwiseVideo << QRgb(0x00FF00) << QtVideo::Rotation::Clockwise270;
}

void tst_QMediaPlayerBackend::play_playsRotatedVideoOutput_whenVideoFileHasOrientationMetadata()
{
    // This test uses 4 video files with a 2x2 color matrix consisting of
    // red (upper left), blue (lower left), yellow (lower right) and green (upper right).
    // The files are identical, except that three of them contain
    // orientation (rotation) metadata specifying that they should be
    // viewed with a 90, 180 and 270 degree clockwise rotation respectively.

    // Fetch path and expected color of upper left area of each file
    QFETCH(MaybeUrl, fileURL);
    QFETCH(QRgb, expectedColor);
    QFETCH(QtVideo::Rotation, expectedRotationAngle);

    CHECK_SELECTED_URL(fileURL);

    // Load video file
    m_fixture->player.setSource(*fileURL);
    QTRY_COMPARE(m_fixture->player.mediaStatus(), QMediaPlayer::LoadedMedia);

    // Compare orientation metadata of QMediaPlayer with expected value
    const auto metaData = m_fixture->player.metaData();
    const auto playerOrientation = metaData.value(QMediaMetaData::Orientation).value<QtVideo::Rotation>();
    QCOMPARE(playerOrientation, expectedRotationAngle);

    // Compare orientation metadata of active video stream with expected value
    const int activeVideoTrack = m_fixture->player.activeVideoTrack();
    const auto videoTrackMetaData = m_fixture->player.videoTracks().at(activeVideoTrack);
    const auto videoTrackOrientation = videoTrackMetaData.value(QMediaMetaData::Orientation).value<QtVideo::Rotation>();
    QCOMPARE(videoTrackOrientation, expectedRotationAngle);

    // Play video file, sample upper left area, compare with expected color
    m_fixture->player.play();
    QTRY_COMPARE(m_fixture->player.playbackState(), QMediaPlayer::PlayingState);
    QVideoFrame videoFrame = m_fixture->surface.waitForFrame();
    QVERIFY(videoFrame.isValid());
    QCOMPARE(QtVideo::Rotation(videoFrame.rotationAngle()), expectedRotationAngle);
    QImage image = videoFrame.toImage();
    QVERIFY(!image.isNull());
    QRgb upperLeftColor = image.pixel(5, 5);
    QCOMPARE_LT(colorDifference(upperLeftColor, expectedColor), 0.004);
}

QTEST_MAIN(tst_QMediaPlayerBackend)
#include "tst_qmediaplayerbackend.moc"

