// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDVIEWSIGNALMANAGER_P_H
#define QANDROIDVIEWSIGNALMANAGER_P_H

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

#include <QtCore/qhash.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qmap.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QAndroidViewSignalManager : public QObject
{
public:
    explicit QAndroidViewSignalManager() : QObject() { }

    int qt_metacall(QMetaObject::Call call, int methodId, void **args) override;

    void removeConnection(int signalIdx);
    int addConnection(const QString &signalName,
                      const QJniArray<jclass> &argTypes,
                      const QJniObject &listener,
                      const QObject &rootView);

private:
    /*
        This will store the necessary information to call the listener
        when the signal is emitted, including the Java function name and
        signature so that we can call it quickly without recalculating those.
    */
    struct ConnectionInfo
    {
        QMetaObject::Connection connection;
        QJniObject listenerObject;
        QString qmlSignalName;

        QList<QMetaType::Type> qmlArgumentTypes;
        bool isPropertySignal;
        std::optional<int> qmlPropertyIndex; // Only filled if isPropertySignal
    };

    bool hasConnection(int signalIdx) const;
    // Key is the signal index
    QMap<int, ConnectionInfo> m_connections;
};

QT_END_NAMESPACE

#endif // QANDROIDVIEWSIGNALMANAGER_P_H
