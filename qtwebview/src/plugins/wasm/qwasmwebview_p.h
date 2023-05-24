// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMWEBVIEW_P_H
#define QWASMWEBVIEW_P_H

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
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformwindow_p.h>
#include <emscripten/val.h>

#include <private/qabstractwebview_p.h>

#include <optional>

QT_BEGIN_NAMESPACE

class QWasmWebViewSettingsPrivate final : public QAbstractWebViewSettings
{
    Q_OBJECT
public:
    explicit QWasmWebViewSettingsPrivate(QObject *p = nullptr);

    bool localStorageEnabled() const final;
    bool javascriptEnabled() const final;
    bool localContentCanAccessFileUrls() const final;
    bool allowFileAccess() const final;

public Q_SLOTS:
    void setLocalContentCanAccessFileUrls(bool enabled) final;
    void setJavascriptEnabled(bool enabled) final;
    void setLocalStorageEnabled(bool enabled) final;
    void setAllowFileAccess(bool enabled) final;
};

class QWasmWebViewPrivate final : public QAbstractWebView
{
    Q_OBJECT
public:
    explicit QWasmWebViewPrivate(QObject *p = nullptr);
    ~QWasmWebViewPrivate() override;

    QString httpUserAgent() const final;
    void setHttpUserAgent(const QString &httpUserAgent) final;
    QUrl url() const final;
    void setUrl(const QUrl &url) final;
    bool canGoBack() const final;
    bool canGoForward() const final;
    QString title() const final;
    int loadProgress() const final;
    bool isLoading() const final;

    void setParentView(QObject *view) final;
    QObject *parentView() const final;
    void setGeometry(const QRect &geometry) final;
    void setVisibility(QWindow::Visibility visibility) final;
    void setVisible(bool visible) final;

public Q_SLOTS:
    void goBack() final;
    void goForward() final;
    void reload() final;
    void stop() final;
    void loadHtml(const QString &html, const QUrl &baseUrl = QUrl()) final;
    void setCookie(const QString &domain, const QString &name, const QString &value) final;
    void deleteCookie(const QString &domain, const QString &name) final;
    void deleteAllCookies() final;

protected:
    void runJavaScriptPrivate(const QString& script,
                              int callbackId) final;
    QAbstractWebViewSettings *getSettings() const final;

private:
    void initializeIFrame();
    void updateGeometry();

    QWasmWebViewSettingsPrivate *m_settings;
    QWindow *m_window = nullptr;
    std::optional<emscripten::val> m_iframe;
    std::optional<QRect> m_geometry;
    QUrl m_currentUrl;
};

QT_END_NAMESPACE

#endif // QWASMWEBVIEW_P_H
