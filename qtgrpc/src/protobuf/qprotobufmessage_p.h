// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_PROTOBUF_MESSAGE_P_H
#define Q_PROTOBUF_MESSAGE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the Qt Protobuf API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qprotobufmessage.h"

#include <QtProtobuf/qtprotobufglobal.h>

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qhash.h>
#include <QtCore/private/qglobal_p.h>

#include <optional>

QT_BEGIN_NAMESPACE

class QMetaProperty;

class QProtobufMessagePrivate
{
public:
    // QHash of form <field index, data>.
    QHash<qint32, QByteArrayList> unknownEntries;
    const QMetaObject *metaObject = nullptr;

    int getPropertyIndex(QAnyStringView propertyName) const;
    void storeUnknownEntry(QByteArrayView entry, int fieldNumber);

    std::optional<QMetaProperty> metaProperty(QAnyStringView name) const;
    std::optional<QMetaProperty>
    metaProperty(QtProtobufPrivate::QProtobufPropertyOrderingInfo ord) const;

    static QProtobufMessagePrivate *get(QProtobufMessage *message) { return message->d_func(); }
    static const QProtobufMessagePrivate *get(const QProtobufMessage *message)
    {
        return message->d_func();
    }
};

QT_END_NAMESPACE

#endif // Q_PROTOBUF_MESSAGE_P_H
