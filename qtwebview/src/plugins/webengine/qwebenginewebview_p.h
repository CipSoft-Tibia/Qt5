// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWEBENGINEWEBVIEW_P_H
#define QWEBENGINEWEBVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtGui/qwindow.h>

#include <QtQml/qqmlcomponent.h>

#include <private/qabstractwebview_p.h>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <QtWebEngineQuick/private/qquickwebenginesettings_p.h>

QT_BEGIN_NAMESPACE

class QQuickWebEngineView;
class QWebEngineLoadingInfo;
class QNetworkCookie;
class QWebEngineWebViewPrivate;

class QWebEngineWebViewSettingsPrivate : public QAbstractWebViewSettings
{
    Q_OBJECT
public:
    explicit QWebEngineWebViewSettingsPrivate(QWebEngineWebViewPrivate *p = nullptr);

    bool localStorageEnabled() const override;
    bool javascriptEnabled() const override;
    bool localContentCanAccessFileUrls() const override;
    bool allowFileAccess() const override;

public Q_SLOTS:
    void setLocalContentCanAccessFileUrls(bool enabled) override;
    void setJavascriptEnabled(bool enabled) override;
    void setLocalStorageEnabled(bool enabled) override;
    void setAllowFileAccess(bool enabled) override;

    void init(QQuickWebEngineSettings *settings);

private:
    QPointer<QQuickWebEngineSettings> m_settings;
    bool m_localStorageEnabled = true;
    bool m_javaScriptEnabled = true;
    bool m_localContentCanAccessFileUrlsEnabled = true;
    bool m_allowFileAccessEnabled = true;
};

class QWebEngineWebViewPrivate : public QAbstractWebView
{
    Q_OBJECT
public:
    explicit QWebEngineWebViewPrivate(QObject *p = nullptr);
    ~QWebEngineWebViewPrivate() override;

    QString httpUserAgent() const override;
    void setHttpUserAgent(const QString &userAgent) override;
    QUrl url() const override;
    void setUrl(const QUrl &url) override;
    bool canGoBack() const override;
    bool canGoForward() const override;
    QString title() const override;
    int loadProgress() const override;
    bool isLoading() const override;

    void setParentView(QObject *parentView) override;
    QObject *parentView() const override;
    void setGeometry(const QRect &geometry) override;
    void setVisibility(QWindow::Visibility visibility) override;
    void setVisible(bool visible) override;
    void setFocus(bool focus) override;
    QAbstractWebViewSettings *getSettings() const override;

public Q_SLOTS:
    void goBack() override;
    void goForward() override;
    void reload() override;
    void stop() override;
    void loadHtml(const QString &html, const QUrl &baseUrl = QUrl()) override;
    void setCookie(const QString &domain, const QString &name,
                   const QString &value) override;
    void deleteCookie(const QString &domain, const QString &name) override;
    void deleteAllCookies() override;

private Q_SLOTS:
    void q_urlChanged();
    void q_loadProgressChanged();
    void q_titleChanged();
    void q_loadingChanged(const QWebEngineLoadingInfo &loadRequest);
    void q_profileChanged();
    void q_httpUserAgentChanged();
    void q_cookieAdded(const QNetworkCookie &cookie);
    void q_cookieRemoved(const QNetworkCookie &cookie);

protected:
    void runJavaScriptPrivate(const QString& script,
                              int callbackId) override;
private:
    friend class QWebEngineWebViewSettingsPrivate;

    QQuickWebEngineProfile *m_profile = nullptr;
    mutable QWebEngineWebViewSettingsPrivate *m_settings = nullptr;
    QString m_httpUserAgent;
    struct QQuickWebEngineViewPtr
    {
        inline QQuickWebEngineView *operator->() const
        {
            if (!m_webEngineView)
                init();
            return m_webEngineView.data();
        }
        void init() const;

        QWebEngineWebViewPrivate *m_parent;
        mutable QScopedPointer<QQuickWebEngineView> m_webEngineView;
    } m_webEngineView;
    struct QWebEngineCookieStorePtr
    {
        inline QWebEngineCookieStore *operator->() const
        {
            if (!m_cookieStore)
                init();
            return m_cookieStore;
        }
        void init() const;

        QQuickWebEngineViewPtr *m_webEngineViewPtr = nullptr;
        mutable QWebEngineCookieStore *m_cookieStore = nullptr;
    } m_cookieStore;
};

QT_END_NAMESPACE

#endif // QWEBENGINEWEBVIEW_P_H
