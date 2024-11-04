// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROTOBUFSERIALIZER_H
#define QPROTOBUFSERIALIZER_H

#include <QtProtobuf/qtprotobufglobal.h>
#include <QtProtobuf/qprotobufbaseserializer.h>
#include <QtProtobuf/qtprotobuftypes.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QProtobufMessage;
class QProtobufSerializerPrivate;
class Q_PROTOBUF_EXPORT QProtobufSerializer final : public QProtobufBaseSerializer
{
    Q_DISABLE_COPY_MOVE(QProtobufSerializer)
public:
    QProtobufSerializer();
    ~QProtobufSerializer() override;

    QProtobufSerializer::DeserializationError deserializationError() const override;
    QString deserializationErrorString() const override;

    void shouldPreserveUnknownFields(bool preserveUnknownFields);
private:
    QByteArray
    serializeMessage(const QProtobufMessage *message,
                     const QtProtobufPrivate::QProtobufPropertyOrdering &ordering
                     ) const override;
    bool deserializeMessage(QProtobufMessage *message,
                            const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                            QByteArrayView data) const override;

    void serializeObject(const QProtobufMessage *message,
                         const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                         const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo)
        const override;
    bool
    deserializeObject(QProtobufMessage *message,
                      const QtProtobufPrivate::QProtobufPropertyOrdering &ordering) const override;

    void serializeListObject(const QProtobufMessage *message,
                             const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                             const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo)
        const override;
    bool deserializeListObject(QProtobufMessage *message,
                               const QtProtobufPrivate::QProtobufPropertyOrdering &ordering)
        const override;

    void serializeMapPair(const QVariant &key, const QVariant &value,
                          const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo)
        const override;
    bool deserializeMapPair(QVariant &key, QVariant &value) const override;

    void
    serializeEnum(QtProtobuf::int64 value, const QMetaEnum &metaEnum,
                  const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const override;
    void serializeEnumList(const QList<QtProtobuf::int64> &value, const QMetaEnum &metaEnum,
                           const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo)
        const override;

    bool deserializeEnum(QtProtobuf::int64 &value, const QMetaEnum &metaEnum) const override;
    bool deserializeEnumList(QList<QtProtobuf::int64> &value,
                             const QMetaEnum &metaEnum) const override;

private:
    std::unique_ptr<QProtobufSerializerPrivate> d_ptr;
};

QT_END_NAMESPACE
#endif // QPROTOBUFSERIALIZER_H
