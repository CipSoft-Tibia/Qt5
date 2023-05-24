// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwebenginewebview_p.h"
#include <QtWebView/private/qwebview_p.h>
#include <QtWebView/private/qwebviewloadrequest_p.h>
#include <QtWebViewQuick/private/qquickwebview_p.h>

#include <QtCore/qmap.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qurl.h>

#include <QtQml/qqml.h>

#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>

#include <QtWebEngineQuick/private/qquickwebengineview_p.h>
#include <QtWebEngineCore/qwebengineloadinginfo.h>

#include <QWebEngineCookieStore>
#include <QNetworkCookie>

QT_BEGIN_NAMESPACE

static QByteArray qmlSource()
{
    return QByteArrayLiteral("import QtWebEngine 1.1\n"
                             "    WebEngineView {\n"
                             "}\n");
}

QWebEngineWebViewPrivate::QWebEngineWebViewPrivate(QObject *p)
    : QAbstractWebView(p), m_profile(nullptr)
{
    m_settings = new QWebEngineWebViewSettingsPrivate(this);
    m_webEngineView.m_parent = this;
    m_cookieStore.m_webEngineViewPtr = &m_webEngineView;
}

QWebEngineWebViewPrivate::~QWebEngineWebViewPrivate()
{
}

QString QWebEngineWebViewPrivate::httpUserAgent() const
{
    return m_httpUserAgent;
}

void QWebEngineWebViewPrivate::setHttpUserAgent(const QString &userAgent)
{
    m_httpUserAgent = userAgent;
    if (m_profile) {
        m_profile->setHttpUserAgent(userAgent);
        Q_EMIT httpUserAgentChanged(userAgent);
    }
}

QUrl QWebEngineWebViewPrivate::url() const
{
    return m_webEngineView->url();
}

void QWebEngineWebViewPrivate::setUrl(const QUrl &url)
{
    m_webEngineView->setUrl(url);
}

void QWebEngineWebViewPrivate::loadHtml(const QString &html, const QUrl &baseUrl)
{
    m_webEngineView->loadHtml(html, baseUrl);
}

bool QWebEngineWebViewPrivate::canGoBack() const
{
    return m_webEngineView->canGoBack();
}

void QWebEngineWebViewPrivate::goBack()
{
    m_webEngineView->goBack();
}

bool QWebEngineWebViewPrivate::canGoForward() const
{
    return m_webEngineView->canGoForward();
}

void QWebEngineWebViewPrivate::goForward()
{
    m_webEngineView->goForward();
}

void QWebEngineWebViewPrivate::reload()
{
    m_webEngineView->reload();
}

QString QWebEngineWebViewPrivate::title() const
{
    return m_webEngineView->title();
}

void QWebEngineWebViewPrivate::setGeometry(const QRect &geometry)
{
    m_webEngineView->setSize(geometry.size());
}

void QWebEngineWebViewPrivate::setVisibility(QWindow::Visibility visibility)
{
    setVisible(visibility != QWindow::Hidden ? true : false);
}

void QWebEngineWebViewPrivate::runJavaScriptPrivate(const QString &script,
                                                    int callbackId)
{
    m_webEngineView->runJavaScript(script, QQuickWebView::takeCallback(callbackId));
}

void QWebEngineWebViewPrivate::setCookie(const QString &domain, const QString &name, const QString &value)
{
    QNetworkCookie cookie;
    cookie.setDomain(domain);
    cookie.setName(QByteArray(name.toUtf8()));
    cookie.setValue(QByteArray(value.toUtf8()));
    cookie.setPath("/");

    m_cookieStore->setCookie(cookie);
}

void QWebEngineWebViewPrivate::deleteCookie(const QString &domain, const QString &name)
{
    QNetworkCookie cookie;
    cookie.setDomain(domain);
    cookie.setName(QByteArray(name.toUtf8()));
    cookie.setPath("/");

    m_cookieStore->deleteCookie(cookie);
}

void QWebEngineWebViewPrivate::deleteAllCookies()
{
    m_cookieStore->deleteAllCookies();
}

void QWebEngineWebViewPrivate::setVisible(bool visible)
{
    m_webEngineView->setVisible(visible);
}

void QWebEngineWebViewPrivate::setFocus(bool focus)
{
    if (focus)
        m_webEngineView->forceActiveFocus();
}

QAbstractWebViewSettings *QWebEngineWebViewPrivate::getSettings() const
{
    return m_settings;
}

int QWebEngineWebViewPrivate::loadProgress() const
{
    return m_webEngineView->loadProgress();
}

bool QWebEngineWebViewPrivate::isLoading() const
{
    return m_webEngineView->isLoading();
}

void QWebEngineWebViewPrivate::setParentView(QObject *parentView)
{
    Q_UNUSED(parentView);
}

QObject *QWebEngineWebViewPrivate::parentView() const
{
    return m_webEngineView->window();
}

void QWebEngineWebViewPrivate::stop()
{
    m_webEngineView->stop();
}

void QWebEngineWebViewPrivate::q_urlChanged()
{
    Q_EMIT urlChanged(m_webEngineView->url());
}

void QWebEngineWebViewPrivate::q_loadProgressChanged()
{
    Q_EMIT loadProgressChanged(m_webEngineView->loadProgress());
}

void QWebEngineWebViewPrivate::q_titleChanged()
{
    Q_EMIT titleChanged(m_webEngineView->title());
}

void QWebEngineWebViewPrivate::q_loadingChanged(const QWebEngineLoadingInfo &loadRequest)
{
    QWebViewLoadRequestPrivate lr(loadRequest.url(),
                                  static_cast<QWebView::LoadStatus>(loadRequest.status()), // These "should" match...
                                  loadRequest.errorString());

    Q_EMIT loadingChanged(lr);
}

void QWebEngineWebViewPrivate::q_profileChanged()
{
    auto profile = m_webEngineView->profile();
    if (profile == m_profile)
        return;

    m_profile = profile;
    auto userAgent = m_profile->httpUserAgent();
    if (m_httpUserAgent == userAgent)
        return;
    m_httpUserAgent = userAgent;
    QObject::connect(m_profile, &QQuickWebEngineProfile::httpUserAgentChanged, this, &QWebEngineWebViewPrivate::q_httpUserAgentChanged);
    Q_EMIT httpUserAgentChanged(userAgent);
}

void QWebEngineWebViewPrivate::q_httpUserAgentChanged()
{
    QString httpUserAgent = m_profile->httpUserAgent();
    if (m_httpUserAgent == httpUserAgent)
        return;
     m_httpUserAgent = httpUserAgent;
     Q_EMIT httpUserAgentChanged(m_httpUserAgent);
}

void QWebEngineWebViewPrivate::q_cookieAdded(const QNetworkCookie &cookie)
{
    Q_EMIT cookieAdded(cookie.domain(), cookie.name());
}

void QWebEngineWebViewPrivate::q_cookieRemoved(const QNetworkCookie &cookie)
{
    Q_EMIT cookieRemoved(cookie.domain(), cookie.name());
}

void QWebEngineWebViewPrivate::QQuickWebEngineViewPtr::init() const
{
    Q_ASSERT(!m_webEngineView);
    QObject *p = qobject_cast<QObject *>(m_parent);
    QQuickItem *parentItem = nullptr;
    while (p) {
        p = p->parent();
        parentItem = qobject_cast<QQuickWebView *>(p);
        if (parentItem)
            break;
    }

    if (!parentItem) {
        qWarning("Could not find QQuickWebView");
        return;
    }
    QQmlEngine *engine = qmlEngine(parentItem);
    if (!engine) {
        qWarning("Could not initialize qmlEngine");
        return;
    }
    QQmlComponent *component = new QQmlComponent(engine);
    component->setData(qmlSource(), QUrl::fromLocalFile(QLatin1String("")));
    QQuickWebEngineView *webEngineView = qobject_cast<QQuickWebEngineView *>(component->create());
    Q_ASSERT(webEngineView);
    QQuickWebEngineProfile *profile = webEngineView->profile();
    Q_ASSERT(profile);
    QQuickWebEngineSettings *settings = webEngineView->settings();
    Q_ASSERT(settings);
    m_parent->m_profile = profile;
    if (!m_parent->m_settings)
        m_parent->m_settings = new QWebEngineWebViewSettingsPrivate(m_parent);
    m_parent->m_settings->init(settings);
    webEngineView->settings()->setErrorPageEnabled(false);
    // When the httpUserAgent is set as a property then it will be set before
    // init() is called
    if (m_parent->m_httpUserAgent.isEmpty())
        m_parent->m_httpUserAgent = profile->httpUserAgent();
    else
        profile->setHttpUserAgent(m_parent->m_httpUserAgent);
    QObject::connect(webEngineView, &QQuickWebEngineView::urlChanged, m_parent, &QWebEngineWebViewPrivate::q_urlChanged);
    QObject::connect(webEngineView, &QQuickWebEngineView::loadProgressChanged, m_parent, &QWebEngineWebViewPrivate::q_loadProgressChanged);
    QObject::connect(webEngineView, &QQuickWebEngineView::loadingChanged, m_parent, &QWebEngineWebViewPrivate::q_loadingChanged);
    QObject::connect(webEngineView, &QQuickWebEngineView::titleChanged, m_parent, &QWebEngineWebViewPrivate::q_titleChanged);
    QObject::connect(webEngineView, &QQuickWebEngineView::profileChanged,m_parent, &QWebEngineWebViewPrivate::q_profileChanged);
    QObject::connect(profile, &QQuickWebEngineProfile::httpUserAgentChanged, m_parent, &QWebEngineWebViewPrivate::q_httpUserAgentChanged);

    webEngineView->setParentItem(parentItem);
    m_webEngineView.reset(webEngineView);

    if (!m_parent->m_cookieStore.m_cookieStore)
        m_parent->m_cookieStore.init();
}

void QWebEngineWebViewPrivate::QWebEngineCookieStorePtr::init() const
{
    if (!m_webEngineViewPtr->m_webEngineView)
        m_webEngineViewPtr->init();
    else {
        QWebEngineWebViewPrivate * parent = m_webEngineViewPtr->m_parent;
        QWebEngineCookieStore *cookieStore = parent->m_profile->cookieStore();
        m_cookieStore = cookieStore;

        QObject::connect(cookieStore, &QWebEngineCookieStore::cookieAdded, parent, &QWebEngineWebViewPrivate::q_cookieAdded);
        QObject::connect(cookieStore, &QWebEngineCookieStore::cookieRemoved, parent, &QWebEngineWebViewPrivate::q_cookieRemoved);
    }
}

QWebEngineWebViewSettingsPrivate::QWebEngineWebViewSettingsPrivate(QWebEngineWebViewPrivate *p)
    : QAbstractWebViewSettings(p)
{

}

bool QWebEngineWebViewSettingsPrivate::localStorageEnabled() const
{
    return m_settings ? m_settings->localStorageEnabled() : m_localStorageEnabled;
}
bool QWebEngineWebViewSettingsPrivate::javascriptEnabled() const
{
    return m_settings ? m_settings->javascriptEnabled() : m_javaScriptEnabled;
}
bool QWebEngineWebViewSettingsPrivate::localContentCanAccessFileUrls() const
{
    return m_settings ? m_settings->localContentCanAccessFileUrls() : m_localContentCanAccessFileUrlsEnabled;
}
bool QWebEngineWebViewSettingsPrivate::allowFileAccess() const
{
    return m_allowFileAccessEnabled;
}
void QWebEngineWebViewSettingsPrivate::setLocalContentCanAccessFileUrls(bool enabled)
{
    if (m_settings)
        m_settings->setLocalContentCanAccessFileUrls(enabled);

    m_localContentCanAccessFileUrlsEnabled  = enabled;
}
void QWebEngineWebViewSettingsPrivate::setJavascriptEnabled(bool enabled)
{
    if (m_settings)
        m_settings->setJavascriptEnabled(enabled);

    m_javaScriptEnabled = enabled;
}
void QWebEngineWebViewSettingsPrivate::setLocalStorageEnabled(bool enabled)
{
    // This separation is a bit different on the mobile platforms, so for now
    // we'll interpret this property to also affect the "off the record" profile setting.
    if (auto webview = qobject_cast<QWebEngineWebViewPrivate *>(parent())) {
        if (webview->m_profile)
            webview->m_profile->setOffTheRecord(enabled);
    }

    if (m_settings)
        m_settings->setLocalStorageEnabled(enabled);

    m_localStorageEnabled = enabled;
}
void QWebEngineWebViewSettingsPrivate::setAllowFileAccess(bool enabled)
{
    Q_UNUSED(enabled);
}

void QWebEngineWebViewSettingsPrivate::init(QQuickWebEngineSettings *settings)
{
    m_settings = settings;

    if (m_settings) {
        // Sync any values already set.
        setLocalContentCanAccessFileUrls(m_localContentCanAccessFileUrlsEnabled);
        setJavascriptEnabled(m_javaScriptEnabled);
        setLocalStorageEnabled(m_localStorageEnabled);
    }
}

QT_END_NAMESPACE
