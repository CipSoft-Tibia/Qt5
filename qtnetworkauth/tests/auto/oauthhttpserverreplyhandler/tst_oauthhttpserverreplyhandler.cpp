// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetworkAuth/qoauthhttpserverreplyhandler.h>

#include <QtCore>
#include <QtTest>
#include <QtNetwork>

typedef QSharedPointer<QNetworkReply> QNetworkReplyPtr;

static constexpr std::chrono::seconds Timeout(20);

using namespace Qt::StringLiterals;

class tst_QOAuthHttpServerReplyHandler : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void callback_data();
    void callback();
    void callbackCaching();
    void callbackWithQuery();
    void badCallbackUris_data();
    void badCallbackUris();
    void badCallbackWrongMethod();
};

void tst_QOAuthHttpServerReplyHandler::callback_data()
{
    QTest::addColumn<QString>("callbackPath");
    QTest::addColumn<QString>("uri");
    QTest::addColumn<bool>("success");

    QTest::newRow("default") << QString() << QString() << true;
    QTest::newRow("empty") << "" << QString() << true;
    QTest::newRow("ascii-path") << "/foobar" << QString() << true;
    QTest::newRow("utf8-path") << "/áéíóú" << QString() << true;
    QTest::newRow("questionmark") << "/?" << QString() << true;
    QTest::newRow("hash") << "/#" << QString() << true;

    QTest::newRow("default-fragment") << QString() << "/#shouldntsee" << true;
    QTest::newRow("default-query") << QString() << "/?some=query" << true;

    QTest::newRow("default-wrongpath") << QString() << "/foo" << false;
    QTest::newRow("changed-wrongpath") << "/foo" << "/bar" << false;
    QTest::newRow("changed-wrongpathprefix") << "/foo" << "/foobar" << false;
    QTest::newRow("changed-wrongpathprefixpath") << "/foo" << "/foo/bar" << false;
}

void tst_QOAuthHttpServerReplyHandler::callback()
{
    QFETCH(QString, callbackPath);
    QFETCH(QString, uri);
    QFETCH(bool, success);

    int count = 0;
    QOAuthHttpServerReplyHandler replyHandler;
    QVERIFY(replyHandler.isListening());
    connect(&replyHandler, &QOAuthHttpServerReplyHandler::callbackReceived, this, [&](
            const QVariantMap &) {
        ++count;
        QTestEventLoop::instance().exitLoop();
    });

    if (!callbackPath.isNull())
        replyHandler.setCallbackPath(callbackPath);
    QUrl callback(replyHandler.callback());
    QVERIFY(!callback.isEmpty());

    // maybe change the URL
    callback = callback.resolved(QUrl(uri));

    QNetworkAccessManager networkAccessManager;
    QNetworkRequest request(callback);
    QNetworkReplyPtr reply;
    reply.reset(networkAccessManager.get(request));
    connect(reply.get(), &QNetworkReply::finished, &QTestEventLoop::instance(),
            &QTestEventLoop::exitLoop);

    if (!success) {
        QByteArray httpUri = callback.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemoveFragment);
        QTest::ignoreMessage(QtWarningMsg, "Invalid request: " + httpUri);
        QTest::ignoreMessage(QtWarningMsg, "Invalid request: " + httpUri);
    }
    QTestEventLoop::instance().enterLoop(Timeout);
    QCOMPARE(count > 0, success);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_QOAuthHttpServerReplyHandler::callbackCaching()
{
    QOAuthHttpServerReplyHandler replyHandler;
    constexpr auto callbackPath = "/foo"_L1;
    constexpr auto callbackHost = "localhost"_L1;

    QVERIFY(replyHandler.isListening());
    replyHandler.setCallbackPath(callbackPath);
    QUrl callback = replyHandler.callback();
    QCOMPARE(callback.path(), callbackPath);
    QCOMPARE(callback.host(), callbackHost);

    replyHandler.close();
    QVERIFY(!replyHandler.isListening());
    callback = replyHandler.callback();
    // Should remain after close
    QCOMPARE(callback.path(), callbackPath);
    QCOMPARE(callback.host(), callbackHost);

    replyHandler.listen();
    QVERIFY(replyHandler.isListening());
    callback = replyHandler.callback();
    QCOMPARE(callback.path(), callbackPath);
    QCOMPARE(callback.host(), callbackHost);
}

void tst_QOAuthHttpServerReplyHandler::callbackWithQuery()
{
    int count = 0;
    QOAuthHttpServerReplyHandler replyHandler;
    QUrlQuery query("callback=test");
    QVERIFY(replyHandler.isListening());
    QUrl callback(replyHandler.callback());
    QVERIFY(!callback.isEmpty());
    callback.setQuery(query);

    connect(&replyHandler, &QOAuthHttpServerReplyHandler::callbackReceived, this, [&](
            const QVariantMap &parameters) {
        for (auto item : query.queryItems()) {
            QVERIFY(parameters.contains(item.first));
            QCOMPARE(parameters[item.first].toString(), item.second);
        }
        count = parameters.size();
        QTestEventLoop::instance().exitLoop();
    });

    QNetworkAccessManager networkAccessManager;
    QNetworkRequest request;
    request.setUrl(callback);
    QNetworkReplyPtr reply;
    reply.reset(networkAccessManager.get(request));
    connect(reply.get(), &QNetworkReply::finished, &QTestEventLoop::instance(),
            &QTestEventLoop::exitLoop);
    QTestEventLoop::instance().enterLoop(Timeout);
    QCOMPARE(count, query.queryItems().size());
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_QOAuthHttpServerReplyHandler::badCallbackUris_data()
{
    QTest::addColumn<QString>("uri");

    QTest::newRow("relative-path") << "foobar";
    QTest::newRow("encoded-slash") << "%2F";
    QTest::newRow("query") << "?some=query";
    QTest::newRow("full-url") << "http://localhost/";
    QTest::newRow("authority") << "//localhost";
    // requires QUrl fix
    //QTest::newRow("double-slash") << "//";
    //QTest::newRow("triple-slash") << "///";
}

void tst_QOAuthHttpServerReplyHandler::badCallbackUris()
{
    QFETCH(QString, uri);

    int count = 0;
    QOAuthHttpServerReplyHandler replyHandler;
    QVERIFY(replyHandler.isListening());
    connect(&replyHandler, &QOAuthHttpServerReplyHandler::callbackReceived, this, [&](
            const QVariantMap &) {
        ++count;
        QTestEventLoop::instance().exitLoop();
    });
    QUrl callback(replyHandler.callback());
    QVERIFY(!callback.isEmpty());

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, replyHandler.port());
    socket.write("GET " + uri.toLocal8Bit() + " HTTP/1.0\r\n"
                 "Host: localhost\r\n"
                 "\r\n");
    connect(&socket, &QTcpSocket::disconnected, &QTestEventLoop::instance(),
            &QTestEventLoop::exitLoop);

    QTest::ignoreMessage(QtWarningMsg, "Invalid request: " + uri.toLocal8Bit());
    QTest::ignoreMessage(QtWarningMsg, "Invalid URL");

    QTestEventLoop::instance().enterLoop(Timeout);
    QCOMPARE(count, 0);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_QOAuthHttpServerReplyHandler::badCallbackWrongMethod()
{
    int count = 0;
    QOAuthHttpServerReplyHandler replyHandler;
    QVERIFY(replyHandler.isListening());
    connect(&replyHandler, &QOAuthHttpServerReplyHandler::callbackReceived, this, [&](
            const QVariantMap &) {
        ++count;
        QTestEventLoop::instance().exitLoop();
    });
    QUrl callback(replyHandler.callback());
    QVERIFY(!callback.isEmpty());

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, replyHandler.port());
    socket.write("EHLO localhost\r\n");
    connect(&socket, &QTcpSocket::disconnected, &QTestEventLoop::instance(),
            &QTestEventLoop::exitLoop);

    QTest::ignoreMessage(QtWarningMsg, "Invalid operation EHLO");
    QTest::ignoreMessage(QtWarningMsg, "Invalid Method");

    QTestEventLoop::instance().enterLoop(Timeout);
    QCOMPARE(count, 0);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

QTEST_MAIN(tst_QOAuthHttpServerReplyHandler)
#include "tst_oauthhttpserverreplyhandler.moc"
