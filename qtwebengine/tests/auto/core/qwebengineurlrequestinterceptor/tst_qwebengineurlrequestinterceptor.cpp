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

#include "../../widgets/util.h"
#include <QtTest/QtTest>
#include <QtWebEngineCore/qwebengineurlrequestinfo.h>
#include <QtWebEngineCore/qwebengineurlrequestinterceptor.h>
#include <QtWebEngineWidgets/qwebenginepage.h>
#include <QtWebEngineWidgets/qwebengineprofile.h>
#include <QtWebEngineWidgets/qwebenginesettings.h>

#include <httpserver.h>
#include <httpreqrep.h>

typedef void (QWebEngineProfile::*InterceptorSetter)(QWebEngineUrlRequestInterceptor *interceptor);
Q_DECLARE_METATYPE(InterceptorSetter)
class tst_QWebEngineUrlRequestInterceptor : public QObject
{
    Q_OBJECT

public:
    tst_QWebEngineUrlRequestInterceptor();
    ~tst_QWebEngineUrlRequestInterceptor();

public Q_SLOTS:
    void init();
    void cleanup();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void interceptRequest_data();
    void interceptRequest();
    void ipv6HostEncoding_data();
    void ipv6HostEncoding();
    void requestedUrl_data();
    void requestedUrl();
    void setUrlSameUrl_data();
    void setUrlSameUrl();
    void firstPartyUrl_data();
    void firstPartyUrl();
    void firstPartyUrlNestedIframes_data();
    void firstPartyUrlNestedIframes();
    void requestInterceptorByResourceType_data();
    void requestInterceptorByResourceType();
    void firstPartyUrlHttp_data();
    void firstPartyUrlHttp();
    void customHeaders_data();
    void customHeaders();
    void initiator_data();
    void initiator();
    void jsServiceWorker_data();
    void jsServiceWorker();
    void replaceInterceptor_data();
    void replaceInterceptor();
    void replaceOnIntercept();
    void multipleRedirects();
};

tst_QWebEngineUrlRequestInterceptor::tst_QWebEngineUrlRequestInterceptor()
{
}

tst_QWebEngineUrlRequestInterceptor::~tst_QWebEngineUrlRequestInterceptor()
{
}

void tst_QWebEngineUrlRequestInterceptor::init()
{
}

void tst_QWebEngineUrlRequestInterceptor::cleanup()
{
}

void tst_QWebEngineUrlRequestInterceptor::initTestCase()
{
}

void tst_QWebEngineUrlRequestInterceptor::cleanupTestCase()
{
}

struct RequestInfo {
    RequestInfo(QWebEngineUrlRequestInfo &info)
        : requestUrl(info.requestUrl())
        , firstPartyUrl(info.firstPartyUrl())
        , initiator(info.initiator())
        , resourceType(info.resourceType())
    {}

    QUrl requestUrl;
    QUrl firstPartyUrl;
    QUrl initiator;
    int resourceType;
};

static const QUrl kRedirectUrl = QUrl("qrc:///resources/content.html");

Q_LOGGING_CATEGORY(lc, "qt.webengine.tests")

class TestRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
public:
    QList<RequestInfo> requestInfos;
    bool shouldRedirect = false;
    QUrl redirectUrl;
    QMap<QUrl, QSet<QUrl>> requestInitiatorUrls;
    QMap<QByteArray, QByteArray> headers;
    std::function<bool (QWebEngineUrlRequestInfo &)> onIntercept;

    void interceptRequest(QWebEngineUrlRequestInfo &info) override
    {
        QCOMPARE(QThread::currentThread() == QCoreApplication::instance()->thread(), !property("deprecated").toBool());
        qCDebug(lc) << this << "Type:" << info.resourceType() << info.requestMethod() << "Navigation:" << info.navigationType()
                    << info.requestUrl() << "Initiator:" << info.initiator();

        // Since 63 we also intercept some unrelated blob requests..
        if (info.requestUrl().scheme() == QLatin1String("blob"))
            return;

        if (onIntercept && !onIntercept(info))
            return;

        bool block = info.requestMethod() != QByteArrayLiteral("GET");
        bool redirect = shouldRedirect && info.requestUrl() != redirectUrl;

        // set additional headers if any required by test
        for (auto it = headers.begin(); it != headers.end(); ++it) info.setHttpHeader(it.key(), it.value());

        if (block) {
            info.block(true);
        } else if (redirect) {
            info.redirect(redirectUrl);
        }

        requestInitiatorUrls[info.requestUrl()].insert(info.initiator());
        requestInfos.append(info);

        // MEMO avoid unintentionally changing request when it is not needed for test logic
        //      since api behavior depends on 'changed' state of the info object
        Q_ASSERT(info.changed() == (block || redirect || !headers.empty()));
    }

    bool shouldSkipRequest(const RequestInfo &requestInfo)
    {
        if (requestInfo.resourceType ==  QWebEngineUrlRequestInfo::ResourceTypeMainFrame ||
                requestInfo.resourceType == QWebEngineUrlRequestInfo::ResourceTypeSubFrame)
            return false;

        // Skip import documents and sandboxed documents.
        // See Document::SiteForCookies() in chromium/third_party/blink/renderer/core/dom/document.cc.
        return requestInfo.firstPartyUrl == QUrl("");
    }

    QList<RequestInfo> getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceType type)
    {
        QList<RequestInfo> infos;

        foreach (auto requestInfo, requestInfos) {
            if (shouldSkipRequest(requestInfo))
                continue;

            if (type == requestInfo.resourceType)
                infos.append(requestInfo);
        }

        return infos;
    }

    bool hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceType type)
    {
        foreach (auto requestInfo, requestInfos) {
            if (shouldSkipRequest(requestInfo))
                continue;

            if (type == requestInfo.resourceType)
                return true;
        }

        return false;
    }

    TestRequestInterceptor(bool redirect = false, const QUrl &url = kRedirectUrl)
        : shouldRedirect(redirect), redirectUrl(url)
    {
    }
};

class TestMultipleRedirectsInterceptor : public QWebEngineUrlRequestInterceptor {
public:
    QList<RequestInfo> requestInfos;
    QMap<QUrl, QUrl> redirectPairs;
    int redirectCount = 0;
    void interceptRequest(QWebEngineUrlRequestInfo &info) override
    {
        QVERIFY(QThread::currentThread() == QCoreApplication::instance()->thread());
        qCDebug(lc) << this << "Type:" << info.resourceType() << info.requestMethod() << "Navigation:" << info.navigationType()
                    << info.requestUrl() << "Initiator:" << info.initiator();
        auto redirectUrl = redirectPairs.constFind(info.requestUrl());
        if (redirectUrl != redirectPairs.constEnd()) {
          info.redirect(redirectUrl.value());
          requestInfos.append(info);
          redirectCount++;
        }
    }

    TestMultipleRedirectsInterceptor()
    {
    }
};

class ConsolePage : public QWebEnginePage {
    Q_OBJECT
public:
    ConsolePage(QWebEngineProfile* profile) : QWebEnginePage(profile) {}

    virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID)
    {
        levels.append(level);
        messages.append(message);
        lineNumbers.append(lineNumber);
        sourceIDs.append(sourceID);
    }

    QList<int> levels;
    QStringList messages;
    QList<int> lineNumbers;
    QStringList sourceIDs;
};

void tst_QWebEngineUrlRequestInterceptor::interceptRequest_data()
{
    QTest::addColumn<InterceptorSetter>("setter");
    QTest::newRow("ui") << &QWebEngineProfile::setUrlRequestInterceptor;
    QTest::newRow("io") << &QWebEngineProfile::setRequestInterceptor;
}

void tst_QWebEngineUrlRequestInterceptor::interceptRequest()
{
    QFETCH(InterceptorSetter, setter);
    QWebEngineProfile profile;
    profile.settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    TestRequestInterceptor interceptor(/* intercept */ false);
    (profile.*setter)(&interceptor);
    QWebEnginePage page(&profile);
    QSignalSpy loadSpy(&page, SIGNAL(loadFinished(bool)));

    page.load(QUrl("qrc:///resources/index.html"));
    QTRY_COMPARE(loadSpy.count(), 1);
    QVariant success = loadSpy.takeFirst().takeFirst();
    QVERIFY(success.toBool());
    loadSpy.clear();
    QVariant ok;

    page.runJavaScript("post();", [&ok](const QVariant result){ ok = result; });
    QTRY_VERIFY(ok.toBool());
    QTRY_COMPARE(loadSpy.count(), 1);
    success = loadSpy.takeFirst().takeFirst();
    // We block non-GET requests, so this should not succeed.
    QVERIFY(!success.toBool());
    loadSpy.clear();

    interceptor.shouldRedirect = true;
    page.load(QUrl("qrc:///resources/__placeholder__"));
    QTRY_COMPARE(loadSpy.count(), 1);
    success = loadSpy.takeFirst().takeFirst();
    // The redirection for __placeholder__ should succeed.
    QVERIFY(success.toBool());
    loadSpy.clear();
    QCOMPARE(interceptor.requestInfos.count(), 4);

    // Make sure that registering an observer does not modify the request.
    TestRequestInterceptor observer(/* intercept */ false);
    (profile.*setter)(&observer);
    page.load(QUrl("qrc:///resources/__placeholder__"));
    QTRY_COMPARE(loadSpy.count(), 1);
    success = loadSpy.takeFirst().takeFirst();
    // Since we do not intercept, loading an invalid path should not succeed.
    QVERIFY(!success.toBool());
    QCOMPARE(observer.requestInfos.count(), 1);
}

class LocalhostContentProvider : public QWebEngineUrlRequestInterceptor
{
public:
    LocalhostContentProvider() { }

    void interceptRequest(QWebEngineUrlRequestInfo &info) override
    {
        // Since 63 we also intercept the original data requests
        if (info.requestUrl().scheme() == QLatin1String("data"))
            return;
        if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeFavicon)
            return;

        requestedUrls.append(info.requestUrl());
        info.redirect(QUrl("data:text/html,<p>hello"));
    }

    QList<QUrl> requestedUrls;
};

void tst_QWebEngineUrlRequestInterceptor::ipv6HostEncoding_data()
{
    interceptRequest_data();
}

void tst_QWebEngineUrlRequestInterceptor::ipv6HostEncoding()
{
    QFETCH(InterceptorSetter, setter);
    QWebEngineProfile profile;
    LocalhostContentProvider contentProvider;
    (profile.*setter)(&contentProvider);

    QWebEnginePage page(&profile);
    QSignalSpy spyLoadFinished(&page, SIGNAL(loadFinished(bool)));

    page.setHtml("<p>Hi", QUrl::fromEncoded("http://[::1]/index.html"));
    QTRY_COMPARE(spyLoadFinished.count(), 1);
    QCOMPARE(contentProvider.requestedUrls.count(), 0);

    evaluateJavaScriptSync(&page, "var r = new XMLHttpRequest();"
            "r.open('GET', 'http://[::1]/test.xml', false);"
            "r.send(null);"
            );

    QCOMPARE(contentProvider.requestedUrls.count(), 1);
    QCOMPARE(contentProvider.requestedUrls.at(0), QUrl::fromEncoded("http://[::1]/test.xml"));
}

void tst_QWebEngineUrlRequestInterceptor::requestedUrl_data()
{
    QTest::addColumn<InterceptorSetter>("setter");
    QTest::addColumn<bool>("interceptInPage");
    QTest::newRow("ui profile intercept") << &QWebEngineProfile::setUrlRequestInterceptor << false;
    QTest::newRow("ui page intercept") << &QWebEngineProfile::setUrlRequestInterceptor << true;
    QTest::newRow("io profile intercept") << &QWebEngineProfile::setRequestInterceptor << false;
}

void tst_QWebEngineUrlRequestInterceptor::requestedUrl()
{
    QFETCH(InterceptorSetter, setter);
    QFETCH(bool, interceptInPage);

    QWebEngineProfile profile;
    profile.settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    TestRequestInterceptor interceptor(/* intercept */ true);
    if (!interceptInPage)
        (profile.*setter)(&interceptor);

    QWebEnginePage page(&profile);
    if (interceptInPage)
        page.setUrlRequestInterceptor(&interceptor);
    QSignalSpy spy(&page, SIGNAL(loadFinished(bool)));

    page.setUrl(QUrl("qrc:///resources/__placeholder__"));
    QVERIFY(spy.wait());
    QTRY_COMPARE(spy.count(), 1);
    QVERIFY(interceptor.requestInfos.count() >= 1);
    QCOMPARE(interceptor.requestInfos.at(0).requestUrl, QUrl("qrc:///resources/content.html"));
    QCOMPARE(page.requestedUrl(), QUrl("qrc:///resources/__placeholder__"));
    QCOMPARE(page.url(), QUrl("qrc:///resources/content.html"));

    interceptor.shouldRedirect = false;

    page.setUrl(QUrl("qrc:/non-existent.html"));
    QTRY_COMPARE(spy.count(), 2);
    QVERIFY(interceptor.requestInfos.count() >= 3);
    QCOMPARE(interceptor.requestInfos.at(2).requestUrl, QUrl("qrc:/non-existent.html"));
    QCOMPARE(page.requestedUrl(), QUrl("qrc:///resources/__placeholder__"));
    QCOMPARE(page.url(), QUrl("qrc:///resources/content.html"));

    page.setUrl(QUrl("http://abcdef.abcdef"));
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 3, 15000);
    QVERIFY(interceptor.requestInfos.count() >= 4);
    QCOMPARE(interceptor.requestInfos.at(3).requestUrl, QUrl("http://abcdef.abcdef/"));
    QCOMPARE(page.requestedUrl(), QUrl("qrc:///resources/__placeholder__"));
    QCOMPARE(page.url(), QUrl("qrc:///resources/content.html"));
}

void tst_QWebEngineUrlRequestInterceptor::setUrlSameUrl_data()
{
    requestedUrl_data();
}

void tst_QWebEngineUrlRequestInterceptor::setUrlSameUrl()
{
    QFETCH(InterceptorSetter, setter);
    QFETCH(bool, interceptInPage);

    QWebEngineProfile profile;
    TestRequestInterceptor interceptor(/* intercept */ true);
    if (!interceptInPage)
        (profile.*setter)(&interceptor);

    QWebEnginePage page(&profile);
    if (interceptInPage)
        page.setUrlRequestInterceptor(&interceptor);
    QSignalSpy spy(&page, SIGNAL(loadFinished(bool)));

    page.setUrl(QUrl("qrc:///resources/__placeholder__"));
    QVERIFY(spy.wait());
    QCOMPARE(page.url(), QUrl("qrc:///resources/content.html"));
    QCOMPARE(spy.count(), 1);

    page.setUrl(QUrl("qrc:///resources/__placeholder__"));
    QVERIFY(spy.wait());
    QCOMPARE(page.url(), QUrl("qrc:///resources/content.html"));
    QCOMPARE(spy.count(), 2);

    // Now a case without redirect.
    page.setUrl(QUrl("qrc:///resources/content.html"));
    QVERIFY(spy.wait());
    QCOMPARE(page.url(), QUrl("qrc:///resources/content.html"));
    QCOMPARE(spy.count(), 3);

    page.setUrl(QUrl("qrc:///resources/__placeholder__"));
    QVERIFY(spy.wait());
    QCOMPARE(page.url(), QUrl("qrc:///resources/content.html"));
    QCOMPARE(spy.count(), 4);
}

void tst_QWebEngineUrlRequestInterceptor::firstPartyUrl_data()
{
    interceptRequest_data();
}

void tst_QWebEngineUrlRequestInterceptor::firstPartyUrl()
{
    QFETCH(InterceptorSetter, setter);
    QWebEngineProfile profile;
    TestRequestInterceptor interceptor(/* intercept */ false);
    (profile.*setter)(&interceptor);

    QWebEnginePage page(&profile);
    QSignalSpy spy(&page, SIGNAL(loadFinished(bool)));

    page.setUrl(QUrl("qrc:///resources/firstparty.html"));
    QVERIFY(spy.wait());
    QVERIFY(interceptor.requestInfos.count() >= 2);
    QCOMPARE(interceptor.requestInfos.at(0).requestUrl, QUrl("qrc:///resources/firstparty.html"));
    QCOMPARE(interceptor.requestInfos.at(1).requestUrl, QUrl("qrc:///resources/content.html"));
    QCOMPARE(interceptor.requestInfos.at(0).firstPartyUrl, QUrl("qrc:///resources/firstparty.html"));
    QCOMPARE(interceptor.requestInfos.at(1).firstPartyUrl, QUrl("qrc:///resources/firstparty.html"));
    QCOMPARE(spy.count(), 1);
}

void tst_QWebEngineUrlRequestInterceptor::firstPartyUrlNestedIframes_data()
{
    QUrl url = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/iframe.html"));
    QTest::addColumn<InterceptorSetter>("setter");
    QTest::addColumn<QUrl>("requestUrl");
    QTest::newRow("ui file") << &QWebEngineProfile::setUrlRequestInterceptor << url;
    QTest::newRow("io file") << &QWebEngineProfile::setRequestInterceptor << url;
    QTest::newRow("ui qrc") << &QWebEngineProfile::setUrlRequestInterceptor
                            << QUrl("qrc:///resources/iframe.html");
    QTest::newRow("io qrc") << &QWebEngineProfile::setRequestInterceptor
                            << QUrl("qrc:///resources/iframe.html");
}

void tst_QWebEngineUrlRequestInterceptor::firstPartyUrlNestedIframes()
{
    QFETCH(InterceptorSetter, setter);
    QFETCH(QUrl, requestUrl);

    if (requestUrl.scheme() == "file" && !QDir(TESTS_SOURCE_DIR).exists())
        W_QSKIP(QString("This test requires access to resources found in '%1'").arg(TESTS_SOURCE_DIR).toLatin1().constData(), SkipAll);

    QString adjustedUrl = requestUrl.adjusted(QUrl::RemoveFilename).toString();

    QWebEngineProfile profile;
    TestRequestInterceptor interceptor(/* intercept */ false);
    (profile.*setter)(&interceptor);

    QWebEnginePage page(&profile);
    QSignalSpy loadSpy(&page, SIGNAL(loadFinished(bool)));
    page.setUrl(requestUrl);
    QTRY_COMPARE(loadSpy.count(), 1);

    QVERIFY(interceptor.requestInfos.count() >= 1);
    RequestInfo info = interceptor.requestInfos.at(0);
    QCOMPARE(info.requestUrl, requestUrl);
    QCOMPARE(info.firstPartyUrl, requestUrl);
    QCOMPARE(info.resourceType, QWebEngineUrlRequestInfo::ResourceTypeMainFrame);

    QVERIFY(interceptor.requestInfos.count() >= 2);
    info = interceptor.requestInfos.at(1);
    QCOMPARE(info.requestUrl, QUrl(adjustedUrl + "iframe2.html"));
    QCOMPARE(info.firstPartyUrl, requestUrl);
    QCOMPARE(info.resourceType, QWebEngineUrlRequestInfo::ResourceTypeSubFrame);

    QVERIFY(interceptor.requestInfos.count() >= 3);
    info = interceptor.requestInfos.at(2);
    QCOMPARE(info.requestUrl, QUrl(adjustedUrl + "iframe3.html"));
    QCOMPARE(info.firstPartyUrl, requestUrl);
    QCOMPARE(info.resourceType, QWebEngineUrlRequestInfo::ResourceTypeSubFrame);
}

void tst_QWebEngineUrlRequestInterceptor::requestInterceptorByResourceType_data()
{
    QUrl firstPartyUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/resource_in_iframe.html"));
    QUrl styleRequestUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/style.css"));
    QUrl scriptRequestUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/script.js"));
    QUrl fontRequestUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/fontawesome.woff"));
    QUrl xhrRequestUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/test"));
    QUrl imageFirstPartyUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/image_in_iframe.html"));
    QUrl imageRequestUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/icons/favicon.png"));
    QUrl mediaFirstPartyUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/media_in_iframe.html"));
    QUrl mediaRequestUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/media.mp4"));
    QUrl faviconFirstPartyUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/favicon.html"));
    QUrl faviconRequestUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebengineurlrequestinterceptor/resources/icons/favicon.png"));

    QTest::addColumn<InterceptorSetter>("setter");
    QTest::addColumn<QUrl>("requestUrl");
    QTest::addColumn<QUrl>("firstPartyUrl");
    QTest::addColumn<int>("resourceType");

    QStringList name = { "ui", "io" };
    QVector<InterceptorSetter> setters = { &QWebEngineProfile::setUrlRequestInterceptor,
                                           &QWebEngineProfile::setRequestInterceptor };
    for (int i = 0; i < 2; i++) {
        QTest::newRow(qPrintable(name[i] + "StyleSheet"))
                << setters[i] << styleRequestUrl << firstPartyUrl
                << static_cast<int>(QWebEngineUrlRequestInfo::ResourceTypeStylesheet);
        QTest::newRow(qPrintable(name[i] + "Script")) << setters[i] << scriptRequestUrl << firstPartyUrl
                                                      << static_cast<int>(QWebEngineUrlRequestInfo::ResourceTypeScript);
        QTest::newRow(qPrintable(name[i] + "Image")) << setters[i] << imageRequestUrl << imageFirstPartyUrl
                                                     << static_cast<int>(QWebEngineUrlRequestInfo::ResourceTypeImage);
        QTest::newRow(qPrintable(name[i] + "FontResource"))
                << setters[i] << fontRequestUrl << firstPartyUrl
                << static_cast<int>(QWebEngineUrlRequestInfo::ResourceTypeFontResource);
        QTest::newRow(qPrintable(name[i] + "Media")) << setters[i] << mediaRequestUrl << mediaFirstPartyUrl
                                                     << static_cast<int>(QWebEngineUrlRequestInfo::ResourceTypeMedia);
        QTest::newRow(qPrintable(name[i] + "Favicon"))
                << setters[i] << faviconRequestUrl << faviconFirstPartyUrl
                << static_cast<int>(QWebEngineUrlRequestInfo::ResourceTypeFavicon);
        QTest::newRow(qPrintable(name[i] + "Xhr")) << setters[i] << xhrRequestUrl << firstPartyUrl
                                                   << static_cast<int>(QWebEngineUrlRequestInfo::ResourceTypeXhr);
    }
}

void tst_QWebEngineUrlRequestInterceptor::requestInterceptorByResourceType()
{
    if (!QDir(TESTS_SOURCE_DIR).exists())
        W_QSKIP(QString("This test requires access to resources found in '%1'").arg(TESTS_SOURCE_DIR).toLatin1().constData(), SkipAll);
    QFETCH(InterceptorSetter, setter);
    QFETCH(QUrl, requestUrl);
    QFETCH(QUrl, firstPartyUrl);
    QFETCH(int, resourceType);

    QWebEngineProfile profile;
    TestRequestInterceptor interceptor(/* intercept */ false);
    (profile.*setter)(&interceptor);

    QWebEnginePage page(&profile);
    QSignalSpy loadSpy(&page, SIGNAL(loadFinished(bool)));
    page.setUrl(firstPartyUrl);
    QTRY_COMPARE(loadSpy.count(), 1);

    QTRY_COMPARE(interceptor.getUrlRequestForType(static_cast<QWebEngineUrlRequestInfo::ResourceType>(resourceType)).count(), 1);
    QList<RequestInfo> infos = interceptor.getUrlRequestForType(static_cast<QWebEngineUrlRequestInfo::ResourceType>(resourceType));
    QVERIFY(infos.count() >= 1);
    QCOMPARE(infos.at(0).requestUrl, requestUrl);
    QCOMPARE(infos.at(0).firstPartyUrl, firstPartyUrl);
    QCOMPARE(infos.at(0).resourceType, resourceType);
}

void tst_QWebEngineUrlRequestInterceptor::firstPartyUrlHttp_data()
{
    interceptRequest_data();
}

void tst_QWebEngineUrlRequestInterceptor::firstPartyUrlHttp()
{
    QFETCH(InterceptorSetter, setter);
    QWebEngineProfile profile;
    TestRequestInterceptor interceptor(/* intercept */ false);
    (profile.*setter)(&interceptor);

    QWebEnginePage page(&profile);
    QSignalSpy loadSpy(&page, SIGNAL(loadFinished(bool)));
    QUrl firstPartyUrl = QUrl("https://www.w3schools.com/tags/tryit.asp?filename=tryhtml5_video");
    page.setUrl(QUrl(firstPartyUrl));
    if (!loadSpy.wait(15000) || !loadSpy.at(0).at(0).toBool())
        QSKIP("Couldn't load page from network, skipping test.");

    QList<RequestInfo> infos;

    // Stylesheet
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeStylesheet));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeStylesheet);
    foreach (auto info, infos)
        QCOMPARE(info.firstPartyUrl, firstPartyUrl);

    // Script
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeScript));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeScript);
    foreach (auto info, infos)
        QCOMPARE(info.firstPartyUrl, firstPartyUrl);

    // Image
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeImage));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeImage);
    foreach (auto info, infos)
        QCOMPARE(info.firstPartyUrl, firstPartyUrl);

    // FontResource
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeFontResource));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeFontResource);
    foreach (auto info, infos)
        QCOMPARE(info.firstPartyUrl, firstPartyUrl);

    // Media
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeMedia));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeMedia);
    foreach (auto info, infos)
        QCOMPARE(info.firstPartyUrl, firstPartyUrl);

    // Favicon
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeFavicon));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeFavicon);
    foreach (auto info, infos)
        QCOMPARE(info.firstPartyUrl, firstPartyUrl);

    // XMLHttpRequest
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeXhr));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeXhr);
    foreach (auto info, infos)
        QCOMPARE(info.firstPartyUrl, firstPartyUrl);
}

void tst_QWebEngineUrlRequestInterceptor::customHeaders_data()
{
    interceptRequest_data();
}

void tst_QWebEngineUrlRequestInterceptor::customHeaders()
{
    QFETCH(InterceptorSetter, setter);
    // Create HTTP Server to parse the request.
    HttpServer httpServer;
    httpServer.setResourceDirs({ TESTS_SOURCE_DIR "qwebengineurlrequestinterceptor/resources" });
    QVERIFY(httpServer.start());

    QWebEngineProfile profile;
    TestRequestInterceptor interceptor(false);
    (profile.*setter)(&interceptor);

    QWebEnginePage page(&profile);
    QSignalSpy spy(&page, SIGNAL(loadFinished(bool)));

    interceptor.headers = {
        { "referer", "http://somereferrer.com/" },
        { "from", "user@example.com" },
        { "user-agent", "mozilla/5.0 (x11; linux x86_64; rv:12.0) gecko/20100101 firefox/12.0" },
    };

    QMap<QByteArray, QByteArray> actual, expected;
    connect(&httpServer, &HttpServer::newRequest, [&] (HttpReqRep *rr) {
        for (auto it = expected.begin(); it != expected.end(); ++it) {
            auto headerValue = rr->requestHeader(it.key());
            actual[it.key()] = headerValue;
            QCOMPARE(headerValue, it.value());
        }
    });

    auto dumpHeaders = [&] () {
        QString s; QDebug d(&s);
        for (auto it = expected.begin(); it != expected.end(); ++it)
            d << "\n\tHeader:" << it.key() << "| actual:" << actual[it.key()] << "expected:" << it.value();
        return s;
    };

    expected = interceptor.headers;
    page.load(httpServer.url("/content.html"));
    QVERIFY(spy.wait());
    QVERIFY2(actual == expected, qPrintable(dumpHeaders()));

    // test that custom headers are also applied on redirect
    interceptor.shouldRedirect = true;
    interceptor.redirectUrl = httpServer.url("/content2.html");
    interceptor.headers = {
        { "referer", "http://somereferrer2.com/" },
        { "from", "user2@example.com" },
        { "user-agent", "mozilla/5.0 (compatible; googlebot/2.1; +http://www.google.com/bot.html)" },
    };

    actual.clear();
    expected = interceptor.headers;
    page.triggerAction(QWebEnginePage::Reload);
    QVERIFY(spy.wait());
    QVERIFY2(actual == expected, qPrintable(dumpHeaders()));

    (void) httpServer.stop();
}

void tst_QWebEngineUrlRequestInterceptor::initiator_data()
{
    interceptRequest_data();
}

void tst_QWebEngineUrlRequestInterceptor::initiator()
{
    QFETCH(InterceptorSetter, setter);
    QWebEngineProfile profile;
    TestRequestInterceptor interceptor(/* intercept */ false);
    (profile.*setter)(&interceptor);

    QWebEnginePage page(&profile);
    QSignalSpy loadSpy(&page, SIGNAL(loadFinished(bool)));
    QUrl url = QUrl("https://www.w3schools.com/tags/tryit.asp?filename=tryhtml5_video");
    page.setUrl(QUrl(url));
    if (!loadSpy.wait(15000) || !loadSpy.at(0).at(0).toBool())
        QSKIP("Couldn't load page from network, skipping test.");

    QList<RequestInfo> infos;

    // Stylesheet
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeStylesheet));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeStylesheet);
    foreach (auto info, infos)
        QVERIFY(interceptor.requestInitiatorUrls[info.requestUrl].contains(info.initiator));

    // Script
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeScript));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeScript);
    foreach (auto info, infos)
        QVERIFY(interceptor.requestInitiatorUrls[info.requestUrl].contains(info.initiator));

    // Image
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeImage));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeImage);
    foreach (auto info, infos)
        QVERIFY(interceptor.requestInitiatorUrls[info.requestUrl].contains(info.initiator));

    // FontResource
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeFontResource));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeFontResource);
    foreach (auto info, infos)
        QVERIFY(interceptor.requestInitiatorUrls[info.requestUrl].contains(info.initiator));

    // Media
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeMedia));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeMedia);
    foreach (auto info, infos)
        QVERIFY(interceptor.requestInitiatorUrls[info.requestUrl].contains(info.initiator));

    // Favicon
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeFavicon));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeFavicon);
    foreach (auto info, infos)
        QVERIFY(interceptor.requestInitiatorUrls[info.requestUrl].contains(info.initiator));

    // XMLHttpRequest
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeXhr));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeXhr);
    foreach (auto info, infos)
        QVERIFY(interceptor.requestInitiatorUrls[info.requestUrl].contains(info.initiator));
}

void tst_QWebEngineUrlRequestInterceptor::jsServiceWorker_data()
{
    interceptRequest_data();
}

void tst_QWebEngineUrlRequestInterceptor::jsServiceWorker()
{
    QFETCH(InterceptorSetter, setter);

    HttpServer server;
    server.setResourceDirs({ TESTS_SOURCE_DIR "qwebengineurlrequestinterceptor/resources" });
    QVERIFY(server.start());

    QWebEngineProfile profile(QStringLiteral("Test"));
    std::unique_ptr<ConsolePage> page;
    page.reset(new ConsolePage(&profile));
    TestRequestInterceptor interceptor(/* intercept */ false);
    (profile.*setter)(&interceptor);
    QVERIFY(loadSync(page.get(), server.url("/sw.html")));

    // We expect only one message here, because logging of services workers is not exposed in our API.
    QTRY_COMPARE(page->messages.count(), 1);
    QCOMPARE(page->levels.at(0), QWebEnginePage::InfoMessageLevel);

    QUrl firstPartyUrl = QUrl(server.url().toString() + "sw.js");
    QList<RequestInfo> infos;
    // Service Worker
    QTRY_VERIFY(interceptor.hasUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeServiceWorker));
    infos = interceptor.getUrlRequestForType(QWebEngineUrlRequestInfo::ResourceTypeServiceWorker);
    foreach (auto info, infos)
        QCOMPARE(info.firstPartyUrl, firstPartyUrl);

    QVERIFY(server.stop());
}

void tst_QWebEngineUrlRequestInterceptor::replaceInterceptor_data()
{
    QTest::addColumn<bool>("firstInterceptIsInPage");
    QTest::addColumn<bool>("keepInterceptionPoint");
    QTest::newRow("page")         << true << true;
    QTest::newRow("page-profile") << true << false;
    QTest::newRow("profile")      << false << true;
    QTest::newRow("profile-page") << false << false;
}

void tst_QWebEngineUrlRequestInterceptor::replaceInterceptor()
{
    QFETCH(bool, firstInterceptIsInPage);
    QFETCH(bool, keepInterceptionPoint);

    HttpServer server;
    server.setResourceDirs({ ":/resources" });
    QVERIFY(server.start());

    QWebEngineProfile profile;
    profile.settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    QWebEnginePage page(&profile);
    QSignalSpy spy(&page, SIGNAL(loadFinished(bool)));
    bool fetchFinished = false;

    auto setInterceptor = [&] (QWebEngineUrlRequestInterceptor *interceptor, bool interceptInPage) {
        interceptInPage ? page.setUrlRequestInterceptor(interceptor) : profile.setUrlRequestInterceptor(interceptor);
    };

    std::vector<TestRequestInterceptor> interceptors(3);
    std::vector<int> requestsOnReplace;
    setInterceptor(&interceptors.front(), firstInterceptIsInPage);

    auto sc = connect(&page, &QWebEnginePage::loadFinished, [&] () {
        auto currentInterceptorIndex = requestsOnReplace.size();
        requestsOnReplace.push_back(interceptors[currentInterceptorIndex].requestInfos.size());

        bool isFirstReinstall = currentInterceptorIndex == 0;
        bool interceptInPage = keepInterceptionPoint ? firstInterceptIsInPage : (isFirstReinstall ^ firstInterceptIsInPage);
        setInterceptor(&interceptors[++currentInterceptorIndex], interceptInPage);
        if (!keepInterceptionPoint)
            setInterceptor(nullptr, !interceptInPage);

        if (isFirstReinstall) {
            page.triggerAction(QWebEnginePage::Reload);
        } else {
            page.runJavaScript("fetch('http://nonexistent.invalid').catch(() => {})", [&, interceptInPage] (const QVariant &) {
                requestsOnReplace.push_back(interceptors.back().requestInfos.size());
                setInterceptor(nullptr, interceptInPage);
                fetchFinished = true;
            });
        }
    });

    page.setUrl(server.url("/favicon.html"));
    QTRY_COMPARE(spy.count(), 2);
    QTRY_VERIFY(fetchFinished);

    QString s; QDebug d(&s);
    for (auto i = 0u; i < interceptors.size(); ++i) {
        auto &&interceptor = interceptors[i];
        auto &&requests = interceptor.requestInfos;
        d << "\nInterceptor [" << i << "] with" << requestsOnReplace[i] << "requests on replace and" << requests.size() << "in the end:";
        for (int j = 0; j < requests.size(); ++j) {
            auto &&r = requests[j];
            d << "\n\t" << j << "| url:" << r.requestUrl << "firstPartyUrl:" << r.firstPartyUrl;
        }
        QVERIFY2(!requests.isEmpty(), qPrintable(s));
        QVERIFY2(requests.size() == requestsOnReplace[i], qPrintable(s));
    }
}

void tst_QWebEngineUrlRequestInterceptor::replaceOnIntercept()
{
    HttpServer server;
    server.setResourceDirs({ ":/resources" });
    QVERIFY(server.start());

    QWebEngineProfile profile;
    profile.settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    QWebEnginePage page(&profile);
    QSignalSpy spy(&page, SIGNAL(loadFinished(bool)));

    struct Interceptor : QWebEngineUrlRequestInterceptor {
        Interceptor(const std::function<void ()> &a) : action(a) { }
        void interceptRequest(QWebEngineUrlRequestInfo &) override { action(); }
        std::function<void ()> action;
        int interceptRequestReceived = 0;
    };

    TestRequestInterceptor profileInterceptor, pageInterceptor1, pageInterceptor2;
    page.setUrlRequestInterceptor(&pageInterceptor1);
    profile.setUrlRequestInterceptor(&profileInterceptor);
    profileInterceptor.onIntercept = [&] (QWebEngineUrlRequestInfo &) {
        page.setUrlRequestInterceptor(&pageInterceptor2);
        return true;
    };

    page.setUrl(server.url("/favicon.html"));
    QTRY_COMPARE(spy.count(), 1);
    QTRY_COMPARE(profileInterceptor.requestInfos.size(), 2);

    // if interceptor for page was replaced on intercept call in profile then, since request first
    // comes to profile, forward to page's interceptor should land to second one
    QCOMPARE(pageInterceptor1.requestInfos.size(), 0);
    QCOMPARE(profileInterceptor.requestInfos.size(), pageInterceptor2.requestInfos.size());

    page.setUrlRequestInterceptor(&pageInterceptor1);
    bool fetchFinished = false;
    page.runJavaScript("fetch('http://nonexistent.invalid').catch(() => {})", [&] (const QVariant &) {
        page.setUrlRequestInterceptor(&pageInterceptor2);
        fetchFinished = true;
    });

    QTRY_VERIFY(fetchFinished);
    QCOMPARE(profileInterceptor.requestInfos.size(), 3);
    QCOMPARE(pageInterceptor1.requestInfos.size(), 0);
    QCOMPARE(profileInterceptor.requestInfos.size(), pageInterceptor2.requestInfos.size());
}

void tst_QWebEngineUrlRequestInterceptor::multipleRedirects()
{
    HttpServer server;
    server.setResourceDirs({ ":/resources" });
    QVERIFY(server.start());

    TestMultipleRedirectsInterceptor multiInterceptor;
    multiInterceptor.redirectPairs.insert(QUrl(server.url("/content.html")), QUrl(server.url("/content2.html")));
    multiInterceptor.redirectPairs.insert(QUrl(server.url("/content2.html")), QUrl(server.url("/content3.html")));

    QWebEngineProfile profile;
    profile.settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    profile.setUrlRequestInterceptor(&multiInterceptor);
    QWebEnginePage page(&profile);
    QSignalSpy spy(&page, SIGNAL(loadFinished(bool)));

    page.setUrl(server.url("/content.html"));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 20000);
    QTRY_COMPARE(multiInterceptor.redirectCount, 2);
    QTRY_COMPARE(multiInterceptor.requestInfos.size(), 2);
}

QTEST_MAIN(tst_QWebEngineUrlRequestInterceptor)
#include "tst_qwebengineurlrequestinterceptor.moc"
