// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLGRPCCALLOPTIONS_P_H
#define QQMLGRPCCALLOPTIONS_P_H

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
#include <QtGrpcQuick/private/qqmlgrpcmetadata_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qglobal.h>
#include <QtGrpc/qgrpccalloptions.h>
#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE

class Q_GRPCQUICK_EXPORT QQmlGrpcCallOptions : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GrpcCallOptions)
    QML_ADDED_IN_VERSION(6, 7)

    Q_PROPERTY(qint64 deadline READ deadline WRITE setDeadline NOTIFY deadlineChanged)
    Q_PROPERTY(QQmlGrpcMetadata *metadata READ metadata WRITE setMetadata NOTIFY metadataChanged)

public:
    QQmlGrpcCallOptions(QObject *parent = nullptr);
    ~QQmlGrpcCallOptions();

    QGrpcCallOptions options() const;
    void setMaxRetryAttempts(qint64 value);
    qint64 deadline() const;
    void setDeadline(qint64 value);
    QQmlGrpcMetadata *metadata() const;
    void setMetadata(QQmlGrpcMetadata *value);

signals:
    void deadlineChanged();
    void metadataChanged();

private:
    QGrpcCallOptions m_options;
    QQmlGrpcMetadata *m_metadata;
};

QT_END_NAMESPACE

#endif // QQMLGRPCCALLOPTIONS_P_H
