// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidwebview_p.h"
#include <private/qwebview_p.h>
#include <private/qwebviewloadrequest_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qjniobject.h>

#include <QtCore/qset.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qabstracteventdispatcher.h>

#include <QtGui/qguiapplication.h>

#include <android/bitmap.h>
#include <android/log.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(Bitmap, "android/graphics/Bitmap");

using namespace QtJniTypes;
using namespace Qt::StringLiterals;

QAndroidWebViewSettingsPrivate::QAndroidWebViewSettingsPrivate(const WebViewController &viewController, QObject *p)
    : QAbstractWebViewSettings(p), m_viewController(viewController)
{
}

bool QAndroidWebViewSettingsPrivate::localStorageEnabled() const
{
     return m_viewController.callMethod<jboolean>("isLocalStorageEnabled");
}

bool QAndroidWebViewSettingsPrivate::javascriptEnabled() const
{
    return m_viewController.callMethod<jboolean>("isJavaScriptEnabled");
}

bool QAndroidWebViewSettingsPrivate::localContentCanAccessFileUrls() const
{
    return m_viewController.callMethod<jboolean>("isAllowFileAccessFromFileURLsEnabled");
}

bool QAndroidWebViewSettingsPrivate::allowFileAccess() const
{
    return m_viewController.callMethod<jboolean>("isAllowFileAccessEnabled");
}

void QAndroidWebViewSettingsPrivate::setLocalContentCanAccessFileUrls(bool enabled)
{
    m_viewController.callMethod<void>("setAllowFileAccessFromFileURLs", enabled);
}

void QAndroidWebViewSettingsPrivate::setJavascriptEnabled(bool enabled)
{
    m_viewController.callMethod<void>("setJavaScriptEnabled", enabled);
}

void QAndroidWebViewSettingsPrivate::setLocalStorageEnabled(bool enabled)
{
    m_viewController.callMethod<void>("setLocalStorageEnabled", enabled);
}

void QAndroidWebViewSettingsPrivate::setAllowFileAccess(bool enabled)
{
    m_viewController.callMethod<void>("setAllowFileAccess", enabled);
}

typedef QSet<QAndroidWebViewPrivate *> WebViews;
Q_GLOBAL_STATIC(WebViews, g_webViews)

QAndroidWebViewPrivate::QAndroidWebViewPrivate(QObject *p)
    : QAbstractWebView(p) , m_callbackId(0) , m_window(nullptr)
    , m_viewController(nullptr) , m_webView(nullptr)
{
    // QtAndroidWebViewController constructor blocks a qGuiThread until
    // the WebView is created and configured in UI thread.
    // That is why we cannot proceed until AndroidDeadlockProtector is locked
    while (!QtAndroidPrivate::acquireAndroidDeadlockProtector()) {
        auto eventDispatcher = QThread::currentThread()->eventDispatcher();
        if (eventDispatcher)
            eventDispatcher->processEvents(
                    QEventLoop::ExcludeUserInputEvents|QEventLoop::ExcludeSocketNotifiers);
    }
    m_viewController = WebViewController(QtAndroidPrivate::activity(), reinterpret_cast<jlong>(this));

    QtAndroidPrivate::releaseAndroidDeadlockProtector();

    m_webView = m_viewController.callMethod<WebView>("getWebView");
    m_settings = new QAndroidWebViewSettingsPrivate(m_viewController, this);

    m_window = QWindow::fromWinId(reinterpret_cast<WId>(m_webView.object()));
    g_webViews->insert(this);
    connect(qApp, &QGuiApplication::applicationStateChanged,
            this, &QAndroidWebViewPrivate::onApplicationStateChanged);
}

QAndroidWebViewPrivate::~QAndroidWebViewPrivate()
{
    g_webViews->remove(this);
    if (m_window != 0) {
        m_window->setVisible(false);
        m_window->setParent(0);
        delete m_window;
    }

    m_viewController.callMethod<void>("destroy");
}

QString QAndroidWebViewPrivate::httpUserAgent() const
{
    return m_viewController.callMethod<QString>("getUserAgent");
}

void QAndroidWebViewPrivate::setHttpUserAgent(const QString &userAgent)
{
    m_viewController.callMethod<void>("setUserAgent", userAgent);
    Q_EMIT httpUserAgentChanged(userAgent);
}

QUrl QAndroidWebViewPrivate::url() const
{
    return QUrl::fromUserInput(m_viewController.callMethod<QString>("getUrl"));
}

void QAndroidWebViewPrivate::setUrl(const QUrl &url)
{
    m_viewController.callMethod<void>("loadUrl", url.toString());
}

void QAndroidWebViewPrivate::loadHtml(const QString &html, const QUrl &baseUrl)
{
    const QString mimeTypeString = u"text/html;charset=UTF-8"_s;

    baseUrl.isEmpty() ? m_viewController.callMethod<void>("loadData", html, mimeTypeString,
                                                          jstring(nullptr))
                      : m_viewController.callMethod<void>("loadDataWithBaseURL",
                                                          baseUrl.toString(),
                                                          html, mimeTypeString,
                                                          jstring(nullptr), jstring(nullptr));
}

bool QAndroidWebViewPrivate::canGoBack() const
{
    return m_viewController.callMethod<bool>("canGoBack");
}

void QAndroidWebViewPrivate::goBack()
{
    m_viewController.callMethod<void>("goBack");
}

bool QAndroidWebViewPrivate::canGoForward() const
{
    return m_viewController.callMethod<bool>("canGoForward");
}

void QAndroidWebViewPrivate::goForward()
{
    m_viewController.callMethod<void>("goForward");
}

void QAndroidWebViewPrivate::reload()
{
    m_viewController.callMethod<void>("reload");
}

QString QAndroidWebViewPrivate::title() const
{
    return m_viewController.callMethod<QString>("getTitle");
}

void QAndroidWebViewPrivate::setGeometry(const QRect &geometry)
{
    if (m_window == 0)
        return;

    m_window->setGeometry(geometry);
}

void QAndroidWebViewPrivate::setVisibility(QWindow::Visibility visibility)
{
    m_window->setVisibility(visibility);
}

void QAndroidWebViewPrivate::runJavaScriptPrivate(const QString &script,
                                                  int callbackId)
{
    if (QtAndroidPrivate::androidSdkVersion() < 19) {
        qWarning("runJavaScript() requires API level 19 or higher.");
        if (callbackId == -1)
            return;

        // Emit signal here to remove the callback.
        Q_EMIT javaScriptResult(callbackId, QVariant());
    }

    m_viewController.callMethod<void>("runJavaScript", script, jlong(callbackId));
}

QAbstractWebViewSettings *QAndroidWebViewPrivate::getSettings() const
{
    return m_settings;
}

void QAndroidWebViewPrivate::setCookie(const QString &domain, const QString &name, const QString &value)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=]() {
        const QString cookie = name + u'=' + value;
        WebViewController::callStaticMethod<void>("setCookie", jlong(this), domain, cookie);
    });
}

void QAndroidWebViewPrivate::deleteCookie(const QString &domain, const QString &name)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=]() {
        const QString cookie = name.split(u'=').at(0) + u'=';
        WebViewController::callStaticMethod<void>("removeCookie", jlong(this), domain, cookie);
    });
}

void QAndroidWebViewPrivate::deleteAllCookies()
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=]() {
        WebViewController::callStaticMethod<void>("removeCookies");
    });
}

void QAndroidWebViewPrivate::setVisible(bool visible)
{
    m_window->setVisible(visible);
}

int QAndroidWebViewPrivate::loadProgress() const
{
    return m_viewController.callMethod<int>("getProgress");
}

bool QAndroidWebViewPrivate::isLoading() const
{
    return m_viewController.callMethod<bool>("isLoading");
}

void QAndroidWebViewPrivate::setParentView(QObject *view)
{
    m_window->setParent(qobject_cast<QWindow *>(view));
}

QObject *QAndroidWebViewPrivate::parentView() const
{
    return m_window->parent();
}

void QAndroidWebViewPrivate::stop()
{
    m_viewController.callMethod<void>("stopLoading");
}

//void QAndroidWebViewPrivate::initialize()
//{
//    // TODO:
//}

void QAndroidWebViewPrivate::onApplicationStateChanged(Qt::ApplicationState state)
{
    if (QtAndroidPrivate::androidSdkVersion() < 11)
        return;

    if (state == Qt::ApplicationActive)
        m_viewController.callMethod<void>("onResume");
    else
        m_viewController.callMethod<void>("onPause");
}

QT_END_NAMESPACE

static void c_onRunJavaScriptResult(JNIEnv *env,
                                    jobject thiz,
                                    jlong id,
                                    jlong callbackId,
                                    jstring result)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    const QString &resultString = QJniObject(result).toString();

    // The result string is in JSON format, lets parse it to see what we got.
    QJsonValue jsonValue;
    const QByteArray &jsonData = "{ \"data\": " + resultString.toUtf8() + " }";
    QJsonParseError error;
    const QJsonDocument &jsonDoc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error == QJsonParseError::NoError && jsonDoc.isObject()) {
        const QJsonObject &object = jsonDoc.object();
        jsonValue = object.value(QStringLiteral("data"));
    }

    Q_EMIT wc->javaScriptResult(int(callbackId),
                                jsonValue.isNull() ? resultString
                                                   : jsonValue.toVariant());
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onRunJavaScriptResult)

static void c_onPageFinished(JNIEnv *env,
                             jobject thiz,
                             jlong id,
                             jstring url)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    QWebViewLoadRequestPrivate loadRequest(QUrl(QJniObject(url).toString()),
                                           QWebView::LoadSucceededStatus,
                                           QString());
    Q_EMIT wc->loadingChanged(loadRequest);
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onPageFinished)

static void c_onPageStarted(JNIEnv *env,
                            jobject thiz,
                            jlong id,
                            jstring url,
                            Bitmap icon)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    Q_UNUSED(icon);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    QWebViewLoadRequestPrivate loadRequest(QUrl(QJniObject(url).toString()),
                                           QWebView::LoadStartedStatus,
                                           QString());
    Q_EMIT wc->loadingChanged(loadRequest);

//    if (!icon)
//        return;

//    QImage image;
//    if (favIcon(env, icon, &image))
//        Q_EMIT wc->iconChanged(image);
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onPageStarted)

static void c_onProgressChanged(JNIEnv *env,
                                jobject thiz,
                                jlong id,
                                jint newProgress)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    Q_EMIT wc->loadProgressChanged(newProgress);
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onProgressChanged)

static void c_onReceivedIcon(JNIEnv *env,
                             jobject thiz,
                             jlong id,
                             Bitmap icon)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    Q_UNUSED(icon);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    if (!icon.isValid())
        return;

//    QImage image;
//    if (favIcon(env, icon, &image))
//        Q_EMIT wc->iconChanged(image);
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onReceivedIcon)

static void c_onReceivedTitle(JNIEnv *env,
                              jobject thiz,
                              jlong id,
                              jstring title)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    const QString &qTitle = QJniObject(title).toString();
    Q_EMIT wc->titleChanged(qTitle);
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onReceivedTitle)

static void c_onReceivedError(JNIEnv *env,
                              jobject thiz,
                              jlong id,
                              jint errorCode,
                              jstring description,
                              jstring url)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    Q_UNUSED(errorCode);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    QWebViewLoadRequestPrivate loadRequest(QUrl(QJniObject(url).toString()),
                                           QWebView::LoadFailedStatus,
                                           QJniObject(description).toString());
    Q_EMIT wc->loadingChanged(loadRequest);
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onReceivedError)

static void c_onCookieAdded(JNIEnv *env,
                            jclass thiz,
                            jlong id,
                            jboolean result,
                            jstring domain,
                            jstring name)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    if (result)
        Q_EMIT wc->cookieAdded(QJniObject(domain).toString(), QJniObject(name).toString());
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onCookieAdded)

static void c_onCookieRemoved(JNIEnv *env,
                              jclass thiz,
                              jlong id,
                              jboolean result,
                              jstring domain,
                              jstring name)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    Q_ASSERT(id);
    QAndroidWebViewPrivate *wc = reinterpret_cast<QAndroidWebViewPrivate *>(id);
    if (!g_webViews->contains(wc))
        return;

    if (result)
        Q_EMIT wc->cookieRemoved(QJniObject(domain).toString(), QJniObject(name).toString());
}
Q_DECLARE_JNI_NATIVE_METHOD(c_onCookieRemoved)

JNIEXPORT jint JNI_OnLoad(JavaVM* /* vm */, void* /*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    if (!WebViewController::registerNativeMethods({
        Q_JNI_NATIVE_METHOD(c_onRunJavaScriptResult),
        Q_JNI_NATIVE_METHOD(c_onPageFinished),
        Q_JNI_NATIVE_METHOD(c_onPageStarted),
        Q_JNI_NATIVE_METHOD(c_onProgressChanged),
        Q_JNI_NATIVE_METHOD(c_onReceivedIcon),
        Q_JNI_NATIVE_METHOD(c_onReceivedTitle),
        Q_JNI_NATIVE_METHOD(c_onReceivedError),
        Q_JNI_NATIVE_METHOD(c_onCookieAdded),
        Q_JNI_NATIVE_METHOD(c_onCookieRemoved),
    })) {
        qCritical("Failed to register native methods for WebViewController");
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
