// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwebview2webview_p.h"
#include <QtWebView/private/qwebviewplugin_p.h>

QT_BEGIN_NAMESPACE

class QWebView2WebViewPlugin : public QWebViewPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWebViewPluginInterface_iid FILE "windows.json")

public:
    QAbstractWebView *create(const QString &key) const override
    {
        return (key == QLatin1String("webview")) ? new QWebView2WebViewPrivate() : nullptr;
    }
};

QT_END_NAMESPACE

#include "qwebview2webviewplugin.moc"
