// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtNetworkAuth/qoauthhttpserverreplyhandler.h>

#include <QtCore>
#include <QtTest>
#include <QtNetwork>

typedef QSharedPointer<QNetworkReply> QNetworkReplyPtr;

class tst_QOAuthHttpServerReplyHandler : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void callback();
};

void tst_QOAuthHttpServerReplyHandler::callback()
{
    int count = 0;
    QOAuthHttpServerReplyHandler replyHandler;
    QUrlQuery query("callback=test");
    QVERIFY(replyHandler.isListening());
    QUrl callback(replyHandler.callback());
    QVERIFY(!callback.isEmpty());
    callback.setQuery(query);
    QEventLoop eventLoop;
    connect(&replyHandler, &QOAuthHttpServerReplyHandler::callbackReceived, [&](
            const QVariantMap &parameters) {
        for (auto item : query.queryItems()) {
            QVERIFY(parameters.contains(item.first));
            QCOMPARE(parameters[item.first].toString(), item.second);
        }
        count = parameters.size();
        eventLoop.quit();
    });

    QNetworkAccessManager networkAccessManager;
    QNetworkRequest request;
    request.setUrl(callback);
    QNetworkReplyPtr reply;
    reply.reset(networkAccessManager.get(request));
    eventLoop.exec();
    QCOMPARE(count, query.queryItems().size());
}

QTEST_MAIN(tst_QOAuthHttpServerReplyHandler)
#include "tst_oauthhttpserverreplyhandler.moc"
