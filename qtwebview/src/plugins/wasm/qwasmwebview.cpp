// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmwebview_p.h"
#include <private/qwebview_p.h>
#include <private/qwebviewloadrequest_p.h>

#include <QtCore/qmap.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>

#include <QAbstractEventDispatcher>
#include <QThread>

#include <iostream>

QT_BEGIN_NAMESPACE

QWasmWebViewSettingsPrivate::QWasmWebViewSettingsPrivate(QObject *p) : QAbstractWebViewSettings(p)
{
}

bool QWasmWebViewSettingsPrivate::localStorageEnabled() const
{
    qWarning("localStorageEnabled() not supported on this platform");
    return false;
}

bool QWasmWebViewSettingsPrivate::javaScriptEnabled() const
{
    qWarning("javaScriptEnabled() not supported on this platform");
    return false;
}

bool QWasmWebViewSettingsPrivate::localContentCanAccessFileUrls() const
{
    qWarning("localContentCanAccessFileUrls() not supported on this platform");
    return false;
}

bool QWasmWebViewSettingsPrivate::allowFileAccess() const
{
    qWarning("allowFileAccess() not supported on this platform");
    return false;
}

void QWasmWebViewSettingsPrivate::setLocalContentCanAccessFileUrls(bool enabled)
{
    Q_UNUSED(enabled);
    qWarning("setLocalContentCanAccessFileUrls() not supported on this platform");
}

void QWasmWebViewSettingsPrivate::setJavaScriptEnabled(bool enabled)
{
    Q_UNUSED(enabled);
    qWarning("setJavaScriptEnabled() not supported on this platform");
}

void QWasmWebViewSettingsPrivate::setLocalStorageEnabled(bool enabled)
{
    Q_UNUSED(enabled);
    qWarning("setLocalStorageEnabled() not supported on this platform");
}

void QWasmWebViewSettingsPrivate::setAllowFileAccess(bool enabled)
{
    Q_UNUSED(enabled);
    qWarning("setAllowFileAccess() not supported on this platform");
}

QWasmWebViewPrivate::QWasmWebViewPrivate(QObject *p) : QAbstractWebView(p), m_window(0)
{
    m_settings = new QWasmWebViewSettingsPrivate(this);
}

QWasmWebViewPrivate::~QWasmWebViewPrivate() { }

void QWasmWebViewPrivate::setParentView(QObject *view)
{
    m_parentWindow = qobject_cast<QWindow *>(view);
    if (m_parentWindow)
        QMetaObject::invokeMethod(this, &QWasmWebViewPrivate::initializeIFrame, Qt::QueuedConnection);
}

void QWasmWebViewPrivate::geometryChange(const QRectF &geometry)
{
    m_geometry = { geometry.toRect() };
    updateGeometry();
}

QString QWasmWebViewPrivate::httpUserAgent() const
{
    if (m_iframe)
        return QString::fromStdString(
                (*m_iframe)["contentWindow"]["navigator"]["userAgent"].as<std::string>());
    return QString();
}

void QWasmWebViewPrivate::setHttpUserAgent(const QString &userAgent)
{
    Q_UNUSED(userAgent);
    qWarning("setHttpUserAgent() not supported on this platform");
}

void QWasmWebViewPrivate::setUrl(const QUrl &url)
{
    m_currentUrl = url;
    if (m_iframe) {
        (*m_iframe).call<void>("removeAttribute", emscripten::val("srcdoc"));
        (*m_iframe).set("src", m_currentUrl.toString().toStdString());
    }
}

void QWasmWebViewPrivate::loadHtml(const QString &html, const QUrl &baseUrl)
{
    if (!baseUrl.isEmpty())
        qWarning("Base URLs for loadHtml() are not supported on this platform.");

    if (m_iframe)
        (*m_iframe).set("srcdoc", html.toStdString());
}

bool QWasmWebViewPrivate::canGoBack() const
{
    qWarning("canGoBack() not supported on this platform");
    return false;
}

void QWasmWebViewPrivate::goBack()
{
    qWarning("goBack() not supported on this platform");
}

bool QWasmWebViewPrivate::canGoForward() const
{
    qWarning("canGoForward() not supported on this platform");
    return false;
}

void QWasmWebViewPrivate::goForward()
{
    qWarning("goForward() not supported on this platform");
}

void QWasmWebViewPrivate::reload()
{
    if (m_iframe) {
        (*m_iframe)["contentWindow"]["location"].call<void>("reload");
        setUrl(m_currentUrl);
        // this order of operation is important, setting URL before reload()
        // did not work, HTTP reload request was being cancelled by browser
    }
}

void QWasmWebViewPrivate::stop()
{
    qWarning("stop() not supported on this platform");
}

QString QWasmWebViewPrivate::title() const
{
    if (m_iframe)
        return QString::fromStdString(
                (*m_iframe)["contentWindow"]["document"]["title"].as<std::string>());
    return QString();
}

int QWasmWebViewPrivate::loadProgress() const
{
    qWarning("loadProgress() not supported on this platform");
    return 100;
}

bool QWasmWebViewPrivate::isLoading() const
{
    qWarning("isLoading() not supported on this platform");
    return false;
}

void QWasmWebViewPrivate::setCookie(const QString &domain, const QString &name,
                                    const QString &value)
{
    Q_UNUSED(domain);
    Q_UNUSED(name);
    Q_UNUSED(value);
    qWarning("setCookie() not supported on this platform");
}

void QWasmWebViewPrivate::deleteCookie(const QString &domain, const QString &name)
{
    Q_UNUSED(domain);
    Q_UNUSED(name);
    qWarning("deleteCookie() not supported on this platform");
}

void QWasmWebViewPrivate::deleteAllCookies()
{
    qWarning("deleteAllCookies() not supported on this platform");
}

void QWasmWebViewPrivate::runJavaScriptPrivate(const QString &script, int callbackId)
{
    Q_UNUSED(script);
    Q_UNUSED(callbackId);
    qWarning("runJavaScriptPrivate() not supported on this platform");
}

QAbstractWebViewSettings *QWasmWebViewPrivate::getSettings() const
{
    return m_settings;
}

void QWasmWebViewPrivate::initializeIFrame()
{
    if (auto wasmWindow = dynamic_cast<QNativeInterface::Private::QWasmWindow *>(m_parentWindow->handle())) {
        auto document = wasmWindow->document();
        auto clientArea = wasmWindow->clientArea();

        m_iframe = document.call<emscripten::val>("createElement", emscripten::val("iframe"));
        clientArea.call<void>("appendChild", *m_iframe);
        (*m_iframe)["style"].set("position", "absolute");
        (*m_iframe)["style"].set("border", "none");
        m_window = QWindow::fromWinId(reinterpret_cast<WId>(&m_iframe.value()));
        Q_EMIT nativeWindowChanged(m_window);
        updateGeometry();
        // NOTE: Make sure any pending url is set now.
        setUrl(m_currentUrl);
    }
}

void QWasmWebViewPrivate::updateGeometry()
{
    if (m_iframe && m_geometry) {
        (*m_iframe)["style"].set("width", std::to_string(m_geometry->width()) + "px");
        (*m_iframe)["style"].set("height", std::to_string(m_geometry->height()) + "px");
        (*m_iframe)["style"].set("top", std::to_string(m_geometry->top()) + "px");
        (*m_iframe)["style"].set("left", std::to_string(m_geometry->left()) + "px");
    }
}

QT_END_NAMESPACE
