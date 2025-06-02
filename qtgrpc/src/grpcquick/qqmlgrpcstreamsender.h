// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLGRPCSTREAMSENDER_H
#define QQMLGRPCSTREAMSENDER_H

#include <QtCore/qdebug.h>
#include <QtGrpc/qgrpcstream.h>
#include <QtProtobuf/qprotobufmessage.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

template <typename Stream, typename Arg,
          typename IsCompatibleStream = std::disjunction<std::is_same<Stream, QGrpcClientStream>,
                                                         std::is_same<Stream, QGrpcBidiStream>>,
          std::enable_if_t<std::is_base_of_v<QProtobufMessage, Arg>, bool> = true>
class QQmlGrpcStreamSender : public QObject
{
protected:
    QQmlGrpcStreamSender(std::shared_ptr<Stream> stream) : m_stream(std::move(stream)) { }

    void writeMessageImpl(const Arg &message)
    {
        Q_ASSERT_X(m_stream, "writeMessageImpl", "stream is nullptr");
        if (m_stream->isFinished()) {
            qWarning() << "Unable to write message; stream is finished";
            return;
        }
        m_stream->writeMessage(message);
    }

private:
    Q_DISABLE_COPY_MOVE(QQmlGrpcStreamSender)
    std::shared_ptr<Stream> m_stream = nullptr;
};

QT_END_NAMESPACE

#endif // QQMLGRPCSTREAMSENDER_H
