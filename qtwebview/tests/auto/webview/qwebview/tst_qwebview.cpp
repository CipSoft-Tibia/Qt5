// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qdir.h>
#include <QtCore/qtemporarydir.h>
#include <QtCore/qfileinfo.h>
#include <QtWebView/private/qwebview_p.h>
#include <QtQml/qqmlengine.h>
#include <QtWebView/private/qwebviewloadrequest_p.h>
#include <QtWebView/private/qwebviewfactory_p.h>
#include <QtWebViewQuick/private/qquickwebview_p.h>

#if QT_CONFIG(webview_webengine_plugin)
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
#include <QtCore/private/qjnihelpers_p.h>
#define ANDROID_REQUIRES_API_LEVEL(N) \
    if (QtAndroidPrivate::androidSdkVersion() < N) \
        QSKIP("This feature is not supported on this version of Android");
#else
#define ANDROID_REQUIRES_API_LEVEL(N)
#endif

// TODO: remove when c++ apis come
class WebViewFactory
{
public:
    WebViewFactory()
        : m_webengine(QWebViewFactory::loadedPluginHasKey("webengine")),
          m_engine(m_webengine ? std::make_unique<QQmlEngine>() : nullptr),
          m_quickView(m_webengine ? std::make_unique<QQuickWebView>() : nullptr),
          m_view(m_webengine ? nullptr : std::make_unique<QWebView>())
    {
        if (m_webengine) {
            QQmlContext *rootContext = m_engine->rootContext();
            QQmlEngine::setContextForObject(m_quickView.get(), rootContext);
        }
    }
    QWebView &webViewRef() { return m_webengine ? m_quickView->webView() : *(m_view.get()); }

private:
    bool m_webengine;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QQuickWebView> m_quickView;
    std::unique_ptr<QWebView> m_view;
};

class tst_QWebView : public QObject
{
    Q_OBJECT
public:
    tst_QWebView() : m_cacheLocation(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)) {}

private slots:
    void initTestCase();
    void load();
    void runJavaScript();
    void loadHtml_data();
    void loadHtml();
    void loadRequest();
    void setAndDeleteCookie();

private:
    const QString m_cacheLocation;
};

void tst_QWebView::initTestCase()
{
    if (!qEnvironmentVariableIsEmpty("QEMU_LD_PREFIX"))
        QSKIP("This test is unstable on QEMU, so it will be skipped.");
#if QT_CONFIG(webview_webengine_plugin)
    if (QWebViewFactory::loadedPluginHasKey("webengine"))
        QtWebEngineQuick::initialize();
#endif
    if (!QFileInfo(m_cacheLocation).isDir()) {
        QDir dir;
        QVERIFY(dir.mkpath(m_cacheLocation));
    }
}

void tst_QWebView::load()
{
    QTemporaryFile file(m_cacheLocation + QStringLiteral("/XXXXXXfile.html"));
    QVERIFY2(file.open(),
             qPrintable(QStringLiteral("Cannot create temporary file:") + file.errorString()));

    file.write("<html><head><title>FooBar</title></head><body />");
    const QString fileName = file.fileName();
    file.close();

    WebViewFactory factory;
    QWebView &view = factory.webViewRef();
    view.getSettings()->setAllowFileAccess(true);
    view.getSettings()->setLocalContentCanAccessFileUrls(true);
    QCOMPARE(view.loadProgress(), 0);
    const QUrl url = QUrl::fromLocalFile(fileName);
    view.setUrl(url);
    QTRY_COMPARE(view.loadProgress(), 100);
    QTRY_VERIFY(!view.isLoading());
    QCOMPARE(view.title(), QStringLiteral("FooBar"));
    QVERIFY(!view.canGoBack());
    QVERIFY(!view.canGoForward());
    QCOMPARE(view.url(), url);
}

void tst_QWebView::runJavaScript()
{
    ANDROID_REQUIRES_API_LEVEL(19)
    const QString tstProperty = QString(QLatin1String("Qt.tst_data"));
    const QString title = QString(QLatin1String("WebViewTitle"));

    QQmlEngine engine;
    QQmlContext *rootContext = engine.rootContext();
    QQuickWebView view;
    QQmlEngine::setContextForObject(&view, rootContext);

    QCOMPARE(view.loadProgress(), 0);
    view.loadHtml(QString("<html><head><title>%1</title></head><body/></html>").arg(title));
    QTRY_COMPARE(view.loadProgress(), 100);
    QTRY_VERIFY(!view.isLoading());
    QCOMPARE(view.title(), title);
    QJSValue callback = engine.evaluate(QString("(function(result) { %1 = result; })").arg(tstProperty));
    QVERIFY2(!callback.isError(), qPrintable(callback.toString()));
    QVERIFY(!callback.isUndefined());
    QVERIFY(callback.isCallable());
    view.runJavaScript(QString(QLatin1String("document.title")), callback);
    QTRY_COMPARE(engine.evaluate(tstProperty).toString(), title);
}

void tst_QWebView::loadHtml_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<QUrl>("loadUrl");
    QTest::addColumn<QUrl>("resultUrl");
    WebViewFactory factory;
    QWebView &view = factory.webViewRef();
    QCOMPARE(view.loadProgress(), 0);
    QSignalSpy loadChangedSingalSpy(&view, SIGNAL(loadingChanged(QWebViewLoadRequestPrivate)));
    const QByteArray content(
            QByteArrayLiteral("<html><title>WebViewTitle</title>"
                              "<body><span style=\"color:#ff0000\">Hello</span></body></html>"));
    QByteArray encoded("data:text/html;charset=UTF-8,");
    encoded.append(content.toPercentEncoding());

    if (!QWebViewFactory::loadedPluginHasKey("webkit")) {
        QTest::newRow("set conent without base url") << content << QUrl() << QUrl(encoded);
    } else {
        QTest::newRow("set conent without base url") << content << QUrl() << QUrl("about:blank");
    }
    QTest::newRow("set content with data base url") << content << QUrl(encoded) << QUrl(encoded);

    if (!QWebViewFactory::loadedPluginHasKey("webview2")) {
        QTest::newRow("set content with non-data base url")
                << content << QUrl("http://foobar.com/") << QUrl("http://foobar.com/");
    } else {
        QTest::newRow("set content with non-data base url")
                << content << QUrl("http://foobar.com/") << QUrl(encoded);
    }
}

void tst_QWebView::loadHtml()
{
    QFETCH(QByteArray, content);
    QFETCH(QUrl, loadUrl);
    QFETCH(QUrl, resultUrl);

    WebViewFactory factory;
    QWebView &view = factory.webViewRef();
    QCOMPARE(view.loadProgress(), 0);
    QSignalSpy loadChangedSingalSpy(&view, SIGNAL(loadingChanged(QWebViewLoadRequestPrivate)));
    QSignalSpy javaScriptResultSpy(&view, SIGNAL(javaScriptResult(int, QVariant)));
    view.loadHtml(content, loadUrl);
    QTRY_COMPARE(view.loadProgress(), 100);
    QTRY_VERIFY(!view.isLoading());
    QCOMPARE(view.title(), QStringLiteral("WebViewTitle"));
    QTRY_COMPARE(loadChangedSingalSpy.size(), 2);
    // take load finished
    const QWebViewLoadRequestPrivate &lr = loadChangedSingalSpy.at(1).at(0).value<QWebViewLoadRequestPrivate>();
    QCOMPARE(lr.m_status, QWebView::LoadSucceededStatus);
    if (QWebViewFactory::loadedPluginHasKey("android_view")) {
        // WebEngine javascript calls work only with qmlengine, however here we use
        // c++ interface
        int callback = 1;
        view.runJavaScriptPrivate("document.baseURI", callback);
        QTRY_COMPARE(javaScriptResultSpy.size(), 1);
        QCOMPARE(javaScriptResultSpy.at(0).at(0), callback);
        QCOMPARE(javaScriptResultSpy.at(0).at(1).value<QUrl>(), resultUrl);
    }

    QVERIFY(view.url().isValid());
    QCOMPARE(view.url(), resultUrl);
}

void tst_QWebView::loadRequest()
{
    // LoadSucceeded
    {
        QTemporaryFile file(m_cacheLocation + QStringLiteral("/XXXXXXfile.html"));
        QVERIFY2(file.open(),
                 qPrintable(QStringLiteral("Cannot create temporary file:") + file.errorString()));

        file.write("<html><head><title>FooBar</title></head><body/></html>");
        const QString fileName = file.fileName();
        file.close();

        WebViewFactory factory;
        QWebView &view = factory.webViewRef();

        view.getSettings()->setAllowFileAccess(true);
        view.getSettings()->setLocalContentCanAccessFileUrls(true);
        QCOMPARE(view.loadProgress(), 0);
        const QUrl url = QUrl::fromLocalFile(fileName);
        QSignalSpy loadChangedSingalSpy(&view, SIGNAL(loadingChanged(QWebViewLoadRequestPrivate)));
        view.setUrl(url);
        QTRY_VERIFY(!view.isLoading());
        QTRY_COMPARE(view.loadProgress(), 100);
        QTRY_COMPARE(view.title(), QStringLiteral("FooBar"));
        QCOMPARE(view.url(), url);
        QTRY_COMPARE(loadChangedSingalSpy.size(), 2);
        {
            const QList<QVariant> &loadStartedArgs = loadChangedSingalSpy.takeFirst();
            const QWebViewLoadRequestPrivate &lr = loadStartedArgs.at(0).value<QWebViewLoadRequestPrivate>();
            QCOMPARE(lr.m_status, QWebView::LoadStartedStatus);
        }
        {
            const QList<QVariant> &loadStartedArgs = loadChangedSingalSpy.takeFirst();
            const QWebViewLoadRequestPrivate &lr = loadStartedArgs.at(0).value<QWebViewLoadRequestPrivate>();
            QCOMPARE(lr.m_status, QWebView::LoadSucceededStatus);
        }
    }

    // LoadFailed
    {
        WebViewFactory factory;
        QWebView &view = factory.webViewRef();
        view.getSettings()->setAllowFileAccess(true);
        view.getSettings()->setLocalContentCanAccessFileUrls(true);
        QCOMPARE(view.loadProgress(), 0);
        QSignalSpy loadChangedSingalSpy(&view, SIGNAL(loadingChanged(QWebViewLoadRequestPrivate)));
        view.setUrl(QUrl(QStringLiteral("file:///file_that_does_not_exist.html")));
        QTRY_VERIFY(!view.isLoading());
        QTRY_COMPARE(loadChangedSingalSpy.size(), 2);
        {
            const QList<QVariant> &loadStartedArgs = loadChangedSingalSpy.takeFirst();
            const QWebViewLoadRequestPrivate &lr = loadStartedArgs.at(0).value<QWebViewLoadRequestPrivate>();
            QCOMPARE(lr.m_status, QWebView::LoadStartedStatus);
        }
        {
            const QList<QVariant> &loadStartedArgs = loadChangedSingalSpy.takeFirst();
            const QWebViewLoadRequestPrivate &lr = loadStartedArgs.at(0).value<QWebViewLoadRequestPrivate>();
            QCOMPARE(lr.m_status, QWebView::LoadFailedStatus);
        }
        if (QWebViewFactory::loadedPluginHasKey("webengine"))
            QCOMPARE(view.loadProgress(), 0); // darwin plugin returns 100
    }
}

void tst_QWebView::setAndDeleteCookie()
{
    WebViewFactory factory;
    QWebView &view = factory.webViewRef();
    view.getSettings()->setLocalStorageEnabled(true);
    view.getSettings()->setAllowFileAccess(true);
    view.getSettings()->setLocalContentCanAccessFileUrls(true);

    QSignalSpy cookieAddedSpy(&view, SIGNAL(cookieAdded(QString,QString)));
    QSignalSpy cookieRemovedSpy(&view, SIGNAL(cookieRemoved(QString,QString)));

    view.setCookie(".example.com", "TestCookie", "testValue");
    view.setCookie(".example2.com", "TestCookie2", "testValue2");
    view.setCookie(".example3.com", "TestCookie3", "testValue3");
    QTRY_COMPARE(cookieAddedSpy.size(), 3);

    view.deleteCookie(".example.com", "TestCookie");
    QTRY_COMPARE(cookieRemovedSpy.size(), 1);

    // deleting a cookie using a name that has not been set
    view.deleteCookie(".example.com", "NewCookieName");
    QTRY_COMPARE(cookieRemovedSpy.size(), 1);

    // deleting a cookie using a domain that has not been set
    view.deleteCookie(".new.domain.com", "TestCookie2");
    QTRY_COMPARE(cookieRemovedSpy.size(), 1);

    view.deleteAllCookies();
    if (QWebViewFactory::loadedPluginHasKey("android_view"))
        QEXPECT_FAIL("", "Notification for deleteAllCookies() is not implemented on Android, yet!", Continue);
    QTRY_COMPARE(cookieRemovedSpy.size(), 3);
}

QTEST_MAIN(tst_QWebView)

#include "tst_qwebview.moc"
