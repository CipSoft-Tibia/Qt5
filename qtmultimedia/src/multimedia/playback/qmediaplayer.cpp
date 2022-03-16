/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmediaplayer.h"
#include "qvideosurfaceoutput_p.h"

#include "qmediaobject_p.h"
#include <qmediaservice.h>
#include <qmediaplayercontrol.h>
#include <qmediaserviceprovider_p.h>
#include <qmediaplaylist.h>
#include <qmediaplaylistcontrol_p.h>
#include <qmediaplaylistsourcecontrol_p.h>
#include <qmedianetworkaccesscontrol.h>
#include <qaudiorolecontrol.h>
#include <qcustomaudiorolecontrol.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qpointer.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qtemporaryfile.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaPlayer
    \brief The QMediaPlayer class allows the playing of a media source.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_playback

    The QMediaPlayer class is a high level media playback class. It can be used
    to playback such content as songs, movies and internet radio. The content
    to playback is specified as a QMediaContent object, which can be thought of as a
    main or canonical URL with additional information attached. When provided
    with a QMediaContent playback may be able to commence.

    \snippet multimedia-snippets/media.cpp Player

    QVideoWidget can be used with QMediaPlayer for video rendering and QMediaPlaylist
    for accessing playlist functionality.

    \snippet multimedia-snippets/media.cpp Movie playlist

    Since QMediaPlayer is a QMediaObject, you can use several of the QMediaObject
    functions for things like:

    \list
    \li Accessing the currently playing media's metadata (\l {QMediaObject::metaData()} and \l {QMediaMetaData}{predefined meta-data keys})
    \li Checking to see if the media playback service is currently available (\l {QMediaObject::availability()})
    \endlist

    \sa QMediaObject, QMediaService, QVideoWidget, QMediaPlaylist
*/

static void qRegisterMediaPlayerMetaTypes()
{
    qRegisterMetaType<QMediaPlayer::State>("QMediaPlayer::State");
    qRegisterMetaType<QMediaPlayer::MediaStatus>("QMediaPlayer::MediaStatus");
    qRegisterMetaType<QMediaPlayer::Error>("QMediaPlayer::Error");
}

Q_CONSTRUCTOR_FUNCTION(qRegisterMediaPlayerMetaTypes)

#define MAX_NESTED_PLAYLISTS 16

class QMediaPlayerPrivate : public QMediaObjectPrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QMediaPlayer)

public:
    QMediaPlayerPrivate()
        : provider(0)
        , control(0)
        , audioRoleControl(0)
        , customAudioRoleControl(0)
        , playlist(0)
        , networkAccessControl(0)
        , state(QMediaPlayer::StoppedState)
        , status(QMediaPlayer::UnknownMediaStatus)
        , error(QMediaPlayer::NoError)
        , ignoreNextStatusChange(-1)
        , nestedPlaylists(0)
        , hasStreamPlaybackFeature(false)
    {}

    QMediaServiceProvider *provider;
    QMediaPlayerControl* control;
    QAudioRoleControl *audioRoleControl;
    QCustomAudioRoleControl *customAudioRoleControl;
    QString errorString;

    QPointer<QObject> videoOutput;
    QMediaPlaylist *playlist;
    QMediaNetworkAccessControl *networkAccessControl;
    QVideoSurfaceOutput surfaceOutput;
    QMediaContent qrcMedia;
    QScopedPointer<QFile> qrcFile;

    QMediaContent rootMedia;
    QMediaContent pendingPlaylist;
    QMediaPlayer::State state;
    QMediaPlayer::MediaStatus status;
    QMediaPlayer::Error error;
    int ignoreNextStatusChange;
    int nestedPlaylists;
    bool hasStreamPlaybackFeature;

    QMediaPlaylist *parentPlaylist(QMediaPlaylist *pls);
    bool isInChain(const QUrl &url);

    void setMedia(const QMediaContent &media, QIODevice *stream = 0);

    void setPlaylist(QMediaPlaylist *playlist);
    void setPlaylistMedia();
    void loadPlaylist();
    void disconnectPlaylist();
    void connectPlaylist();

    void _q_stateChanged(QMediaPlayer::State state);
    void _q_mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void _q_error(int error, const QString &errorString);
    void _q_updateMedia(const QMediaContent&);
    void _q_playlistDestroyed();
    void _q_handleMediaChanged(const QMediaContent&);
    void _q_handlePlaylistLoaded();
    void _q_handlePlaylistLoadFailed();
};

QMediaPlaylist *QMediaPlayerPrivate::parentPlaylist(QMediaPlaylist *pls)
{
    // This function finds a parent playlist for an item in the active chain of playlists.
    // Every item in the chain comes from currentMedia() of its parent.
    // We don't need to travers the whole tree of playlists,
    // but only the subtree of active ones.
    for (QMediaPlaylist *current = rootMedia.playlist(); current && current != pls; current = current->currentMedia().playlist())
        if (current->currentMedia().playlist() == pls)
            return current;
    return 0;
}

bool QMediaPlayerPrivate::isInChain(const QUrl &url)
{
    // Check whether a URL is already in the chain of playlists.
    // Also see a comment in parentPlaylist().
    for (QMediaPlaylist *current = rootMedia.playlist(); current && current != playlist; current = current->currentMedia().playlist())
        if (current->currentMedia().canonicalUrl() == url) {
            return true;
        }
    return false;
}

void QMediaPlayerPrivate::_q_stateChanged(QMediaPlayer::State ps)
{
    Q_Q(QMediaPlayer);

    // Backend switches into stopped state every time new media is about to be loaded.
    // If media player has a playlist loaded make sure player doesn' stop.
    if (playlist && playlist->currentIndex() != -1 && ps != state && ps == QMediaPlayer::StoppedState) {
        if (control->mediaStatus() == QMediaPlayer::EndOfMedia ||
                control->mediaStatus() == QMediaPlayer::InvalidMedia) {
            // if media player is not stopped, and
            // we have finished playback for the current media,
            // advance to the next item in the playlist
            Q_ASSERT(state != QMediaPlayer::StoppedState);
            playlist->next();
            return;
        } else if (control->mediaStatus() == QMediaPlayer::LoadingMedia) {
            return;
        }
    }

    if (ps != state) {
        state = ps;

        if (ps == QMediaPlayer::PlayingState)
            q->addPropertyWatch("position");
        else
            q->removePropertyWatch("position");

        emit q->stateChanged(ps);
    }
}

void QMediaPlayerPrivate::_q_mediaStatusChanged(QMediaPlayer::MediaStatus s)
{
    Q_Q(QMediaPlayer);

    if (int(s) == ignoreNextStatusChange) {
        ignoreNextStatusChange = -1;
        return;
    }

    if (s != status) {
        status = s;

        switch (s) {
        case QMediaPlayer::StalledMedia:
        case QMediaPlayer::BufferingMedia:
            q->addPropertyWatch("bufferStatus");
            break;
        default:
            q->removePropertyWatch("bufferStatus");
            break;
        }

        emit q->mediaStatusChanged(s);
    }
}

void QMediaPlayerPrivate::_q_error(int error, const QString &errorString)
{
    Q_Q(QMediaPlayer);

    if (error == int(QMediaPlayer::MediaIsPlaylist)) {
        loadPlaylist();
    } else {
        this->error = QMediaPlayer::Error(error);
        this->errorString = errorString;
        emit q->error(this->error);

        if (playlist)
            playlist->next();
    }
}

void QMediaPlayerPrivate::_q_updateMedia(const QMediaContent &media)
{
    Q_Q(QMediaPlayer);

    if (!control)
        return;

    // check if the current playlist is a top-level playlist
    Q_ASSERT(playlist);
    if (media.isNull() && playlist != rootMedia.playlist()) {
        // switch back to parent playlist
        QMediaPlaylist *pls = parentPlaylist(playlist);
        Q_ASSERT(pls);
        disconnectPlaylist();
        playlist = pls;
        connectPlaylist();

        Q_ASSERT(!pendingPlaylist.playlist());
        nestedPlaylists--;
        Q_ASSERT(nestedPlaylists >= 0);

        playlist->next();
        return;
    }

    if (media.playlist()) {
        if (nestedPlaylists < MAX_NESTED_PLAYLISTS) {
            nestedPlaylists++;
            Q_ASSERT(!pendingPlaylist.playlist());

            // disconnect current playlist
            disconnectPlaylist();
            // new playlist signals are connected
            // in the call to setPlaylist() in _q_handlePlaylistLoaded()
            playlist = media.playlist();
            emit q->currentMediaChanged(media);
            _q_handlePlaylistLoaded();
            return;
        } else if (playlist) {
            playlist->next();
        }
        return;
    }

    const QMediaPlayer::State currentState = state;

    setMedia(media, 0);

    if (!media.isNull()) {
        switch (currentState) {
        case QMediaPlayer::PlayingState:
            control->play();
            break;
        case QMediaPlayer::PausedState:
            control->pause();
            break;
        default:
            break;
        }
    }

    _q_stateChanged(control->state());
}

void QMediaPlayerPrivate::_q_playlistDestroyed()
{
    playlist = 0;
    setMedia(QMediaContent(), 0);
}

void QMediaPlayerPrivate::setMedia(const QMediaContent &media, QIODevice *stream)
{
    Q_Q(QMediaPlayer);

    if (!control)
        return;

    QScopedPointer<QFile> file;

    // Backends can't play qrc files directly.
    // If the backend supports StreamPlayback, we pass a QFile for that resource.
    // If it doesn't, we copy the data to a temporary file and pass its path.
    if (!media.isNull() && !stream && media.canonicalUrl().scheme() == QLatin1String("qrc")) {
        qrcMedia = media;

        file.reset(new QFile(QLatin1Char(':') + media.canonicalUrl().path()));
        if (!file->open(QFile::ReadOnly)) {
            QMetaObject::invokeMethod(q, "_q_error", Qt::QueuedConnection,
                                      Q_ARG(int, QMediaPlayer::ResourceError),
                                      Q_ARG(QString, QMediaPlayer::tr("Attempting to play invalid Qt resource")));
            QMetaObject::invokeMethod(q, "_q_mediaStatusChanged", Qt::QueuedConnection,
                                      Q_ARG(QMediaPlayer::MediaStatus, QMediaPlayer::InvalidMedia));
            file.reset();
            // Ignore the next NoMedia status change, we just want to clear the current media
            // on the backend side since we can't load the new one and we want to be in the
            // InvalidMedia status.
            ignoreNextStatusChange = QMediaPlayer::NoMedia;
            control->setMedia(QMediaContent(), 0);

        } else if (hasStreamPlaybackFeature) {
            control->setMedia(media, file.data());
        } else {
#if QT_CONFIG(temporaryfile)
            QTemporaryFile *tempFile = new QTemporaryFile;

            // Preserve original file extension, some backends might not load the file if it doesn't
            // have an extension.
            const QString suffix = QFileInfo(*file).suffix();
            if (!suffix.isEmpty())
                tempFile->setFileTemplate(tempFile->fileTemplate() + QLatin1Char('.') + suffix);

            // Copy the qrc data into the temporary file
            tempFile->open();
            char buffer[4096];
            while (true) {
                qint64 len = file->read(buffer, sizeof(buffer));
                if (len < 1)
                    break;
                tempFile->write(buffer, len);
            }
            tempFile->close();

            file.reset(tempFile);
            control->setMedia(QMediaContent(QUrl::fromLocalFile(file->fileName())), 0);
#else
            qWarning("Qt was built with -no-feature-temporaryfile: playback from resource file is not supported!");
#endif
        }
    } else {
        qrcMedia = QMediaContent();
        control->setMedia(media, stream);
    }

    qrcFile.swap(file); // Cleans up any previous file
}

void QMediaPlayerPrivate::_q_handleMediaChanged(const QMediaContent &media)
{
    Q_Q(QMediaPlayer);

    emit q->currentMediaChanged(qrcMedia.isNull() ? media : qrcMedia);
}

void QMediaPlayerPrivate::setPlaylist(QMediaPlaylist *pls)
{
    disconnectPlaylist();
    playlist = pls;

    setPlaylistMedia();
}

void QMediaPlayerPrivate::setPlaylistMedia()
{
    // This function loads current playlist media into backend.
    // If current media is a playlist, the function recursively
    // loads media from the playlist.
    // It also makes sure the correct playlist signals are connected.
    Q_Q(QMediaPlayer);

    if (playlist) {
        connectPlaylist();
        if (playlist->currentMedia().playlist()) {
            if (nestedPlaylists < MAX_NESTED_PLAYLISTS) {
                emit q->currentMediaChanged(playlist->currentMedia());
                // rewind nested playlist to start
                playlist->currentMedia().playlist()->setCurrentIndex(0);
                nestedPlaylists++;
                setPlaylist(playlist->currentMedia().playlist());
            } else {
                playlist->next();
            }
            return;
        } else {
            // If we've just switched to a new playlist,
            // then last emitted currentMediaChanged was a playlist.
            // Make sure we emit currentMediaChanged if new playlist has
            // the same media as the previous one:
            // sample.m3u
            //      test.wav     -- processed by backend
            //      nested.m3u   -- processed by frontend
            //          test.wav -- processed by backend,
            //                      media is not changed,
            //                      frontend needs to emit currentMediaChanged
            bool isSameMedia = (q->currentMedia() == playlist->currentMedia());
            setMedia(playlist->currentMedia(), 0);
            if (isSameMedia) {
                emit q->currentMediaChanged(q->currentMedia());
            }
        }
    } else {
        setMedia(QMediaContent(), 0);
    }
}

void QMediaPlayerPrivate::loadPlaylist()
{
    Q_Q(QMediaPlayer);
    Q_ASSERT(pendingPlaylist.isNull());

    // Do not load a playlist if there are more than MAX_NESTED_PLAYLISTS in the chain already,
    // or if the playlist URL is already in the chain, i.e. do not allow recursive playlists and loops.
    if (nestedPlaylists < MAX_NESTED_PLAYLISTS && !q->currentMedia().canonicalUrl().isEmpty() && !isInChain(q->currentMedia().canonicalUrl())) {
        pendingPlaylist = QMediaContent(new QMediaPlaylist, q->currentMedia().canonicalUrl(), true);
        QObject::connect(pendingPlaylist.playlist(), SIGNAL(loaded()), q, SLOT(_q_handlePlaylistLoaded()));
        QObject::connect(pendingPlaylist.playlist(), SIGNAL(loadFailed()), q, SLOT(_q_handlePlaylistLoadFailed()));
        pendingPlaylist.playlist()->load(pendingPlaylist.canonicalRequest());
    } else if (playlist) {
        playlist->next();
    }
}

void QMediaPlayerPrivate::disconnectPlaylist()
{
    Q_Q(QMediaPlayer);
    if (playlist) {
        QObject::disconnect(playlist, SIGNAL(currentMediaChanged(QMediaContent)),
                            q, SLOT(_q_updateMedia(QMediaContent)));
        QObject::disconnect(playlist, SIGNAL(destroyed()), q, SLOT(_q_playlistDestroyed()));
        q->unbind(playlist);
    }
}

void QMediaPlayerPrivate::connectPlaylist()
{
    Q_Q(QMediaPlayer);
    if (playlist) {
        q->bind(playlist);
        QObject::connect(playlist, SIGNAL(currentMediaChanged(QMediaContent)),
                         q, SLOT(_q_updateMedia(QMediaContent)));
        QObject::connect(playlist, SIGNAL(destroyed()), q, SLOT(_q_playlistDestroyed()));
    }
}

void QMediaPlayerPrivate::_q_handlePlaylistLoaded()
{
    Q_Q(QMediaPlayer);

    if (pendingPlaylist.playlist()) {
        Q_ASSERT(!q->currentMedia().playlist());
        // if there is an active playlist
        if (playlist) {
            Q_ASSERT(playlist->currentIndex() >= 0);
            disconnectPlaylist();
            playlist->insertMedia(playlist->currentIndex() + 1, pendingPlaylist);
            playlist->removeMedia(playlist->currentIndex());
            nestedPlaylists++;
        } else {
            Q_ASSERT(!rootMedia.playlist());
            rootMedia = pendingPlaylist;
            emit q->mediaChanged(rootMedia);
        }

        playlist = pendingPlaylist.playlist();
        emit q->currentMediaChanged(pendingPlaylist);
    }
    pendingPlaylist = QMediaContent();

    playlist->next();
    setPlaylistMedia();

    switch (state) {
    case QMediaPlayer::PausedState:
        control->pause();
        break;
    case QMediaPlayer::PlayingState:
        control->play();
        break;
    case QMediaPlayer::StoppedState:
        break;
    }
}

void QMediaPlayerPrivate::_q_handlePlaylistLoadFailed()
{
    pendingPlaylist = QMediaContent();

    if (!control)
        return;

    if (playlist)
        playlist->next();
    else
        setMedia(QMediaContent(), 0);
}

static QMediaService *playerService(QMediaPlayer::Flags flags)
{
    QMediaServiceProvider *provider = QMediaServiceProvider::defaultServiceProvider();
    if (flags) {
        QMediaServiceProviderHint::Features features = 0;
        if (flags & QMediaPlayer::LowLatency)
            features |= QMediaServiceProviderHint::LowLatencyPlayback;

        if (flags & QMediaPlayer::StreamPlayback)
            features |= QMediaServiceProviderHint::StreamPlayback;

        if (flags & QMediaPlayer::VideoSurface)
            features |= QMediaServiceProviderHint::VideoSurface;

        return provider->requestService(Q_MEDIASERVICE_MEDIAPLAYER,
                                        QMediaServiceProviderHint(features));
    }

    return provider->requestService(Q_MEDIASERVICE_MEDIAPLAYER);
}


/*!
    Construct a QMediaPlayer instance
    parented to \a parent and with \a flags.
*/

QMediaPlayer::QMediaPlayer(QObject *parent, QMediaPlayer::Flags flags):
    QMediaObject(*new QMediaPlayerPrivate,
                 parent,
                 playerService(flags))
{
    Q_D(QMediaPlayer);

    d->provider = QMediaServiceProvider::defaultServiceProvider();
    if (d->service == 0) {
        d->error = ServiceMissingError;
    } else {
        d->control = qobject_cast<QMediaPlayerControl*>(d->service->requestControl(QMediaPlayerControl_iid));
        d->networkAccessControl = qobject_cast<QMediaNetworkAccessControl*>(d->service->requestControl(QMediaNetworkAccessControl_iid));
        if (d->control != 0) {
            connect(d->control, SIGNAL(mediaChanged(QMediaContent)), SLOT(_q_handleMediaChanged(QMediaContent)));
            connect(d->control, SIGNAL(stateChanged(QMediaPlayer::State)), SLOT(_q_stateChanged(QMediaPlayer::State)));
            connect(d->control, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
                    SLOT(_q_mediaStatusChanged(QMediaPlayer::MediaStatus)));
            connect(d->control, SIGNAL(error(int,QString)), SLOT(_q_error(int,QString)));

            connect(d->control, SIGNAL(durationChanged(qint64)), SIGNAL(durationChanged(qint64)));
            connect(d->control, SIGNAL(positionChanged(qint64)), SIGNAL(positionChanged(qint64)));
            connect(d->control, SIGNAL(audioAvailableChanged(bool)), SIGNAL(audioAvailableChanged(bool)));
            connect(d->control, SIGNAL(videoAvailableChanged(bool)), SIGNAL(videoAvailableChanged(bool)));
            connect(d->control, SIGNAL(volumeChanged(int)), SIGNAL(volumeChanged(int)));
            connect(d->control, SIGNAL(mutedChanged(bool)), SIGNAL(mutedChanged(bool)));
            connect(d->control, SIGNAL(seekableChanged(bool)), SIGNAL(seekableChanged(bool)));
            connect(d->control, SIGNAL(playbackRateChanged(qreal)), SIGNAL(playbackRateChanged(qreal)));
            connect(d->control, SIGNAL(bufferStatusChanged(int)), SIGNAL(bufferStatusChanged(int)));

            d->state = d->control->state();
            d->status = d->control->mediaStatus();

            if (d->state == PlayingState)
                addPropertyWatch("position");

            if (d->status == StalledMedia || d->status == BufferingMedia)
                addPropertyWatch("bufferStatus");

            d->hasStreamPlaybackFeature = d->provider->supportedFeatures(d->service).testFlag(QMediaServiceProviderHint::StreamPlayback);

            d->audioRoleControl = qobject_cast<QAudioRoleControl*>(d->service->requestControl(QAudioRoleControl_iid));
            if (d->audioRoleControl) {
                connect(d->audioRoleControl, &QAudioRoleControl::audioRoleChanged,
                        this, &QMediaPlayer::audioRoleChanged);

                d->customAudioRoleControl = qobject_cast<QCustomAudioRoleControl *>(
                        d->service->requestControl(QCustomAudioRoleControl_iid));
                if (d->customAudioRoleControl) {
                    connect(d->customAudioRoleControl,
                            &QCustomAudioRoleControl::customAudioRoleChanged,
                            this,
                            &QMediaPlayer::customAudioRoleChanged);
                }
            }
        }
        if (d->networkAccessControl != 0) {
            connect(d->networkAccessControl, SIGNAL(configurationChanged(QNetworkConfiguration)),
            this, SIGNAL(networkConfigurationChanged(QNetworkConfiguration)));
        }
    }
}


/*!
    Destroys the player object.
*/

QMediaPlayer::~QMediaPlayer()
{
    Q_D(QMediaPlayer);

    d->disconnectPlaylist();
    // Disconnect everything to prevent notifying
    // when a receiver is already destroyed.
    disconnect();

    if (d->service) {
        if (d->control)
            d->service->releaseControl(d->control);
        if (d->audioRoleControl)
            d->service->releaseControl(d->audioRoleControl);
        if (d->customAudioRoleControl)
            d->service->releaseControl(d->customAudioRoleControl);

        d->provider->releaseService(d->service);
    }
}

QMediaContent QMediaPlayer::media() const
{
    Q_D(const QMediaPlayer);

    return d->rootMedia;
}

/*!
    Returns the stream source of media data.

    This is only valid if a stream was passed to setMedia().

    \sa setMedia()
*/

const QIODevice *QMediaPlayer::mediaStream() const
{
    Q_D(const QMediaPlayer);

    // When playing a resource file, we might have passed a QFile to the backend. Hide it from
    // the user.
    if (d->control && d->qrcMedia.isNull())
        return d->control->mediaStream();

    return 0;
}

QMediaPlaylist *QMediaPlayer::playlist() const
{
    Q_D(const QMediaPlayer);

    return d->rootMedia.playlist();
}

QMediaContent QMediaPlayer::currentMedia() const
{
    Q_D(const QMediaPlayer);

    // When playing a resource file, don't return the backend's current media, which
    // can be a temporary file.
    if (!d->qrcMedia.isNull())
        return d->qrcMedia;

    if (d->control)
        return d->control->media();

    return QMediaContent();
}

void QMediaPlayer::setPlaylist(QMediaPlaylist *playlist)
{
    QMediaContent m(playlist, QUrl(), false);
    setMedia(m);
}

/*!
    Sets the network access points for remote media playback.
    \a configurations contains, in ascending preferential order, a list of
    configuration  that can be used for network access.

    This will invalidate the choice of previous configurations.
*/
void QMediaPlayer::setNetworkConfigurations(const QList<QNetworkConfiguration> &configurations)
{
    Q_D(QMediaPlayer);

    if (d->networkAccessControl)
        d->networkAccessControl->setConfigurations(configurations);
}

QMediaPlayer::State QMediaPlayer::state() const
{
    Q_D(const QMediaPlayer);

    // In case if EndOfMedia status is already received
    // but state is not.
    if (d->control != 0
        && d->status == QMediaPlayer::EndOfMedia
        && d->state != d->control->state()) {
        return d->control->state();
    }

    return d->state;
}

QMediaPlayer::MediaStatus QMediaPlayer::mediaStatus() const
{
    return d_func()->status;
}

qint64 QMediaPlayer::duration() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->duration();

    return -1;
}

qint64 QMediaPlayer::position() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->position();

    return 0;
}

int QMediaPlayer::volume() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->volume();

    return 0;
}

bool QMediaPlayer::isMuted() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->isMuted();

    return false;
}

int QMediaPlayer::bufferStatus() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->bufferStatus();

    return 0;
}

bool QMediaPlayer::isAudioAvailable() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->isAudioAvailable();

    return false;
}

bool QMediaPlayer::isVideoAvailable() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->isVideoAvailable();

    return false;
}

bool QMediaPlayer::isSeekable() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->isSeekable();

    return false;
}

qreal QMediaPlayer::playbackRate() const
{
    Q_D(const QMediaPlayer);

    if (d->control != 0)
        return d->control->playbackRate();

    return 0.0;
}

/*!
    Returns the current error state.
*/

QMediaPlayer::Error QMediaPlayer::error() const
{
    return d_func()->error;
}

QString QMediaPlayer::errorString() const
{
    return d_func()->errorString;
}

/*!
    Returns the current network access point  in use.
    If a default contructed QNetworkConfiguration is returned
    this feature is not available or that none of the
    current supplied configurations are in use.
*/
QNetworkConfiguration QMediaPlayer::currentNetworkConfiguration() const
{
    Q_D(const QMediaPlayer);

    if (d->networkAccessControl)
        return d_func()->networkAccessControl->currentConfiguration();

    return QNetworkConfiguration();
}

//public Q_SLOTS:
/*!
    Start or resume playing the current source.
*/

void QMediaPlayer::play()
{
    Q_D(QMediaPlayer);

    if (d->control == 0) {
        QMetaObject::invokeMethod(this, "_q_error", Qt::QueuedConnection,
                                    Q_ARG(int, QMediaPlayer::ServiceMissingError),
                                    Q_ARG(QString, tr("The QMediaPlayer object does not have a valid service")));
        return;
    }

    //if playlist control is available, the service should advance itself
    if (d->rootMedia.playlist() && !d->rootMedia.playlist()->isEmpty()) {
        // switch to playing state
        if (d->state != QMediaPlayer::PlayingState)
            d->_q_stateChanged(QMediaPlayer::PlayingState);

        if (d->rootMedia.playlist()->currentIndex() == -1) {
            if (d->playlist != d->rootMedia.playlist())
                d->setPlaylist(d->rootMedia.playlist());
            Q_ASSERT(d->playlist == d->rootMedia.playlist());

            emit currentMediaChanged(d->rootMedia);
            d->playlist->setCurrentIndex(0);
        }
    }

    // Reset error conditions
    d->error = NoError;
    d->errorString = QString();

    d->control->play();
}

/*!
    Pause playing the current source.
*/

void QMediaPlayer::pause()
{
    Q_D(QMediaPlayer);

    if (d->control != 0)
        d->control->pause();
}

/*!
    Stop playing, and reset the play position to the beginning.
*/

void QMediaPlayer::stop()
{
    Q_D(QMediaPlayer);

    if (d->control != 0)
        d->control->stop();

    // If media player didn't stop in response to control.
    // This happens if we have an active playlist and control
    // media status is
    // QMediaPlayer::LoadingMedia, QMediaPlayer::InvalidMedia, or QMediaPlayer::EndOfMedia
    // see QMediaPlayerPrivate::_q_stateChanged()
    if (d->playlist && d->state != QMediaPlayer::StoppedState) {
        d->state = QMediaPlayer::StoppedState;
        removePropertyWatch("position");
        emit stateChanged(QMediaPlayer::StoppedState);
    }
}

void QMediaPlayer::setPosition(qint64 position)
{
    Q_D(QMediaPlayer);

    if (d->control == 0)
        return;

    d->control->setPosition(qMax(position, 0ll));
}

void QMediaPlayer::setVolume(int v)
{
    Q_D(QMediaPlayer);

    if (d->control == 0)
        return;

    int clamped = qBound(0, v, 100);
    if (clamped == volume())
        return;

    d->control->setVolume(clamped);
}

void QMediaPlayer::setMuted(bool muted)
{
    Q_D(QMediaPlayer);

    if (d->control == 0 || muted == isMuted())
        return;

    d->control->setMuted(muted);
}

void QMediaPlayer::setPlaybackRate(qreal rate)
{
    Q_D(QMediaPlayer);

    if (d->control != 0)
        d->control->setPlaybackRate(rate);
}

/*!
    Sets the current \a media source.

    If a \a stream is supplied; media data will be read from it instead of resolving the media
    source. In this case the media source may still be used to resolve additional information
    about the media such as mime type. The \a stream must be open and readable.

    Setting the media to a null QMediaContent will cause the player to discard all
    information relating to the current media source and to cease all I/O operations related
    to that media.

    \note This function returns immediately after recording the specified source of the media.
    It does not wait for the media to finish loading and does not check for errors. Listen for
    the mediaStatusChanged() and error() signals to be notified when the media is loaded and
    when an error occurs during loading.

    Since Qt 5.12.2, the url scheme \c gst-pipeline provides custom pipelines
    for the GStreamer backend.

    \snippet multimedia-snippets/media.cpp Pipeline

    If the pipeline contains a video sink element named \c qtvideosink,
    current QVideoWidget can be used to render the video.

    If the pipeline contains appsrc element, it will be used to push data from \a stream.

    \snippet multimedia-snippets/media.cpp Pipeline appsrc
*/

void QMediaPlayer::setMedia(const QMediaContent &media, QIODevice *stream)
{
    Q_D(QMediaPlayer);
    stop();

    QMediaContent oldMedia = d->rootMedia;
    d->disconnectPlaylist();
    d->playlist = 0;
    d->rootMedia = media;
    d->nestedPlaylists = 0;

    if (oldMedia != media)
        emit mediaChanged(d->rootMedia);

    if (media.playlist()) {
        // reset playlist to the 1st item
        media.playlist()->setCurrentIndex(0);
        d->setPlaylist(media.playlist());
    } else {
        d->setMedia(media, stream);
    }
}

/*!
    \internal
*/

bool QMediaPlayer::bind(QObject *obj)
{
    return QMediaObject::bind(obj);
}

/*!
    \internal
*/

void QMediaPlayer::unbind(QObject *obj)
{
    QMediaObject::unbind(obj);
}

/*!
    Returns the level of support a media player has for a \a mimeType and a set of \a codecs.

    The \a flags argument allows additional requirements such as performance indicators to be
    specified.
*/
QMultimedia::SupportEstimate QMediaPlayer::hasSupport(const QString &mimeType,
                                               const QStringList& codecs,
                                               Flags flags)
{
    return QMediaServiceProvider::defaultServiceProvider()->hasSupport(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER),
                                                                    mimeType,
                                                                    codecs,
                                                                    flags);
}

/*!
    \deprecated
    Returns a list of MIME types supported by the media player.

    The \a flags argument causes the resultant list to be restricted to MIME types which can be supported
    given additional requirements, such as performance indicators.

    This function may not return useful results on some platforms, and support for a specific file of a
    given mime type is not guaranteed even if the mime type is in general supported.  In addition, in some
    cases this function will need to load all available media plugins and query them for their support, which
    may take some time.
*/
QStringList QMediaPlayer::supportedMimeTypes(Flags flags)
{
    return QMediaServiceProvider::defaultServiceProvider()->supportedMimeTypes(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER),
                                                                               flags);
}

/*!
    \fn void QMediaPlayer::setVideoOutput(QVideoWidget* output)

    Attach a QVideoWidget video \a output to the media player.

    If the media player has already video output attached,
    it will be replaced with a new one.
*/
void QMediaPlayer::setVideoOutput(QVideoWidget *output)
{
    Q_D(QMediaPlayer);

    if (d->videoOutput)
        unbind(d->videoOutput);

    // We don't know (in this library) that QVideoWidget inherits QObject
    QObject *outputObject = reinterpret_cast<QObject*>(output);

    d->videoOutput = outputObject && bind(outputObject) ? outputObject : 0;
}

/*!
    \fn void QMediaPlayer::setVideoOutput(QGraphicsVideoItem* output)

    Attach a QGraphicsVideoItem video \a output to the media player.

    If the media player has already video output attached,
    it will be replaced with a new one.
*/
void QMediaPlayer::setVideoOutput(QGraphicsVideoItem *output)
{
    Q_D(QMediaPlayer);

    if (d->videoOutput)
        unbind(d->videoOutput);

    // We don't know (in this library) that QGraphicsVideoItem (multiply) inherits QObject
    // but QObject inheritance depends on QObject coming first, so try this out.
    QObject *outputObject = reinterpret_cast<QObject*>(output);

    d->videoOutput = outputObject && bind(outputObject) ? outputObject : 0;
}

/*!
    Sets a video \a surface as the video output of a media player.

    If a video output has already been set on the media player the new surface
    will replace it.
*/

void QMediaPlayer::setVideoOutput(QAbstractVideoSurface *surface)
{
    Q_D(QMediaPlayer);

    d->surfaceOutput.setVideoSurface(surface);

    if (d->videoOutput != &d->surfaceOutput) {
        if (d->videoOutput)
            unbind(d->videoOutput);

        d->videoOutput = 0;

        if (surface && bind(&d->surfaceOutput))
            d->videoOutput =  &d->surfaceOutput;
    }  else if (!surface) {
        //unbind the surfaceOutput if null surface is set
        unbind(&d->surfaceOutput);
        d->videoOutput = 0;
    }
}

/*! \reimp */
QMultimedia::AvailabilityStatus QMediaPlayer::availability() const
{
    Q_D(const QMediaPlayer);

    if (!d->control)
        return QMultimedia::ServiceMissing;

    return QMediaObject::availability();
}

QAudio::Role QMediaPlayer::audioRole() const
{
    Q_D(const QMediaPlayer);

    if (d->audioRoleControl != NULL)
        return d->audioRoleControl->audioRole();

    return QAudio::UnknownRole;
}

void QMediaPlayer::setAudioRole(QAudio::Role audioRole)
{
    Q_D(QMediaPlayer);

    if (d->audioRoleControl) {
        if (d->customAudioRoleControl != nullptr && d->audioRoleControl->audioRole() != audioRole) {
            d->customAudioRoleControl->setCustomAudioRole(QString());
        }

        d->audioRoleControl->setAudioRole(audioRole);
    }
}

/*!
    Returns a list of supported audio roles.

    If setting the audio role is not supported, an empty list is returned.

    \since 5.6
    \sa audioRole
*/
QList<QAudio::Role> QMediaPlayer::supportedAudioRoles() const
{
    Q_D(const QMediaPlayer);

    if (d->audioRoleControl)
        return d->audioRoleControl->supportedAudioRoles();

    return QList<QAudio::Role>();
}

QString QMediaPlayer::customAudioRole() const
{
    Q_D(const QMediaPlayer);

    if (audioRole() != QAudio::CustomRole)
        return QString();

    if (d->customAudioRoleControl != nullptr)
        return d->customAudioRoleControl->customAudioRole();

    return QString();
}

void QMediaPlayer::setCustomAudioRole(const QString &audioRole)
{
    Q_D(QMediaPlayer);

    if (d->customAudioRoleControl) {
        Q_ASSERT(d->audioRoleControl);
        setAudioRole(QAudio::CustomRole);
        d->customAudioRoleControl->setCustomAudioRole(audioRole);
    }
}

/*!
    Returns a list of supported custom audio roles. An empty list may
    indicate that the supported custom audio roles aren't known. The
    list may not be complete.

    \since 5.11
    \sa customAudioRole
*/
QStringList QMediaPlayer::supportedCustomAudioRoles() const
{
    Q_D(const QMediaPlayer);

    if (d->customAudioRoleControl)
        return d->customAudioRoleControl->supportedCustomAudioRoles();

    return QStringList();
}

// Enums
/*!
    \enum QMediaPlayer::State

    Defines the current state of a media player.

    \value StoppedState The media player is not playing content, playback will begin from the start
    of the current track.
    \value PlayingState The media player is currently playing content.
    \value PausedState The media player has paused playback, playback of the current track will
    resume from the position the player was paused at.
*/

/*!
    \enum QMediaPlayer::MediaStatus

    Defines the status of a media player's current media.

    \value UnknownMediaStatus The status of the media cannot be determined.
    \value NoMedia The is no current media.  The player is in the StoppedState.
    \value LoadingMedia The current media is being loaded. The player may be in any state.
    \value LoadedMedia The current media has been loaded. The player is in the StoppedState.
    \value StalledMedia Playback of the current media has stalled due to insufficient buffering or
    some other temporary interruption.  The player is in the PlayingState or PausedState.
    \value BufferingMedia The player is buffering data but has enough data buffered for playback to
    continue for the immediate future.  The player is in the PlayingState or PausedState.
    \value BufferedMedia The player has fully buffered the current media.  The player is in the
    PlayingState or PausedState.
    \value EndOfMedia Playback has reached the end of the current media.  The player is in the
    StoppedState.
    \value InvalidMedia The current media cannot be played.  The player is in the StoppedState.
*/

/*!
    \enum QMediaPlayer::Error

    Defines a media player error condition.

    \value NoError No error has occurred.
    \value ResourceError A media resource couldn't be resolved.
    \value FormatError The format of a media resource isn't (fully) supported.  Playback may still
    be possible, but without an audio or video component.
    \value NetworkError A network error occurred.
    \value AccessDeniedError There are not the appropriate permissions to play a media resource.
    \value ServiceMissingError A valid playback service was not found, playback cannot proceed.
    \omitvalue MediaIsPlaylist
*/

// Signals
/*!
    \fn QMediaPlayer::error(QMediaPlayer::Error error)

    Signals that an \a error condition has occurred.

    \sa errorString()
*/

/*!
    \fn void QMediaPlayer::stateChanged(State state)

    Signal the \a state of the Player object has changed.
*/

/*!
    \fn QMediaPlayer::mediaStatusChanged(QMediaPlayer::MediaStatus status)

    Signals that the \a status of the current media has changed.

    \sa mediaStatus()
*/

/*!
    \fn void QMediaPlayer::mediaChanged(const QMediaContent &media);

    Signals that the media source has been changed to \a media.

    \sa media(), currentMediaChanged()
*/

/*!
    \fn void QMediaPlayer::currentMediaChanged(const QMediaContent &media);

    Signals that the current playing content has been changed to \a media.

    \sa currentMedia(), mediaChanged()
*/

/*!
    \fn void QMediaPlayer::playbackRateChanged(qreal rate);

    Signals the playbackRate has changed to \a rate.
*/

/*!
    \fn void QMediaPlayer::seekableChanged(bool seekable);

    Signals the \a seekable status of the player object has changed.
*/

/*!
    \fn void QMediaPlayer::audioRoleChanged(QAudio::Role role)

    Signals that the audio \a role of the media player has changed.

    \since 5.6
*/

/*!
    \fn void QMediaPlayer::customAudioRoleChanged(const QString &role)

    Signals that the audio \a role of the media player has changed.

    \since 5.11
*/

// Properties
/*!
    \property QMediaPlayer::state
    \brief the media player's playback state.

    By default this property is QMediaPlayer::Stopped

    \sa mediaStatus(), play(), pause(), stop()
*/

/*!
    \property QMediaPlayer::error
    \brief a string describing the last error condition.

    \sa error()
*/

/*!
    \property QMediaPlayer::media
    \brief the active media source being used by the player object.

    The player object will use the QMediaContent for selection of the content to
    be played.

    By default this property has a null QMediaContent.

    Setting this property to a null QMediaContent will cause the player to discard all
    information relating to the current media source and to cease all I/O operations related
    to that media.

    \sa QMediaContent, currentMedia()
*/

/*!
    \property QMediaPlayer::currentMedia
    \brief the current active media content being played by the player object.
    This value could be different from QMediaPlayer::media property if a playlist is used.
    In this case currentMedia indicates the current media content being processed
    by the player, while QMediaPlayer::media property contains the original playlist.

    \sa QMediaContent, media()
*/

/*!
    \property QMediaPlayer::playlist
    \brief the media playlist being used by the player object.

    The player object will use the current playlist item for selection of the content to
    be played.

    By default this property is set to null.

    If the media playlist is used as a source, QMediaPlayer::currentMedia is updated with
    a current playlist item. The current source should be selected with
    QMediaPlaylist::setCurrentIndex(int) instead of QMediaPlayer::setMedia(),
    otherwise the current playlist will be discarded.

    \sa QMediaContent
*/


/*!
    \property QMediaPlayer::mediaStatus
    \brief the status of the current media stream.

    The stream status describes how the playback of the current stream is
    progressing.

    By default this property is QMediaPlayer::NoMedia

    \sa state
*/

/*!
    \property QMediaPlayer::duration
    \brief the duration of the current media.

    The value is the total playback time in milliseconds of the current media.
    The value may change across the life time of the QMediaPlayer object and
    may not be available when initial playback begins, connect to the
    durationChanged() signal to receive status notifications.
*/

/*!
    \property QMediaPlayer::position
    \brief the playback position of the current media.

    The value is the current playback position, expressed in milliseconds since
    the beginning of the media. Periodically changes in the position will be
    indicated with the signal positionChanged(), the interval between updates
    can be set with QMediaObject's method setNotifyInterval().
*/

/*!
    \property QMediaPlayer::volume
    \brief the current playback volume.

    The playback volume is scaled linearly, ranging from \c 0 (silence) to \c 100 (full volume).
    Values outside this range will be clamped.

    By default the volume is \c 100.

    UI volume controls should usually be scaled nonlinearly. For example, using a logarithmic scale
    will produce linear changes in perceived loudness, which is what a user would normally expect
    from a volume control. See QAudio::convertVolume() for more details.
*/

/*!
    \property QMediaPlayer::muted
    \brief the muted state of the current media.

    The value will be true if the playback volume is muted; otherwise false.
*/

/*!
    \property QMediaPlayer::bufferStatus
    \brief the percentage of the temporary buffer filled before playback begins or resumes, from
    \c 0 (empty) to \c 100 (full).

    When the player object is buffering; this property holds the percentage of
    the temporary buffer that is filled. The buffer will need to reach 100%
    filled before playback can start or resume, at which time mediaStatus() will return
    BufferedMedia or BufferingMedia. If the value is anything lower than \c 100, mediaStatus() will
    return StalledMedia.

    \sa mediaStatus()
*/

/*!
    \property QMediaPlayer::audioAvailable
    \brief the audio availabilty status for the current media.

    As the life time of QMediaPlayer can be longer than the playback of one
    QMediaContent, this property may change over time, the
    audioAvailableChanged signal can be used to monitor it's status.
*/

/*!
    \property QMediaPlayer::videoAvailable
    \brief the video availability status for the current media.

    If available, the QVideoWidget class can be used to view the video. As the
    life time of QMediaPlayer can be longer than the playback of one
    QMediaContent, this property may change over time, the
    videoAvailableChanged signal can be used to monitor it's status.

    \sa QVideoWidget, QMediaContent
*/

/*!
    \property QMediaPlayer::seekable
    \brief the seek-able status of the current media

    If seeking is supported this property will be true; false otherwise. The
    status of this property may change across the life time of the QMediaPlayer
    object, use the seekableChanged signal to monitor changes.
*/

/*!
    \property QMediaPlayer::playbackRate
    \brief the playback rate of the current media.

    This value is a multiplier applied to the media's standard play rate. By
    default this value is 1.0, indicating that the media is playing at the
    standard pace. Values higher than 1.0 will increase the rate of play.
    Values less than zero can be set and indicate the media will rewind at the
    multiplier of the standard pace.

    Not all playback services support change of the playback rate. It is
    framework defined as to the status and quality of audio and video
    while fast forwarding or rewinding.
*/

/*!
    \property QMediaPlayer::audioRole
    \brief the role of the audio stream played by the media player.

    It can be set to specify the type of audio being played, allowing the system to make
    appropriate decisions when it comes to volume, routing or post-processing.

    The audio role must be set before calling setMedia().

    customAudioRole is cleared when this property is set to anything other than
    QAudio::CustomRole.

    \since 5.6
    \sa supportedAudioRoles()
*/

/*!
    \property QMediaPlayer::customAudioRole
    \brief the role of the audio stream played by the media player.

    It can be set to specify the type of audio being played when the backend supports
    audio roles unknown to Qt. Specifying a role allows the system to make appropriate
    decisions when it comes to volume, routing or post-processing.

    The audio role must be set before calling setMedia().

    audioRole is set to QAudio::CustomRole when this property is set.

    \since 5.11
    \sa supportedCustomAudioRoles()
*/

/*!
    \fn void QMediaPlayer::durationChanged(qint64 duration)

    Signal the duration of the content has changed to \a duration, expressed in milliseconds.
*/

/*!
    \fn void QMediaPlayer::positionChanged(qint64 position)

    Signal the position of the content has changed to \a position, expressed in
    milliseconds.
*/

/*!
    \fn void QMediaPlayer::volumeChanged(int volume)

    Signal the playback volume has changed to \a volume.
*/

/*!
    \fn void QMediaPlayer::mutedChanged(bool muted)

    Signal the mute state has changed to \a muted.
*/

/*!
    \fn void QMediaPlayer::videoAvailableChanged(bool videoAvailable)

    Signal the availability of visual content has changed to \a videoAvailable.
*/

/*!
    \fn void QMediaPlayer::audioAvailableChanged(bool available)

    Signals the availability of audio content has changed to \a available.
*/

/*!
    \fn void QMediaPlayer::bufferStatusChanged(int percentFilled)

    Signal the amount of the local buffer filled as a percentage by \a percentFilled.
*/

/*!
   \fn void QMediaPlayer::networkConfigurationChanged(const QNetworkConfiguration &configuration)

    Signal that the active in use network access point  has been changed to \a configuration and all subsequent network access will use this \a configuration.
*/

/*!
    \enum QMediaPlayer::Flag

    \value LowLatency       The player is expected to be used with simple audio formats,
            but playback should start without significant delay.
            Such playback service can be used for beeps, ringtones, etc.

    \value StreamPlayback   The player is expected to play QIODevice based streams.
            If passed to QMediaPlayer constructor, the service supporting
            streams playback will be chosen.

    \value VideoSurface     The player is expected to be able to render to a
            QAbstractVideoSurface \l {setVideoOutput()}{output}.
*/

#include "moc_qmediaplayer.cpp"
QT_END_NAMESPACE

