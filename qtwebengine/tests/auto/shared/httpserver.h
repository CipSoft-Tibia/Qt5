/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httpreqrep.h"

#include <QTcpServer>
#include <QUrl>

// Listens on a TCP socket and creates HttpReqReps for each connection.
//
// Usage:
//
//     HttpServer server;
//     connect(&server, &HttpServer::newRequest, [](HttpReqRep *rr) {
//         if (rr->requestPath() == "/myPage.html") {
//             rr->setResponseBody("<html><body>Hello, World!</body></html>");
//             rr->sendResponse();
//         }
//     });
//     QVERIFY(server.start());
//     /* do stuff */
//     QVERIFY(server.stop());
//
// HttpServer owns the HttpReqRep objects. The signal handler should not store
// references to HttpReqRep objects.
//
// Only if a handler calls sendResponse() will a response be actually sent. This
// means that multiple handlers can be connected to the signal, with different
// handlers responsible for different paths.
class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);
    explicit HttpServer(QTcpServer *server, const QString &protocol, QObject *parent = nullptr);

    ~HttpServer() override;

    // Must be called to start listening.
    //
    // Returns true if a TCP port has been successfully bound.
    Q_INVOKABLE Q_REQUIRED_RESULT bool start();

    // Stops listening and performs final error checks.
    Q_INVOKABLE Q_REQUIRED_RESULT bool stop();

    Q_INVOKABLE void setExpectError(bool b);

    // Full URL for given relative path
    Q_INVOKABLE QUrl url(const QString &path = QStringLiteral("/")) const;

    Q_INVOKABLE void setResourceDirs(const QStringList &dirs) { m_dirs = dirs; }

    Q_INVOKABLE void setHostDomain(const QString &host) { m_url.setHost(host); }

Q_SIGNALS:
    // Emitted after a HTTP request has been successfully parsed.
    void newRequest(HttpReqRep *reqRep);

private Q_SLOTS:
    void handleNewConnection();

private:
    QTcpServer *m_tcpServer;
    QUrl m_url;
    QStringList m_dirs;
    bool m_error = false;
    bool m_ignoreNewConnection = false;
    bool m_expectingError = false;
};

#endif // !HTTPSERVER_H
