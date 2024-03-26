// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "connectionmanager.h"
#include "filemanager.h"
#include "metainfo.h"
#include "torrentclient.h"
#include "torrentserver.h"
#include "trackerclient.h"
#include "peerwireclient.h"
#include "ratecontroller.h"

#include <QtCore>
#include <QNetworkInterface>

#include <algorithm>
#include <chrono>

// These constants could also be configurable by the user.
static const int ServerMinPort = 6881;
static const int ServerMaxPort = /* 6889 */ 7000;
static const int BlockSize = 16384;
static const int MaxBlocksInProgress = 5;
static const int MaxBlocksInMultiMode = 2;
static const int MaxConnectionPerPeer = 1;
static const int RateControlWindowLength = 10;
static const std::chrono::seconds RateControlTimerDelay(1);
static const int MinimumTimeBeforeRevisit = 30;
static const int MaxUploads = 4;
static const std::chrono::seconds UploadScheduleInterval(10);

struct TorrentPiece {
    QBitArray completedBlocks;
    QBitArray requestedBlocks;
    int index = 0;
    int length = 0;
    bool inProgress = false;
};

class TorrentClientPrivate
{
public:
    TorrentClientPrivate(TorrentClient *qq);

    // State / error
    void setError(TorrentClient::Error error);
    void setState(TorrentClient::State state);
    TorrentClient::Error error;
    TorrentClient::State state;
    QString errorString;
    QString stateString;

    // Where to save data
    QString destinationFolder;
    MetaInfo metaInfo;

    // Announce tracker and file manager
    QByteArray peerId;
    QByteArray infoHash;
    TrackerClient trackerClient;
    FileManager fileManager;

    // Connections
    QList<PeerWireClient *> connections;
    QList<TorrentPeer *> peers;
    bool schedulerCalled;
    void callScheduler();
    bool connectingToClients;
    void callPeerConnector();
    int uploadScheduleTimer;

    // Pieces
    QMap<qint32, PeerWireClient *> readIds;
    QMultiMap<PeerWireClient *, TorrentPiece *> payloads;
    QMap<qint32, TorrentPiece *> pendingPieces;
    QBitArray completedPieces;
    QBitArray incompletePieces;
    int pieceCount;

    // Progress
    int lastProgressValue;
    qint64 downloadedBytes;
    qint64 uploadedBytes;
    int downloadRate[RateControlWindowLength];
    int uploadRate[RateControlWindowLength];
    int transferRateTimer;

    TorrentClient *q;
};

TorrentClientPrivate::TorrentClientPrivate(TorrentClient *qq)
    : trackerClient(qq), q(qq)
{
    error = TorrentClient::UnknownError;
    state = TorrentClient::Idle;
    errorString = QT_TRANSLATE_NOOP(TorrentClient, "Unknown error");
    stateString = QT_TRANSLATE_NOOP(TorrentClient, "Idle");
    schedulerCalled = false;
    connectingToClients = false;
    uploadScheduleTimer = 0;
    lastProgressValue = -1;
    pieceCount = 0;
    downloadedBytes = 0;
    uploadedBytes = 0;
    memset(downloadRate, 0, sizeof(downloadRate));
    memset(uploadRate, 0, sizeof(uploadRate));
    transferRateTimer = 0;
}

void TorrentClientPrivate::setError(TorrentClient::Error errorCode)
{
    this->error = errorCode;
    switch (error) {
    case TorrentClient::UnknownError:
        errorString = QT_TRANSLATE_NOOP(TorrentClient, "Unknown error");
        break;
    case TorrentClient::TorrentParseError:
        errorString = QT_TRANSLATE_NOOP(TorrentClient, "Invalid torrent data");
        break;
    case TorrentClient::InvalidTrackerError:
        errorString = QT_TRANSLATE_NOOP(TorrentClient, "Unable to connect to tracker");
        break;
    case TorrentClient::FileError:
        errorString = QT_TRANSLATE_NOOP(TorrentClient, "File error");
        break;
    case TorrentClient::ServerError:
        errorString = QT_TRANSLATE_NOOP(TorrentClient, "Unable to initialize server");
        break;
    }
    emit q->error(errorCode);
}

void TorrentClientPrivate::setState(TorrentClient::State state)
{
    this->state = state;
    switch (state) {
    case TorrentClient::Idle:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Idle");
        break;
    case TorrentClient::Paused:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Paused");
        break;
    case TorrentClient::Stopping:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Stopping");
        break;
    case TorrentClient::Preparing:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Preparing");
        break;
    case TorrentClient::Searching:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Searching");
        break;
    case TorrentClient::Connecting:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Connecting");
        break;
    case TorrentClient::WarmingUp:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Warming up");
        break;
    case TorrentClient::Downloading:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Downloading");
        break;
    case TorrentClient::Endgame:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Finishing");
        break;
    case TorrentClient::Seeding:
        stateString = QT_TRANSLATE_NOOP(TorrentClient, "Seeding");
        break;
    }
    emit q->stateChanged(state);
}

void TorrentClientPrivate::callScheduler()
{
    if (!schedulerCalled) {
        schedulerCalled = true;
        QMetaObject::invokeMethod(q, "scheduleDownloads", Qt::QueuedConnection);
    }
}

void TorrentClientPrivate::callPeerConnector()
{
    if (!connectingToClients) {
        connectingToClients = true;
        QTimer::singleShot(10000, q, &TorrentClient::connectToPeers);
    }
}

TorrentClient::TorrentClient(QObject *parent)
    : QObject(parent), d(new TorrentClientPrivate(this))
{
    // Connect the file manager
    connect(&d->fileManager, &FileManager::dataRead,
            this, &TorrentClient::sendToPeer);
    connect(&d->fileManager, &FileManager::verificationProgress,
            this, &TorrentClient::updateProgress);
    connect(&d->fileManager, &FileManager::verificationDone,
            this, &TorrentClient::fullVerificationDone);
    connect(&d->fileManager, &FileManager::pieceVerified,
            this, &TorrentClient::pieceVerified);
    connect(&d->fileManager, &FileManager::error,
            this, &TorrentClient::handleFileError);

    // Connect the tracker client
    connect(&d->trackerClient, &TrackerClient::peerListUpdated,
            this, &TorrentClient::addToPeerList);
    connect(&d->trackerClient, &TrackerClient::stopped,
            this, &TorrentClient::stopped);
}

TorrentClient::~TorrentClient()
{
    auto rateController = RateController::instance();
    const auto childSockets = findChildren<PeerWireClient *>(Qt::FindDirectChildrenOnly);
    for (PeerWireClient *socket : childSockets)
        rateController->removeSocket(socket);
    qDeleteAll(d->peers);
    qDeleteAll(d->pendingPieces);
    delete d;
}

bool TorrentClient::setTorrent(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly) || !setTorrent(file.readAll())) {
        d->setError(TorrentParseError);
        return false;
    }
    return true;
}

bool TorrentClient::setTorrent(const QByteArray &torrentData)
{
    if (!d->metaInfo.parse(torrentData)) {
        d->setError(TorrentParseError);
        return false;
    }

    // Calculate SHA1 hash of the "info" section in the torrent
    QByteArray infoValue = d->metaInfo.infoValue();
    d->infoHash = QCryptographicHash::hash(infoValue, QCryptographicHash::Sha1);

    return true;
}

MetaInfo TorrentClient::metaInfo() const
{
    return d->metaInfo;
}

void TorrentClient::setDestinationFolder(const QString &directory)
{
    d->destinationFolder = directory;
}

QString TorrentClient::destinationFolder() const
{
    return d->destinationFolder;
}

void TorrentClient::setDumpedState(const QByteArray &dumpedState)
{
    // Recover partially completed pieces
    QDataStream stream(dumpedState);

    quint16 version = 0;
    stream >> version;
    if (version != 2)
        return;

    stream >> d->completedPieces;

    while (!stream.atEnd()) {
        qint32 index;
        qint32 length;
        QBitArray completed;
        stream >> index >> length >> completed;
        if (stream.status() != QDataStream::Ok) {
            d->completedPieces.clear();
            break;
        }

        TorrentPiece *piece = new TorrentPiece;
        piece->index = index;
        piece->length = length;
        piece->completedBlocks = completed;
        piece->requestedBlocks.resize(completed.size());
        piece->inProgress = false;
        d->pendingPieces[index] = piece;
    }
}

QByteArray TorrentClient::dumpedState() const
{
    QByteArray partials;
    QDataStream stream(&partials, QIODevice::WriteOnly);

    stream << quint16(2);
    stream << d->completedPieces;

    // Save the state of all partially downloaded pieces into a format
    // suitable for storing in settings.
    auto it = d->pendingPieces.constBegin();
    while (it != d->pendingPieces.constEnd()) {
        TorrentPiece *piece = it.value();
        if (blocksLeftForPiece(piece) > 0 && blocksLeftForPiece(piece) < piece->completedBlocks.size()) {
            stream << piece->index;
            stream << piece->length;
            stream << piece->completedBlocks;
        }
        ++it;
    }

    return partials;
}

qint64 TorrentClient::progress() const
{
    return d->lastProgressValue;
}

void TorrentClient::setDownloadedBytes(qint64 bytes)
{
    d->downloadedBytes = bytes;
}

qint64 TorrentClient::downloadedBytes() const
{
    return d->downloadedBytes;
}

void TorrentClient::setUploadedBytes(qint64 bytes)
{
    d->uploadedBytes = bytes;
}

qint64 TorrentClient::uploadedBytes() const
{
    return d->uploadedBytes;
}

int TorrentClient::connectedPeerCount() const
{
    int tmp = 0;
    for (PeerWireClient *client : d->connections) {
        if (client->state() == QAbstractSocket::ConnectedState)
            ++tmp;
    }
    return tmp;
}

int TorrentClient::seedCount() const
{
    int tmp = 0;
    for (PeerWireClient *client : d->connections) {
        if (client->availablePieces().count(true) == d->pieceCount)
            ++tmp;
    }
    return tmp;
}

TorrentClient::State TorrentClient::state() const
{
    return d->state;
}

QString TorrentClient::stateString() const
{
    return d->stateString;
}

TorrentClient::Error TorrentClient::error() const
{
    return d->error;
}

QString TorrentClient::errorString() const
{
    return d->errorString;
}

QByteArray TorrentClient::peerId() const
{
    return d->peerId;
}

QByteArray TorrentClient::infoHash() const
{
    return d->infoHash;
}

void TorrentClient::start()
{
    if (d->state != Idle)
        return;

    TorrentServer::instance()->addClient(this);

    // Initialize the file manager
    d->setState(Preparing);
    d->fileManager.setMetaInfo(d->metaInfo);
    d->fileManager.setDestinationFolder(d->destinationFolder);
    d->fileManager.setCompletedPieces(d->completedPieces);
    d->fileManager.start(QThread::LowestPriority);
    d->fileManager.startDataVerification();
}

void TorrentClient::stop()
{
    if (d->state == Stopping)
        return;

    TorrentServer::instance()->removeClient(this);

    // Update the state
    State oldState = d->state;
    d->setState(Stopping);

    // Stop the timer
    if (d->transferRateTimer) {
        killTimer(d->transferRateTimer);
        d->transferRateTimer = 0;
    }

    // Abort all existing connections
    for (PeerWireClient *client : std::as_const(d->connections)) {
        RateController::instance()->removeSocket(client);
        ConnectionManager::instance()->removeConnection(client);
        client->abort();
    }
    d->connections.clear();

    // Perhaps stop the tracker
    if (oldState > Preparing) {
        d->trackerClient.stop();
    } else {
        d->setState(Idle);
        emit stopped();
    }
}

void TorrentClient::setPaused(bool paused)
{
    if (paused) {
        // Abort all connections, and set the max number of
        // connections to 0. Keep the list of peers, so we can quickly
        // resume later.
        d->setState(Paused);
        for (PeerWireClient *client : std::as_const(d->connections))
            client->abort();
        d->connections.clear();
        TorrentServer::instance()->removeClient(this);
    } else {
        // Restore the max number of connections, and start the peer
        // connector. We should also quickly start receiving incoming
        // connections.
        d->setState(d->completedPieces.count(true) == d->fileManager.pieceCount()
                    ? Seeding : Searching);
        connectToPeers();
        TorrentServer::instance()->addClient(this);
    }
}

void TorrentClient::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == d->uploadScheduleTimer) {
        // Update the state of who's choked and who's not
        scheduleUploads();
        return;
    }

    if (event->timerId() != d->transferRateTimer) {
        QObject::timerEvent(event);
        return;
    }

    // Calculate average upload/download rate
    qint64 uploadBytesPerSecond = 0;
    qint64 downloadBytesPerSecond = 0;
    for (int i = 0; i < RateControlWindowLength; ++i) {
        uploadBytesPerSecond += d->uploadRate[i];
        downloadBytesPerSecond += d->downloadRate[i];
    }
    uploadBytesPerSecond /= qint64(RateControlWindowLength);
    downloadBytesPerSecond /= qint64(RateControlWindowLength);
    for (int i = RateControlWindowLength - 2; i >= 0; --i) {
        d->uploadRate[i + 1] = d->uploadRate[i];
        d->downloadRate[i + 1] = d->downloadRate[i];
    }
    d->uploadRate[0] = 0;
    d->downloadRate[0] = 0;
    emit uploadRateUpdated(int(uploadBytesPerSecond));
    emit downloadRateUpdated(int(downloadBytesPerSecond));

    // Stop the timer if there is no activity.
    if (downloadBytesPerSecond == 0 && uploadBytesPerSecond == 0) {
        killTimer(d->transferRateTimer);
        d->transferRateTimer = 0;
    }
}

void TorrentClient::sendToPeer(qint32 readId, qint32 pieceIndex, qint32 begin,
                               const QByteArray &data)
{
    // Send the requested block to the peer if the client connection
    // still exists; otherwise do nothing. This slot is called by the
    // file manager after it has read a block of data.
    PeerWireClient *client = d->readIds.value(readId);
    if (client) {
        if ((client->peerWireState() & PeerWireClient::ChokingPeer) == 0)
            client->sendBlock(pieceIndex, begin, data);
    }
    d->readIds.remove(readId);
}

void TorrentClient::fullVerificationDone()
{
    // Update our list of completed and incomplete pieces.
    d->completedPieces = d->fileManager.completedPieces();
    d->incompletePieces.resize(d->completedPieces.size());
    d->pieceCount = d->completedPieces.size();
    for (qint32 i = 0; i < d->fileManager.pieceCount(); ++i) {
        if (!d->completedPieces.testBit(i))
            d->incompletePieces.setBit(i);
    }

    updateProgress();

    // If the checksums show that what the dumped state thought was
    // partial was in fact complete, then we trust the checksums.
    auto it = d->pendingPieces.begin();
    while (it != d->pendingPieces.end()) {
        if (d->completedPieces.testBit(it.key()))
            it = d->pendingPieces.erase(it);
        else
            ++it;
    }

    d->uploadScheduleTimer = startTimer(UploadScheduleInterval);

    // Start the server
    TorrentServer *server = TorrentServer::instance();
    if (!server->isListening()) {
        // Set up the peer wire server
        for (int i = ServerMinPort; i <= ServerMaxPort; ++i) {
            if (server->listen(QHostAddress::Any, i))
                break;
        }
        if (!server->isListening()) {
            d->setError(ServerError);
            return;
        }
    }

    d->setState(d->completedPieces.count(true) == d->pieceCount ? Seeding : Searching);

    // Start the tracker client
    d->trackerClient.start(d->metaInfo);
}

void TorrentClient::pieceVerified(qint32 pieceIndex, bool ok)
{
    TorrentPiece *piece = d->pendingPieces.value(pieceIndex);

    // Remove this piece from all payloads
    QMultiMap<PeerWireClient *, TorrentPiece *>::Iterator it = d->payloads.begin();
    while (it != d->payloads.end()) {
        if (it.value()->index == pieceIndex)
            it = d->payloads.erase(it);
        else
            ++it;
    }

    if (!ok) {
        // If a piece did not pass the SHA1 check, we'll simply clear
        // its state, and the scheduler will re-request it
        piece->inProgress = false;
        piece->completedBlocks.fill(false);
        piece->requestedBlocks.fill(false);
        d->callScheduler();
        return;
    }

    // Update the peer list so we know who's still interesting.
    for (TorrentPeer *peer : std::as_const(d->peers)) {
        if (!peer->interesting)
            continue;
        bool interesting = false;
        for (qint32 i = 0; i < d->pieceCount; ++i) {
            if (peer->pieces.testBit(i) && d->incompletePieces.testBit(i)) {
                interesting = true;
                break;
            }
        }
        peer->interesting = interesting;
    }

    // Delete the piece and update our structures.
    delete piece;
    d->pendingPieces.remove(pieceIndex);
    d->completedPieces.setBit(pieceIndex);
    d->incompletePieces.clearBit(pieceIndex);

    // Notify connected peers.
    for (PeerWireClient *client : std::as_const(d->connections)) {
        if (client->state() == QAbstractSocket::ConnectedState
            && !client->availablePieces().testBit(pieceIndex)) {
            client->sendPieceNotification(pieceIndex);
        }
    }

    // Notify the tracker if we've entered Seeding status; otherwise
    // call the scheduler.
    int completed = d->completedPieces.count(true);
    if (completed == d->pieceCount) {
        if (d->state != Seeding) {
            d->setState(Seeding);
            d->trackerClient.startSeeding();
        }
    } else {
        if (completed == 1)
            d->setState(Downloading);
        else if (d->incompletePieces.count(true) < 5 && d->pendingPieces.size() > d->incompletePieces.count(true))
            d->setState(Endgame);
        d->callScheduler();
    }

    updateProgress();
}

void TorrentClient::handleFileError()
{
    if (d->state == Paused)
        return;
    setPaused(true);
    emit error(FileError);
}

void TorrentClient::connectToPeers()
{
    d->connectingToClients = false;

    if (d->state == Stopping || d->state == Idle || d->state == Paused)
        return;

    if (d->state == Searching)
        d->setState(Connecting);

    // Find the list of peers we are not currently connected to, where
    // the more interesting peers are listed more than once.
    QList<TorrentPeer *> weighedPeers = weighedFreePeers();

    // Start as many connections as we can
    while (!weighedPeers.isEmpty() && ConnectionManager::instance()->canAddConnection()
           && (QRandomGenerator::global()->bounded(ConnectionManager::instance()->maxConnections() / 2))) {
        PeerWireClient *client = new PeerWireClient(ConnectionManager::instance()->clientId(), this);
        RateController::instance()->addSocket(client);
        ConnectionManager::instance()->addConnection(client);

        initializeConnection(client);
        d->connections << client;

        // Pick a random peer from the list of weighed peers.
        TorrentPeer *peer = weighedPeers.takeAt(QRandomGenerator::global()->bounded(weighedPeers.size()));
        weighedPeers.removeAll(peer);
        peer->connectStart = QDateTime::currentSecsSinceEpoch();
        peer->lastVisited = peer->connectStart;

        // Connect to the peer.
        client->setPeer(peer);
        client->connectToHost(peer->address, peer->port);
    }
}

QList<TorrentPeer *> TorrentClient::weighedFreePeers() const
{
    QList<TorrentPeer *> weighedPeers;

    // Generate a list of peers that we want to connect to.
    qint64 now = QDateTime::currentSecsSinceEpoch();
    QList<TorrentPeer *> freePeers;
    QMap<QString, int> connectionsPerPeer;
    for (TorrentPeer *peer : std::as_const(d->peers)) {
        bool busy = false;
        for (PeerWireClient *client : std::as_const(d->connections)) {
            if (client->state() == PeerWireClient::ConnectedState
                && client->peerAddress() == peer->address
                && client->peerPort() == peer->port) {
                if (++connectionsPerPeer[peer->address.toString()] >= MaxConnectionPerPeer) {
                    busy = true;
                    break;
                }
            }
        }
        if (!busy && (now - peer->lastVisited) > uint(MinimumTimeBeforeRevisit))
            freePeers << peer;
    }

    // Nothing to connect to
    if (freePeers.isEmpty())
        return weighedPeers;

    // Assign points based on connection speed and pieces available.
    QList<QPair<int, TorrentPeer *> > points;
    for (TorrentPeer *peer : std::as_const(freePeers)) {
        int tmp = 0;
        if (peer->interesting) {
            tmp += peer->numCompletedPieces;
            if (d->state == Seeding)
                tmp = d->pieceCount - tmp;
            if (!peer->connectStart) // An unknown peer is as interesting as a seed
                tmp += d->pieceCount;

            // 1/5 of the total score for each second below 5 it takes to
            // connect.
            if (peer->connectTime < 5)
                tmp += (d->pieceCount / 10) * (5 - peer->connectTime);
        }
        points << QPair<int, TorrentPeer *>(tmp, peer);
    }
    std::sort(points.begin(), points.end());

    // Minimize the list so the point difference is never more than 1.
    typedef QPair<int,TorrentPeer*> PointPair;
    QMultiMap<int, TorrentPeer *> pointMap;
    int lowestScore = 0;
    int lastIndex = 0;
    for (const PointPair &point : std::as_const(points)) {
        if (point.first > lowestScore) {
            lowestScore = point.first;
            ++lastIndex;
        }
        pointMap.insert(lastIndex, point.second);
    }

    // Now make up a list of peers where the ones with more points are
    // listed many times.
    QMultiMap<int, TorrentPeer *>::ConstIterator it = pointMap.constBegin();
    while (it != pointMap.constEnd()) {
        for (int i = 0; i < it.key() + 1; ++i)
            weighedPeers << it.value();
        ++it;
    }

    return weighedPeers;
}

void TorrentClient::setupIncomingConnection(PeerWireClient *client)
{
    // Connect signals
    initializeConnection(client);

    // Initialize this client
    RateController::instance()->addSocket(client);
    d->connections << client;

    client->initialize(d->infoHash, d->pieceCount);
    client->sendPieceList(d->completedPieces);

    emit peerInfoUpdated();

    if (d->state == Searching || d->state == Connecting) {
        int completed = d->completedPieces.count(true);
        if (completed == 0)
            d->setState(WarmingUp);
        else if (d->incompletePieces.count(true) < 5 && d->pendingPieces.size() > d->incompletePieces.count(true))
            d->setState(Endgame);
    }

    if (d->connections.isEmpty())
        scheduleUploads();
}

void TorrentClient::setupOutgoingConnection()
{
    PeerWireClient *client = qobject_cast<PeerWireClient *>(sender());

    // Update connection statistics.
    for (TorrentPeer *peer : std::as_const(d->peers)) {
        if (peer->port == client->peerPort() && peer->address == client->peerAddress()) {
            peer->connectTime = peer->lastVisited - peer->connectStart;
            break;
        }
    }

    // Send handshake and piece list
    client->initialize(d->infoHash, d->pieceCount);
    client->sendPieceList(d->completedPieces);

    emit peerInfoUpdated();

    if (d->state == Searching || d->state == Connecting) {
        int completed = d->completedPieces.count(true);
        if (completed == 0)
            d->setState(WarmingUp);
        else if (d->incompletePieces.count(true) < 5 && d->pendingPieces.size() > d->incompletePieces.count(true))
            d->setState(Endgame);
    }
}

void TorrentClient::initializeConnection(PeerWireClient *client)
{
    connect(client, &PeerWireClient::connected,
            this, &TorrentClient::setupOutgoingConnection);
    connect(client, &PeerWireClient::disconnected,
            this, &TorrentClient::removeClient);
    connect(client, &PeerWireClient::errorOccurred,
            this, &TorrentClient::removeClient);
    connect(client, &PeerWireClient::piecesAvailable,
            this, &TorrentClient::peerPiecesAvailable);
    connect(client, &PeerWireClient::blockRequested,
            this, &TorrentClient::peerRequestsBlock);
    connect(client, &PeerWireClient::blockReceived,
            this, &TorrentClient::blockReceived);
    connect(client, &PeerWireClient::choked,
            this, &TorrentClient::peerChoked);
    connect(client, &PeerWireClient::unchoked,
            this, &TorrentClient::peerUnchoked);
    connect(client, &PeerWireClient::bytesWritten,
            this, &TorrentClient::peerWireBytesWritten);
    connect(client, &PeerWireClient::bytesReceived,
            this, &TorrentClient::peerWireBytesReceived);
}

void TorrentClient::removeClient()
{
    PeerWireClient *client = static_cast<PeerWireClient *>(sender());

    // Remove the host from our list of known peers if the connection
    // failed.
    if (client->peer() && client->error() == QAbstractSocket::ConnectionRefusedError)
        d->peers.removeAll(client->peer());

    // Remove the client from RateController and all structures.
    RateController::instance()->removeSocket(client);
    d->connections.removeAll(client);
    for (auto it = d->payloads.find(client); it != d->payloads.end() && it.key() == client; /*erasing*/) {
        TorrentPiece *piece = it.value();
        piece->inProgress = false;
        piece->requestedBlocks.fill(false);
        it = d->payloads.erase(it);
    }

    // Remove pending read requests.
    for (auto it = d->readIds.begin(), end = d->readIds.end(); it != end; /*erasing*/) {
        if (it.value() == client)
            it = d->readIds.erase(it);
        else
            ++it;
    }

    // Delete the client later.
    disconnect(client, &PeerWireClient::disconnected,
               this, &TorrentClient::removeClient);
    client->deleteLater();
    ConnectionManager::instance()->removeConnection(client);

    emit peerInfoUpdated();
    d->callPeerConnector();
}

void TorrentClient::peerPiecesAvailable(const QBitArray &pieces)
{
    PeerWireClient *client = qobject_cast<PeerWireClient *>(sender());

    // Find the peer in our list of announced peers. If it's there,
    // then we can use the piece list into to gather statistics that
    // help us decide what peers to connect to.
    TorrentPeer *peer = nullptr;
    QList<TorrentPeer *>::Iterator it = d->peers.begin();
    while (it != d->peers.end()) {
        if ((*it)->address == client->peerAddress() && (*it)->port == client->peerPort()) {
            peer = *it;
            break;
        }
        ++it;
    }

    // If the peer is a seed, and we are in seeding mode, then the
    // peer is uninteresting.
    if (pieces.count(true) == d->pieceCount) {
        if (peer)
            peer->seed = true;
        emit peerInfoUpdated();
        if (d->state == Seeding) {
            client->abort();
            return;
        } else {
            if (peer)
                peer->interesting = true;
            if ((client->peerWireState() & PeerWireClient::InterestedInPeer) == 0)
                client->sendInterested();
            d->callScheduler();
            return;
        }
    }

    // Update our list of available pieces.
    if (peer) {
        peer->pieces = pieces;
        peer->numCompletedPieces = pieces.count(true);
    }

    // Check for interesting pieces, and tell the peer whether we are
    // interested or not.
    bool interested = false;
    qint32 piecesSize = pieces.size();
    for (qint32 pieceIndex = 0; pieceIndex < piecesSize; ++pieceIndex) {
        if (!pieces.testBit(pieceIndex))
            continue;
        if (!d->completedPieces.testBit(pieceIndex)) {
            interested = true;
            if ((client->peerWireState() & PeerWireClient::InterestedInPeer) == 0) {
                if (peer)
                    peer->interesting = true;
                client->sendInterested();
            }

            QMultiMap<PeerWireClient *, TorrentPiece *>::Iterator it = d->payloads.find(client);
            qint32 inProgress = 0;
            while (it != d->payloads.end() && it.key() == client) {
                if (it.value()->inProgress)
                    inProgress += it.value()->requestedBlocks.count(true);
                ++it;
            }
            if (!inProgress)
                d->callScheduler();
            break;
        }
    }
    if (!interested && (client->peerWireState() & PeerWireClient::InterestedInPeer)) {
        if (peer)
            peer->interesting = false;
        client->sendNotInterested();
    }
}

void TorrentClient::peerRequestsBlock(qint32 pieceIndex, qint32 begin, qint32 length)
{
    PeerWireClient *client = qobject_cast<PeerWireClient *>(sender());

    // Silently ignore requests from choked peers
    if (client->peerWireState() & PeerWireClient::ChokingPeer)
        return;

    // Silently ignore requests for pieces we don't have.
    if (!d->completedPieces.testBit(pieceIndex))
        return;

    // Request the block from the file manager
    d->readIds.insert(d->fileManager.read(pieceIndex, begin, length),
                      qobject_cast<PeerWireClient *>(sender()));
}

void TorrentClient::blockReceived(qint32 pieceIndex, qint32 begin, const QByteArray &data)
{
    PeerWireClient *client = qobject_cast<PeerWireClient *>(sender());
    if (data.size() == 0) {
        client->abort();
        return;
    }

    // Ignore it if we already have this block.
    qint32 blockBit = begin / BlockSize;
    TorrentPiece *piece = d->pendingPieces.value(pieceIndex);
    if (!piece || piece->completedBlocks.testBit(blockBit)) {
        // Discard blocks that we already have, and fill up the pipeline.
        requestMore(client);
        return;
    }

    // If we are in warmup or endgame mode, cancel all duplicate
    // requests for this block.
    if (d->state == WarmingUp || d->state == Endgame) {
        QMultiMap<PeerWireClient *, TorrentPiece *>::Iterator it = d->payloads.begin();
        while (it != d->payloads.end()) {
            PeerWireClient *otherClient = it.key();
            if (otherClient != client && it.value()->index == pieceIndex) {
                if (otherClient->incomingBlocks().contains(TorrentBlock(pieceIndex, begin, data.size())))
                    it.key()->cancelRequest(pieceIndex, begin, data.size());
            }
            ++it;
        }
    }

    if (d->state != Downloading && d->state != Endgame && d->completedPieces.count(true) > 0)
        d->setState(Downloading);

    // Store this block
    d->fileManager.write(pieceIndex, begin, data);
    piece->completedBlocks.setBit(blockBit);
    piece->requestedBlocks.clearBit(blockBit);

    if (blocksLeftForPiece(piece) == 0) {
        // Ask the file manager to verify the newly downloaded piece
        d->fileManager.verifyPiece(piece->index);

        // Remove this piece from all payloads
        QMultiMap<PeerWireClient *, TorrentPiece *>::Iterator it = d->payloads.begin();
        while (it != d->payloads.end()) {
            if (!it.value() || it.value()->index == piece->index)
                it = d->payloads.erase(it);
            else
                ++it;
        }
    }

    // Fill up the pipeline.
    requestMore(client);
}

void TorrentClient::peerWireBytesWritten(qint64 size)
{
    if (!d->transferRateTimer)
        d->transferRateTimer = startTimer(RateControlTimerDelay);

    d->uploadRate[0] += size;
    d->uploadedBytes += size;
    emit dataSent(size);
}

void TorrentClient::peerWireBytesReceived(qint64 size)
{
    if (!d->transferRateTimer)
        d->transferRateTimer = startTimer(RateControlTimerDelay);

    d->downloadRate[0] += size;
    d->downloadedBytes += size;
    emit dataSent(size);
}

qint32 TorrentClient::blocksLeftForPiece(const TorrentPiece *piece) const
{
    return piece->completedBlocks.count(false);
}

void TorrentClient::scheduleUploads()
{
    // Generate a list of clients sorted by their transfer
    // speeds.  When leeching, we sort by download speed, and when
    // seeding, we sort by upload speed. Seeds are left out; there's
    // no use in unchoking them.
    QList<PeerWireClient *> allClients = d->connections;
    QList<QPair<qint64, PeerWireClient *>> transferSpeeds;
    for (PeerWireClient *client : std::as_const(allClients)) {
        if (client->state() == QAbstractSocket::ConnectedState
            && client->availablePieces().count(true) != d->pieceCount) {
            if (d->state == Seeding) {
                transferSpeeds.push_back({client->uploadSpeed(), client});
            } else {
                transferSpeeds.push_back({client->downloadSpeed(), client});
            }
        }
    }

    std::sort(transferSpeeds.begin(), transferSpeeds.end());

    // Unchoke the top 'MaxUploads' downloaders (peers that we are
    // uploading to) and choke all others.
    int maxUploaders = MaxUploads;
    for (auto rit = transferSpeeds.crbegin(), rend = transferSpeeds.crend(); rit != rend; ++rit) {
        PeerWireClient *client = rit->second;
        bool interested = (client->peerWireState() & PeerWireClient::PeerIsInterested);

        if (maxUploaders) {
            allClients.removeAll(client);
            if (client->peerWireState() & PeerWireClient::ChokingPeer)
                client->unchokePeer();
            --maxUploaders;
            continue;
        }

        if ((client->peerWireState() & PeerWireClient::ChokingPeer) == 0) {
            if ((QRandomGenerator::global()->bounded(10)) == 0)
                client->abort();
            else
                client->chokePeer();
            allClients.removeAll(client);
        }
        if (!interested)
            allClients.removeAll(client);
    }

    // Only interested peers are left in allClients. Unchoke one
    // random peer to allow it to compete for a position among the
    // downloaders.  (This is known as an "optimistic unchoke".)
    if (!allClients.isEmpty()) {
        PeerWireClient *client = allClients[QRandomGenerator::global()->bounded(allClients.size())];
        if (client->peerWireState() & PeerWireClient::ChokingPeer)
            client->unchokePeer();
    }
}

void TorrentClient::scheduleDownloads()
{
    d->schedulerCalled = false;

    if (d->state == Stopping || d->state == Paused || d->state == Idle)
        return;

    // Check what each client is doing, and assign payloads to those
    // who are either idle or done.
    for (PeerWireClient *client : std::as_const(d->connections))
        schedulePieceForClient(client);
}

void TorrentClient::schedulePieceForClient(PeerWireClient *client)
{
    // Only schedule connected clients.
    if (client->state() != QTcpSocket::ConnectedState)
        return;

    // The peer has choked us; try again later.
    if (client->peerWireState() & PeerWireClient::ChokedByPeer)
        return;

    // Make a list of all the client's pending pieces, and count how
    // many blocks have been requested.
    QList<qint32> currentPieces;
    bool somePiecesAreNotInProgress = false;
    TorrentPiece *lastPendingPiece = nullptr;
    QMultiMap<PeerWireClient *, TorrentPiece *>::Iterator it = d->payloads.find(client);
    while (it != d->payloads.end() && it.key() == client) {
        lastPendingPiece = it.value();
        if (lastPendingPiece->inProgress) {
            currentPieces << lastPendingPiece->index;
        } else {
            somePiecesAreNotInProgress = true;
        }
        ++it;
    }

    // Skip clients that already have too many blocks in progress.
    if (client->incomingBlocks().size() >= ((d->state == Endgame || d->state == WarmingUp)
                                            ? MaxBlocksInMultiMode : MaxBlocksInProgress))
        return;

    // If all pieces are in progress, but we haven't filled up our
    // block requesting quota, then we need to schedule another piece.
    if (!somePiecesAreNotInProgress || client->incomingBlocks().size() > 0)
        lastPendingPiece = nullptr;
    TorrentPiece *piece = lastPendingPiece;

    // In warmup state, all clients request blocks from the same pieces.
    if (d->state == WarmingUp && d->pendingPieces.size() >= 4) {
        piece = d->payloads.value(client);
        if (!piece) {
            QList<TorrentPiece *> values = d->pendingPieces.values();
            piece = values.value(QRandomGenerator::global()->bounded(values.size()));
            piece->inProgress = true;
            d->payloads.insert(client, piece);
        }
        if (piece->completedBlocks.count(false) == client->incomingBlocks().size())
            return;
    }

    // If no pieces are currently in progress, schedule a new one.
    if (!piece) {
        // Build up a list of what pieces that we have not completed
        // are available to this client.
        QBitArray incompletePiecesAvailableToClient = d->incompletePieces;

        // Remove all pieces that are marked as being in progress
        // already (i.e., pieces that this or other clients are
        // already waiting for). A special rule applies to warmup and
        // endgame mode; there, we allow several clients to request
        // the same piece. In endgame mode, this only applies to
        // clients that are currently uploading (more than 1.0KB/s).
        if ((d->state == Endgame && client->uploadSpeed() < 1024) || d->state != WarmingUp) {
            auto it = d->pendingPieces.constBegin();
            while (it != d->pendingPieces.constEnd()) {
                if (it.value()->inProgress)
                    incompletePiecesAvailableToClient.clearBit(it.key());
                ++it;
            }
        }

        // Remove all pieces that the client cannot download.
        incompletePiecesAvailableToClient &= client->availablePieces();

        // Remove all pieces that this client has already requested.
        for (qint32 i : std::as_const(currentPieces))
            incompletePiecesAvailableToClient.clearBit(i);

        // Only continue if more pieces can be scheduled. If no pieces
        // are available and no blocks are in progress, just leave
        // the connection idle; it might become interesting later.
        if (incompletePiecesAvailableToClient.count(true) == 0)
            return;

        // Check if any of the partially completed pieces can be
        // recovered, and if so, pick a random one of them.
        QList<TorrentPiece *> partialPieces;
        auto it = d->pendingPieces.constBegin();
        while (it != d->pendingPieces.constEnd()) {
            TorrentPiece *tmp = it.value();
            if (incompletePiecesAvailableToClient.testBit(it.key())) {
                if (!tmp->inProgress || d->state == WarmingUp || d->state == Endgame) {
                    partialPieces << tmp;
                    break;
                }
            }
            ++it;
        }
        if (!partialPieces.isEmpty())
            piece = partialPieces.value(QRandomGenerator::global()->bounded(partialPieces.size()));

        if (!piece) {
            // Pick a random piece 3 out of 4 times; otherwise, pick either
            // one of the most common or the least common pieces available,
            // depending on the state we're in.
            int pieceIndex = 0;
            if (d->state == WarmingUp || (QRandomGenerator::global()->generate() & 4) == 0) {
                int *occurrences = new int[d->pieceCount];
                memset(occurrences, 0, d->pieceCount * sizeof(int));

                // Count how many of each piece are available.
                for (PeerWireClient *peer : std::as_const(d->connections)) {
                    QBitArray peerPieces = peer->availablePieces();
                    int peerPiecesSize = peerPieces.size();
                    for (int i = 0; i < peerPiecesSize; ++i) {
                        if (peerPieces.testBit(i))
                            ++occurrences[i];
                    }
                }

                // Find the rarest or most common pieces.
                int numOccurrences = d->state == WarmingUp ? 0 : 99999;
                QList<int> piecesReadyForDownload;
                for (int i = 0; i < d->pieceCount; ++i) {
                    if (d->state == WarmingUp) {
                        // Add common pieces
                        if (occurrences[i] >= numOccurrences
                            && incompletePiecesAvailableToClient.testBit(i)) {
                            if (occurrences[i] > numOccurrences)
                                piecesReadyForDownload.clear();
                            piecesReadyForDownload.append(i);
                            numOccurrences = occurrences[i];
                        }
                    } else {
                        // Add rare pieces
                        if (occurrences[i] <= numOccurrences
                            && incompletePiecesAvailableToClient.testBit(i)) {
                            if (occurrences[i] < numOccurrences)
                                piecesReadyForDownload.clear();
                            piecesReadyForDownload.append(i);
                            numOccurrences = occurrences[i];
                        }
                    }
                }

                // Select one piece randomly
                pieceIndex = piecesReadyForDownload.at(QRandomGenerator::global()->bounded(piecesReadyForDownload.size()));
                delete [] occurrences;
            } else {
                // Make up a list of available piece indices, and pick
                // a random one.
                QList<int> values;
                int incompletePiecesAvailableToClientSize = incompletePiecesAvailableToClient.size();
                for (int i = 0; i < incompletePiecesAvailableToClientSize; ++i) {
                    if (incompletePiecesAvailableToClient.testBit(i))
                        values << i;
                }
                pieceIndex = values.at(QRandomGenerator::global()->bounded(values.size()));
            }

            // Create a new TorrentPiece and fill in all initial
            // properties.
            piece = new TorrentPiece;
            piece->index = pieceIndex;
            piece->length = d->fileManager.pieceLengthAt(pieceIndex);
            int numBlocks = piece->length / BlockSize;
            if (piece->length % BlockSize)
                ++numBlocks;
            piece->completedBlocks.resize(numBlocks);
            piece->requestedBlocks.resize(numBlocks);
            d->pendingPieces.insert(pieceIndex, piece);
        }

        piece->inProgress = true;
        d->payloads.insert(client, piece);
    }

    // Request more blocks from all pending pieces.
    requestMore(client);
}

void TorrentClient::requestMore(PeerWireClient *client)
{
    // Make a list of all pieces this client is currently waiting for,
    // and count the number of blocks in progress.
    QMultiMap<PeerWireClient *, TorrentPiece *>::Iterator it = d->payloads.find(client);
    int numBlocksInProgress = client->incomingBlocks().size();
    QList<TorrentPiece *> piecesInProgress;
    while (it != d->payloads.end() && it.key() == client) {
        TorrentPiece *piece = it.value();
        if (piece->inProgress || (d->state == WarmingUp || d->state == Endgame))
            piecesInProgress << piece;
        ++it;
    }

    // If no pieces are in progress, call the scheduler.
    if (piecesInProgress.isEmpty() && d->incompletePieces.count(true)) {
        d->callScheduler();
        return;
    }

    // If too many pieces are in progress, there's nothing to do.
    int maxInProgress = ((d->state == Endgame || d->state == WarmingUp)
                         ? MaxBlocksInMultiMode : MaxBlocksInProgress);
    if (numBlocksInProgress == maxInProgress)
        return;

    // Starting with the first piece that we're waiting for, request
    // blocks until the quota is filled up.
    for (TorrentPiece *piece : std::as_const(piecesInProgress)) {
        numBlocksInProgress += requestBlocks(client, piece, maxInProgress - numBlocksInProgress);
        if (numBlocksInProgress == maxInProgress)
            break;
    }

    // If we still didn't fill up the quota, we need to schedule more
    // pieces.
    if (numBlocksInProgress < maxInProgress && d->state != WarmingUp)
        d->callScheduler();
}

int TorrentClient::requestBlocks(PeerWireClient *client, TorrentPiece *piece, int maxBlocks)
{
    // Generate the list of incomplete blocks for this piece.
    QList<int> bits;
    int completedBlocksSize = piece->completedBlocks.size();
    for (int i = 0; i < completedBlocksSize; ++i) {
        if (!piece->completedBlocks.testBit(i) && !piece->requestedBlocks.testBit(i))
            bits << i;
    }

    // Nothing more to request.
    if (bits.size() == 0) {
        if (d->state != WarmingUp && d->state != Endgame)
            return 0;
        bits.clear();
        for (int i = 0; i < completedBlocksSize; ++i) {
            if (!piece->completedBlocks.testBit(i))
                bits << i;
        }
    }

    if (d->state == WarmingUp || d->state == Endgame) {
        // By randomizing the list of blocks to request, we
        // significantly speed up the warmup and endgame modes, where
        // the same blocks are requested from multiple peers. The
        // speedup comes from an increased chance of receiving
        // different blocks from the different peers.
        for (int i = 0; i < bits.size(); ++i) {
            int a = QRandomGenerator::global()->bounded(bits.size());
            int b = QRandomGenerator::global()->bounded(bits.size());
            int tmp = bits[a];
            bits[a] = bits[b];
            bits[b] = tmp;
        }
    }

    // Request no more blocks than we've been asked to.
    int blocksToRequest = qMin(maxBlocks, bits.size());

    // Calculate the offset and size of each block, and send requests.
    for (int i = 0; i < blocksToRequest; ++i) {
        int blockSize = BlockSize;
        if ((piece->length % BlockSize) && bits.at(i) == completedBlocksSize - 1)
            blockSize = piece->length % BlockSize;
        client->requestBlock(piece->index, bits.at(i) * BlockSize, blockSize);
        piece->requestedBlocks.setBit(bits.at(i));
    }

    return blocksToRequest;
}

void TorrentClient::peerChoked()
{
    PeerWireClient *client = qobject_cast<PeerWireClient *>(sender());
    if (!client)
        return;

    // When the peer chokes us, we immediately forget about all blocks
    // we've requested from it. We also remove the piece from out
    // payload, making it available to other clients.
    QMultiMap<PeerWireClient *, TorrentPiece *>::Iterator it = d->payloads.find(client);
    while (it != d->payloads.end() && it.key() == client) {
        it.value()->inProgress = false;
        it.value()->requestedBlocks.fill(false);
        it = d->payloads.erase(it);
    }
}

void TorrentClient::peerUnchoked()
{
    PeerWireClient *client = qobject_cast<PeerWireClient *>(sender());
    if (!client)
        return;

    // We got unchoked, which means we can request more blocks.
    if (d->state != Seeding)
        d->callScheduler();
}

void TorrentClient::addToPeerList(const QList<TorrentPeer> &peerList)
{
    // Add peers we don't already know of to our list of peers.
    const QList<QHostAddress> addresses =  QNetworkInterface::allAddresses();
    for (const TorrentPeer &peer : peerList) {
        if (addresses.contains(peer.address)
            && peer.port == TorrentServer::instance()->serverPort()) {
            // Skip our own server.
            continue;
        }

        bool known = false;
        for (const TorrentPeer *knownPeer : std::as_const(d->peers)) {
            if (knownPeer->port == peer.port
                && knownPeer->address == peer.address) {
                known = true;
                break;
            }
        }
        if (!known) {
            TorrentPeer *newPeer = new TorrentPeer;
            *newPeer = peer;
            newPeer->interesting = true;
            newPeer->seed = false;
            newPeer->lastVisited = 0;
            newPeer->connectStart = 0;
            newPeer->connectTime = 999999;
            newPeer->pieces.resize(d->pieceCount);
            newPeer->numCompletedPieces = 0;
            d->peers << newPeer;
        }
    }

    // If we've got more peers than we can connect to, we remove some
    // of the peers that have no (or low) activity.
    int maxPeers = ConnectionManager::instance()->maxConnections() * 3;
    if (d->peers.size() > maxPeers) {
        auto tooMany = d->peers.size() - maxPeers;

        // Find what peers are currently connected & active
        const auto firstNInactivePeers = [&tooMany, this] (TorrentPeer *peer) {
            if (!tooMany)
                return false;
            for (const PeerWireClient *client : std::as_const(d->connections)) {
                if (client->peer() == peer && (client->downloadSpeed() + client->uploadSpeed()) > 1024)
                    return false;
            }
            --tooMany;
            return true;
        };
        // Remove inactive peers from the peer list until we're below
        // the max connections count.
        d->peers.removeIf(firstNInactivePeers);
        // If we still have too many peers, remove the oldest ones.
        d->peers.erase(d->peers.begin(), d->peers.begin() + tooMany);
    }

    if (d->state != Paused && d->state != Stopping && d->state != Idle) {
        if (d->state == Searching || d->state == WarmingUp)
            connectToPeers();
        else
            d->callPeerConnector();
    }
}

void TorrentClient::trackerStopped()
{
    d->setState(Idle);
    emit stopped();
}

void TorrentClient::updateProgress(int progress)
{
    if (progress == -1 && d->pieceCount > 0) {
        int newProgress = (d->completedPieces.count(true) * 100) / d->pieceCount;
        if (d->lastProgressValue != newProgress) {
            d->lastProgressValue = newProgress;
            emit progressUpdated(newProgress);
        }
    } else if (d->lastProgressValue != progress) {
        d->lastProgressValue = progress;
        emit progressUpdated(progress);
    }
}
