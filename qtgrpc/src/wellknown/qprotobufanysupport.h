// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_PROTOBUF_ANYSUPPORT_H
#define Q_PROTOBUF_ANYSUPPORT_H

#include <QtProtobufWellKnownTypes/qtprotobufwellknowntypesglobal.h>

#include <QtProtobuf/qprotobufmessage.h>
#include <QtProtobuf/qtprotobuftypes.h>

#include <QtCore/qanystringview.h>

#include <utility>
#include <optional>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtProtobuf {
class AnyPrivate;
class Q_PROTOBUFWELLKNOWNTYPES_EXPORT Any : public QProtobufMessage
{
    Q_GADGET
    Q_PROPERTY(QString typeUrl READ typeUrl WRITE setTypeUrl SCRIPTABLE true)
    Q_PROPERTY(QByteArray value READ value WRITE setValue SCRIPTABLE true)
public:
    static void registerTypes();

    Any();
    ~Any();
    Any(const Any &other);
    Any &operator=(const Any &other);
    Any(Any &&other) noexcept
        : QProtobufMessage(std::move(other)), d_ptr(std::exchange(other.d_ptr, {}))
    {
    }
    Any &operator=(Any &&other) noexcept
    {
        qt_ptr_swap(d_ptr, other.d_ptr);
        QProtobufMessage::operator=(std::move(other));
        return *this;
    }

    QString typeUrl() const;
    QByteArray value() const;
    void setTypeUrl(const QString &typeUrl);
    void setValue(const QByteArray &value);

    template <typename T>
    std::optional<T> as() const
    {
        if constexpr (std::is_same_v<T, Any>) {
            return asAnyImpl();
        } else {
            static_assert(QtProtobufPrivate::HasProtobufPropertyOrdering<T>,
                          "T must have the Q_PROTOBUF_OBJECT macro");
            T obj;
            if (asImpl(&obj, T::propertyOrdering))
                return { std::move(obj) };
        }
        return std::nullopt;
    }

    template <typename T>
    static Any fromMessage(const T &message, QAnyStringView typeUrlPrefix = defaultUrlPrefix())
    {
        if constexpr (std::is_same_v<T, Any>)
            return fromAnyMessageImpl(&message, typeUrlPrefix);

        static_assert(QtProtobufPrivate::HasProtobufPropertyOrdering<T>,
                      "T must have the Q_PROTOBUF_OBJECT macro");
        return fromMessageImpl(&message, T::propertyOrdering, typeUrlPrefix);
    }

private:
    AnyPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Any)

    bool asImpl(QProtobufMessage *message,
                QtProtobufPrivate::QProtobufPropertyOrdering ordering) const;
    std::optional<Any> asAnyImpl() const;
    static Any fromMessageImpl(const QProtobufMessage *message,
                               QtProtobufPrivate::QProtobufPropertyOrdering ordering,
                               QAnyStringView typeUrlPrefix);
    static Any fromAnyMessageImpl(const Any *message, QAnyStringView typeUrlPrefix);

    static QAnyStringView defaultUrlPrefix();

    [[nodiscard]]
    bool equals(const Any &other) const noexcept;
    friend bool operator==(const Any &lhs, const Any &rhs) noexcept
    {
        return lhs.equals(rhs);
    }
    friend bool operator!=(const Any &lhs, const Any &rhs) noexcept
    {
        return !lhs.equals(rhs);
    }
};
} // namespace QtProtobuf

QT_END_NAMESPACE

#endif // Q_PROTOBUF_ANYSUPPORT_H
