// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWEBVIEW2WEBVIEW_P_H
#define QWEBVIEW2WEBVIEW_P_H

#include <private/qabstractwebview_p.h>

#include <QMap>
#include <QPointer>
#include <webview2.h>
#include <wrl.h>
#include <wrl/client.h>

QT_BEGIN_NAMESPACE

using namespace Microsoft::WRL;

class QWebview2WebViewSettingsPrivate final : public QAbstractWebViewSettings
{
    Q_OBJECT
public:
    explicit QWebview2WebViewSettingsPrivate(QObject *p = nullptr);

    void init(ICoreWebView2Controller* viewController);

    bool localStorageEnabled() const final;
    bool javaScriptEnabled() const final;
    bool localContentCanAccessFileUrls() const final;
    bool allowFileAccess() const final;

public Q_SLOTS:
    void setLocalContentCanAccessFileUrls(bool enabled) final;
    void setJavaScriptEnabled(bool enabled) final;
    void setLocalStorageEnabled(bool enabled) final;
    void setAllowFileAccess(bool enabled) final;

private:
    ComPtr<ICoreWebView2Controller> m_webviewController;
    ComPtr<ICoreWebView2> m_webview;
    bool m_allowFileAccess = false;
    bool m_localContentCanAccessFileUrls = false;
    bool m_javaScriptEnabled = true;
};

// This is used to store informations before webview2 is initialized
// Because WebView2 initialization is async
struct QWebViewInitData{
    QString m_html;
    struct CookieData{
        QString domain;
        QString name;
        QString value;
    };
    QMap<QString, CookieData > m_cookies;
    QString m_httpUserAgent;
};

class QWebView2WebViewPrivate : public QAbstractWebView
{
    Q_OBJECT
public:
    explicit QWebView2WebViewPrivate(QObject *parent = nullptr);
    ~QWebView2WebViewPrivate() override;

    QString httpUserAgent() const override;
    void setHttpUserAgent(const QString &userAgent) override;
    void setUrl(const QUrl &url) override;
    bool canGoBack() const override;
    bool canGoForward() const override;
    QString title() const override;
    int loadProgress() const override;
    bool isLoading() const override;

    QWindow* nativeWindow() const override;

public Q_SLOTS:
    void goBack() override;
    void goForward() override;
    void reload() override;
    void stop() override;
    void loadHtml(const QString &html, const QUrl &baseUrl = QUrl()) override;
    void setCookie(const QString &domain, const QString &name, const QString &value) override;
    void deleteCookie(const QString &domain, const QString &name) override;
    void deleteAllCookies() override;

private Q_SLOTS:
    HRESULT onNavigationStarting(ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args);
    HRESULT onNavigationCompleted(ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args);
    HRESULT onWebResourceRequested(ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args);
    HRESULT onContentLoading(ICoreWebView2* webview, ICoreWebView2ContentLoadingEventArgs* args);
    HRESULT onNewWindowRequested(ICoreWebView2* webview, ICoreWebView2NewWindowRequestedEventArgs* args);
    void updateWindowGeometry();
    void initialize(HWND hWnd);

protected:
    void runJavaScriptPrivate(const QString &script, int callbackId) override;
    QAbstractWebViewSettings *getSettings() const override;

private:
    ComPtr<ICoreWebView2Controller> m_webviewController;
    ComPtr<ICoreWebView2> m_webview;
    ComPtr<ICoreWebView2CookieManager> m_cookieManager;
    QWebview2WebViewSettingsPrivate *m_settings;
    QPointer<QWindow> m_window;
    bool m_isLoading;
    QUrl m_url;
    QWebViewInitData m_initData;
};

QT_END_NAMESPACE

#endif // QWEBVIEW2WEBVIEW_P_H
