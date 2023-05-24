// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qdir.h>
#include <QtCore/qtemporarydir.h>
#include <QtCore/qfileinfo.h>
#include <QtWebView/private/qwebview_p.h>
#include <QtQml/qqmlengine.h>
#include <QtWebView/private/qwebviewloadrequest_p.h>

#ifdef QT_QQUICKWEBVIEW_TESTS
#include <QtWebViewQuick/private/qquickwebview_p.h>
#endif // QT_NO_QQUICKWEBVIEW_TESTS

#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
#include <QtWebEngineQuick>
#endif // QT_WEBVIEW_WEBENGINE_BACKEND

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
#include <QtCore/private/qjnihelpers_p.h>
#define ANDROID_REQUIRES_API_LEVEL(N) \
    if (QtAndroidPrivate::androidSdkVersion() < N) \
        QSKIP("This feature is not supported on this version of Android");
#else
#define ANDROID_REQUIRES_API_LEVEL(N)
#endif

class tst_QWebView : public QObject
{
    Q_OBJECT
public:
    tst_QWebView() : m_cacheLocation(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)) {}

private slots:
    void initTestCase();
    void load();
    void runJavaScript();
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
#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
    QtWebEngineQuick::initialize();
#endif // QT_WEBVIEW_WEBENGINE_BACKEND
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

#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
    QQmlEngine engine;
    QQmlContext *rootContext = engine.rootContext();
    QQuickWebView qview;
    QQmlEngine::setContextForObject(&qview, rootContext);
    QWebView &view = qview.webView();
#else
    QWebView view;
    view.getSettings()->setAllowFileAccess(true);
    view.getSettings()->setLocalContentCanAccessFileUrls(true);
#endif
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
#ifdef QT_QQUICKWEBVIEW_TESTS
#ifndef QT_WEBVIEW_WEBENGINE_BACKEND
    ANDROID_REQUIRES_API_LEVEL(19)
#endif
    const QString tstProperty = QString(QLatin1String("Qt.tst_data"));
    const QString title = QString(QLatin1String("WebViewTitle"));

    QQmlEngine engine;
    QQmlContext *rootContext = engine.rootContext();
    QQuickWebView view;
    QQmlEngine::setContextForObject(&view, rootContext);

    QCOMPARE(view.loadProgress(), 0);
    view.loadHtml(QString("<html><head><title>%1</title></head><body /></html>").arg(title));
    QTRY_COMPARE(view.loadProgress(), 100);
    QTRY_VERIFY(!view.isLoading());
    QCOMPARE(view.title(), title);
    QJSValue callback = engine.evaluate(QString("(function(result) { %1 = result; })").arg(tstProperty));
    QVERIFY2(!callback.isError(), qPrintable(callback.toString()));
    QVERIFY(!callback.isUndefined());
    QVERIFY(callback.isCallable());
    view.runJavaScript(QString(QLatin1String("document.title")), callback);
    QTRY_COMPARE(engine.evaluate(tstProperty).toString(), title);
#endif // QT_QQUICKWEBVIEW_TESTS
}

void tst_QWebView::loadHtml()
{
#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
    QQmlEngine engine;
    QQmlContext *rootContext = engine.rootContext();
    QQuickWebView qview;
    QQmlEngine::setContextForObject(&qview, rootContext);
    QWebView &view = qview.webView();
#else
    QWebView view;
#endif
    QCOMPARE(view.loadProgress(), 0);
    view.loadHtml(QString("<html><head><title>WebViewTitle</title></head><body />"));
    QTRY_COMPARE(view.loadProgress(), 100);
    QTRY_VERIFY(!view.isLoading());
    QCOMPARE(view.title(), QStringLiteral("WebViewTitle"));
}

void tst_QWebView::loadRequest()
{
    // LoadSucceeded
    {
        QTemporaryFile file(m_cacheLocation + QStringLiteral("/XXXXXXfile.html"));
        QVERIFY2(file.open(),
                 qPrintable(QStringLiteral("Cannot create temporary file:") + file.errorString()));

        file.write("<html><head><title>FooBar</title></head><body />");
        const QString fileName = file.fileName();
        file.close();
#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
        QQmlEngine engine;
        QQmlContext *rootContext = engine.rootContext();
        QQuickWebView qview;
        QQmlEngine::setContextForObject(&qview, rootContext);
        QWebView &view = qview.webView();
#else
        QWebView view;
        view.getSettings()->setAllowFileAccess(true);
        view.getSettings()->setLocalContentCanAccessFileUrls(true);
#endif
        QCOMPARE(view.loadProgress(), 0);
        const QUrl url = QUrl::fromLocalFile(fileName);
        QSignalSpy loadChangedSingalSpy(&view, SIGNAL(loadingChanged(const QWebViewLoadRequestPrivate &)));
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
#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
        QQmlEngine engine;
        QQmlContext *rootContext = engine.rootContext();
        QQuickWebView qview;
        QQmlEngine::setContextForObject(&qview, rootContext);
        QWebView &view = qview.webView();
#else
        QWebView view;
        view.getSettings()->setAllowFileAccess(true);
        view.getSettings()->setLocalContentCanAccessFileUrls(true);
#endif
        QCOMPARE(view.loadProgress(), 0);
        QSignalSpy loadChangedSingalSpy(&view, SIGNAL(loadingChanged(const QWebViewLoadRequestPrivate &)));
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
#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
        QCOMPARE(view.loadProgress(), 0); // darwin plugin returns 100
#endif
    }
}

void tst_QWebView::setAndDeleteCookie()
{
#ifdef QT_WEBVIEW_WEBENGINE_BACKEND
    QQmlEngine engine;
    QQmlContext * rootContext = engine.rootContext();
    QQuickWebView qview;
    QQmlEngine::setContextForObject(&qview, rootContext);
    QWebView & view = qview.webView();
#else
    QWebView view;
    view.getSettings()->setLocalStorageEnabled(true);
    view.getSettings()->setAllowFileAccess(true);
    view.getSettings()->setLocalContentCanAccessFileUrls(true);
#endif

    QSignalSpy cookieAddedSpy(&view, SIGNAL(cookieAdded(const QString &, const QString &)));
    QSignalSpy cookieRemovedSpy(&view, SIGNAL(cookieRemoved(const QString &, const QString &)));

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
#ifdef Q_OS_ANDROID
    QEXPECT_FAIL("", "Notification for deleteAllCookies() is not implemented on Android, yet!", Continue);
#endif
    QTRY_COMPARE(cookieRemovedSpy.size(), 3);
}

QTEST_MAIN(tst_QWebView)

#include "tst_qwebview.moc"
