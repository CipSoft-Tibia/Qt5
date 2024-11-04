// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLGRPCCHANNELOPTIONS_P_H
#define QQMLGRPCCHANNELOPTIONS_P_H

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

#include <QtGrpcQuick/qtgrpcquickexports.h>
#include <QtCore/qobject.h>
#include <QtGrpc/qgrpcchanneloptions.h>
#include <QtQml/qqmlregistration.h>

#if QT_CONFIG(ssl)
#include <QtQmlNetwork/private/qqmlsslconfiguration_p.h>
#endif // QT_CONFIG(ssl)

QT_BEGIN_NAMESPACE

class QQmlGrpcMetadata;
class QQmlGrpcChannelOptionsPrivate;
class Q_GRPCQUICK_EXPORT QQmlGrpcChannelOptions : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GrpcChannelOptions)
    QML_ADDED_IN_VERSION(6, 7)

    Q_PROPERTY(QUrl host READ host WRITE setHost NOTIFY hostChanged REQUIRED)
    Q_PROPERTY(qint64 deadline READ deadline WRITE setDeadline NOTIFY deadlineChanged)
    Q_PROPERTY(QQmlGrpcMetadata *metadata READ metadata WRITE setMetadata NOTIFY metadataChanged)
#if QT_CONFIG(ssl)
    Q_PROPERTY(QQmlSslConfiguration sslConfiguration READ sslConfiguration
                       WRITE setSslConfiguration NOTIFY sslConfigurationChanged)
#endif // QT_CONFIG(ssl)

public:
    QQmlGrpcChannelOptions(QObject *parent = nullptr);

    const QGrpcChannelOptions &options() const;
    QUrl host() const;
    void setHost(const QUrl &newUrl);
    qint64 deadline() const;
    void setDeadline(qint64 value);
    QQmlGrpcMetadata *metadata() const;
    void setMetadata(QQmlGrpcMetadata *value);
#if QT_CONFIG(ssl)
    QQmlSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QQmlSslConfiguration &config);
#endif // QT_CONFIG(ssl)

signals:
    void hostChanged();
    void deadlineChanged();
    void metadataChanged();
#if QT_CONFIG(ssl)
    void sslConfigurationChanged();
#endif // QT_CONFIG(ssl)

private:
    Q_DECLARE_PRIVATE(QQmlGrpcChannelOptions)
};

QT_END_NAMESPACE

#endif // QQMLGRPCCHANNELOPTIONS_P_H
