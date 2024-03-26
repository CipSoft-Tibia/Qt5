// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLLINTSUGGESTIONS_P_H
#define QMLLINTSUGGESTIONS_P_H

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

#include "qlanguageserver_p.h"
#include "qqmlcodemodel_p.h"

#include <optional>

QT_BEGIN_NAMESPACE
namespace QmlLsp {
struct LastLintUpdate
{
    std::optional<int> version;
    std::optional<QDateTime> invalidUpdatesSince;
};

class QmlLintSuggestions : public QLanguageServerModule
{
    Q_OBJECT
public:
    QmlLintSuggestions(QLanguageServer *server, QmlLsp::QQmlCodeModel *codeModel);

    QString name() const override { return QLatin1StringView("QmlLint Suggestions"); }
public Q_SLOTS:
    void diagnose(const QByteArray &uri);
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;

private:
    QMutex m_mutex;
    QHash<QByteArray, LastLintUpdate> m_lastUpdate;
    QLanguageServer *m_server;
    QmlLsp::QQmlCodeModel *m_codeModel;
};
} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QMLLINTSUGGESTIONS_P_H
