/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qpacketprotocol_p.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QtEndian>

#include <private/qiodevice_p.h>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

/*!
  \class QPacketProtocol
  \internal

  \brief The QPacketProtocol class encapsulates communicating discrete packets
  across fragmented IO channels, such as TCP sockets.

  QPacketProtocol makes it simple to send arbitrary sized data "packets" across
  fragmented transports such as TCP and UDP.

  As transmission boundaries are not respected, sending packets over protocols
  like TCP frequently involves "stitching" them back together at the receiver.
  QPacketProtocol makes this easier by performing this task for you.  Packet
  data sent using QPacketProtocol is prepended with a 4-byte size header
  allowing the receiving QPacketProtocol to buffer the packet internally until
  it has all been received.  QPacketProtocol does not perform any sanity
  checking on the size or on the data, so this class should only be used in
  prototyping or trusted situations where DOS attacks are unlikely.

  QPacketProtocol does not perform any communications itself.  Instead it can
  operate on any QIODevice that supports the QIODevice::readyRead() signal.  A
  logical "packet" is simply a QByteArray. The following example how to send
  data using QPacketProtocol.

  \code
  QTcpSocket socket;
  // ... connect socket ...

  QPacketProtocol protocol(&socket);

  // Send a packet
  QDataStream packet;
  packet << "Hello world" << 123;
  protocol.send(packet.data());
  \endcode

  Likewise, the following shows how to read data from QPacketProtocol, assuming
  that the QPacketProtocol::readyRead() signal has been emitted.

  \code
  // ... QPacketProtocol::readyRead() is emitted ...

  int a;
  QByteArray b;

  // Receive packet
  QDataStream packet(protocol.read());
  p >> a >> b;
  \endcode

  \ingroup io
*/

class QPacketProtocolPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QPacketProtocol)
public:
    QPacketProtocolPrivate(QIODevice *dev);

    bool writeToDevice(const char *bytes, qint64 size);
    bool readFromDevice(char *buffer, qint64 size);

    QList<qint32> sendingPackets;
    QList<QByteArray> packets;
    QByteArray inProgress;
    qint32 inProgressSize;
    bool waitingForPacket;
    QIODevice *dev;
};

/*!
  Construct a QPacketProtocol instance that works on \a dev with the
  specified \a parent.
 */
QPacketProtocol::QPacketProtocol(QIODevice *dev, QObject *parent)
    : QObject(*(new QPacketProtocolPrivate(dev)), parent)
{
    Q_ASSERT(4 == sizeof(qint32));
    Q_ASSERT(dev);

    QObject::connect(dev, &QIODevice::readyRead, this, &QPacketProtocol::readyToRead);
    QObject::connect(dev, &QIODevice::bytesWritten, this, &QPacketProtocol::bytesWritten);
}

/*!
  \fn void QPacketProtocol::send(const QByteArray &data)

  Transmit the \a packet.
 */
void QPacketProtocol::send(const QByteArray &data)
{
    Q_D(QPacketProtocol);
    static const qint32 maxSize = std::numeric_limits<qint32>::max() - sizeof(qint32);

    if (data.isEmpty())
        return; // We don't send empty packets

    if (data.size() > maxSize) {
        emit error();
        return;
    }

    const qint32 sendSize = data.size() + static_cast<qint32>(sizeof(qint32));
    d->sendingPackets.append(sendSize);

    qint32 sendSizeLE = qToLittleEndian(sendSize);
    if (!d->writeToDevice((const char *)&sendSizeLE, sizeof(qint32))
            || !d->writeToDevice(data.data(), data.size())) {
        emit error();
    }
}

/*!
  Returns the number of received packets yet to be read.
  */
qint64 QPacketProtocol::packetsAvailable() const
{
    Q_D(const QPacketProtocol);
    return d->packets.count();
}

/*!
  Return the next unread packet, or an empty QByteArray if no packets
  are available.  This method does NOT block.
  */
QByteArray QPacketProtocol::read()
{
    Q_D(QPacketProtocol);
    return d->packets.isEmpty() ? QByteArray() : d->packets.takeFirst();
}

/*!
  This function locks until a new packet is available for reading and the
  \l{QIODevice::}{readyRead()} signal has been emitted. The function
  will timeout after \a msecs milliseconds; the default timeout is
  30000 milliseconds.

  The function returns true if the readyRead() signal is emitted and
  there is new data available for reading; otherwise it returns false
  (if an error occurred or the operation timed out).
  */

bool QPacketProtocol::waitForReadyRead(int msecs)
{
    Q_D(QPacketProtocol);
    if (!d->packets.isEmpty())
        return true;

    QElapsedTimer stopWatch;
    stopWatch.start();

    d->waitingForPacket = true;
    do {
        if (!d->dev->waitForReadyRead(msecs))
            return false;
        if (!d->waitingForPacket)
            return true;
        msecs = qt_subtract_from_timeout(msecs, stopWatch.elapsed());
    } while (true);
}

void QPacketProtocol::bytesWritten(qint64 bytes)
{
    Q_D(QPacketProtocol);
    Q_ASSERT(!d->sendingPackets.isEmpty());

    while (bytes) {
        if (d->sendingPackets.at(0) > bytes) {
            d->sendingPackets[0] -= bytes;
            bytes = 0;
        } else {
            bytes -= d->sendingPackets.at(0);
            d->sendingPackets.removeFirst();
        }
    }
}

void QPacketProtocol::readyToRead()
{
    Q_D(QPacketProtocol);
    while (true) {
        // Need to get trailing data
        if (-1 == d->inProgressSize) {
            // We need a size header of sizeof(qint32)
            if (static_cast<qint64>(sizeof(qint32)) > d->dev->bytesAvailable())
                return;

            // Read size header
            qint32 inProgressSizeLE;
            if (!d->readFromDevice((char *)&inProgressSizeLE, sizeof(qint32))) {
                emit error();
                return;
            }
            d->inProgressSize = qFromLittleEndian(inProgressSizeLE);

            // Check sizing constraints
            if (d->inProgressSize < qint32(sizeof(qint32))) {
                disconnect(d->dev, &QIODevice::readyRead, this, &QPacketProtocol::readyToRead);
                disconnect(d->dev, &QIODevice::bytesWritten, this, &QPacketProtocol::bytesWritten);
                d->dev = nullptr;
                emit error();
                return;
            }

            d->inProgressSize -= sizeof(qint32);
        } else {

            const int bytesToRead = static_cast<int>(
                        qMin(d->dev->bytesAvailable(),
                             static_cast<qint64>(d->inProgressSize - d->inProgress.size())));

            QByteArray toRead(bytesToRead, Qt::Uninitialized);
            if (!d->readFromDevice(toRead.data(), toRead.length())) {
                emit error();
                return;
            }

            d->inProgress.append(toRead);
            if (d->inProgressSize == d->inProgress.size()) {
                // Packet has arrived!
                d->packets.append(d->inProgress);
                d->inProgressSize = -1;
                d->inProgress.clear();

                d->waitingForPacket = false;
                emit readyRead();
            } else
                return;
        }
    }
}

QPacketProtocolPrivate::QPacketProtocolPrivate(QIODevice *dev) :
    inProgressSize(-1), waitingForPacket(false), dev(dev)
{
}

bool QPacketProtocolPrivate::writeToDevice(const char *bytes, qint64 size)
{
    qint64 totalWritten = 0;
    while (totalWritten < size) {
        const qint64 chunkSize = dev->write(bytes + totalWritten, size - totalWritten);
        if (chunkSize < 0)
            return false;
        totalWritten += chunkSize;
    }
    return totalWritten == size;
}

bool QPacketProtocolPrivate::readFromDevice(char *buffer, qint64 size)
{
    qint64 totalRead = 0;
    while (totalRead < size) {
        const qint64 chunkSize = dev->read(buffer + totalRead, size - totalRead);
        if (chunkSize < 0)
            return false;
        totalRead += chunkSize;
    }
    return totalRead == size;
}

/*!
  \fn void QPacketProtocol::readyRead()

  Emitted whenever a new packet is received.  Applications may use
  QPacketProtocol::read() to retrieve this packet.
 */

/*!
  \fn void QPacketProtocol::invalidPacket()

  A packet larger than the maximum allowable packet size was received.  The
  packet will be discarded and, as it indicates corruption in the protocol, no
  further packets will be received.
 */

QT_END_NAMESPACE
