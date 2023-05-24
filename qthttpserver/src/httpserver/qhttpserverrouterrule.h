// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERROUTERRULE_H
#define QHTTPSERVERROUTERRULE_H

#include <QtHttpServer/qhttpserverrequest.h>

#include <QtCore/qcontainerfwd.h>

#include <functional> // for std::function
#include <initializer_list>
#include <memory>

QT_BEGIN_NAMESPACE

class QString;
class QHttpServerRequest;
class QHttpServerResponder;
class QRegularExpressionMatch;
class QHttpServerRouter;

class QHttpServerRouterRulePrivate;
class Q_HTTPSERVER_EXPORT QHttpServerRouterRule
{
    Q_DECLARE_PRIVATE(QHttpServerRouterRule)
    Q_DISABLE_COPY_MOVE(QHttpServerRouterRule)

public:
    using RouterHandler = std::function<void(const QRegularExpressionMatch &,
                                             const QHttpServerRequest &, QHttpServerResponder &&)>;

    explicit QHttpServerRouterRule(const QString &pathPattern, RouterHandler routerHandler);
    explicit QHttpServerRouterRule(const QString &pathPattern,
                                   const QHttpServerRequest::Methods methods,
                                   RouterHandler routerHandler);
    virtual ~QHttpServerRouterRule();

protected:
    bool exec(const QHttpServerRequest &request, QHttpServerResponder &responder) const;

    bool hasValidMethods() const;

    bool createPathRegexp(std::initializer_list<QMetaType> metaTypes,
                          const QHash<QMetaType, QString> &converters);

    virtual bool matches(const QHttpServerRequest &request,
                         QRegularExpressionMatch *match) const;

    QHttpServerRouterRule(QHttpServerRouterRulePrivate *d);

private:
    std::unique_ptr<QHttpServerRouterRulePrivate> d_ptr;

    friend class QHttpServerRouter;
};

QT_END_NAMESPACE

#endif // QHTTPSERVERROUTERRULE_H
