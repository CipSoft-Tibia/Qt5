// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnearfieldtagtype2_p.h"
#include <QtNfc/private/qnearfieldtarget_p.h>

#include <QtCore/QVariant>
#include <QtCore/QCoreApplication>
#include <QtCore/QTime>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QNearFieldTagType2
    \brief The QNearFieldTagType2 class provides an interface for communicating with an NFC Tag
           Type 2 tag.

    \ingroup connectivity-nfc
    \inmodule QtNfc
    \internal
*/

/*!
    \fn Type QNearFieldTagType2::type() const
    \reimp
*/

struct SectorSelectState {
    int timerId;    // id of timer used for passive ack
    quint8 sector;  // sector being selected
};

class QNearFieldTagType2Private
{
public:
    QNearFieldTagType2Private() : m_currentSector(0) { }

    QMap<QNearFieldTarget::RequestId, QByteArray> m_pendingInternalCommands;

    quint8 m_currentSector;

    QMap<QNearFieldTarget::RequestId, SectorSelectState> m_pendingSectorSelectCommands;
};

static QVariant decodeResponse(const QByteArray &command, const QByteArray &response)
{
    quint8 opcode = command.at(0);

    switch (opcode) {
    case 0xa2:  // WRITE
        return quint8(response.at(0)) == 0x0a;
    case 0xc2:  // SECTOR SELECT (Command Packet 1)
        return quint8(response.at(0)) == 0x0a;
    }

    return QVariant();
}

/*!
    Constructs a new tag type 2 near field target with \a parent.
*/
QNearFieldTagType2::QNearFieldTagType2(QObject *parent)
:   QNearFieldTargetPrivate(parent), d_ptr(new QNearFieldTagType2Private)
{
}

/*!
    Destroys the tag type 2 near field target.
*/
QNearFieldTagType2::~QNearFieldTagType2()
{
    delete d_ptr;
}

/*!
    \reimp
*/
bool QNearFieldTagType2::hasNdefMessage()
{
    qWarning() << Q_FUNC_INFO << "is unimplemeted";
    return false;
}

/*!
    \reimp
*/
QNearFieldTarget::RequestId QNearFieldTagType2::readNdefMessages()
{
    return QNearFieldTarget::RequestId();
}

/*!
    \reimp
*/
QNearFieldTarget::RequestId QNearFieldTagType2::writeNdefMessages(const QList<QNdefMessage> &messages)
{
    Q_UNUSED(messages);

    return QNearFieldTarget::RequestId();
}

/*!
    Returns the NFC Tag Type 2 specification version number that the tag supports.
*/
quint8 QNearFieldTagType2::version()
{
    Q_D(QNearFieldTagType2);
    if (d->m_currentSector != 0) {
        QNearFieldTarget::RequestId id = selectSector(0);
        if (!waitForRequestCompleted(id))
            return 0;
    }

    QNearFieldTarget::RequestId id = readBlock(0);
    if (!waitForRequestCompleted(id))
        return 0;

    const QByteArray data = requestResponse(id).toByteArray();
    return data.at(13);
}

/*!
    Returns the memory size in bytes of the tag.
*/
int QNearFieldTagType2::memorySize()
{
    Q_D(QNearFieldTagType2);
    if (d->m_currentSector != 0) {
        QNearFieldTarget::RequestId id = selectSector(0);
        if (!waitForRequestCompleted(id))
            return 0;
    }

    QNearFieldTarget::RequestId id = readBlock(0);
    if (!waitForRequestCompleted(id))
        return 0;

    const QByteArray data = requestResponse(id).toByteArray();
    return 8 * quint8(data.at(14));
}

/*!
    Requests 16 bytes of data starting at \a blockAddress. Returns a request id which can be used
    to track the completion status of the request.

    Once the request completes successfully the response can be retrieved from the
    requestResponse() function. The response of this request will be a QByteArray.

    \sa requestCompleted(), waitForRequestCompleted()
*/
QNearFieldTarget::RequestId QNearFieldTagType2::readBlock(quint8 blockAddress)
{
    QByteArray command;
    command.append(char(0x30));         // READ
    command.append(char(blockAddress)); // Block address

    return sendCommand(command);
}

/*!
    Writes 4 bytes of \a data to the block at \a blockAddress. Returns a request id which can be
    used to track the completion status of the request.

    Once the request completes the response can be retrieved from the requestResponse() function.
    The response of this request will be a boolean value, true for success; otherwise false.

    Returns true on success; otherwise returns false.
*/
QNearFieldTarget::RequestId QNearFieldTagType2::writeBlock(quint8 blockAddress,
                                                           const QByteArray &data)
{
    if (data.size() != 4)
        return QNearFieldTarget::RequestId();

    QByteArray command;
    command.append(char(0xa2));         // WRITE
    command.append(char(blockAddress)); // Block address
    command.append(data);               // Data

    QNearFieldTarget::RequestId id = sendCommand(command);

    Q_D(QNearFieldTagType2);

    d->m_pendingInternalCommands.insert(id, command);

    return id;
}

/*!
    Selects the \a sector upon which subsequent readBlock() and writeBlock() operations will act.

    Returns a request id which can be used to track the completion status of the request.

    Once the request completes the response can be retrieved from the requestResponse() function.
    The response of this request will be a boolean value, true for success; otherwise false.

    \note this request has a passive acknowledgement mechanism. The operation is deemed successful
    if no response is received within 1ms. It will therefore take a minimum of 1 millisecond for
    the requestCompleted() signal to be emitted and calling waitForRequestCompleted() on the
    returned request id may cause the current thread to block for up to 1 millisecond.
*/
QNearFieldTarget::RequestId QNearFieldTagType2::selectSector(quint8 sector)
{
    QByteArray command;
    command.append(char(0xc2));     // SECTOR SELECT (Command Packet 1)
    command.append(char(0xff));

    QNearFieldTarget::RequestId id = sendCommand(command);

    Q_D(QNearFieldTagType2);

    d->m_pendingInternalCommands.insert(id, command);

    SectorSelectState state;
    state.timerId = -1;
    state.sector = sector;

    d->m_pendingSectorSelectCommands.insert(id, state);

    return id;
}

/*!
    \reimp
*/
void QNearFieldTagType2::setResponseForRequest(const QNearFieldTarget::RequestId &id,
                                               const QVariant &response,
                                               bool emitRequestCompleted)
{
    Q_D(QNearFieldTagType2);

    if (d->m_pendingInternalCommands.contains(id)) {
        const QByteArray command = d->m_pendingInternalCommands.take(id);

        QVariant decodedResponse = decodeResponse(command, response.toByteArray());
        if (quint8(command.at(0)) == 0xc2 && decodedResponse.toBool()) {
            // SECTOR SELECT (Command Packet 2)
            SectorSelectState &state = d->m_pendingSectorSelectCommands[id];

            QByteArray packet2;
            packet2.append(char(state.sector));
            packet2.append(QByteArray(3, 0x00));

            sendCommand(packet2);

            state.timerId = startTimer(1);
        } else {
            QNearFieldTargetPrivate::setResponseForRequest(id, decodedResponse, emitRequestCompleted);
        }

        return;
    }

    if (d->m_pendingSectorSelectCommands.contains(id)) {
        if (!response.toByteArray().isEmpty()) {
            d->m_pendingSectorSelectCommands.remove(id);
            QNearFieldTargetPrivate::setResponseForRequest(id, false, emitRequestCompleted);

            return;
        }
    }

    QNearFieldTargetPrivate::setResponseForRequest(id, response, emitRequestCompleted);
}

/*!
    \internal
*/
void QNearFieldTagType2::timerEvent(QTimerEvent *event)
{
    Q_D(QNearFieldTagType2);

    killTimer(event->timerId());

    for (auto i = d->m_pendingSectorSelectCommands.begin(), end = d->m_pendingSectorSelectCommands.end(); i != end; ++i) {

        SectorSelectState &state = i.value();

        if (state.timerId == event->timerId()) {
            d->m_currentSector = state.sector;

            QNearFieldTargetPrivate::setResponseForRequest(i.key(), true);

            d->m_pendingSectorSelectCommands.erase(i);
            break;
        }
    }
}

QT_END_NAMESPACE
