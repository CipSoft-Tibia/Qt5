// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwebview_p.h"
#include "qwebviewplugin_p.h"
#include "qwebviewloadrequest_p.h"
#include "qwebviewfactory_p.h"


QT_BEGIN_NAMESPACE

QWebView::QWebView(QObject *p)
    : QAbstractWebView(p)
    , d(QWebViewFactory::createWebView())
    , m_settings(new QWebViewSettings(d->getSettings()))
    , m_progress(0)
{
    d->setParent(this);
    qRegisterMetaType<QWebViewLoadRequestPrivate>();

    connect(d, &QAbstractWebView::titleChanged, this, &QWebView::onTitleChanged);
    connect(d, &QAbstractWebView::urlChanged, this, &QWebView::onUrlChanged);
    connect(d, &QAbstractWebView::loadingChanged, this, &QWebView::onLoadingChanged);
    connect(d, &QAbstractWebView::loadProgressChanged, this, &QWebView::onLoadProgressChanged);
    connect(d, &QAbstractWebView::httpUserAgentChanged, this, &QWebView::onHttpUserAgentChanged);
    connect(d, &QAbstractWebView::javaScriptResult,
            this, &QWebView::javaScriptResult);
    connect(d, &QAbstractWebView::cookieAdded, this, &QWebView::cookieAdded);
    connect(d, &QAbstractWebView::cookieRemoved, this, &QWebView::cookieRemoved);
}

QWebView::~QWebView()
{
}

QString QWebView::httpUserAgent() const
{
    if (m_httpUserAgent.isEmpty()){
        m_httpUserAgent = d->httpUserAgent();
    }
    return m_httpUserAgent;
}

void QWebView::setHttpUserAgent(const QString &userAgent)
{
    return d->setHttpUserAgent(userAgent);
}

QUrl QWebView::url() const
{
    return m_url;
}

void QWebView::setUrl(const QUrl &url)
{
    d->setUrl(url);
}

bool QWebView::canGoBack() const
{
    return d->canGoBack();
}

void QWebView::goBack()
{
    d->goBack();
}

bool QWebView::canGoForward() const
{
    return d->canGoForward();
}

void QWebView::goForward()
{
    d->goForward();
}

void QWebView::reload()
{
    d->reload();
}

void QWebView::stop()
{
    d->stop();
}

QString QWebView::title() const
{
    return m_title;
}

int QWebView::loadProgress() const
{
    return m_progress;
}

bool QWebView::isLoading() const
{
    return d->isLoading();
}

QWebViewSettings *QWebView::getSettings() const
{
    return m_settings;
}

QWindow *QWebView::nativeWindow() const
{
    return d->nativeWindow();
}

void QWebView::loadHtml(const QString &html, const QUrl &baseUrl)
{
    d->loadHtml(html, baseUrl);
}

void QWebView::runJavaScriptPrivate(const QString &script,
                                    int callbackId)
{
    d->runJavaScriptPrivate(script, callbackId);
}

void QWebView::setCookie(const QString &domain, const QString &name, const QString &value)
{
    d->setCookie(domain, name, value);
}

void QWebView::deleteCookie(const QString &domain, const QString &name)
{
    d->deleteCookie(domain, name);
}

void QWebView::deleteAllCookies()
{
    d->deleteAllCookies();
}

void QWebView::onTitleChanged(const QString &title)
{
    if (m_title == title)
        return;

    m_title = title;
    Q_EMIT titleChanged();
}

void QWebView::onUrlChanged(const QUrl &url)
{
    if (m_url == url)
        return;

    m_url = url;
    Q_EMIT urlChanged();
}

void QWebView::onLoadProgressChanged(int progress)
{
    if (m_progress == progress)
        return;

    m_progress = progress;
    Q_EMIT loadProgressChanged();
}

void QWebView::onLoadingChanged(const QWebViewLoadRequestPrivate &loadRequest)
{
    if (loadRequest.m_status == QWebView::LoadFailedStatus)
        m_progress = 0;

    onUrlChanged(loadRequest.m_url);
    Q_EMIT loadingChanged(loadRequest);
}

void QWebView::onHttpUserAgentChanged(const QString &userAgent)
{
    if (m_httpUserAgent == userAgent)
        return;
    m_httpUserAgent = userAgent;
    Q_EMIT httpUserAgentChanged();
}

QWebViewSettings::QWebViewSettings(QAbstractWebViewSettings *settings)
    : d(settings)
{
    Q_ASSERT(settings != nullptr);
}

QWebViewSettings::~QWebViewSettings()
{

}

bool QWebViewSettings::localStorageEnabled() const
{
    return d->localStorageEnabled();
}

void QWebViewSettings::setLocalStorageEnabled(bool enabled)
{
    if (d->localStorageEnabled() == enabled)
        return;

    d->setLocalStorageEnabled(enabled);
    emit localStorageEnabledChanged();
}

bool QWebViewSettings::javaScriptEnabled() const
{
    return d->javaScriptEnabled();
}

void QWebViewSettings::setJavaScriptEnabled(bool enabled)
{
    if (d->javaScriptEnabled() == enabled)
        return;

    d->setJavaScriptEnabled(enabled);
    emit javaScriptEnabledChanged();
}

void QWebViewSettings::setAllowFileAccess(bool enabled)
{
    if (d->allowFileAccess() == enabled)
        return;

    d->setAllowFileAccess(enabled);
    emit allowFileAccessChanged();
}

bool QWebViewSettings::allowFileAccess() const
{
    return d->allowFileAccess();
}

bool QWebViewSettings::localContentCanAccessFileUrls() const
{
    return d->localContentCanAccessFileUrls();
}

void QWebViewSettings::setLocalContentCanAccessFileUrls(bool enabled)
{
    if (d->localContentCanAccessFileUrls() == enabled)
        return;

    d->setLocalContentCanAccessFileUrls(enabled);
    emit localContentCanAccessFileUrlsChanged();
}

QT_END_NAMESPACE
