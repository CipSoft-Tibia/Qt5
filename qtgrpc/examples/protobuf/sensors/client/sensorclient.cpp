// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "sensorclient.h"

#include <QNetworkDatagram>
#include <QDebug>

#include "sensors.qpb.h"
#include "tlv.qpb.h"

SensorClient::SensorClient(QObject *parent) : QObject(parent)
{
    [[maybe_unused]] bool bound = m_client.bind(QHostAddress::LocalHost, 65500);
    Q_ASSERT_X(bound, "SensorClient", "Unable to bind to port 65500");
    QObject::connect(&m_client, &QUdpSocket::readyRead, this, &SensorClient::receive);
}

void SensorClient::receive()
{
    while (m_client.hasPendingDatagrams()) {
//! [0]
        const auto datagram = m_client.receiveDatagram();
        qt::examples::sensors::tlv::TlvMessage msg;
        msg.deserialize(&m_serializer, datagram.data());
        if (m_serializer.deserializationError()
                != QAbstractProtobufSerializer::NoError) {
            qWarning().nospace() << "Unable to deserialize datagram ("
                       << m_serializer.deserializationError() << ")"
                       << m_serializer.deserializationErrorString();
            continue;
        }
//! [0]

        switch (msg.type()) {
        case qt::examples::sensors::tlv::MessageTypeGadget::Coordinates: {
//! [1]
            qt::examples::sensors::Coordinates coord;
            coord.deserialize(&m_serializer, msg.value());
            emit coordinatesUpdated(coord);
            break;
//! [1]
        }
        case qt::examples::sensors::tlv::MessageTypeGadget::Temperature: {
            qt::examples::sensors::Temperature temp;
            temp.deserialize(&m_serializer, msg.value());
            emit temperatureUpdated(temp);
            break;
        }
        case qt::examples::sensors::tlv::MessageTypeGadget::WarningNotification: {
            qt::examples::sensors::WarningNotification warn;
            warn.deserialize(&m_serializer, msg.value());
            emit warning(warn);
            break;
        }
        }

        if (m_serializer.deserializationError()
                != QAbstractProtobufSerializer::NoError) {
            qWarning().nospace() << "Unable to deserialize message ("
                       << m_serializer.deserializationError() << ")"
                       << m_serializer.deserializationErrorString();
            continue;
        }
    }
}

#include "moc_sensorclient.cpp"
