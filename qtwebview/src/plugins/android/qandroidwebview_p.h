// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDWEBVIEW_P_H
#define QANDROIDWEBVIEW_P_H

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
#include <QtCore/qjniobject.h>

#include <private/qabstractwebview_p.h>

QT_BEGIN_NAMESPACE

class QAndroidWebViewSettingsPrivate : public QAbstractWebViewSettings
{
    Q_OBJECT
public:
    explicit QAndroidWebViewSettingsPrivate(QJniObject viewController, QObject *p = nullptr);

    bool localStorageEnabled() const;
    bool javascriptEnabled() const;
    bool localContentCanAccessFileUrls() const;
    bool allowFileAccess() const;

public Q_SLOTS:
    void setLocalContentCanAccessFileUrls(bool enabled);
    void setJavascriptEnabled(bool enabled);
    void setLocalStorageEnabled(bool enabled);
    void setAllowFileAccess(bool enabled);

private:
    QJniObject m_viewController;
};

class QAndroidWebViewPrivate : public QAbstractWebView
{
    Q_OBJECT
public:
    explicit QAndroidWebViewPrivate(QObject *p = nullptr);
    ~QAndroidWebViewPrivate() override;

    QString httpUserAgent() const override;
    void setHttpUserAgent(const QString &httpUserAgent) override;
    QUrl url() const override;
    void setUrl(const QUrl &url) override;
    bool canGoBack() const override;
    bool canGoForward() const override;
    QString title() const override;
    int loadProgress() const override;
    bool isLoading() const override;

    void setParentView(QObject *view) override;
    QObject *parentView() const override;
    void setGeometry(const QRect &geometry) override;
    void setVisibility(QWindow::Visibility visibility) override;
    void setVisible(bool visible) override;

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
    QAbstractWebViewSettings *getSettings() const override;

private Q_SLOTS:
    void onApplicationStateChanged(Qt::ApplicationState state);

private:
    quintptr m_id;
    quint64 m_callbackId;
    QWindow *m_window;
    QJniObject m_viewController;
    QJniObject m_webView;
    QAndroidWebViewSettingsPrivate *m_settings;
};

QT_END_NAMESPACE

#endif // QANDROIDWEBVIEW_P_H
