// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTPROTOBUFQTTYPESCOMMON_P_H
#define QTPROTOBUFQTTYPESCOMMON_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtProtobufQtCoreTypes/qtprotobufqtcoretypesexports.h>

#include <QtProtobuf/qtprotobuftypes.h>
#include <QtProtobuf/qprotobufserializer.h>

#include <QtCore/qvariant.h>
#include <QtCore/private/qglobal_p.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace QtProtobufPrivate {

Q_DECL_COLD_FUNCTION Q_PROTOBUFQTCORETYPES_EXPORT void warnTypeConversionError();

template <typename T>
constexpr inline bool is_optional_v = false;
template <typename T>
constexpr inline bool is_optional_v<std::optional<T>> = true;

template<typename QType, typename PType>
void registerQtTypeHandler()
{
    registerHandler(QMetaType::fromType<QType>(),
                    { [](const QProtobufBaseSerializer *serializer, const QVariant &value,
                         const QProtobufPropertyOrderingInfo &info) {
                         auto do_convert = [](const QType &qtype) {
                             auto res = convert(qtype);
                             if constexpr (is_optional_v<decltype(res)>) {
                                 return res;
                             } else {
                                 return std::optional<PType>(std::move(res));
                             }
                         };

                         std::optional<PType> object = do_convert(value.value<QType>());
                         if (object) {
                             serializer->serializeObject(&(object.value()), PType::propertyOrdering,
                                                         info);
                         } else {
                             warnTypeConversionError();
                         }
                     },
                      [](const QProtobufBaseSerializer *serializer, QVariant &value) {
                          PType object;
                          serializer->deserializeObject(&object, PType::propertyOrdering);
                          auto res = convert(object);
                          if constexpr (is_optional_v<decltype(res)>) {
                              if (!res)
                                  warnTypeConversionError();
                              else
                                  value = QVariant::fromValue<QType>(*res);
                          } else {
                              value = QVariant::fromValue<QType>(res);
                          }
                      } });
}
} // namespace QtProtobufPrivate

QT_END_NAMESPACE

#endif // QTPROTOBUFQTTYPESCOMMON_P_H
