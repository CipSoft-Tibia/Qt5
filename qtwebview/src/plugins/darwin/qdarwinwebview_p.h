// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDARWINWEBVIEW_P_H
#define QDARWINWEBVIEW_P_H

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

#include <private/qabstractwebview_p.h>

#include <QtCore/qpointer.h>

Q_FORWARD_DECLARE_OBJC_CLASS(WKWebView);
Q_FORWARD_DECLARE_OBJC_CLASS(WKNavigation);
Q_FORWARD_DECLARE_OBJC_CLASS(WKWebViewConfiguration);

QT_BEGIN_NAMESPACE

class QDarwinWebViewSettingsPrivate : public QAbstractWebViewSettings
{
    Q_OBJECT
public:
    explicit QDarwinWebViewSettingsPrivate(WKWebViewConfiguration *conf, QObject *p = nullptr);

    bool localStorageEnabled() const;
    bool javaScriptEnabled() const;
    bool localContentCanAccessFileUrls() const;
    bool allowFileAccess() const;

public Q_SLOTS:
    void setLocalContentCanAccessFileUrls(bool enabled);
    void setJavaScriptEnabled(bool enabled);
    void setLocalStorageEnabled(bool enabled);
    void setAllowFileAccess(bool enabled);

private:
    WKWebViewConfiguration *m_conf = nullptr;
    bool m_allowFileAccess = false;
    bool m_localContentCanAccessFileUrls = false;
};

class QDarwinWebViewPrivate : public QAbstractWebView
{
    Q_OBJECT
public:
    explicit QDarwinWebViewPrivate(QObject *p = nullptr);
    ~QDarwinWebViewPrivate() override;

    QString httpUserAgent() const override;
    void setHttpUserAgent(const QString &httpUserAgent) override;
    QUrl url() const;
    void setUrl(const QUrl &url) override;
    bool canGoBack() const override;
    bool canGoForward() const override;
    QString title() const override;
    int loadProgress() const override;
    bool isLoading() const override;

    QWindow *nativeWindow() const override { return m_window; }
    QAbstractWebViewSettings *getSettings() const override;

public Q_SLOTS:
    void goBack() override;
    void goForward() override;
    void reload() override;
    void stop() override;
    void loadHtml(const QString &html, const QUrl &baseUrl = QUrl()) override;
    void setCookie(const QString &domain, const QString &name, const QString &value) override;
    void deleteCookie(const QString &domain, const QString &name) override;
    void deleteAllCookies() override;

protected:
    void runJavaScriptPrivate(const QString& script,
                              int callbackId) override;

public:
    WKWebView *wkWebView;
    WKNavigation *wkNavigation;
    QDarwinWebViewSettingsPrivate *m_settings = nullptr;
    QWindow *m_window = nullptr;
};

QT_END_NAMESPACE

#endif // QDARWINWEBVIEW_P_H
