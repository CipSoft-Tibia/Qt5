// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTWEBVIEW_P_H
#define QABSTRACTWEBVIEW_P_H

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

#include "qwebviewinterface_p.h"
#include "qnativeviewcontroller_p.h"

QT_BEGIN_NAMESPACE

class QWebView;
class QWebViewLoadRequestPrivate;

class Q_WEBVIEW_EXPORT QAbstractWebViewSettings : public QObject
{
    Q_OBJECT
public:
    virtual bool localStorageEnabled() const = 0;
    virtual bool javascriptEnabled() const = 0;
    virtual bool localContentCanAccessFileUrls() const = 0;
    virtual bool allowFileAccess() const = 0;

    virtual void setLocalContentCanAccessFileUrls(bool) = 0;
    virtual void setJavascriptEnabled(bool) = 0;
    virtual void setLocalStorageEnabled(bool) = 0;
    virtual void setAllowFileAccess(bool) = 0;

protected:
    explicit QAbstractWebViewSettings(QObject *p = nullptr) : QObject(p) {}
};

class Q_WEBVIEW_EXPORT QAbstractWebView
        : public QObject
        , public QWebViewInterface
        , public QNativeViewController
{
    Q_OBJECT

public:
    virtual QAbstractWebViewSettings *getSettings() const = 0;

Q_SIGNALS:
    void titleChanged(const QString &title);
    void urlChanged(const QUrl &url);
    void loadingChanged(const QWebViewLoadRequestPrivate &loadRequest);
    void loadProgressChanged(int progress);
    void javaScriptResult(int id, const QVariant &result);
    void requestFocus(bool focus);
    void httpUserAgentChanged(const QString &httpUserAgent);
    void cookieAdded(const QString &domain, const QString &name);
    void cookieRemoved(const QString &domain, const QString &name);

protected:
    explicit QAbstractWebView(QObject *p = nullptr) : QObject(p) { }
};

QT_END_NAMESPACE

#endif // QABSTRACTWEBVIEW_P_H

