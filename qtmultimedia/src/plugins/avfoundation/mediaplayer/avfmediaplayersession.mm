/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfmediaplayersession.h"
#include "avfmediaplayerservice.h"
#include "avfvideooutput.h"

#include <qpointer.h>

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

//AVAsset Keys
static NSString* const AVF_TRACKS_KEY       = @"tracks";
static NSString* const AVF_PLAYABLE_KEY     = @"playable";

//AVPlayerItem keys
static NSString* const AVF_STATUS_KEY                   = @"status";
static NSString* const AVF_BUFFER_LIKELY_KEEP_UP_KEY    = @"playbackLikelyToKeepUp";

//AVPlayer keys
static NSString* const AVF_RATE_KEY                     = @"rate";
static NSString* const AVF_CURRENT_ITEM_KEY             = @"currentItem";
static NSString* const AVF_CURRENT_ITEM_DURATION_KEY    = @"currentItem.duration";

static void *AVFMediaPlayerSessionObserverRateObservationContext = &AVFMediaPlayerSessionObserverRateObservationContext;
static void *AVFMediaPlayerSessionObserverStatusObservationContext = &AVFMediaPlayerSessionObserverStatusObservationContext;
static void *AVFMediaPlayerSessionObserverBufferLikelyToKeepUpContext = &AVFMediaPlayerSessionObserverBufferLikelyToKeepUpContext;
static void *AVFMediaPlayerSessionObserverCurrentItemObservationContext = &AVFMediaPlayerSessionObserverCurrentItemObservationContext;
static void *AVFMediaPlayerSessionObserverCurrentItemDurationObservationContext = &AVFMediaPlayerSessionObserverCurrentItemDurationObservationContext;

@interface AVFMediaPlayerSessionObserver : NSObject
{
@private
    AVFMediaPlayerSession *m_session;
    AVPlayer *m_player;
    AVPlayerItem *m_playerItem;
    AVPlayerLayer *m_playerLayer;
    NSURL *m_URL;
    BOOL m_bufferIsLikelyToKeepUp;
}

@property (readonly, getter=player) AVPlayer* m_player;
@property (readonly, getter=playerItem) AVPlayerItem* m_playerItem;
@property (readonly, getter=playerLayer) AVPlayerLayer* m_playerLayer;
@property (readonly, getter=session) AVFMediaPlayerSession* m_session;

- (AVFMediaPlayerSessionObserver *) initWithMediaPlayerSession:(AVFMediaPlayerSession *)session;
- (void) setURL:(NSURL *)url;
- (void) unloadMedia;
- (void) prepareToPlayAsset:(AVURLAsset *)asset withKeys:(NSArray *)requestedKeys;
- (void) assetFailedToPrepareForPlayback:(NSError *)error;
- (void) playerItemDidReachEnd:(NSNotification *)notification;
- (void) playerItemTimeJumped:(NSNotification *)notification;
- (void) observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object
                         change:(NSDictionary *)change context:(void *)context;
- (void) detatchSession;
- (void) dealloc;
@end

@implementation AVFMediaPlayerSessionObserver

@synthesize m_player, m_playerItem, m_playerLayer, m_session;

- (AVFMediaPlayerSessionObserver *) initWithMediaPlayerSession:(AVFMediaPlayerSession *)session
{
    if (!(self = [super init]))
        return nil;

    self->m_session = session;
    self->m_bufferIsLikelyToKeepUp = FALSE;
    return self;
}

- (void) setURL:(NSURL *)url
{
    if (m_URL != url)
    {
        [m_URL release];
        m_URL = [url copy];

        //Create an asset for inspection of a resource referenced by a given URL.
        //Load the values for the asset keys "tracks", "playable".

        // use __block to avoid maintaining strong references on variables captured by the
        // following block callback
        __block AVURLAsset *asset = [[AVURLAsset URLAssetWithURL:m_URL options:nil] retain];
        __block NSArray *requestedKeys = [[NSArray arrayWithObjects:AVF_TRACKS_KEY, AVF_PLAYABLE_KEY, nil] retain];

        __block AVFMediaPlayerSessionObserver *blockSelf = self;
        QPointer<AVFMediaPlayerSession> session(m_session);

        // Tells the asset to load the values of any of the specified keys that are not already loaded.
        [asset loadValuesAsynchronouslyForKeys:requestedKeys completionHandler:
         ^{
             dispatch_async( dispatch_get_main_queue(),
                           ^{
                                if (session)
                                    [blockSelf prepareToPlayAsset:asset withKeys:requestedKeys];
                                 [asset release];
                                 [requestedKeys release];
                            });
         }];
    }
}

- (void) unloadMedia
{
    if (m_playerItem) {
        [m_playerItem removeObserver:self forKeyPath:AVF_STATUS_KEY];
        [m_playerItem removeObserver:self forKeyPath:AVF_BUFFER_LIKELY_KEEP_UP_KEY];

        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVPlayerItemDidPlayToEndTimeNotification
                                                    object:m_playerItem];
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVPlayerItemTimeJumpedNotification
                                                    object:m_playerItem];
        m_playerItem = 0;
    }
    if (m_player) {
        [m_player setRate:0.0];
        [m_player removeObserver:self forKeyPath:AVF_CURRENT_ITEM_DURATION_KEY];
        [m_player removeObserver:self forKeyPath:AVF_CURRENT_ITEM_KEY];
        [m_player removeObserver:self forKeyPath:AVF_RATE_KEY];
        [m_player release];
        m_player = 0;
    }
    if (m_playerLayer) {
        [m_playerLayer release];
        m_playerLayer = 0;
    }
}

- (void) prepareToPlayAsset:(AVURLAsset *)asset
                   withKeys:(NSArray *)requestedKeys
{
    //Make sure that the value of each key has loaded successfully.
    for (NSString *thisKey in requestedKeys)
    {
        NSError *error = nil;
        AVKeyValueStatus keyStatus = [asset statusOfValueForKey:thisKey error:&error];
#ifdef QT_DEBUG_AVF
        qDebug() << Q_FUNC_INFO << [thisKey UTF8String] << " status: " << keyStatus;
#endif
        if (keyStatus == AVKeyValueStatusFailed)
        {
            [self assetFailedToPrepareForPlayback:error];
            return;
        }
    }

    //Use the AVAsset playable property to detect whether the asset can be played.
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "isPlayable: " << [asset isPlayable];
#endif
    if (!asset.playable)
    {
        //Generate an error describing the failure.
        NSString *localizedDescription = NSLocalizedString(@"Item cannot be played", @"Item cannot be played description");
        NSString *localizedFailureReason = NSLocalizedString(@"The assets tracks were loaded, but could not be made playable.", @"Item cannot be played failure reason");
        NSDictionary *errorDict = [NSDictionary dictionaryWithObjectsAndKeys:
                localizedDescription, NSLocalizedDescriptionKey,
                localizedFailureReason, NSLocalizedFailureReasonErrorKey,
                nil];
        NSError *assetCannotBePlayedError = [NSError errorWithDomain:@"StitchedStreamPlayer" code:0 userInfo:errorDict];

        [self assetFailedToPrepareForPlayback:assetCannotBePlayedError];

        return;
    }

    //At this point we're ready to set up for playback of the asset.
    //Stop observing our prior AVPlayerItem, if we have one.
    if (m_playerItem)
    {
        //Remove existing player item key value observers and notifications.
        [self unloadMedia];
    }

    //Create a new instance of AVPlayerItem from the now successfully loaded AVAsset.
    m_playerItem = [AVPlayerItem playerItemWithAsset:asset];

    //Observe the player item "status" key to determine when it is ready to play.
    [m_playerItem addObserver:self
                   forKeyPath:AVF_STATUS_KEY
                      options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                      context:AVFMediaPlayerSessionObserverStatusObservationContext];

    [m_playerItem addObserver:self
                   forKeyPath:AVF_BUFFER_LIKELY_KEEP_UP_KEY
                      options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                      context:AVFMediaPlayerSessionObserverBufferLikelyToKeepUpContext];

    //When the player item has played to its end time we'll toggle
    //the movie controller Pause button to be the Play button
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(playerItemDidReachEnd:)
                                                 name:AVPlayerItemDidPlayToEndTimeNotification
                                               object:m_playerItem];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(playerItemTimeJumped:)
                                                 name:AVPlayerItemTimeJumpedNotification
                                               object:m_playerItem];

    //Get a new AVPlayer initialized to play the specified player item.
    m_player = [AVPlayer playerWithPlayerItem:m_playerItem];
    [m_player retain];

    //Set the initial volume on new player object
    if (self.session) {
        [m_player setVolume:m_session->volume() / 100.0f];
        [m_player setMuted:m_session->isMuted()];
    }

    //Create a new player layer if we don't have one already
    if (!m_playerLayer)
    {
        m_playerLayer = [AVPlayerLayer playerLayerWithPlayer:m_player];
        [m_playerLayer retain];
        m_playerLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
        m_playerLayer.anchorPoint = CGPointMake(0.0f, 0.0f);
    }

    //Observe the AVPlayer "currentItem" property to find out when any
    //AVPlayer replaceCurrentItemWithPlayerItem: replacement will/did
    //occur.
    [m_player addObserver:self
                          forKeyPath:AVF_CURRENT_ITEM_KEY
                          options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                          context:AVFMediaPlayerSessionObserverCurrentItemObservationContext];

    //Observe the AVPlayer "rate" property to update the scrubber control.
    [m_player addObserver:self
                          forKeyPath:AVF_RATE_KEY
                          options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                          context:AVFMediaPlayerSessionObserverRateObservationContext];

    //Observe the duration for getting the buffer state
    [m_player addObserver:self
                          forKeyPath:AVF_CURRENT_ITEM_DURATION_KEY
                          options:0
                          context:AVFMediaPlayerSessionObserverCurrentItemDurationObservationContext];

}

-(void) assetFailedToPrepareForPlayback:(NSError *)error
{
    Q_UNUSED(error)
    QMetaObject::invokeMethod(m_session, "processMediaLoadError", Qt::AutoConnection);
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
    qDebug() << [[error localizedDescription] UTF8String];
    qDebug() << [[error localizedFailureReason] UTF8String];
    qDebug() << [[error localizedRecoverySuggestion] UTF8String];
#endif
}

- (void) playerItemDidReachEnd:(NSNotification *)notification
{
    Q_UNUSED(notification)
    if (self.session)
        QMetaObject::invokeMethod(m_session, "processEOS", Qt::AutoConnection);
}

- (void) playerItemTimeJumped:(NSNotification *)notification
{
    Q_UNUSED(notification)
    if (self.session)
        QMetaObject::invokeMethod(m_session, "processPositionChange", Qt::AutoConnection);
}

- (void) observeValueForKeyPath:(NSString*) path
                       ofObject:(id)object
                         change:(NSDictionary*)change
                        context:(void*)context
{
    //AVPlayerItem "status" property value observer.
    if (context == AVFMediaPlayerSessionObserverStatusObservationContext)
    {
        AVPlayerStatus status = (AVPlayerStatus)[[change objectForKey:NSKeyValueChangeNewKey] integerValue];
        switch (status)
        {
            //Indicates that the status of the player is not yet known because
            //it has not tried to load new media resources for playback
            case AVPlayerStatusUnknown:
            {
                //QMetaObject::invokeMethod(m_session, "processLoadStateChange", Qt::AutoConnection);
            }
            break;

            case AVPlayerStatusReadyToPlay:
            {
                //Once the AVPlayerItem becomes ready to play, i.e.
                //[playerItem status] == AVPlayerItemStatusReadyToPlay,
                //its duration can be fetched from the item.
                if (self.session)
                    QMetaObject::invokeMethod(m_session, "processLoadStateChange", Qt::AutoConnection);
            }
            break;

            case AVPlayerStatusFailed:
            {
                AVPlayerItem *playerItem = (AVPlayerItem *)object;
                [self assetFailedToPrepareForPlayback:playerItem.error];

                if (self.session)
                    QMetaObject::invokeMethod(m_session, "processLoadStateFailure", Qt::AutoConnection);
            }
            break;
        }
    }
    else if (context == AVFMediaPlayerSessionObserverBufferLikelyToKeepUpContext)
    {
        const bool isPlaybackLikelyToKeepUp = [m_playerItem isPlaybackLikelyToKeepUp];
        if (isPlaybackLikelyToKeepUp != m_bufferIsLikelyToKeepUp) {
            m_bufferIsLikelyToKeepUp = isPlaybackLikelyToKeepUp;
            QMetaObject::invokeMethod(m_session, "processBufferStateChange", Qt::AutoConnection,
                                      Q_ARG(int, isPlaybackLikelyToKeepUp ? 100 : 0));
        }
    }
    //AVPlayer "rate" property value observer.
    else if (context == AVFMediaPlayerSessionObserverRateObservationContext)
    {
        //QMetaObject::invokeMethod(m_session, "setPlaybackRate",  Qt::AutoConnection, Q_ARG(qreal, [m_player rate]));
    }
    //AVPlayer "currentItem" property observer.
    //Called when the AVPlayer replaceCurrentItemWithPlayerItem:
    //replacement will/did occur.
    else if (context == AVFMediaPlayerSessionObserverCurrentItemObservationContext)
    {
        AVPlayerItem *newPlayerItem = [change objectForKey:NSKeyValueChangeNewKey];
        if (m_playerItem != newPlayerItem)
            m_playerItem = newPlayerItem;
    }
    else if (context == AVFMediaPlayerSessionObserverCurrentItemDurationObservationContext)
    {
        const CMTime time = [m_playerItem duration];
        const qint64 duration =  static_cast<qint64>(float(time.value) / float(time.timescale) * 1000.0f);
        if (self.session)
            QMetaObject::invokeMethod(m_session, "processDurationChange",  Qt::AutoConnection, Q_ARG(qint64, duration));
    }
    else
    {
        [super observeValueForKeyPath:path ofObject:object change:change context:context];
    }
}

- (void) detatchSession
{
#ifdef QT_DEBUG_AVF
        qDebug() << Q_FUNC_INFO;
#endif
        m_session = 0;
}

- (void) dealloc
{
#ifdef QT_DEBUG_AVF
        qDebug() << Q_FUNC_INFO;
#endif
    [self unloadMedia];

    if (m_URL) {
        [m_URL release];
    }

    [super dealloc];
}

@end

AVFMediaPlayerSession::AVFMediaPlayerSession(AVFMediaPlayerService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_videoOutput(0)
    , m_state(QMediaPlayer::StoppedState)
    , m_mediaStatus(QMediaPlayer::NoMedia)
    , m_mediaStream(0)
    , m_muted(false)
    , m_tryingAsync(false)
    , m_volume(100)
    , m_rate(1.0)
    , m_requestedPosition(-1)
    , m_duration(0)
    , m_bufferStatus(0)
    , m_videoAvailable(false)
    , m_audioAvailable(false)
    , m_seekable(false)
{
    m_observer = [[AVFMediaPlayerSessionObserver alloc] initWithMediaPlayerSession:this];
}

AVFMediaPlayerSession::~AVFMediaPlayerSession()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    //Detatch the session from the sessionObserver (which could still be alive trying to communicate with this session).
    [(AVFMediaPlayerSessionObserver*)m_observer detatchSession];
    [(AVFMediaPlayerSessionObserver*)m_observer release];
}

void AVFMediaPlayerSession::setVideoOutput(AVFVideoOutput *output)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << output;
#endif

    if (m_videoOutput == output)
        return;

    //Set the current output layer to null to stop rendering
    if (m_videoOutput) {
        m_videoOutput->setLayer(0);
    }

    m_videoOutput = output;

    if (m_videoOutput && m_state != QMediaPlayer::StoppedState)
        m_videoOutput->setLayer([(AVFMediaPlayerSessionObserver*)m_observer playerLayer]);
}

void *AVFMediaPlayerSession::currentAssetHandle()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    AVAsset *currentAsset = [[(AVFMediaPlayerSessionObserver*)m_observer playerItem] asset];
    return currentAsset;
}

QMediaPlayer::State AVFMediaPlayerSession::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus AVFMediaPlayerSession::mediaStatus() const
{
    return m_mediaStatus;
}

QMediaContent AVFMediaPlayerSession::media() const
{
    return m_resources;
}

const QIODevice *AVFMediaPlayerSession::mediaStream() const
{
    return m_mediaStream;
}

void AVFMediaPlayerSession::setMedia(const QMediaContent &content, QIODevice *stream)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << content.canonicalUrl();
#endif

    [(AVFMediaPlayerSessionObserver*)m_observer unloadMedia];

    m_resources = content;
    m_mediaStream = stream;

    setAudioAvailable(false);
    setVideoAvailable(false);
    setSeekable(false);
    m_requestedPosition = -1;
    Q_EMIT positionChanged(position());

    const QMediaPlayer::MediaStatus oldMediaStatus = m_mediaStatus;
    const QMediaPlayer::State oldState = m_state;

    if (content.isNull() || content.canonicalUrl().isEmpty()) {
        m_mediaStatus = QMediaPlayer::NoMedia;
        if (m_mediaStatus != oldMediaStatus)
            Q_EMIT mediaStatusChanged(m_mediaStatus);

        m_state = QMediaPlayer::StoppedState;
        if (m_state != oldState)
            Q_EMIT stateChanged(m_state);

        return;
    }

    m_mediaStatus = QMediaPlayer::LoadingMedia;
    if (m_mediaStatus != oldMediaStatus)
        Q_EMIT mediaStatusChanged(m_mediaStatus);

    //Load AVURLAsset
    //initialize asset using content's URL
    NSString *urlString = [NSString stringWithUTF8String:content.canonicalUrl().toEncoded().constData()];
    NSURL *url = [NSURL URLWithString:urlString];
    [(AVFMediaPlayerSessionObserver*)m_observer setURL:url];

    m_state = QMediaPlayer::StoppedState;
    if (m_state != oldState)
        Q_EMIT stateChanged(m_state);
}

qint64 AVFMediaPlayerSession::position() const
{
    AVPlayerItem *playerItem = [(AVFMediaPlayerSessionObserver*)m_observer playerItem];

    if (!playerItem)
        return m_requestedPosition != -1 ? m_requestedPosition : 0;

    CMTime time = [playerItem currentTime];
    return static_cast<quint64>(float(time.value) / float(time.timescale) * 1000.0f);
}

qint64 AVFMediaPlayerSession::duration() const
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    return m_duration;
}

int AVFMediaPlayerSession::bufferStatus() const
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    return m_bufferStatus;
}

int AVFMediaPlayerSession::volume() const
{
    return m_volume;
}

bool AVFMediaPlayerSession::isMuted() const
{
    return m_muted;
}

void AVFMediaPlayerSession::setAudioAvailable(bool available)
{
    if (m_audioAvailable == available)
        return;

    m_audioAvailable = available;
    Q_EMIT audioAvailableChanged(available);
}

bool AVFMediaPlayerSession::isAudioAvailable() const
{
    return m_audioAvailable;
}

void AVFMediaPlayerSession::setVideoAvailable(bool available)
{
    if (m_videoAvailable == available)
        return;

    m_videoAvailable = available;
    Q_EMIT videoAvailableChanged(available);
}

bool AVFMediaPlayerSession::isVideoAvailable() const
{
    return m_videoAvailable;
}

bool AVFMediaPlayerSession::isSeekable() const
{
    return m_seekable;
}

void AVFMediaPlayerSession::setSeekable(bool seekable)
{
    if (m_seekable == seekable)
        return;

    m_seekable = seekable;
    Q_EMIT seekableChanged(seekable);
}

QMediaTimeRange AVFMediaPlayerSession::availablePlaybackRanges() const
{
    AVPlayerItem *playerItem = [(AVFMediaPlayerSessionObserver*)m_observer playerItem];

    if (playerItem) {
        QMediaTimeRange timeRanges;

        NSArray *ranges = [playerItem loadedTimeRanges];
        for (NSValue *timeRange in ranges) {
            CMTimeRange currentTimeRange = [timeRange CMTimeRangeValue];
            qint64 startTime = qint64(float(currentTimeRange.start.value) / currentTimeRange.start.timescale * 1000.0);
            timeRanges.addInterval(startTime, startTime + qint64(float(currentTimeRange.duration.value) / currentTimeRange.duration.timescale * 1000.0));
        }
        if (!timeRanges.isEmpty())
            return timeRanges;
    }
    return QMediaTimeRange(0, duration());
}

qreal AVFMediaPlayerSession::playbackRate() const
{
    return m_rate;
}

void AVFMediaPlayerSession::setPlaybackRate(qreal rate)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << rate;
#endif

    if (qFuzzyCompare(m_rate, rate))
        return;

    m_rate = rate;

    AVPlayer *player = [(AVFMediaPlayerSessionObserver*)m_observer player];
    if (player && m_state == QMediaPlayer::PlayingState)
        [player setRate:m_rate];

    Q_EMIT playbackRateChanged(m_rate);
}

void AVFMediaPlayerSession::setPosition(qint64 pos)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << pos;
#endif

    if (pos == position())
        return;

    AVPlayerItem *playerItem = [(AVFMediaPlayerSessionObserver*)m_observer playerItem];
    if (!playerItem) {
        m_requestedPosition = pos;
        Q_EMIT positionChanged(m_requestedPosition);
        return;
    }

    if (!isSeekable()) {
        if (m_requestedPosition != -1) {
            m_requestedPosition = -1;
            Q_EMIT positionChanged(position());
        }
        return;
    }

    pos = qMax(qint64(0), pos);
    if (duration() > 0)
        pos = qMin(pos, duration());

    CMTime newTime = [playerItem currentTime];
    newTime.value = (pos / 1000.0f) * newTime.timescale;
    [playerItem seekToTime:newTime];

    Q_EMIT positionChanged(pos);

    // Reset media status if the current status is EndOfMedia
    if (m_mediaStatus == QMediaPlayer::EndOfMedia) {
        QMediaPlayer::MediaStatus newMediaStatus = (m_state == QMediaPlayer::PausedState) ? QMediaPlayer::BufferedMedia
                                                                                          : QMediaPlayer::LoadedMedia;
        Q_EMIT mediaStatusChanged((m_mediaStatus = newMediaStatus));
    }
}

void AVFMediaPlayerSession::play()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << m_state;
#endif

    if (m_mediaStatus == QMediaPlayer::NoMedia || m_mediaStatus == QMediaPlayer::InvalidMedia)
        return;

    if (m_state == QMediaPlayer::PlayingState)
        return;

    if (m_videoOutput) {
        m_videoOutput->setLayer([(AVFMediaPlayerSessionObserver*)m_observer playerLayer]);
    }

    // Reset media status if the current status is EndOfMedia
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        setPosition(0);

    if (m_mediaStatus == QMediaPlayer::LoadedMedia || m_mediaStatus == QMediaPlayer::BufferedMedia) {
        // Setting the rate starts playback
        [[(AVFMediaPlayerSessionObserver*)m_observer player] setRate:m_rate];
    }

    m_state = QMediaPlayer::PlayingState;
    processLoadStateChange();

    Q_EMIT stateChanged(m_state);
}

void AVFMediaPlayerSession::pause()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << m_state;
#endif

    if (m_mediaStatus == QMediaPlayer::NoMedia)
        return;

    if (m_state == QMediaPlayer::PausedState)
        return;

    m_state = QMediaPlayer::PausedState;

    if (m_videoOutput) {
        m_videoOutput->setLayer([(AVFMediaPlayerSessionObserver*)m_observer playerLayer]);
    }

    [[(AVFMediaPlayerSessionObserver*)m_observer player] pause];

    // Reset media status if the current status is EndOfMedia
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        setPosition(0);


    Q_EMIT stateChanged(m_state);
}

void AVFMediaPlayerSession::stop()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << m_state;
#endif

    if (m_state == QMediaPlayer::StoppedState)
        return;

    // AVPlayer doesn't have stop(), only pause() and play().
    [[(AVFMediaPlayerSessionObserver*)m_observer player] pause];
    setPosition(0);

    if (m_videoOutput) {
        m_videoOutput->setLayer(0);
    }

    if (m_mediaStatus == QMediaPlayer::BufferedMedia)
        Q_EMIT mediaStatusChanged((m_mediaStatus = QMediaPlayer::LoadedMedia));

    Q_EMIT positionChanged(position());
    Q_EMIT stateChanged((m_state = QMediaPlayer::StoppedState));
}

void AVFMediaPlayerSession::setVolume(int volume)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << volume;
#endif

    if (m_volume == volume)
        return;

    m_volume = volume;

    AVPlayer *player = [(AVFMediaPlayerSessionObserver*)m_observer player];
    if (player)
        [player setVolume:volume / 100.0f];

    Q_EMIT volumeChanged(m_volume);
}

void AVFMediaPlayerSession::setMuted(bool muted)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << muted;
#endif

    if (m_muted == muted)
        return;

    m_muted = muted;

    AVPlayer *player = [(AVFMediaPlayerSessionObserver*)m_observer player];
    if (player)
        [player setMuted:muted];

    Q_EMIT mutedChanged(muted);
}

void AVFMediaPlayerSession::processEOS()
{
    //AVPlayerItem has reached end of track/stream
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    Q_EMIT positionChanged(position());
    m_mediaStatus = QMediaPlayer::EndOfMedia;
    Q_EMIT mediaStatusChanged(m_mediaStatus);

    m_state = QMediaPlayer::StoppedState;

    // At this point, frames should not be rendered anymore.
    // Clear the output layer to make sure of that.
    if (m_videoOutput)
        m_videoOutput->setLayer(0);

    Q_EMIT stateChanged(m_state);
}

void AVFMediaPlayerSession::processLoadStateChange(QMediaPlayer::State newState)
{
    AVPlayerStatus currentStatus = [[(AVFMediaPlayerSessionObserver*)m_observer player] status];

#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << currentStatus << ", " << m_mediaStatus << ", " << newState;
#endif

    if (m_mediaStatus == QMediaPlayer::NoMedia)
        return;

    if (currentStatus == AVPlayerStatusReadyToPlay) {

        QMediaPlayer::MediaStatus newStatus = m_mediaStatus;

        AVPlayerItem *playerItem = [(AVFMediaPlayerSessionObserver*)m_observer playerItem];

        if (playerItem) {
            // Check each track for audio and video content
            AVAssetTrack *videoTrack = nil;
            NSArray *tracks =  playerItem.tracks;
            for (AVPlayerItemTrack *track in tracks) {
                AVAssetTrack *assetTrack = track.assetTrack;
                if (assetTrack) {
                    if ([assetTrack.mediaType isEqualToString:AVMediaTypeAudio])
                        setAudioAvailable(true);
                    if ([assetTrack.mediaType isEqualToString:AVMediaTypeVideo]) {
                        setVideoAvailable(true);
                        if (!videoTrack)
                            videoTrack = assetTrack;
                    }
                }
            }

            setSeekable([[playerItem seekableTimeRanges] count] > 0);

            // Get the native size of the video, and reset the bounds of the player layer
            AVPlayerLayer *playerLayer = [(AVFMediaPlayerSessionObserver*)m_observer playerLayer];
            if (videoTrack && playerLayer) {
                playerLayer.bounds = CGRectMake(0.0f, 0.0f,
                                                videoTrack.naturalSize.width,
                                                videoTrack.naturalSize.height);

                if (m_videoOutput && newState != QMediaPlayer::StoppedState) {
                    m_videoOutput->setLayer(playerLayer);
                }
            }

            if (m_requestedPosition != -1) {
                setPosition(m_requestedPosition);
                m_requestedPosition = -1;
            }
        }

        newStatus = (newState != QMediaPlayer::StoppedState) ? QMediaPlayer::BufferedMedia
                                                             : QMediaPlayer::LoadedMedia;

        if (newStatus != m_mediaStatus)
            Q_EMIT mediaStatusChanged((m_mediaStatus = newStatus));

    }

    if (newState == QMediaPlayer::PlayingState && [(AVFMediaPlayerSessionObserver*)m_observer player]) {
        // Setting the rate is enough to start playback, no need to call play()
        [[(AVFMediaPlayerSessionObserver*)m_observer player] setRate:m_rate];
    }
}


void AVFMediaPlayerSession::processLoadStateChange()
{
    processLoadStateChange(m_state);
}


void AVFMediaPlayerSession::processLoadStateFailure()
{
    Q_EMIT stateChanged((m_state = QMediaPlayer::StoppedState));
}

void AVFMediaPlayerSession::processBufferStateChange(int bufferStatus)
{
    if (bufferStatus == m_bufferStatus)
        return;

    m_bufferStatus = bufferStatus;
    Q_EMIT bufferStatusChanged(bufferStatus);
}

void AVFMediaPlayerSession::processDurationChange(qint64 duration)
{
    if (duration == m_duration)
        return;

    m_duration = duration;
    Q_EMIT durationChanged(duration);
}

void AVFMediaPlayerSession::processPositionChange()
{
    if (m_state == QMediaPlayer::StoppedState)
        return;

    Q_EMIT positionChanged(position());
}

void AVFMediaPlayerSession::processMediaLoadError()
{
    if (m_requestedPosition != -1) {
        m_requestedPosition = -1;
        Q_EMIT positionChanged(position());
    }

    Q_EMIT mediaStatusChanged((m_mediaStatus = QMediaPlayer::InvalidMedia));

    Q_EMIT error(QMediaPlayer::FormatError, tr("Failed to load media"));
}
