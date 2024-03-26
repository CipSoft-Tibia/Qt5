// Copyright (C) 2017-2016 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qconnection_qnx_global_p.h"
#include "qconnection_qnx_qiodevices_p.h"
#include "qconnection_qnx_server_p.h"

QT_BEGIN_NAMESPACE

QQnxNativeServer::QQnxNativeServer(QObject *parent)
    : QObject(*new QQnxNativeServerPrivate, parent)
{
}

QQnxNativeServer::~QQnxNativeServer()
{
}

void QQnxNativeServer::close()
{
    Q_D(QQnxNativeServer);
    d->teardownServer();
}

bool QQnxNativeServer::hasPendingConnections() const
{
    Q_D(const QQnxNativeServer);
    d->mutex.lock();
    const int len = d->pending.length();
    d->mutex.unlock();
    return len > 0;
}

bool QQnxNativeServer::isListening() const
{
    Q_D(const QQnxNativeServer);
    return !(d->serverName.isEmpty());
}

bool QQnxNativeServer::listen(const QString &name)
{
    Q_D(QQnxNativeServer);
    if (isListening()) {
        qCWarning(QT_REMOTEOBJECT) << "QQnxNativeServer::listen() called when already listening";
        return false;
    }

    if (name.isEmpty()) {
        d->error = QAbstractSocket::HostNotFoundError;
        QString function = QLatin1String("QQnxNativeServer::listen");
        d->errorString = tr("%1: Name error").arg(function);
        return false;
    }

    if (!d->listen(name)) {
        d->serverName.clear();
        return false;
    }

    d->serverName = name;
    return true;
}

QSharedPointer<QIOQnxSource> QQnxNativeServer::nextPendingConnection()
{
    Q_D(QQnxNativeServer);
    d->mutex.lock();
    Q_ASSERT(d->pending.length() > 0);
    auto io = d->pending.takeFirst();
    d->mutex.unlock();
    return io;
}

QString QQnxNativeServer::serverName() const
{
    Q_D(const QQnxNativeServer);
    return d->serverName;
}

bool QQnxNativeServer::waitForNewConnection(int msec, bool *timedOut)
{
    //TODO - This method isn't used by Qt Remote Objects, but would
    //need to be implemented before this class could be used as a
    //connection server (like QTcpServer or QLocalServer).
    Q_UNUSED(msec)
    Q_UNUSED(timedOut)
    Q_ASSERT(false);
    return false;
}

void QQnxNativeServer::onSourceClosed()
{
    Q_D(QQnxNativeServer);
    QIOQnxSource *conn = qobject_cast<QIOQnxSource *>(sender());
    Q_ASSERT(conn);

    d->cleanupIOSource(conn);
}

QQnxNativeServerPrivate::QQnxNativeServerPrivate()
    : error(QAbstractSocket::UnknownSocketError)
    , thread(this, QStringLiteral("NativeServer"))
{
}

QQnxNativeServerPrivate::~QQnxNativeServerPrivate()
{
    if (thread.isRunning())
        teardownServer();
}

// method (run in a thread) to watch for connections and handle receiving data
void QQnxNativeServerPrivate::thread_func()
{
    struct _msg_info msg_info;
    recv_msgs recv_buf;
    QList<iov_t> resp_repeat_iov(5);

    qCDebug(QT_REMOTEOBJECT) << "Server thread_func running";

    while (running.loadRelaxed()) {
        // wait for messages and pulses
        int rcvid = MsgReceive(attachStruct->chid, &recv_buf, sizeof(_pulse), &msg_info);
        qCDebug(QT_REMOTEOBJECT) << "MsgReceive unblocked.  Rcvid" << rcvid << " Scoid" << msg_info.scoid;
        if (rcvid == -1) {
            if (errno != ESRCH) // ESRCH means the channel closed
                WARNING(MsgReceive)
            break;
        }

        if (0 == rcvid) {
            qCDebug(QT_REMOTEOBJECT) << "Pulse" << recv_buf.pulse.code;
            /* we received a pulse
             */
            switch (recv_buf.pulse.code) {
            case _PULSE_CODE_DISCONNECT:
            {
                /* a client has disconnected.  Verify that it is
                 * our client, and if so, clean up our saved state
                 */
                const int scoid = recv_buf.pulse.scoid;
                const QSet<int> coids = connections.take(scoid);
                for (int coid : coids)
                {
                    const uint64_t uid = static_cast<uint64_t>(scoid) << 32 | static_cast<uint32_t>(coid);
                    QSharedPointer<QIOQnxSource> io;
                    mutex.lock();
                    if (sources.contains(uid))
                        io = sources.take(uid);
                    mutex.unlock();
#ifdef USE_HAM
                    ham_action_t *action = hamActions.take(uid);
                    ham_action_remove(action, 0);
                    ham_action_handle_free(action);
#endif

                    if (!io.isNull()) {
                        io->d_func()->m_serverClosing.ref();
                        QMetaObject::invokeMethod(io.data(),"onDisconnected",Qt::QueuedConnection);
                    }
                }

                /* always do the ConnectDetach() */
                //Don't care about return value
                ConnectDetach(recv_buf.pulse.scoid);
                qCDebug(QT_REMOTEOBJECT) << "disconnect from client" << recv_buf.pulse.scoid;
            }
                break;
            case _PULSE_CODE_COIDDEATH:
            {
                /* a connection has gone away.  Verify that it is
                 * our client, and if so, clean up our saved state
                 */
                const int coid = recv_buf.pulse.value.sival_int;

                if (ConnectServerInfo(0, coid, nullptr) != coid) {
                    const int scoid = recv_buf.pulse.scoid;
                    if (connections.value(scoid).contains(coid))
                        connections[scoid].remove(coid);
                    QSharedPointer<QIOQnxSource> io;
                    const uint64_t uid = static_cast<uint64_t>(scoid) << 32 | static_cast<uint32_t>(coid);
                    mutex.lock();
                    if (sources.contains(uid))
                        io = sources.take(uid);
                    mutex.unlock();
#ifdef USE_HAM
                    ham_action_t *action = hamActions.take(uid);
                    ham_action_remove(action, 0);
                    ham_action_handle_free(action);
#endif

                    if (!io.isNull()) {
                        io->d_func()->m_serverClosing.ref();
                        QMetaObject::invokeMethod(io.data(),"onDisconnected",Qt::QueuedConnection);
                    }
                    qCDebug(QT_REMOTEOBJECT) << "Connection dropped" << coid;
                }
                break;
            }
                break;
            case _PULSE_CODE_UNBLOCK:
                if (running.loadRelaxed()) {
                    // did we forget to Reply to our client?
                    qCWarning(QT_REMOTEOBJECT) << "got an unblock pulse, did you forget to reply to your client?";
                    WARN_ON_ERROR(MsgError, recv_buf.pulse.value.sival_int, EINTR)
                }
                break;
            default:
                qCWarning(QT_REMOTEOBJECT) << "unexpected pulse code: " << recv_buf.pulse.code << __FILE__ << __LINE__;
                break;
            }
            continue;
        }

        /* not an error, not a pulse, therefore a message */
        switch (recv_buf.type)
        {
        qCDebug(QT_REMOTEOBJECT) << "Msg Received" << recv_buf.type;

        case _IO_CONNECT:
            /* _IO_CONNECT because someone did a name_open() to us and we are
             * in the network case (gns running).  We must EOK it. */
            if (connections.contains(msg_info.scoid)
                && connections.value(msg_info.scoid).contains(msg_info.coid))
            {
                qCWarning(QT_REMOTEOBJECT) << "Already connected rcvid seen" << connections << rcvid;
                FATAL_ON_ERROR(MsgError, rcvid, EADDRINUSE)
            } else {
                if (!connections.contains(msg_info.scoid))
                    connections.insert(msg_info.scoid, QSet<int>());
                connections[msg_info.scoid] << msg_info.coid;
                qCDebug(QT_REMOTEOBJECT) << "New connection (qns)" << msg_info.coid << msg_info.scoid;
                const uint64_t uid = static_cast<uint64_t>(msg_info.scoid) << 32 | static_cast<uint32_t>(msg_info.coid);
                createSource(rcvid, uid, msg_info.pid);  // Reads more and then calls MsgReply
            }
            break;

        case MsgType::REPLICA_INIT:
            qCDebug(QT_REMOTEOBJECT) << "MsgType::INIT received" << rcvid << msg_info.scoid << msg_info.tid << msg_info.chid << msg_info.coid;
            //Check if this is a new connection (not gns)
            if (connections.contains(msg_info.scoid)
                && connections.value(msg_info.scoid).contains(msg_info.coid))
            {
                qCWarning(QT_REMOTEOBJECT) << "Already connected rcvid seen" << connections << rcvid;
                FATAL_ON_ERROR(MsgError, rcvid, EADDRINUSE)
            } else {
                if (!connections.contains(msg_info.scoid))
                    connections.insert(msg_info.scoid, QSet<int>());
                connections[msg_info.scoid] << msg_info.coid;
                qCDebug(QT_REMOTEOBJECT) << "New connection (non-gns)" << rcvid << msg_info.coid << msg_info.scoid;
                const uint64_t uid = static_cast<uint64_t>(msg_info.scoid) << 32 | static_cast<uint32_t>(msg_info.coid);
                createSource(rcvid, uid, msg_info.pid);  // Reads more and then calls MsgReply
            }

            break;
        case MsgType::SOURCE_TX_RESP:
        {
            WARN_ON_ERROR(MsgInfo, rcvid, &msg_info);
            const uint64_t uid = static_cast<uint64_t>(msg_info.scoid) << 32 | static_cast<uint32_t>(msg_info.coid);
            mutex.lock();
            Q_ASSERT(sources.contains(uid));
            auto io = sources.value(uid);
            mutex.unlock();

            io->d_func()->obLock.lockForWrite(); //NAR (Not-An-Error)
            const QByteArray data = io->d_func()->obuffer.read();
            const int bytesLeft = io->d_func()->obuffer.size();
            io->d_func()->obLock.unlock();

            qCDebug(QT_REMOTEOBJECT) << "server received SOURCE_TX_RESP with length" << msg_info.dstmsglen << "/" << data.length() << "Available:" << bytesLeft;
            Q_ASSERT(data.length() == static_cast<int>(msg_info.dstmsglen - sizeof(bytesLeft)));

            iov_t reply_vector[2];
            SETIOV(reply_vector, &bytesLeft, sizeof(bytesLeft));
            SETIOV(reply_vector+1, data.data(), size_t(data.length()));

            FATAL_ON_ERROR(MsgReplyv, rcvid, EOK, reply_vector, 2)
        }
            break;
        case MsgType::SOURCE_TX_RESP_REPEAT:
        {
            WARN_ON_ERROR(MsgInfo, rcvid, &msg_info);
            const uint64_t uid = static_cast<uint64_t>(msg_info.scoid) << 32 | static_cast<uint32_t>(msg_info.coid);
            mutex.lock();
            Q_ASSERT(sources.contains(uid));
            auto io = sources.value(uid);
            mutex.unlock();

            int len_taken = 0;
            const int to_send = msg_info.dstmsglen - sizeof(int); //Exclude the buffer count
            QByteArrayList qba_array;
            io->d_func()->obLock.lockForWrite(); //NAR (Not-An-Error)
            qCDebug(QT_REMOTEOBJECT) << "server received SOURCE_TX_RESP_REPEAT with length" << msg_info.dstmsglen << "Available:" << io->d_func()->obuffer.size();
            while (len_taken != to_send)
            {
                QByteArray data = io->d_func()->obuffer.read();
                qba_array << data;
                len_taken += data.length();
                if (data.length() == 0) { // We somehow reached the end
                    qCWarning(QT_REMOTEOBJECT) << "Reached the end of the buffer before getting the requested amount of data." << Q_FUNC_INFO << __FILE__ << __LINE__;
                    break;
                }
            }
            qCDebug(QT_REMOTEOBJECT) << "grabbing more data" << len_taken << to_send << qba_array.length();
            io->d_func()->obLock.unlock();

            Q_ASSERT(len_taken == to_send);
            const int buffers_taken = qba_array.length();

            resp_repeat_iov.resize(buffers_taken+1);
            SETIOV(&resp_repeat_iov[0], &buffers_taken, sizeof(buffers_taken));
            for (int i = 1; i <= buffers_taken; ++i)
                SETIOV(&resp_repeat_iov[i], qba_array.at(i-1).constData(), size_t(qba_array.at(i-1).length()));
            FATAL_ON_ERROR(MsgReplyv, rcvid, EOK, resp_repeat_iov.data(), buffers_taken+1)
        }
            break;
        case MsgType::REPLICA_TX_RECV:
        {
            FATAL_ON_ERROR(MsgInfo,rcvid, &msg_info)
            const uint64_t uid = static_cast<uint64_t>(msg_info.scoid) << 32 | static_cast<uint32_t>(msg_info.coid);
            mutex.lock();
            Q_ASSERT(sources.contains(uid));
            auto io = sources.value(uid);
            mutex.unlock();

            //Long-lock, use buffer+memcpy if we run into trouble
            io->d_func()->ibLock.lockForWrite();
            qint32 toRead = msg_info.msglen-sizeof(MsgType);
            char *payload = io->d_func()->buffer.reserve(toRead);

            const int res = MsgRead(rcvid, payload, toRead, sizeof(MsgType));
            if (-1 == res) {
                io->d_func()->buffer.chop(toRead);
                io->d_func()->ibLock.unlock();
                qFatal("MsgRead");
            }
            Q_ASSERT(res == toRead);

            if (res < toRead)
                io->d_func()->buffer.chop(toRead - res);
            io->d_func()->ibLock.unlock();

            FATAL_ON_ERROR(MsgReply, rcvid, EOK, nullptr, 0)

            qCDebug(QT_REMOTEOBJECT) << "server received REPLICA_TX_RECV" << payload << toRead;

            emit io->readyRead();
        }
            break;
        default:
            /* some other unexpected message */
            qCWarning(QT_REMOTEOBJECT) << "unexpected message type" << recv_buf.type << __FILE__ << __LINE__;
            WARN_ON_ERROR(MsgError, rcvid, ENOSYS)
            break;
        }
    }
    mutex.lock();
    for (auto io: sources)
        io->d_func()->m_serverClosing.ref();
    mutex.unlock();
    name_detach(attachStruct, 0);
    attachStruct = nullptr;
    qCDebug(QT_REMOTEOBJECT) << "Server thread_func stopped";
}

bool QQnxNativeServerPrivate::listen(const QString &name)
{
    attachStruct = name_attach(nullptr, qPrintable(name), 0);
    if (attachStruct == nullptr) {
        qCDebug(QT_REMOTEOBJECT, "name_attach call failed");
        return false;
    }
    terminateCoid = ConnectAttach(ND_LOCAL_NODE, 0, attachStruct->chid, _NTO_SIDE_CHANNEL, 0);
    if (terminateCoid == -1) {
        qCDebug(QT_REMOTEOBJECT, "ConnectAttach failed");
        return false;
    }

    running.ref();
    thread.start();

    return true;
}

void QQnxNativeServerPrivate::cleanupIOSource(QIOQnxSource *conn)
{
    QSharedPointer<QIOQnxSource> io;
    mutex.lock();
    for (auto i = sources.begin(), end = sources.end(); i != end; ++i) {
        if (i.value().data() == conn) {
            io = std::move(i.value());
            sources.erase(i);
            break;
        }
    }
    mutex.unlock();
    if (!io.isNull()) {
        io->d_func()->m_serverClosing.ref();
        io->close();
    }
}

void QQnxNativeServerPrivate::teardownServer()
{
    if (attachStruct == nullptr)
        return;

    running.deref();
    MsgSendPulse(terminateCoid, SIGEV_PULSE_PRIO_INHERIT, _PULSE_CODE_UNBLOCK, 0);
    ConnectDetach(terminateCoid);
    thread.wait();

    //Existing QIOQnxSources will be deleted along with object
    //threads gone, don't need to use mutex
    sources.clear();
#ifdef USE_HAM
    if (hamAvailable)
        closeHamResources();
#endif
}

void QQnxNativeServerPrivate::createSource(int rcvid, uint64_t uid, pid_t toPid)
{
    Q_Q(QQnxNativeServer);
#ifndef USE_HAM
    Q_UNUSED(toPid)
#endif
    auto io = QSharedPointer<QIOQnxSource>(new QIOQnxSource(rcvid));
    io->moveToThread(q->thread());
    QObject::connect(io.data(), &QIOQnxSource::aboutToClose,
                     q,  &QQnxNativeServer::onSourceClosed);

    QIOQnxSourcePrivate *iop = io->d_func();
    FATAL_ON_ERROR(MsgRead, rcvid, &(iop->m_event), sizeof(sigevent), sizeof(MsgType))
    int sentChannelId;
    FATAL_ON_ERROR(MsgRead, rcvid, &sentChannelId, sizeof(int), sizeof(MsgType)+sizeof(sigevent))
    FATAL_ON_ERROR(MsgReply, rcvid, EOK, nullptr, 0)

    mutex.lock();
    sources.insert(uid, io);
    pending.append(io);
    mutex.unlock();

    //push an event into the main threads eventloop to emit newConnection.
    QMetaObject::invokeMethod(q,"newConnection",Qt::QueuedConnection);
#ifdef USE_HAM
    if (!hamInitialized) {
        hamInitialized = true;
        hamAvailable = initializeHam();
    }
    if (hamAvailable)
        configureHamDeath(sentChannelId, toPid, uid);
#endif
}

#ifdef USE_HAM
bool QQnxNativeServerPrivate::initializeHam()
{
    if (access("/proc/ham",F_OK) != 0) {
        WARNING(access(/proc/ham))
        return false;
    }
    ham_connect(0);
    pid_t pid = getpid();
    hamEntityHandle = ham_attach(qPrintable(serverName), ND_LOCAL_NODE, pid, NULL, 0);
    if (!hamEntityHandle) {
        WARNING(ham_attach)
        ham_disconnect(0);
        return false;
    }
    hamConditionHandle = ham_condition(hamEntityHandle, CONDDEATH, "death", 0);
    if (!hamConditionHandle) {
        WARNING(ham_condition)
        ham_detach(hamEntityHandle, 0);
        ham_entity_handle_free(hamEntityHandle);
        ham_disconnect(0);
        return false;
    }
    qCDebug(QT_REMOTEOBJECT, "HAM initialized for %s (pid = %d)", qPrintable(serverName), pid);
    return true;
}

void QQnxNativeServerPrivate::configureHamDeath(int sentChannelId, pid_t toPid, uint64_t uid)
{
    char identifier[25];
    snprintf(identifier, 25, "%d_%d", toPid, sentChannelId);
    ham_action_t *a = ham_action_notify_pulse(hamConditionHandle, identifier,
                                             ND_LOCAL_NODE, toPid,
                                             sentChannelId, NODE_DEATH, 0, 0);
    if (!a)
        WARNING(ham_action_notify_pulse)
    else
        hamActions.insert(uid, a);
}

void QQnxNativeServerPrivate::closeHamResources()
{
    ham_entity_handle_free(hamEntityHandle);
    ham_condition_handle_free(hamConditionHandle);
    ham_disconnect(0);
}
#endif

QT_END_NAMESPACE
