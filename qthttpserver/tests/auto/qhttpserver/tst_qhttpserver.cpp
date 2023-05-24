// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtHttpServer/qhttpserver.h>
#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qhttpserverrouterrule.h>

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtConcurrent/qtconcurrentrun.h>

#include <QtCore/qurl.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qtimer.h>

#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>

#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslkey.h>

#include <array>

#if QT_CONFIG(ssl)

static const char g_privateKey[] = R"(-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----)";

static const char g_certificate[] = R"(-----BEGIN CERTIFICATE-----
MIIFszCCA5ugAwIBAgIUfpP54qSLfus/pFUIBDizbnrDjE4wDQYJKoZIhvcNAQEL
BQAwaDELMAkGA1UEBhMCRlIxDzANBgNVBAgMBkZyYW5jZTERMA8GA1UEBwwIR3Jl
bm9ibGUxFjAUBgNVBAoMDVF0Q29udHJpYnV0b3IxHTAbBgNVBAMMFHFodHRwc3Nl
cnZlcnRlc3QuY29tMCAXDTIyMDIwNzE0MzE0NVoYDzIyNjgwNzA3MTQzMTQ1WjBo
MQswCQYDVQQGEwJGUjEPMA0GA1UECAwGRnJhbmNlMREwDwYDVQQHDAhHcmVub2Js
ZTEWMBQGA1UECgwNUXRDb250cmlidXRvcjEdMBsGA1UEAwwUcWh0dHBzc2VydmVy
dGVzdC5jb20wggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC92u1m1Wq7
CIbXb693c6NFWJt0rY5vvcM45aBvmsiBD4HZTNw4VrTGyJHJz9tqb0IQSqCflgdK
f3kske83d1uwDy+sWohdOSKwUC4cmqyaxhlsAiIJfy3W2VxSZCwmaNnUgqGTfuWA
YwqRzbw6H8IdtOuEllZ1DKgFs1vwp6pIeR49i7T8Ey07+WuRcmNXoFeArwGA6ol8
6qsYKwG9b6ypnP0PnVkEFVvdgKsFk/bQ1UDnFlSo4XBLeOf72E7VeUlzCIUDdXNT
u5JW3bOPOFfSIfDoY5VuNHghK2of/LShr1H2ZBrSGbNDstxX02KCFfmQzw+yBP2L
peG/aeayOk+t4tiBeheqkNGsvQ9HQJK2za5xF1+EyFjxWI2hqtnXZnYJNo3GIc8b
M1g0k9pAbjSjcsRtDcY/uKBnyCj999JX6RklLmO4AcKxS81QDiC8V9VcvHOgbqp7
AtdIJeioPIn1PuDgvzVbX2GwHFWnyAOpUCIGemD4ZMtoLr1Y6b04xcRE5M6WKfuD
GVDSlk77b8ze4UWj+FeUgf8xvk08qWk0GqAumojC0Yy6C3sPH0mgzBa8QGXMyB7J
xiY1fBIex1DmTEVLm5i5kSuLrZeuT9ZaV6/88MJF1xJSGuNVI2i0VC94YpLALnPY
U4AhmKgDSY/GwGLFJpBjx+GJoLnJ1TDgXQIDAQABo1MwUTAdBgNVHQ4EFgQUK7Un
0JA3DBUVhclrm6pIZsO60U4wHwYDVR0jBBgwFoAUK7Un0JA3DBUVhclrm6pIZsO6
0U4wDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAgEAuvSFAgfgurDT
/dbXuJ0O+FuGg4NOTNRil5ji3DnMzReIHpSiKiXu76PHHRFnlosvfAFOHlpYntun
LhbUAxim/iIgWZR33uzvqXMXBORZ0zffjy2SjGCW8ZJYyTmg9c0tc0jEjv7owtlU
m6tUXMOs9U0CzvEKLt0K0dMALaLkXtscuzEWA4PHVvnvTu0Wyjj/8n+DgYzY09kC
YF0lJfcG6bddDgspmYyFpULeGGP7+qwgGh4cVBtY5I4Htr3p7hDo6UGDF6AsMQZF
1CAEgBVRbJgI2GTnptpm9k3EFKwQ81z5O+NnP3ZsuuZ3CEVaPHyQf/POLAIhmZLt
0vS9qoRiS4uMUJDXz2kJFBOFHki073eMvHiKtlpYOlJXMQ4MkHCydjeeuhHcgUCq
ZDWuQMmq/8tMwf4YtvxYtXzAMVW9dM8BgWu2G8/JwPMGUGhLfKkHmc8dlQzGDe/W
K/uVHlJZNF4Y0eXVlq9DUhpvKOjGc8A208wQlsTUgPxljgJ2+4F3D+t0luc3h65m
25iw8eRGuYDoCQLG7u7MI0g8A0H+0h9Xrt8PQql86vmQhmTUhKfedVGOo2t2Bcfn
ignL7f4e1m2jh0oWTLhuP1hnVFN4KAKpVIJXhbEkH59cLCN6ARXiEHCM9rmK5Rgk
NQZlAZc2w1Ha9lqisaWWpt42QVhQM64=
-----END CERTIFICATE-----)";

#endif // QT_CONFIG(ssl)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QueryRequireRouterRule : public QHttpServerRouterRule
{
public:
    QueryRequireRouterRule(const QString &pathPattern,
                           const char *queryKey,
                           RouterHandler routerHandler)
        : QHttpServerRouterRule(pathPattern, std::move(routerHandler)),
          m_queryKey(queryKey)
    {
    }

    bool matches(const QHttpServerRequest &request, QRegularExpressionMatch *match) const override
    {
        if (QHttpServerRouterRule::matches(request, match)) {
            if (request.query().hasQueryItem(m_queryKey))
                return true;
        }

        return false;
    }

private:
    const char * m_queryKey;
};

class tst_QHttpServer final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void routeGet_data();
    void routeGet();
    void routeKeepAlive();
    void routePost_data();
    void routePost();
    void routeDelete_data();
    void routeDelete();
    void routeExtraHeaders();
    void invalidRouterArguments();
    void checkRouteLambdaCapture();
    void afterRequest();
    void disconnectedInEventLoop();
    void multipleRequests();
    void pipelinedRequests();
    void missingHandler();
    void pipelinedFutureRequests();
    void multipleResponses();

private:
    void checkReply(QNetworkReply *reply, const QString &response);

private:
    QHttpServer httpserver;
    QString urlBase;
    QString sslUrlBase;
    QNetworkAccessManager networkAccessManager;
};

struct CustomArg {
    int data = 10;

    CustomArg() {} ;
    CustomArg(const QString &urlArg) : data(urlArg.toInt()) {}
};

static void reqAndRespHandler(QHttpServerResponder &&resp, const QHttpServerRequest &req)
{
    resp.write(req.body(), "text/html"_ba);
}

static void testHandler(QHttpServerResponder &&responder)
{
    responder.write("test msg", "text/html"_ba);
}

class MessageHandler {
public:
    explicit MessageHandler(const char *message) : message(message) {}

    const char *operator()() const {
        return message;
    }
private:
    const char *message;
};

void tst_QHttpServer::initTestCase()
{
    httpserver.route("/req-and-resp", reqAndRespHandler);

    httpserver.route("/resp-and-req", [] (const QHttpServerRequest &req,
                                          QHttpServerResponder &&resp) {
        resp.write(req.body(), "text/html"_ba);
    });

    auto testHandlerPtr = testHandler;
    httpserver.route("/test", testHandlerPtr);

    auto l = []() { return "Hello world get"; };

    httpserver.route("/", QHttpServerRequest::Method::Get, l);

    httpserver.route("/", QHttpServerRequest::Method::Post, MessageHandler("Hello world post"));

    httpserver.route("/post-and-get",
                     QHttpServerRequest::Method::Get | QHttpServerRequest::Method::Post,
                     [](const QHttpServerRequest &request) {
                         if (request.method() == QHttpServerRequest::Method::Get)
                             return "Hello world get";
                         else if (request.method() == QHttpServerRequest::Method::Post)
                             return "Hello world post";

                         return "This should not work";
                     });

    httpserver.route(
            "/any", QHttpServerRequest::Method::AnyKnown, [](const QHttpServerRequest &request) {
                static const auto metaEnum = QMetaEnum::fromType<QHttpServerRequest::Method>();
                return metaEnum.valueToKey(static_cast<int>(request.method()));
            });

    httpserver.route("/page/", [] (const qint32 number) {
        return QString("page: %1").arg(number);
    });

    httpserver.route("/page/<arg>/detail", [] (const quint32 number) {
        return QString("page: %1 detail").arg(number);
    });

    httpserver.route("/user/", [] (const QString &name) {
        return QString("%1").arg(name);
    });

    httpserver.route("/user/<arg>/", [] (const QString &name, const QByteArray &ba) {
        return QString("%1-%2").arg(name).arg(QString::fromLatin1(ba));
    });

    httpserver.route("/test/", [] (const QUrl &url) {
        return QString("path: %1").arg(url.path());
    });

    httpserver.route("/api/v", [] (const float api) {
        return QString("api %1v").arg(api);
    });

    httpserver.route("/api/v<arg>/user/", [] (const float api, const quint64 user) {
        return QString("api %1v, user id - %2").arg(api).arg(user);
    });

    httpserver.route("/api/v<arg>/user/<arg>/settings", [] (const float api, const quint64 user,
                                                             const QHttpServerRequest &request) {
        const auto &role = request.query().queryItemValue(QString::fromLatin1("role"));
        const auto &fragment = request.url().fragment();

        return QString("api %1v, user id - %2, set settings role=%3#'%4'")
                   .arg(api).arg(user).arg(role, fragment);
    });

    httpserver.route<QueryRequireRouterRule>(
            "/custom/",
            "key",
            [] (const quint64 num, const QHttpServerRequest &request) {
        return QString("Custom router rule: %1, key=%2")
                    .arg(num)
                    .arg(request.query().queryItemValue("key"));
    });

    httpserver.router()->addConverter<CustomArg>("[+-]?\\d+"_L1);
    httpserver.route("/check-custom-type/", [] (const CustomArg &customArg) {
        return QString("data = %1").arg(customArg.data);
    });

    httpserver.route("/post-body", QHttpServerRequest::Method::Post,
                     [](const QHttpServerRequest &request) { return request.body(); });

    httpserver.route("/file/", [] (const QString &file) {
        return QHttpServerResponse::fromFile(QFINDTESTDATA("data/"_L1 + file));
    });

    httpserver.route("/json-object/", [] () {
        return QJsonObject{
            {"property", "test"},
            {"value", 1}
        };
    });

    httpserver.route("/json-array/", [] () {
        return QJsonArray{
            1, "2",
            QJsonObject{
                {"name", "test"}
            }
        };
    });

    httpserver.route("/data-and-custom-status-code/", []() {
        return QHttpServerResponse(QJsonObject{ { "key", "value" } },
                                   QHttpServerResponder::StatusCode::Accepted);
    });

    httpserver.route("/chunked/", [] (QHttpServerResponder &&responder) {
        responder.writeStatusLine(QHttpServerResponder::StatusCode::Ok);
        responder.writeHeaders({
                {"Content-Type", "text/plain"},
                {"Transfer-Encoding", "chunked"} });

        auto writeChunk = [&responder] (const char *message) {
            responder.writeBody(QByteArray::number(qstrlen(message), 16));
            responder.writeBody("\r\n");
            responder.writeBody(message);
            responder.writeBody("\r\n");
        };

        writeChunk("part 1 of the message, ");
        writeChunk("part 2 of the message");
        writeChunk("");
    });

    httpserver.route("/extra-headers", [] () {
        QHttpServerResponse resp("");
        resp.setHeader("Content-Type", "application/x-empty");
        resp.setHeader("Server", "test server");
        return resp;
    });

    httpserver.route("/processing", [](QHttpServerResponder &&responder) {
        responder.sendResponse(QHttpServerResponse(QHttpServerResponder::StatusCode::Processing));
        responder.sendResponse(QHttpServerResponse("done"));
    });

    httpserver.afterRequest([] (QHttpServerResponse &&resp) {
        return std::move(resp);
    });

#if QT_CONFIG(concurrent)
    httpserver.route("/future/", [] (int id) {
        return QtConcurrent::run([id] () -> QHttpServerResponse {
             if (id == 0)
                return QHttpServerResponse::StatusCode::NotFound;

            QTest::qSleep(500);
            return QHttpServerResponse("future is coming");
        });
    });

    httpserver.route("/user/<arg>/delayed/", [](const QString &user, unsigned long delayMs) {
        return QtConcurrent::run([user, delayMs]() -> QHttpServerResponse {
            QThread::msleep(delayMs);
            return user;
        });
    });
#endif

    quint16 port = httpserver.listen();
    if (!port)
        qCritical("Http server listen failed");

    urlBase = QStringLiteral("http://localhost:%1%2").arg(port);

#if QT_CONFIG(ssl)
    httpserver.sslSetup(QSslCertificate(g_certificate),
                        QSslKey(g_privateKey, QSsl::Rsa));

    port = httpserver.listen();
    if (!port)
        qCritical("Http server listen failed");

    sslUrlBase = QStringLiteral("https://localhost:%1%2").arg(port);

    const QList<QSslError> expectedSslErrors = {
        QSslError(QSslError::SelfSignedCertificate, QSslCertificate(g_certificate)),
        // Non-OpenSSL backends are not able to report a specific error code
        // for self-signed certificates.
        QSslError(QSslError::CertificateUntrusted, QSslCertificate(g_certificate)),
        QSslError(QSslError::HostNameMismatch, QSslCertificate(g_certificate)),
    };

    connect(&networkAccessManager, &QNetworkAccessManager::sslErrors,
            [expectedSslErrors](QNetworkReply *reply,
                                const QList<QSslError> &errors) {
        for (const auto &error: errors) {
            if (!expectedSslErrors.contains(error))
                qCritical() << "Got unexpected ssl error:" << error << error.certificate();
        }
        reply->ignoreSslErrors(expectedSslErrors);
    });
#endif
}

void tst_QHttpServer::routeGet_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("body");

    QTest::addRow("hello world")
        << urlBase.arg("/")
        << 200
        << "text/plain"
        << "Hello world get";

    QTest::addRow("test msg")
        << urlBase.arg("/test")
        << 200
        << "text/html"
        << "test msg";

    QTest::addRow("not found")
        << urlBase.arg("/not-found")
        << 404
        << "application/x-empty"
        << "";

    QTest::addRow("arg:int")
        << urlBase.arg("/page/10")
        << 200
        << "text/plain"
        << "page: 10";

    QTest::addRow("arg:-int")
        << urlBase.arg("/page/-10")
        << 200
        << "text/plain"
        << "page: -10";

    QTest::addRow("arg:uint")
        << urlBase.arg("/page/10/detail")
        << 200
        << "text/plain"
        << "page: 10 detail";

    QTest::addRow("arg:-uint")
        << urlBase.arg("/page/-10/detail")
        << 404
        << "application/x-empty"
        << "";

    QTest::addRow("arg:string")
        << urlBase.arg("/user/test")
        << 200
        << "text/plain"
        << "test";

    QTest::addRow("arg:string")
        << urlBase.arg("/user/test test ,!a+.")
        << 200
        << "text/plain"
        << "test test ,!a+.";

    QTest::addRow("arg:string,ba")
        << urlBase.arg("/user/james/bond")
        << 200
        << "text/plain"
        << "james-bond";

    QTest::addRow("arg:url")
        << urlBase.arg("/test/api/v0/cmds?val=1")
        << 200
        << "text/plain"
        << "path: api/v0/cmds";

    QTest::addRow("arg:float 5.1")
        << urlBase.arg("/api/v5.1")
        << 200
        << "text/plain"
        << "api 5.1v";

    QTest::addRow("arg:float 5.")
        << urlBase.arg("/api/v5.")
        << 200
        << "text/plain"
        << "api 5v";

    QTest::addRow("arg:float 6.0")
        << urlBase.arg("/api/v6.0")
        << 200
        << "text/plain"
        << "api 6v";

    QTest::addRow("arg:float,uint")
        << urlBase.arg("/api/v5.1/user/10")
        << 200
        << "text/plain"
        << "api 5.1v, user id - 10";

    QTest::addRow("arg:float,uint,query")
        << urlBase.arg("/api/v5.2/user/11/settings?role=admin")
        << 200
        << "text/plain"
        << "api 5.2v, user id - 11, set settings role=admin#''";

    // The fragment isn't actually sent via HTTP (it's information for the user agent)
    QTest::addRow("arg:float,uint, query+fragment")
        << urlBase.arg("/api/v5.2/user/11/settings?role=admin#tag")
        << 200
        << "text/plain"
        << "api 5.2v, user id - 11, set settings role=admin#''";

    QTest::addRow("custom route rule")
        << urlBase.arg("/custom/15")
        << 404
        << "application/x-empty"
        << "";

    QTest::addRow("custom route rule + query")
        << urlBase.arg("/custom/10?key=11&g=1")
        << 200
        << "text/plain"
        << "Custom router rule: 10, key=11";

    QTest::addRow("custom route rule + query key req")
        << urlBase.arg("/custom/10?g=1&key=12")
        << 200
        << "text/plain"
        << "Custom router rule: 10, key=12";

    QTest::addRow("post-and-get, get")
        << urlBase.arg("/post-and-get")
        << 200
        << "text/plain"
        << "Hello world get";

    QTest::addRow("invalid-rule-method, get")
        << urlBase.arg("/invalid-rule-method")
        << 404
        << "application/x-empty"
        << "";

    QTest::addRow("check custom type, data=1")
        << urlBase.arg("/check-custom-type/1")
        << 200
        << "text/plain"
        << "data = 1";

    QTest::addRow("any, get")
        << urlBase.arg("/any")
        << 200
        << "text/plain"
        << "Get";

    QTest::addRow("response from html file")
        << urlBase.arg("/file/text.html")
        << 200
        << "text/html"
        << "<html></html>";

    QTest::addRow("response from json file")
        << urlBase.arg("/file/application.json")
        << 200
        << "application/json"
        << "{ \"key\": \"value\" }";

    QTest::addRow("json-object")
        << urlBase.arg("/json-object/")
        << 200
        << "application/json"
        << "{\"property\":\"test\",\"value\":1}";

    QTest::addRow("json-array")
        << urlBase.arg("/json-array/")
        << 200
        << "application/json"
        << "[1,\"2\",{\"name\":\"test\"}]";

    QTest::addRow("data-and-custom-status-code")
            << urlBase.arg("/data-and-custom-status-code/") << 202 << "application/json"
            << "{\"key\":\"value\"}";

    QTest::addRow("chunked")
        << urlBase.arg("/chunked/")
        << 200
        << "text/plain"
        << "part 1 of the message, part 2 of the message";

#if QT_CONFIG(concurrent)
    QTest::addRow("future")
        << urlBase.arg("/future/1")
        << 200
        << "text/plain"
        << "future is coming";

    QTest::addRow("future-not-found")
        << urlBase.arg("/future/0")
        << 404
        << "application/x-empty"
        << "";
#endif

#if QT_CONFIG(ssl)

    QTest::addRow("hello world, ssl")
        << sslUrlBase.arg("/")
        << 200
        << "text/plain"
        << "Hello world get";

    QTest::addRow("post-and-get, get, ssl")
        << sslUrlBase.arg("/post-and-get")
        << 200
        << "text/plain"
        << "Hello world get";

    QTest::addRow("invalid-rule-method, get, ssl")
        << sslUrlBase.arg("/invalid-rule-method")
        << 404
        << "application/x-empty"
        << "";

    QTest::addRow("check custom type, data=1, ssl")
        << sslUrlBase.arg("/check-custom-type/1")
        << 200
        << "text/plain"
        << "data = 1";

#endif // QT_CONFIG(ssl)
}

void tst_QHttpServer::routeGet()
{
    QFETCH(QString, url);
    QFETCH(int, code);
    QFETCH(QString, type);
    QFETCH(QString, body);

    auto reply = networkAccessManager.get(QNetworkRequest(url));

    QTRY_VERIFY(reply->isFinished());

    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader), type);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), code);
    QCOMPARE(reply->readAll().trimmed(), body);

    reply->deleteLater();
}

void tst_QHttpServer::routeKeepAlive()
{
    httpserver.route("/keep-alive", [] (const QHttpServerRequest &req) -> QHttpServerResponse {
        if (!req.value("Connection").contains("keep-alive"))
            return QHttpServerResponse::StatusCode::NotFound;

        return QString("header: %1, query: %2, body: %3, method: %4")
            .arg(req.value("CustomHeader"),
                 req.url().query(),
                 req.body())
            .arg(static_cast<int>(req.method()));
    });

    QNetworkRequest request(urlBase.arg("/keep-alive"));
    request.setRawHeader(QByteArray("Connection"), QByteArray("keep-alive"));

    checkReply(networkAccessManager.get(request),
               QString("header: , query: , body: , method: %1")
                 .arg(static_cast<int>(QHttpServerRequest::Method::Get)));
    if (QTest::currentTestFailed())
        return;

    request.setUrl(urlBase.arg("/keep-alive?po=98"));
    request.setRawHeader("CustomHeader", "1");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/html"_ba);

    checkReply(networkAccessManager.post(request, QByteArray("test")),
               QString("header: 1, query: po=98, body: test, method: %1")
                 .arg(static_cast<int>(QHttpServerRequest::Method::Post)));
    if (QTest::currentTestFailed())
        return;

    request = QNetworkRequest(urlBase.arg("/keep-alive"));
    request.setRawHeader(QByteArray("Connection"), QByteArray("keep-alive"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/html"_ba);

    checkReply(networkAccessManager.post(request, QByteArray("")),
               QString("header: , query: , body: , method: %1")
                 .arg(static_cast<int>(QHttpServerRequest::Method::Post)));
    if (QTest::currentTestFailed())
        return;

    checkReply(networkAccessManager.get(request),
               QString("header: , query: , body: , method: %1")
                 .arg(static_cast<int>(QHttpServerRequest::Method::Get)));
    if (QTest::currentTestFailed())
        return;
}

void tst_QHttpServer::routePost_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("data");
    QTest::addColumn<QString>("body");

    QTest::addRow("hello world")
        << urlBase.arg("/")
        << 200
        << "text/plain"
        << ""
        << "Hello world post";

    QTest::addRow("post-and-get, post")
        << urlBase.arg("/post-and-get")
        << 200
        << "text/plain"
        << ""
        << "Hello world post";

    QTest::addRow("any, post")
        << urlBase.arg("/any")
        << 200
        << "text/plain"
        << ""
        << "Post";

    QTest::addRow("post-body")
        << urlBase.arg("/post-body")
        << 200
        << "text/plain"
        << "some post data"
        << "some post data";

    QString body;
    for (int i = 0; i < 10000; i++)
        body.append(QString::number(i));

    QTest::addRow("post-body - huge body, chunk test")
        << urlBase.arg("/post-body")
        << 200
        << "text/plain"
        << body
        << body;

    QTest::addRow("req-and-resp")
        << urlBase.arg("/req-and-resp")
        << 200
        << "text/html"
        << "test"
        << "test";

    QTest::addRow("resp-and-req")
        << urlBase.arg("/resp-and-req")
        << 200
        << "text/html"
        << "test"
        << "test";

#if QT_CONFIG(ssl)

    QTest::addRow("post-and-get, post, ssl")
        << sslUrlBase.arg("/post-and-get")
        << 200
        << "text/plain"
        << ""
        << "Hello world post";

    QTest::addRow("any, post, ssl")
        << sslUrlBase.arg("/any")
        << 200
        << "text/plain"
        << ""
        << "Post";

    QTest::addRow("post-body, ssl")
        << sslUrlBase.arg("/post-body")
        << 200
        << "text/plain"
        << "some post data"
        << "some post data";

    QTest::addRow("post-body - huge body, chunk test, ssl")
        << sslUrlBase.arg("/post-body")
        << 200
        << "text/plain"
        << body
        << body;

#endif // QT_CONFIG(ssl)
}

void tst_QHttpServer::routePost()
{
    QFETCH(QString, url);
    QFETCH(int, code);
    QFETCH(QString, type);
    QFETCH(QString, data);
    QFETCH(QString, body);

    QNetworkRequest request(url);
    if (data.size())
        request.setHeader(QNetworkRequest::ContentTypeHeader, "text/html"_ba);

    auto reply = networkAccessManager.post(request, data.toUtf8());

    QTRY_VERIFY(reply->isFinished());

    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader), type);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), code);
    QCOMPARE(reply->readAll(), body);

    reply->deleteLater();
}

void tst_QHttpServer::routeDelete_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("data");

    QTest::addRow("post-and-get, delete")
        << urlBase.arg("/post-and-get")
        << 404
        << "application/x-empty"
        << "";

    QTest::addRow("any, delete")
        << urlBase.arg("/any")
        << 200
        << "text/plain"
        << "Delete";

#if QT_CONFIG(ssl)

    QTest::addRow("post-and-get, delete, ssl")
        << sslUrlBase.arg("/post-and-get")
        << 404
        << "application/x-empty"
        << "";

    QTest::addRow("any, delete, ssl")
        << sslUrlBase.arg("/any")
        << 200
        << "text/plain"
        << "Delete";

#endif // QT_CONFIG(ssl)
}

void tst_QHttpServer::routeDelete()
{
    QFETCH(QString, url);
    QFETCH(int, code);
    QFETCH(QString, type);
    QFETCH(QString, data);

    auto reply = networkAccessManager.deleteResource(QNetworkRequest(url));

    QTRY_VERIFY(reply->isFinished());

    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader), type);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), code);

    reply->deleteLater();
}

void tst_QHttpServer::routeExtraHeaders()
{
    const QUrl requestUrl(urlBase.arg("/extra-headers"));
    auto reply = networkAccessManager.get(QNetworkRequest(requestUrl));

    QTRY_VERIFY(reply->isFinished());

    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader), "application/x-empty");
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    QCOMPARE(reply->header(QNetworkRequest::ServerHeader), "test server");
}

struct CustomType {
    CustomType() {}
    CustomType(const QString &) {}
};

void tst_QHttpServer::invalidRouterArguments()
{
    QTest::ignoreMessage(QtWarningMsg, "Can not find converter for type: QDateTime");
    QCOMPARE(
        httpserver.route("/datetime/", [] (const QDateTime &datetime) {
            return QString("datetime: %1").arg(datetime.toString());
        }),
        false);

    QTest::ignoreMessage(QtWarningMsg,
                         "CustomType has not registered a converter to QString. "
                         "Use QHttpServerRouter::addConveter<Type>(converter).");
    QCOMPARE(
        httpserver.route("/implicit-conversion-to-qstring-has-no-registered/",
                         [] (const CustomType &) {
            return "";
        }),
        false);
}

void tst_QHttpServer::checkRouteLambdaCapture()
{
    httpserver.route("/capture-this/", [this] () {
        return urlBase;
    });

    QString msg = urlBase + "/pod";
    httpserver.route("/capture-non-pod-data/", [&msg] () {
        return msg;
    });

    checkReply(networkAccessManager.get(
                   QNetworkRequest(QUrl(urlBase.arg("/capture-this/")))),
               urlBase);
    if (QTest::currentTestFailed())
        return;

    checkReply(networkAccessManager.get(
                   QNetworkRequest(QUrl(urlBase.arg("/capture-non-pod-data/")))),
               msg);
    if (QTest::currentTestFailed())
        return;
}

void tst_QHttpServer::afterRequest()
{
    httpserver.afterRequest([] (QHttpServerResponse &&resp,
                                const QHttpServerRequest &request) {
        if (request.url().path() == "/test-after-request")
            resp.setHeader("Arguments-Order-1", "resp, request");

        return std::move(resp);
    });

    httpserver.afterRequest([] (const QHttpServerRequest &request,
                                QHttpServerResponse &&resp) {
        if (request.url().path() == "/test-after-request")
            resp.setHeader("Arguments-Order-2", "request, resp");

        return std::move(resp);
    });

    const QUrl requestUrl(urlBase.arg("/test-after-request"));
    auto reply = networkAccessManager.get(QNetworkRequest(requestUrl));

    QTRY_VERIFY(reply->isFinished());

    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader), "application/x-empty");
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    QCOMPARE(reply->rawHeader("Arguments-Order-1"), "resp, request");
    QCOMPARE(reply->rawHeader("Arguments-Order-2"), "request, resp");
    reply->deleteLater();
}

void tst_QHttpServer::checkReply(QNetworkReply *reply, const QString &response) {
    QTRY_VERIFY(reply->isFinished());

    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader).toByteArray(), "text/plain");
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    QCOMPARE(reply->readAll(), response);

    reply->deleteLater();
};

void tst_QHttpServer::disconnectedInEventLoop()
{
    httpserver.route("/event-loop/", [] () {
        QEventLoop loop;
        QTimer::singleShot(1000, &loop, &QEventLoop::quit);
        loop.exec();
        return QHttpServerResponse::StatusCode::Ok;
    });

    const QUrl requestUrl(urlBase.arg("/event-loop/"));
    auto reply = networkAccessManager.get(QNetworkRequest(requestUrl));
    QTimer::singleShot(500, reply, &QNetworkReply::abort); // cancel connection
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();
}

void tst_QHttpServer::multipleRequests()
{
    // Test to ensure that the passed lambda is not moved away after the
    // first handled request
    httpserver.route("/do-not-move", [v = std::vector<int>{1, 2, 3}] () {
        return QString::number(v.size());
    });

    checkReply(networkAccessManager.get(
                   QNetworkRequest(QUrl(urlBase.arg("/do-not-move")))),
               "3");
    if (QTest::currentTestFailed())
        return;

    checkReply(networkAccessManager.get(
                   QNetworkRequest(QUrl(urlBase.arg("/do-not-move")))),
               "3");
    if (QTest::currentTestFailed())
        return;
}

void tst_QHttpServer::pipelinedRequests()
{
    QNetworkReply *replies[10];

    for (std::size_t i = 0; i < std::size(replies); i++) {
        QNetworkRequest req(QUrl(urlBase.arg("/user/") + QString::number(i)));
        req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
        replies[i] = networkAccessManager.get(req);
    }

    for (std::size_t i = 0; i < std::size(replies); i++)
        checkReply(replies[i], QString::number(i));
}

void tst_QHttpServer::missingHandler()
{
    const QUrl requestUrl(urlBase.arg("/missing"));
    auto reply = networkAccessManager.get(QNetworkRequest(requestUrl));
    QTRY_VERIFY(reply->isFinished());
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    reply->deleteLater();

    {
        auto guard = QScopeGuard([this]() { httpserver.setMissingHandler({}); });

        httpserver.setMissingHandler(
                [](const QHttpServerRequest &, QHttpServerResponder &&responder) {
                    responder.write(QHttpServerResponder::StatusCode::Ok);
                });

        reply = networkAccessManager.get(QNetworkRequest(requestUrl));
        QTRY_VERIFY(reply->isFinished());
        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
        reply->deleteLater();
    }

    reply = networkAccessManager.get(QNetworkRequest(requestUrl));
    QTRY_VERIFY(reply->isFinished());
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    reply->deleteLater();
}

#if QT_CONFIG(concurrent)
// Test that responses to pipelined requests come in correct order, see also: QTBUG-105202
void tst_QHttpServer::pipelinedFutureRequests()
{
    std::array<QNetworkReply *, 10> replies;
    QThreadPool::globalInstance()->setMaxThreadCount(static_cast<int>(replies.size()));

    for (std::size_t i = 0; i < replies.size(); i++) {
        auto delayMs = 1000 / replies.size() * (replies.size() - i);

        QString path = u"/user/%1/delayed/%2"_s.arg(i).arg(delayMs);
        QNetworkRequest req(QUrl(urlBase.arg(path)));
        req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
        replies[i] = networkAccessManager.get(req);
    }

    for (std::size_t i = 0; i < replies.size(); i++)
        checkReply(replies[i], QString::number(i));
}
#endif // QT_CONFIG(concurrent)

void tst_QHttpServer::multipleResponses()
{
    const QUrl requestUrl(urlBase.arg("/processing"));
    auto reply = networkAccessManager.get(QNetworkRequest(requestUrl));

    QTRY_VERIFY(reply->isFinished());

    QEXPECT_FAIL("", "QTBUG-108068: QNetworkAccessManager should ignore informational replies",
                 Abort);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    QCOMPARE(reply->header(QNetworkRequest::ContentTypeHeader), "text/plain");
    QCOMPARE(reply->readAll(), "done");
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(CustomArg);
Q_DECLARE_METATYPE(CustomType);

QTEST_MAIN(tst_QHttpServer)

#include "tst_qhttpserver.moc"
