// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtHttpServer/qhttpserverresponder.h>
#include <QtHttpServer/qabstracthttpserver.h>
#include <QtHttpServer/qhttpserverrouter.h>
#include <QtHttpServer/qhttpserverrouterrule.h>

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtNetwork/qnetworkaccessmanager.h>

Q_DECLARE_METATYPE(QNetworkAccessManager::Operation);

QT_BEGIN_NAMESPACE

struct HttpServer : QAbstractHttpServer {
    QHttpServerRouter router;

    HttpServer() = default;

    template<typename ViewHandler>
    void route(const char *path, const QHttpServerRequest::Methods methods, ViewHandler &&viewHandler)
    {
        auto rule = std::make_unique<QHttpServerRouterRule>(
                path, methods,
                [this, viewHandler = std::forward<ViewHandler>(viewHandler)](
                        const QRegularExpressionMatch &match, const QHttpServerRequest &,
                        QHttpServerResponder &&responder) mutable {
                    auto boundViewHandler = router.bindCaptured(viewHandler, match);
                    boundViewHandler(std::move(responder));
                });

        router.addRule<ViewHandler>(std::move(rule));
    }

    template<typename ViewHandler>
    void route(const char *path, ViewHandler &&viewHandler)
    {
        route(path, QHttpServerRequest::Method::AnyKnown, std::forward<ViewHandler>(viewHandler));
    }

    bool handleRequest(const QHttpServerRequest &request, QHttpServerResponder &responder) override
    {
        return router.handleRequest(request, responder);
    }

    void missingHandler(const QHttpServerRequest &, QHttpServerResponder &&responder) override
    {
        responder.write(QHttpServerResponder::StatusCode::NotFound);
    }
};

class tst_QHttpServerRouter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void routerRule_data();
    void routerRule();
    void viewHandlerNoArg();
    void viewHandlerOneArg();
    void viewHandlerTwoArgs();
    void viewHandlerResponder();
    void viewHandlerRequest();
    void viewHandlerLastTwoSpecials();

private:
    HttpServer httpserver;
    QString urlBase;
};

static void getTest(QHttpServerResponder &&responder)
{
    responder.write(QString("get-test").toUtf8(), "text/plain");
}

void tst_QHttpServerRouter::initTestCase()
{
    auto pageHandler = [] (const quint64 &page, QHttpServerResponder &&responder) {
        responder.write(QString("page: %1").arg(page).toUtf8(), "text/plain");
    };

    httpserver.route("/page/", pageHandler);

    httpserver.route("/post-only", QHttpServerRequest::Method::Post,
                     [] (QHttpServerResponder &&responder) {
        responder.write(QString("post-test").toUtf8(), "text/plain");
    });

    httpserver.route("/get-only", QHttpServerRequest::Method::Get, getTest);

    urlBase = QStringLiteral("http://localhost:%1%2").arg(httpserver.listen());
}

void tst_QHttpServerRouter::routerRule_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("body");
    QTest::addColumn<QNetworkAccessManager::Operation>("replyType");

    QTest::addRow("/page/1")
        << "/page/1"
        << 200
        << "text/plain"
        << "page: 1"
        << QNetworkAccessManager::GetOperation;

    QTest::addRow("/page/-1")
        << "/page/-1"
        << 404
        << "application/x-empty"
        << ""
        << QNetworkAccessManager::GetOperation;

    QTest::addRow("/post-only [GET]")
        << "/post-only"
        << 404
        << "application/x-empty"
        << ""
        << QNetworkAccessManager::GetOperation;

    QTest::addRow("/post-only [DELETE]")
        << "/post-only"
        << 404
        << "application/x-empty"
        << ""
        << QNetworkAccessManager::DeleteOperation;

    QTest::addRow("/post-only [POST]")
        << "/post-only"
        << 200
        << "text/plain"
        << "post-test"
        << QNetworkAccessManager::PostOperation;

    QTest::addRow("/get-only [GET]")
        << "/get-only"
        << 200
        << "text/plain"
        << "get-test"
        << QNetworkAccessManager::GetOperation;

    QTest::addRow("/get-only [POST]")
        << "/get-only"
        << 404
        << "application/x-empty"
        << ""
        << QNetworkAccessManager::PostOperation;

    QTest::addRow("/get-only [DELETE]")
        << "/get-only"
        << 404
        << "application/x-empty"
        << ""
        << QNetworkAccessManager::DeleteOperation;
}

void tst_QHttpServerRouter::routerRule()
{
    QFETCH(QString, url);
    QFETCH(int, code);
    QFETCH(QString, type);
    QFETCH(QString, body);
    QFETCH(QNetworkAccessManager::Operation, replyType);

    QNetworkAccessManager networkAccessManager;
    QNetworkReply *reply;
    QNetworkRequest request(QUrl(urlBase.arg(url)));

    switch (replyType) {
    case QNetworkAccessManager::GetOperation:
        reply = networkAccessManager.get(request);
        break;
    case QNetworkAccessManager::PostOperation:
        request.setHeader(QNetworkRequest::ContentTypeHeader, type);
        reply = networkAccessManager.post(request, QByteArray("post body"));
        break;
    case QNetworkAccessManager::DeleteOperation:
        reply = networkAccessManager.deleteResource(request);
        break;
    default:
        QFAIL("The replyType does not supported");
    }

    QTRY_VERIFY(reply->isFinished());

    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), code);
    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader), type);
    QCOMPARE(reply->readAll(), body);
}

void tst_QHttpServerRouter::viewHandlerNoArg()
{
    auto viewNonArg = [] () {
    };

    using ViewTraits = QHttpServerRouterViewTraits<decltype(viewNonArg), true>;
    using Args = typename ViewTraits::Arguments;

    static_assert(Args::Count == 0,
                  "viewNonArg: Args::Count == 0");
    static_assert(Args::CapturableCount == 0,
                  "viewNonArg: Args::CapturableCount == 0");
    static_assert(Args::PlaceholdersCount == 0,
                  "viewNonArg: Args::PlaceholdersCount == 0");

    static_assert(Args::Valid, "viewNonArg: Args::Valid");
    static_assert(Args::StaticAssert, "viewNonArg: Args::StaticAssert");
}

void tst_QHttpServerRouter::viewHandlerOneArg()
{
    auto view = [] (const quint64 &) {
    };

    using ViewTraits = QHttpServerRouterViewTraits<decltype(view), true>;
    using Args = typename ViewTraits::Arguments;

    static_assert(Args::Count == 1,
                  "viewOneArg: Args::Count == 1");
    static_assert(Args::CapturableCount == 1,
                  "viewOneArg: Args::CapturableCount == 1");
    static_assert(Args::PlaceholdersCount == 0,
                  "viewOneArg: Args::PlaceholdersCount == 0");
    static_assert(Args::Last::IsRequest::Value == 0,
                  "viewOneArg: Args::Last::IsRequest::Value == 0");
    static_assert(Args::Last::IsRequest::Valid == 0,
                  "viewOneArg: Args::Last::IsRequest::Valid == 0");
    static_assert(Args::Last::IsResponder::Value == 0,
                  "viewOneArg: Args::Last::IsResponder::Value == 0");
    static_assert(Args::Last::IsResponder::Valid == 0,
                  "viewOneArg: Args::Last::IsResponder::Valid == 0");
    static_assert(Args::Last::IsSpecial::Value == 0,
                  "viewOneArg: Args::Last::IsSpecial::Value == 0");
    static_assert(Args::Last::IsSpecial::Valid == 0,
                  "viewOneArg: Args::Last::IsSpecial::Valid == 0");
    static_assert(Args::Last::IsSimple::Value,
                  "viewOneArg: Args::Last::IsSimple::Value");
    static_assert(Args::Last::IsSimple::Valid,
                  "viewOneArg: Args::Last::IsSimple::Valid");
    static_assert(Args::Last::Valid,
                  "viewOneArg: Args::Last::Valid");
    static_assert(Args::Last::StaticAssert,
                  "viewOneArg: Args::Last::StaticAssert");
    static_assert(std::is_same<Args::Last::Type, const quint64 &>::value,
                  "viewNonArg: std::is_same<Args::Last::Type, const quint64 &>");
    static_assert(Args::Valid, "viewOneArg: Args::Valid");
    static_assert(Args::StaticAssert, "viewOneArg: Args::StaticAssert");
}

void tst_QHttpServerRouter::viewHandlerTwoArgs()
{
    auto view = [] (const quint64 &, const QHttpServerResponder &) {
    };

    using ViewTraits = QHttpServerRouterViewTraits<decltype(view), true>;
    using Args = typename ViewTraits::Arguments;

    static_assert(Args::Count == 2,
                  "viewTwoArgs: Args::Count == 2");
    static_assert(Args::CapturableCount == 1,
                  "viewTwoArgs: Args::CapturableCount == 1");
    static_assert(Args::PlaceholdersCount == 1,
                  "viewTwoArgs: Args::PlaceholdersCount == 0");

    using Arg0 = typename Args::template Arg<0>;
    static_assert(Arg0::IsRequest::Value == 0,
                  "viewTwoArgs: Args::Arg0::IsRequest::Value == 0");
    static_assert(Arg0::IsRequest::Valid == 0,
                  "viewTwoArgs: Args::Arg0::IsRequest::Valid == 0");
    static_assert(Arg0::IsResponder::Value == 0,
                  "viewTwoArgs: Args::Arg0::IsResponder::Value == 0");
    static_assert(Arg0::IsResponder::Valid == 0,
                  "viewTwoArgs: Args::Arg0::IsResponder::Valid == 0");
    static_assert(Arg0::IsSpecial::Value == 0,
                  "viewTwoArgs: Args::Arg0::IsSpecial::Value == 0");
    static_assert(Arg0::IsSpecial::Valid == 0,
                  "viewTwoArgs: Args::Arg0::IsSpecial::Valid == 0");
    static_assert(Arg0::IsSimple::Value,
                  "viewTwoArgs: Args::Arg0::IsSimple::Value");
    static_assert(Arg0::IsSimple::Valid,
                  "viewTwoArgs: Args::Arg0::IsSimple::Valid");
    static_assert(Arg0::Valid,
                  "viewTwoArgs: Args::Arg0::Valid");
    static_assert(Arg0::StaticAssert,
                  "viewTwoArgs: Args::Arg0::StaticAssert");
    static_assert(std::is_same<Arg0::Type, const quint64 &>::value,
                  "viewNonArg: std::is_same<Args::Arg0::Type, const quint64>");

    using Arg1 = typename Args::template Arg<1>;
    static_assert(Arg1::IsRequest::Value == 0,
                  "viewTwoArgs: Args::Arg1::IsRequest::Value == 0");
    static_assert(Arg1::IsRequest::Valid == 0,
                  "viewTwoArgs: Args::Arg1::IsRequest::Valid == 0");
    static_assert(Arg1::IsResponder::Value,
                  "viewTwoArgs: Args::Arg1::IsResponder::Value");
    static_assert(Arg1::IsResponder::Valid == 0,
                  "viewTwoArgs: Args::Arg1::IsResponder::Valid == 0");
    static_assert(Arg1::IsSpecial::Value == 1,
                  "viewTwoArgs: Args::Arg1::IsSpecial::Value");
    static_assert(Arg1::IsSpecial::Valid == 0,
                  "viewTwoArgs: Args::Arg1::IsSpecial::Valid == 0");
    static_assert(Arg1::IsSimple::Value == 0,
                  "viewTwoArgs: Args::Arg1::IsSimple::Value == 0");
    static_assert(Arg1::IsSimple::Valid == 0,
                  "viewTwoArgs: Args::Arg1::IsSimple::Valid == 0");
    static_assert(Arg1::Valid == 0,
                  "viewTwoArgs: Args::Arg1::Valid");
    // StaticAssert is disabled in tests
    static_assert(Arg1::StaticAssert,
                  "viewOneArg: Args::Arg1::StaticAssert");
    static_assert(std::is_same<Arg1::Type, const QHttpServerResponder &>::value,
                  "viewTwoArgs: std::is_same<Args::Arg1::Type, const QHttpServerResponder &>)");

    static_assert(Args::Valid == 0, "viewTwoArgs: Args::Valid == 0");
    // StaticAssert is disabled in tests
    static_assert(Args::StaticAssert, "viewTwoArgs: Args::StaticAssert");
}

void tst_QHttpServerRouter::viewHandlerResponder()
{
    auto view = [] (QHttpServerResponder &&) {
    };

    using ViewTraits = QHttpServerRouterViewTraits<decltype(view), true>;
    using Args = typename ViewTraits::Arguments;

    static_assert(Args::Count == 1,
                  "viewResponder: Args::Count == 1");
    static_assert(Args::CapturableCount == 0,
                  "viewResponder: Args::CapturableCount == 0");
    static_assert(Args::PlaceholdersCount == 1,
                  "viewResponder: Args::PlaceholdersCount == 1");
    static_assert(Args::Last::IsRequest::Value == 0,
                  "viewResponder: Args::Last::IsRequest::Value == 0");
    static_assert(Args::Last::IsRequest::Valid == 0,
                  "viewResponder: Args::Last::IsRequest::Valid == 0");
    static_assert(Args::Last::IsResponder::Value,
                  "viewResponder: Args::Last::IsResponder::Value");
    static_assert(Args::Last::IsResponder::Valid,
                  "viewResponder: Args::Last::IsResponder::Valid");
    static_assert(Args::Last::IsSpecial::Value,
                  "viewResponder: Args::Last::IsSpecial::Value");
    static_assert(Args::Last::IsSpecial::Valid,
                  "viewResponder: Args::Last::IsSpecial::Valid");
    static_assert(Args::Last::IsSimple::Value == 0,
                  "viewResponder: Args::Last::IsSimple::Value == 0");
    static_assert(Args::Last::IsSimple::Valid == 0,
                  "viewResponder: Args::Last::IsSimple::Valid == 0");
    static_assert(Args::Last::Valid,
                  "viewResponder: Args::Last::Valid");
    static_assert(Args::Last::StaticAssert,
                  "viewResponder: Args::Last::StaticAssert");
    static_assert(std::is_same<Args::Last::Type, QHttpServerResponder &&>::value,
                  "viewNonArg: std::is_same<Args::Last::Type, QHttpServerResponder &&>");
    static_assert(Args::Valid, "viewResponder: Args::Valid");
    static_assert(Args::StaticAssert, "viewResponder: Args::StaticAssert");
}

void tst_QHttpServerRouter::viewHandlerRequest()
{
    auto view = [] (const QHttpServerRequest &) {
    };

    using ViewTraits = QHttpServerRouterViewTraits<decltype(view), true>;
    using Args = typename ViewTraits::Arguments;

    static_assert(Args::Count == 1,
                  "viewResponder: Args::Count == 1");
    static_assert(Args::CapturableCount == 0,
                  "viewResponder: Args::CapturableCount == 0");
    static_assert(Args::PlaceholdersCount == 1,
                  "viewResponder: Args::PlaceholdersCount == 1");
    static_assert(Args::Last::IsRequest::Value,
                  "viewResponder: Args::Last::IsRequest::Value");
    static_assert(Args::Last::IsRequest::Valid,
                  "viewResponder: Args::Last::IsRequest::Valid");
    static_assert(Args::Last::IsResponder::Value == 0,
                  "viewResponder: Args::Last::IsResponder::Value == 0");
    static_assert(Args::Last::IsResponder::Valid == 0,
                  "viewResponder: Args::Last::IsResponder::Valid == 0");
    static_assert(Args::Last::IsSpecial::Value,
                  "viewResponder: Args::Last::IsSpecial::Value");
    static_assert(Args::Last::IsSpecial::Valid,
                  "viewResponder: Args::Last::IsSpecial::Valid");
    static_assert(Args::Last::IsSimple::Value == 0,
                  "viewResponder: Args::Last::IsSimple::Value == 0");
    static_assert(Args::Last::IsSimple::Valid == 0,
                  "viewResponder: Args::Last::IsSimple::Valid == 0");
    static_assert(Args::Last::Valid,
                  "viewResponder: Args::Last::Valid");
    static_assert(Args::Last::StaticAssert,
                  "viewResponder: Args::Last::StaticAssert");
    static_assert(std::is_same<Args::Last::Type, const QHttpServerRequest &>::value,
                  "viewNonArg: std::is_same<Args::Last::Type, const QHttpServerRequest &>");
    static_assert(Args::Valid, "viewResponder: Args::Valid");
    static_assert(Args::StaticAssert, "viewResponder: Args::StaticAssert");
}

void tst_QHttpServerRouter::viewHandlerLastTwoSpecials()
{
    auto view = [] (const QHttpServerRequest &, QHttpServerResponder &&) {
    };

    using ViewTraits = QHttpServerRouterViewTraits<decltype(view), true>;
    using Args = typename ViewTraits::Arguments;

    static_assert(Args::Count == 2,
                  "viewTwoSpecialArgs: Args::Count == 2");
    static_assert(Args::CapturableCount == 0,
                  "viewTwoSpecialArgs: Args::CapturableCount == 1");
    static_assert(Args::PlaceholdersCount == 2,
                  "viewTwoSpecialArgs: Args::PlaceholdersCount == 0");

    using Arg0 = typename Args::template Arg<0>;
    static_assert(Arg0::IsRequest::Value,
                  "viewTwoSpecialArgs: Args::Arg0::IsRequest::Value");
    static_assert(Arg0::IsRequest::Valid,
                  "viewTwoSpecialArgs: Args::Arg0::IsRequest::Valid");
    static_assert(Arg0::IsResponder::Value == 0,
                  "viewTwoSpecialArgs: Args::Arg0::IsResponder::Value == 0");
    static_assert(Arg0::IsResponder::Valid == 0,
                  "viewTwoSpecialArgs: Args::Arg0::IsResponder::Valid == 0");
    static_assert(Arg0::IsSpecial::Value,
                  "viewTwoSpecialArgs: Args::Arg0::IsSpecial::Value");
    static_assert(Arg0::IsSpecial::Valid,
                  "viewTwoSpecialArgs: Args::Arg0::IsSpecial::Valid");
    static_assert(Arg0::IsSimple::Value == 0,
                  "viewTwoSpecialArgs: Args::Arg0::IsSimple::Value == 0");
    static_assert(Arg0::IsSimple::Valid == 0,
                  "viewTwoSpecialArgs: Args::Arg0::IsSimple::Valid == 0");
    static_assert(Arg0::Valid,
                  "viewTwoSpecialArgs: Args::Arg0::Valid");
    // StaticAssert is disabled in tests
    static_assert(Arg0::StaticAssert,
                  "viewTwoSpecialArgs: Args::Arg0::StaticAssert");
    static_assert(std::is_same<Arg0::Type, const QHttpServerRequest &>::value,
                  "viewNonArg: std::is_same<Args::Arg0::Type, const QHttpServerRequest &>");

    using Arg1 = typename Args::template Arg<1>;
    static_assert(Arg1::IsRequest::Value == 0,
                  "viewTwoSpecialArgs: Args::Arg1::IsRequest::Value == 0");
    static_assert(Arg1::IsRequest::Valid == 0,
                  "viewTwoSpecialArgs: Args::Arg1::IsRequest::Valid == 0");
    static_assert(Arg1::IsResponder::Value,
                  "viewTwoSpecialArgs: Args::Arg1::IsResponder::Value");
    static_assert(Arg1::IsResponder::Valid,
                  "viewTwoSpecialArgs: Args::Arg1::IsResponder::Valid");
    static_assert(Arg1::IsSpecial::Value,
                  "viewTwoSpecialArgs: Args::Arg1::IsSpecial::Value");
    static_assert(Arg1::IsSpecial::Valid,
                  "viewTwoSpecialArgs: Args::Arg1::IsSpecial::Valid");
    static_assert(Arg1::IsSimple::Value == 0,
                  "viewTwoSpecialArgs: Args::Arg1::IsSimple::Value == 0");
    static_assert(Arg1::IsSimple::Valid == 0,
                  "viewTwoSpecialArgs: Args::Arg1::IsSimple::Valid == 0");
    static_assert(Arg1::Valid,
                  "viewTwoSpecialArgs: Args::Arg1::Valid");
    static_assert(Arg1::StaticAssert,
                  "viewTwoSpecialArgs: Args::Arg1::StaticAssert");
    static_assert(std::is_same<Arg1::Type, QHttpServerResponder &&>::value,
                  "viewTwoSpecialArgs: std::is_same<Args::Arg1::Type, QHttpServerResponder &&>");

    static_assert(Args::Valid, "viewTwoSpecialArgs: Args::Valid");
    // StaticAssert is disabled in tests
    static_assert(Args::StaticAssert, "viewTwoSpecialArgs: Args::StaticAssert");
}

QT_END_NAMESPACE

QTEST_MAIN(tst_QHttpServerRouter)

#include "tst_qhttpserverrouter.moc"
