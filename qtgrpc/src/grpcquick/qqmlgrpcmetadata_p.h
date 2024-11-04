// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLGRPCCMETADATA_P_H
#define QQMLGRPCCMETADATA_P_H

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

#include <QtCore/qmap.h>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtGrpc/qgrpcmetadata.h>
#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE

class Q_GRPCQUICK_EXPORT QQmlGrpcMetadata : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GrpcMetadata)
    QML_ADDED_IN_VERSION(6, 7)

    Q_PROPERTY(QVariantMap data READ data WRITE setData NOTIFY dataChanged)
public:
    QQmlGrpcMetadata(QObject *parent = nullptr);
    ~QQmlGrpcMetadata() override;

    const QGrpcMetadata &metadata() const { return m_metadata; }
    const QVariantMap &data() const { return m_variantdata; }
    void setData(const QVariantMap &data);

signals:
    void dataChanged();

private:
    QVariantMap m_variantdata;
    QGrpcMetadata m_metadata;
};

QT_END_NAMESPACE

#endif // QQMLGRPCCMETADATA_P_H
