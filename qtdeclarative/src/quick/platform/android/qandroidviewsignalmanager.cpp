// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "platform/android/qandroidviewsignalmanager_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>

#include <QtCore/private/qandroidtypeconverter_p.h>
#include <QtQuick/private/qandroidviewsignalmanager_p.h>

QT_BEGIN_NAMESPACE

QByteArray qmlTypeArgStringFromJavaTypeArgs(const QJniArray<jclass> &javaArgClasses)
{
    static const QHash<QByteArray, QMetaType::Type> javaToQMetaTypeMap = {
        { "java/lang/Void", QMetaType::Type::Void },
        { "java/lang/String", QMetaType::Type::QString },
        { "java/lang/Integer", QMetaType::Type::Int },
        { "java/lang/Double", QMetaType::Type::Double },
        { "java/lang/Float", QMetaType::Type::Float },
        { "java/lang/Boolean", QMetaType::Type::Bool }
    };

    QList<QByteArray> qmlArgTypes;
    for (const auto javaArgClass : javaArgClasses) {
        const auto javaArgClassName = QJniObject(javaArgClass).className();
        const auto javaArgMetaType =
                javaToQMetaTypeMap.value(javaArgClassName, QMetaType::Type::UnknownType);

        if (javaArgMetaType == QMetaType::Type::UnknownType) {
            qWarning() << "Unknown type for signal argument" << javaArgClassName;
            return "";
        }

        qmlArgTypes.emplace_back(QMetaType(javaArgMetaType).name());
    }
    return qmlArgTypes.join(',');
}

QByteArray qmlSignalSignature(const QString &signalName, const QJniArray<jclass> &javaArgClasses)
{
    const auto qmlArgTypes = qmlTypeArgStringFromJavaTypeArgs(javaArgClasses);
    return QMetaObject::normalizedSignature(
            qPrintable(QStringLiteral("%1(%2)").arg(signalName).arg(qmlArgTypes)));
}

QList<QMetaType::Type> metaMethodArgumentTypes(const QMetaMethod &method)
{
    QList<QMetaType::Type> types;
    for (auto i = 0; i < method.parameterCount(); ++i)
        types.push_back(static_cast<QMetaType::Type>(method.parameterType(i)));
    return types;
}

int indexOfSignal(const QMetaObject &metaObject, const QString &signalName,
                  const QJniArray<jclass> &javaArgClasses)
{
    int signalIndex = metaObject.indexOfSignal(qmlSignalSignature(signalName, javaArgClasses));
    if (signalIndex == -1) // Signal is maybe a parameterless property signal
        signalIndex =
                metaObject.indexOfSignal(QMetaObject::normalizedSignature(qPrintable(signalName)));
    return signalIndex;
}

std::optional<int> propertyIndexForSignal(const QMetaMethod &signal, const QMetaObject &object)
{
    for (int i = 0; i < object.propertyCount(); ++i) {
        const auto metaProperty = object.property(i);
        if (signal.methodSignature() == metaProperty.notifySignal().methodSignature())
            return metaProperty.propertyIndex();
    }
    return std::nullopt;
}

int QAndroidViewSignalManager::addConnection(const QString &signalName,
                                             const QJniArray<jclass> &argTypes,
                                             const QJniObject &listener,
                                             const QObject &rootView)
{
    const auto *rootMetaObject = rootView.metaObject();
    int signalIndex = indexOfSignal(*rootMetaObject, signalName, argTypes);
    if (signalIndex == -1) {
        qWarning("Failed to find matching signal from root object for signal: %s",
                 qPrintable(signalName));
        return -1;
    }

    if (hasConnection(signalIndex))
        return signalIndex;

    // Connect the signal to this class' qt_metacall
    const auto connection =
            QMetaObject::connect(&rootView, signalIndex, this, QObject::metaObject()->methodCount(),
                                 Qt::QueuedConnection);

    const auto signal = rootView.metaObject()->method(signalIndex);
    const auto propertyIndex = propertyIndexForSignal(signal, *rootMetaObject);
    const auto argumentTypes = metaMethodArgumentTypes(signal);

    m_connections.insert(signalIndex,
                         { .connection = connection,
                           .listenerObject = listener,
                           .qmlSignalName = signalName,
                           .qmlArgumentTypes = argumentTypes,
                           .isPropertySignal = propertyIndex.has_value(),
                           .qmlPropertyIndex = propertyIndex });

    return signalIndex;
}

QJniObject voidStarToQJniObject(const QMetaType::Type type, const void *data)
{
    switch (type) {
    case QMetaType::Type::Bool:
        return QtJniTypes::Boolean::construct(*reinterpret_cast<const bool *>(data));
    case QMetaType::Type::Int:
        return QtJniTypes::Integer::construct(*reinterpret_cast<const int *>(data));
    case QMetaType::Type::Double:
        return QtJniTypes::Double::construct(*reinterpret_cast<const double *>(data));
    case QMetaType::Type::Float:
        return QtJniTypes::Float::construct(*reinterpret_cast<const float *>(data));
    case QMetaType::Type::QString:
        return QtJniTypes::String::construct(*reinterpret_cast<const QString *>(data));
    default:
        qWarning() << "Unknown type for signal argument" << type;
        break;
    }
    return {};
}

QJniObject qVariantToQJniObject(const QVariant &data)
{
    const auto type = data.metaType().id();
    switch (type) {
    case QMetaType::Type::Bool:
        return QtJniTypes::Boolean::construct(data.toBool());
    case QMetaType::Type::Int:
        return QtJniTypes::Integer::construct(data.toInt());
    case QMetaType::Type::Double:
        return QtJniTypes::Double::construct(data.toDouble());
    case QMetaType::Type::Float:
        return QtJniTypes::Float::construct(data.toFloat());
    case QMetaType::Type::QString:
        return QtJniTypes::String::construct(data.toString());
    default:
        qWarning() << "Unknown type for signal argument" << type;
        break;
    }
    return {};
}

int QAndroidViewSignalManager::qt_metacall(QMetaObject::Call call, int methodId, void **args)
{
    Q_UNUSED(call);
    if (!m_connections.contains(senderSignalIndex())) {
        qWarning() << "QAndroidViewSignalManager::qt_metacall received unexpected signal";
        return methodId;
    }

    const auto connection = m_connections.value(senderSignalIndex());

    if (connection.isPropertySignal) {
        const auto senderMetaObject = sender()->metaObject();
        const auto property = senderMetaObject->property(connection.qmlPropertyIndex.value());
        const auto data = property.read(sender());

        connection.listenerObject.callMethod<void>("onSignalEmitted", connection.qmlSignalName,
                                                   qVariantToQJniObject(data));
    } else {
        QJniArray<QJniObject> data(connection.qmlArgumentTypes.size());
        for (auto i = 0; i < connection.qmlArgumentTypes.size(); ++i) {
            const auto argType = connection.qmlArgumentTypes.at(i);
            const auto receivedRawData = args[i + 1];

            if (receivedRawData == nullptr) {
                qWarning() << "Received null data for signal" << connection.qmlSignalName;
                return methodId;
            }

            const auto receivedData = voidStarToQJniObject(argType, receivedRawData);
            if (!receivedData.isValid()) {
                qWarning() << "Received invalid data for signal" << connection.qmlSignalName;
                return methodId;
            }

            data.setValue(i, receivedData);
        }

        if (data.size() == 0) { // Signal with no params, uses QtSignalListener<Void>
            QNativeInterface::QAndroidApplication::runOnAndroidMainThread([connection] {
                connection.listenerObject.callMethod<void>("onSignalEmitted",
                                                           connection.qmlSignalName,
                                                           QtJniTypes::Void::construct().object());
            });
        } else if (data.size() == 1) { // Signal with a single param, uses QtSignalListener<T>
            QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
                    [connection, signalArg = data.at(0)] {
                        connection.listenerObject.callMethod<void>("onSignalEmitted",
                                                                   connection.qmlSignalName,
                                                                   signalArg);
                    });
        } else { // Signal with multiple params, uses custom listener interface with Object[] API
            QNativeInterface::QAndroidApplication::runOnAndroidMainThread([connection, data] {
                connection.listenerObject.callMethod<void>("onSignalEmitted", data);
            });
        }
    }
    return methodId;
}

bool QAndroidViewSignalManager::hasConnection(int signalIdx) const
{
    return m_connections.contains(signalIdx);
}

void QAndroidViewSignalManager::removeConnection(int signalIdx)
{
    if (!hasConnection(signalIdx))
        return;
    const auto info = m_connections.value(signalIdx);
    QObject::disconnect(info.connection);
    m_connections.remove(signalIdx);
}

QT_END_NAMESPACE
