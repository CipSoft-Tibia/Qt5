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

#include "qwavedecoder_p.h"

#include <QtCore/qtimer.h>
#include <QtCore/qendian.h>

QT_BEGIN_NAMESPACE

QWaveDecoder::QWaveDecoder(QIODevice *s, QObject *parent):
    QIODevice(parent),
    haveFormat(false),
    dataSize(0),
    source(s),
    state(QWaveDecoder::InitialState),
    junkToSkip(0),
    bigEndian(false)
{
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    if (enoughDataAvailable())
        QTimer::singleShot(0, this, SLOT(handleData()));
    else
        connect(source, SIGNAL(readyRead()), SLOT(handleData()));
}

QWaveDecoder::~QWaveDecoder()
{
}

QAudioFormat QWaveDecoder::audioFormat() const
{
    return format;
}

int QWaveDecoder::duration() const
{
    return size() * 1000 / (format.sampleSize() / 8) / format.channelCount() / format.sampleRate();
}

qint64 QWaveDecoder::size() const
{
    return haveFormat ? dataSize : 0;
}

bool QWaveDecoder::isSequential() const
{
    return source->isSequential();
}

qint64 QWaveDecoder::bytesAvailable() const
{
    return haveFormat ? source->bytesAvailable() : 0;
}

qint64 QWaveDecoder::readData(char *data, qint64 maxlen)
{
    return haveFormat ? source->read(data, maxlen) : 0;
}

qint64 QWaveDecoder::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return -1;
}

void QWaveDecoder::parsingFailed()
{
    Q_ASSERT(source);
    source->disconnect(SIGNAL(readyRead()), this, SLOT(handleData()));
    emit parsingError();
}

void QWaveDecoder::handleData()
{
    // As a special "state", if we have junk to skip, we do
    if (junkToSkip > 0) {
        discardBytes(junkToSkip); // this also updates junkToSkip

        // If we couldn't skip all the junk, return
        if (junkToSkip > 0) {
            // We might have run out
            if (source->atEnd())
                parsingFailed();
            return;
        }
    }

    if (state == QWaveDecoder::InitialState) {
        if (source->bytesAvailable() < qint64(sizeof(RIFFHeader)))
            return;

        RIFFHeader riff;
        source->read(reinterpret_cast<char *>(&riff), sizeof(RIFFHeader));

        // RIFF = little endian RIFF, RIFX = big endian RIFF
        if (((qstrncmp(riff.descriptor.id, "RIFF", 4) != 0) && (qstrncmp(riff.descriptor.id, "RIFX", 4) != 0))
                || qstrncmp(riff.type, "WAVE", 4) != 0) {
            parsingFailed();
            return;
        } else {
            state = QWaveDecoder::WaitingForFormatState;
            if (qstrncmp(riff.descriptor.id, "RIFX", 4) == 0)
                bigEndian = true;
            else
                bigEndian = false;
        }
    }

    if (state == QWaveDecoder::WaitingForFormatState) {
        if (findChunk("fmt ")) {
            chunk descriptor;
            peekChunk(&descriptor);

            quint32 rawChunkSize = descriptor.size + sizeof(chunk);
            if (source->bytesAvailable() < qint64(rawChunkSize))
                return;

            WAVEHeader wave;
            source->read(reinterpret_cast<char *>(&wave), sizeof(WAVEHeader));

            if (rawChunkSize > sizeof(WAVEHeader))
                discardBytes(rawChunkSize - sizeof(WAVEHeader));

            // Swizzle this
            if (bigEndian) {
                wave.audioFormat = qFromBigEndian<quint16>(wave.audioFormat);
            } else {
                wave.audioFormat = qFromLittleEndian<quint16>(wave.audioFormat);
            }

            if (wave.audioFormat != 0 && wave.audioFormat != 1) {
                // 32bit wave files have format == 0xFFFE (WAVE_FORMAT_EXTENSIBLE).
                // but don't support them at the moment.
                parsingFailed();
                return;
            } else {
                format.setCodec(QLatin1String("audio/pcm"));

                if (bigEndian) {
                    int bps = qFromBigEndian<quint16>(wave.bitsPerSample);

                    format.setSampleType(bps == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);
                    format.setByteOrder(QAudioFormat::BigEndian);
                    format.setSampleRate(qFromBigEndian<quint32>(wave.sampleRate));
                    format.setSampleSize(bps);
                    format.setChannelCount(qFromBigEndian<quint16>(wave.numChannels));
                } else {
                    int bps = qFromLittleEndian<quint16>(wave.bitsPerSample);

                    format.setSampleType(bps == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);
                    format.setByteOrder(QAudioFormat::LittleEndian);
                    format.setSampleRate(qFromLittleEndian<quint32>(wave.sampleRate));
                    format.setSampleSize(bps);
                    format.setChannelCount(qFromLittleEndian<quint16>(wave.numChannels));
                }

                state = QWaveDecoder::WaitingForDataState;
            }
        }
    }

    if (state == QWaveDecoder::WaitingForDataState) {
        if (findChunk("data")) {
            source->disconnect(SIGNAL(readyRead()), this, SLOT(handleData()));

            chunk descriptor;
            source->read(reinterpret_cast<char *>(&descriptor), sizeof(chunk));
            if (bigEndian)
                descriptor.size = qFromBigEndian<quint32>(descriptor.size);
            else
                descriptor.size = qFromLittleEndian<quint32>(descriptor.size);

            dataSize = descriptor.size;

            haveFormat = true;
            connect(source, SIGNAL(readyRead()), SIGNAL(readyRead()));
            emit formatKnown();

            return;
        }
    }

    // If we hit the end without finding data, it's a parsing error
    if (source->atEnd()) {
        parsingFailed();
    }
}

bool QWaveDecoder::enoughDataAvailable()
{
    chunk descriptor;
    if (!peekChunk(&descriptor, false))
        return false;

    // This is only called for the RIFF/RIFX header, before bigEndian is set,
    // so we have to manually swizzle
    if (qstrncmp(descriptor.id, "RIFX", 4) == 0)
        descriptor.size = qFromBigEndian<quint32>(descriptor.size);
    if (qstrncmp(descriptor.id, "RIFF", 4) == 0)
        descriptor.size = qFromLittleEndian<quint32>(descriptor.size);

    if (source->bytesAvailable() < qint64(sizeof(chunk) + descriptor.size))
        return false;

    return true;
}

bool QWaveDecoder::findChunk(const char *chunkId)
{
    chunk descriptor;

    do {
        if (!peekChunk(&descriptor))
            return false;

        if (qstrncmp(descriptor.id, chunkId, 4) == 0)
            return true;

        // It's possible that bytes->available() is less than the chunk size
        // if it's corrupt.
        junkToSkip = qint64(sizeof(chunk) + descriptor.size);

        // Skip the current amount
        if (junkToSkip > 0)
            discardBytes(junkToSkip);

        // If we still have stuff left, just exit and try again later
        // since we can't call peekChunk
        if (junkToSkip > 0)
            return false;

    } while (source->bytesAvailable() > 0);

    return false;
}

bool QWaveDecoder::peekChunk(chunk *pChunk, bool handleEndianness)
{
    if (source->bytesAvailable() < qint64(sizeof(chunk)))
        return false;

    source->peek(reinterpret_cast<char *>(pChunk), sizeof(chunk));
    if (handleEndianness) {
        if (bigEndian)
            pChunk->size = qFromBigEndian<quint32>(pChunk->size);
        else
            pChunk->size = qFromLittleEndian<quint32>(pChunk->size);
    }
    return true;
}

void QWaveDecoder::discardBytes(qint64 numBytes)
{
    // Discards a number of bytes
    // If the iodevice doesn't have this many bytes in it,
    // remember how much more junk we have to skip.
    if (source->isSequential()) {
        QByteArray r = source->read(qMin(numBytes, qint64(16384))); // uggh, wasted memory, limit to a max of 16k
        if (r.size() < numBytes)
            junkToSkip = numBytes - r.size();
        else
            junkToSkip = 0;
    } else {
        quint64 origPos = source->pos();
        source->seek(source->pos() + numBytes);
        junkToSkip = origPos + numBytes - source->pos();
    }
}

QT_END_NAMESPACE

#include "moc_qwavedecoder_p.cpp"
