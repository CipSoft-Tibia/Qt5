// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwebviewsettings_p.h"

#include <QtWebView/private/qwebview_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype WebViewSettings
    //! \instantiates QQuickWebViewSettings
    \inqmlmodule QtWebView
    \since QtWebView 6.5
    \brief Allows configuration of browser properties and attributes.

    The WebViewSettings type can be used to configure browser properties and generic
    attributes, such as JavaScript support, file access and local storage features.
    This type is uncreatable, but the default settings for all web views can be accessed by using
    the \l [QML] {WebView::settings}{WebView.settings} property.

    The default values are left as set by the different platforms.
*/

QQuickWebViewSettings::QQuickWebViewSettings(QWebViewSettings *webviewsettings, QObject *p)
    : QObject(p)
    , d(webviewsettings)
{
    connect(d, &QWebViewSettings::localStorageEnabledChanged,
            this, &QQuickWebViewSettings::localStorageEnabledChanged);
    connect(d, &QWebViewSettings::javaScriptEnabledChanged,
            this, &QQuickWebViewSettings::javaScriptEnabledChanged);
    connect(d, &QWebViewSettings::localContentCanAccessFileUrlsChanged,
            this, &QQuickWebViewSettings::localContentCanAccessFileUrlsChanged);
    connect(d, &QWebViewSettings::allowFileAccessChanged,
            this, &QQuickWebViewSettings::allowFileAccessChanged);
}

QQuickWebViewSettings::~QQuickWebViewSettings()
{

}

/*!
    \qmlproperty bool WebViewSettings::localStorageEnabled

    Enables support for the HTML 5 local storage feature.
*/
bool QQuickWebViewSettings::localStorageEnabled() const
{
    return d->localStorageEnabled();
}

void QQuickWebViewSettings::setLocalStorageEnabled(bool enabled)
{
    d->setLocalStorageEnabled(enabled);
}

/*!
    \qmlproperty bool WebViewSettings::javaScriptEnabled

    Enables the running of JavaScript programs.
*/
bool QQuickWebViewSettings::javaScriptEnabled() const
{
    return d->javaScriptEnabled();
}

void QQuickWebViewSettings::setJavaScriptEnabled(bool enabled)
{
    d->setJavaScriptEnabled(enabled);
}

/*!
    \qmlproperty bool WebViewSettings::localContentCanAccessFileUrls

    Allows locally loaded documents to access other local URLs.
*/
bool QQuickWebViewSettings::localContentCanAccessFileUrls() const
{
    return d->localContentCanAccessFileUrls();
}

void QQuickWebViewSettings::setLocalContentCanAccessFileUrls(bool enabled)
{
    d->setLocalContentCanAccessFileUrls(enabled);
}

/*!
    \qmlproperty bool WebViewSettings::allowFileAccess

    Enables the WebView to load file URLs.
*/
bool QQuickWebViewSettings::allowFileAccess() const
{
    return d->allowFileAccess();
}

void QQuickWebViewSettings::setAllowFileAccess(bool enabled)
{
    d->setAllowFileAccess(enabled);
}

QT_END_NAMESPACE
