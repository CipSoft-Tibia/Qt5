// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPSERVERCONFIGURATION_H
#define QHTTPSERVERCONFIGURATION_H

#include <QtHttpServer/qthttpserverglobal.h>

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QHttpServerConfigurationPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QHttpServerConfigurationPrivate)

class QHttpServerConfiguration
{
public:
    Q_HTTPSERVER_EXPORT QHttpServerConfiguration();
    Q_HTTPSERVER_EXPORT QHttpServerConfiguration(const QHttpServerConfiguration &other);
    QHttpServerConfiguration(QHttpServerConfiguration &&other) noexcept = default;
    Q_HTTPSERVER_EXPORT QHttpServerConfiguration &operator = (const QHttpServerConfiguration &other);

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QHttpServerConfiguration)
    void swap(QHttpServerConfiguration &other) noexcept { d.swap(other.d); }

    Q_HTTPSERVER_EXPORT ~QHttpServerConfiguration();

    Q_HTTPSERVER_EXPORT void setRateLimitPerSecond(quint32 maxRequests);
    Q_HTTPSERVER_EXPORT quint32 rateLimitPerSecond() const;

private:
    QExplicitlySharedDataPointer<QHttpServerConfigurationPrivate> d;

    friend Q_HTTPSERVER_EXPORT bool
    comparesEqual(const QHttpServerConfiguration &lhs, const QHttpServerConfiguration &rhs) noexcept;
    Q_DECLARE_EQUALITY_COMPARABLE(QHttpServerConfiguration)
};
Q_DECLARE_SHARED(QHttpServerConfiguration)

QT_END_NAMESPACE

#endif // QHTTPSERVERCONFIGURATION_H
