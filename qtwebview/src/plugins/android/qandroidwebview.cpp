// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidwebview_p.h"
#include <private/qwebview_p.h>
#include <private/qwebviewloadrequest_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qjniobject.h>

#include <QtCore/qmap.h>
#include <android/bitmap.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>

#include <QAbstractEventDispatcher>
#include <QThread>

QT_BEGIN_NAMESPACE

QAndroidWebViewSettingsPrivate::QAndroidWebViewSettingsPrivate(QJniObject viewController, QObject *p)
    : QAbstractWebViewSettings(p)
    , m_viewController(viewController)
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
    m_viewController.callMethod<void>("setAllowFileAccessFromFileURLs", "(Z)V", enabled);
}

void QAndroidWebViewSettingsPrivate::setJavascriptEnabled(bool enabled)
{
    m_viewController.callMethod<void>("setJavaScriptEnabled", "(Z)V", enabled);
}

void QAndroidWebViewSettingsPrivate::setLocalStorageEnabled(bool enabled)
{
    m_viewController.callMethod<void>("setLocalStorageEnabled", "(Z)V", enabled);
}

void QAndroidWebViewSettingsPrivate::setAllowFileAccess(bool enabled)
{
    m_viewController.callMethod<void>("setAllowFileAccess", "(Z)V", enabled);
}

static const char qtAndroidWebViewControllerClass[] = "org/qtproject/qt/android/view/QtAndroidWebViewController";

//static bool favIcon(JNIEnv *env, jobject icon, QImage *image)
//{
//    // TODO:
//    AndroidBitmapInfo bitmapInfo;
//    if (AndroidBitmap_getInfo(env, icon, &bitmapInfo) != ANDROID_BITMAP_RESULT_SUCCESS)
//        return false;

//    void *pixelData;
//    if (AndroidBitmap_lockPixels(env, icon, &pixelData) != ANDROID_BITMAP_RESULT_SUCCESS)
//        return false;

//    *image = QImage::fromData(static_cast<const uchar *>(pixelData), bitmapInfo.width * bitmapInfo.height);
//    AndroidBitmap_unlockPixels(env, icon);

//    return true;
//}

typedef QMap<quintptr, QAndroidWebViewPrivate *> WebViews;
Q_GLOBAL_STATIC(WebViews, g_webViews)

QAndroidWebViewPrivate::QAndroidWebViewPrivate(QObject *p)
    : QAbstractWebView(p)
    , m_id(reinterpret_cast<quintptr>(this))
    , m_callbackId(0)
    , m_window(0)
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
    m_viewController = QJniObject(qtAndroidWebViewControllerClass,
                                  "(Landroid/app/Activity;J)V",
                                  QtAndroidPrivate::activity(),
                                  m_id);

    QtAndroidPrivate::releaseAndroidDeadlockProtector();

    m_webView = m_viewController.callObjectMethod("getWebView",
                                                  "()Landroid/webkit/WebView;");
    m_settings = new QAndroidWebViewSettingsPrivate(m_viewController, this);

    m_window = QWindow::fromWinId(reinterpret_cast<WId>(m_webView.object()));
    g_webViews->insert(m_id, this);
    connect(qApp, &QGuiApplication::applicationStateChanged,
            this, &QAndroidWebViewPrivate::onApplicationStateChanged);
}

QAndroidWebViewPrivate::~QAndroidWebViewPrivate()
{
    g_webViews->take(m_id);
    if (m_window != 0) {
        m_window->setVisible(false);
        m_window->setParent(0);
        delete m_window;
    }

    m_viewController.callMethod<void>("destroy");
}

QString QAndroidWebViewPrivate::httpUserAgent() const
{
    return QString( m_viewController.callObjectMethod<jstring>("getUserAgent").toString());
}

void QAndroidWebViewPrivate::setHttpUserAgent(const QString &userAgent)
{
    m_viewController.callMethod<void>("setUserAgent",
                                      "(Ljava/lang/String;)V",
                                      QJniObject::fromString(userAgent).object());
    Q_EMIT httpUserAgentChanged(userAgent);
}

QUrl QAndroidWebViewPrivate::url() const
{
    return QUrl::fromUserInput(m_viewController.callObjectMethod<jstring>("getUrl").toString());
}

void QAndroidWebViewPrivate::setUrl(const QUrl &url)
{
    m_viewController.callMethod<void>("loadUrl",
                                      "(Ljava/lang/String;)V",
                                      QJniObject::fromString(url.toString()).object());
}

void QAndroidWebViewPrivate::loadHtml(const QString &html, const QUrl &baseUrl)
{
    const QJniObject &htmlString = QJniObject::fromString(html);
    const QJniObject &mimeTypeString = QJniObject::fromString(QLatin1String("text/html;charset=UTF-8"));

    baseUrl.isEmpty() ? m_viewController.callMethod<void>("loadData",
                                                          "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                                                          htmlString.object(),
                                                          mimeTypeString.object(),
                                                          0)

                      : m_viewController.callMethod<void>("loadDataWithBaseURL",
                                                          "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                                                          QJniObject::fromString(baseUrl.toString()).object(),
                                                          htmlString.object(),
                                                          mimeTypeString.object(),
                                                          0,
                                                          0);
}

bool QAndroidWebViewPrivate::canGoBack() const
{
    return m_viewController.callMethod<jboolean>("canGoBack");
}

void QAndroidWebViewPrivate::goBack()
{
    m_viewController.callMethod<void>("goBack");
}

bool QAndroidWebViewPrivate::canGoForward() const
{
    return m_viewController.callMethod<jboolean>("canGoForward");
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
    return m_viewController.callObjectMethod<jstring>("getTitle").toString();
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

    m_viewController.callMethod<void>("runJavaScript",
                                      "(Ljava/lang/String;J)V",
                                      static_cast<jstring>(QJniObject::fromString(script).object()),
                                      callbackId);
}

QAbstractWebViewSettings *QAndroidWebViewPrivate::getSettings() const
{
    return m_settings;
}

void QAndroidWebViewPrivate::setCookie(const QString &domain, const QString &name, const QString &value)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=]() {
        m_viewController.callMethod<void>("setCookie",
                                          "(Ljava/lang/String;Ljava/lang/String;)V",
                                          static_cast<jstring>(QJniObject::fromString(domain).object()),
                                          static_cast<jstring>(QJniObject::fromString(name + "=" + value).object()));
    });
}

void QAndroidWebViewPrivate::deleteCookie(const QString &domain, const QString &name)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=]() {
        m_viewController.callMethod<void>("removeCookie",
                                          "(Ljava/lang/String;Ljava/lang/String;)V",
                                          static_cast<jstring>(QJniObject::fromString(domain).object()),
                                          static_cast<jstring>(QJniObject::fromString(name.split(u'=').at(0) + u'=').object()));
    });
}

void QAndroidWebViewPrivate::deleteAllCookies()
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=]() {
        m_viewController.callMethod<void>("removeCookies");
    });
}

void QAndroidWebViewPrivate::setVisible(bool visible)
{
    m_window->setVisible(visible);
}

int QAndroidWebViewPrivate::loadProgress() const
{
    return m_viewController.callMethod<jint>("getProgress");
}

bool QAndroidWebViewPrivate::isLoading() const
{
    return m_viewController.callMethod<jboolean>("isLoading");
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

    const WebViews &wv = (*g_webViews);
    QAndroidWebViewPrivate *wc = static_cast<QAndroidWebViewPrivate *>(wv[id]);
    if (!wc)
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

static void c_onPageFinished(JNIEnv *env,
                             jobject thiz,
                             jlong id,
                             jstring url)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    const WebViews &wv = (*g_webViews);
    QAndroidWebViewPrivate *wc = wv[id];
    if (!wc)
        return;

    QWebViewLoadRequestPrivate loadRequest(QUrl(QJniObject(url).toString()),
                                           QWebView::LoadSucceededStatus,
                                           QString());
    Q_EMIT wc->loadingChanged(loadRequest);
}

static void c_onPageStarted(JNIEnv *env,
                            jobject thiz,
                            jlong id,
                            jstring url,
                            jobject icon)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    Q_UNUSED(icon);
    const WebViews &wv = (*g_webViews);
    QAndroidWebViewPrivate *wc = wv[id];
    if (!wc)
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

static void c_onProgressChanged(JNIEnv *env,
                                jobject thiz,
                                jlong id,
                                jint newProgress)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    const WebViews &wv = (*g_webViews);
    QAndroidWebViewPrivate *wc = wv[id];
    if (!wc)
        return;

    Q_EMIT wc->loadProgressChanged(newProgress);
}

static void c_onReceivedIcon(JNIEnv *env,
                             jobject thiz,
                             jlong id,
                             jobject icon)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    Q_UNUSED(id);
    Q_UNUSED(icon);

    const WebViews &wv = (*g_webViews);
    QAndroidWebViewPrivate *wc = wv[id];
    if (!wc)
        return;

    if (!icon)
        return;

//    QImage image;
//    if (favIcon(env, icon, &image))
//        Q_EMIT wc->iconChanged(image);
}

static void c_onReceivedTitle(JNIEnv *env,
                              jobject thiz,
                              jlong id,
                              jstring title)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    const WebViews &wv = (*g_webViews);
    QAndroidWebViewPrivate *wc = wv[id];
    if (!wc)
        return;

    const QString &qTitle = QJniObject(title).toString();
    Q_EMIT wc->titleChanged(qTitle);
}

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

    const WebViews &wv = (*g_webViews);
    QAndroidWebViewPrivate *wc = wv[id];
    if (!wc)
        return;
    QWebViewLoadRequestPrivate loadRequest(QUrl(QJniObject(url).toString()),
                                           QWebView::LoadFailedStatus,
                                           QJniObject(description).toString());
    Q_EMIT wc->loadingChanged(loadRequest);
}

static void c_onCookieAdded(JNIEnv *env,
                            jobject thiz,
                            jlong id,
                            jboolean result,
                            jstring domain,
                            jstring name)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    if (result) {
        const WebViews &wv = (*g_webViews);
        QAndroidWebViewPrivate *wc = wv[id];
        if (!wc)
            return;
        Q_EMIT wc->cookieAdded(QJniObject(domain).toString(), QJniObject(name).toString());
    }
}

static void c_onCookieRemoved(JNIEnv *env,
                              jobject thiz,
                              jlong id,
                              jboolean result,
                              jstring domain,
                              jstring name)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    if (result) {
        const WebViews &wv = (*g_webViews);
        QAndroidWebViewPrivate *wc = wv[id];
        if (!wc)
            return;
        Q_EMIT wc->cookieRemoved(QJniObject(domain).toString(), QJniObject(name).toString());
    }
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    typedef union {
        JNIEnv *nativeEnvironment;
        void *venv;
    } UnionJNIEnvToVoid;

    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;

    JNIEnv *env = uenv.nativeEnvironment;
    jclass clazz = QtAndroidPrivate::findClass(qtAndroidWebViewControllerClass, env);
    if (!clazz)
        return JNI_ERR;

    JNINativeMethod methods[] = {
        {"c_onPageFinished", "(JLjava/lang/String;)V", reinterpret_cast<void *>(c_onPageFinished)},
        {"c_onPageStarted", "(JLjava/lang/String;Landroid/graphics/Bitmap;)V", reinterpret_cast<void *>(c_onPageStarted)},
        {"c_onProgressChanged", "(JI)V", reinterpret_cast<void *>(c_onProgressChanged)},
        {"c_onReceivedIcon", "(JLandroid/graphics/Bitmap;)V", reinterpret_cast<void *>(c_onReceivedIcon)},
        {"c_onReceivedTitle", "(JLjava/lang/String;)V", reinterpret_cast<void *>(c_onReceivedTitle)},
        {"c_onRunJavaScriptResult", "(JJLjava/lang/String;)V", reinterpret_cast<void *>(c_onRunJavaScriptResult)},
        {"c_onReceivedError", "(JILjava/lang/String;Ljava/lang/String;)V", reinterpret_cast<void *>(c_onReceivedError)},
        {"c_onCookieAdded", "(JZLjava/lang/String;Ljava/lang/String;)V", reinterpret_cast<void *>(c_onCookieAdded)},
        {"c_onCookieRemoved", "(JZLjava/lang/String;Ljava/lang/String;)V", reinterpret_cast<void *>(c_onCookieRemoved)}
    };

    const int nMethods = sizeof(methods) / sizeof(methods[0]);

    if (env->RegisterNatives(clazz, methods, nMethods) != JNI_OK)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
