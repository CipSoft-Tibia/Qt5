// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwebenginewebview_p.h"
#include <private/qwebviewplugin_p.h>

QT_BEGIN_NAMESPACE

class QWebEngineWebViewPlugin : public QWebViewPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWebViewPluginInterface_iid FILE "webengine.json")

public:
    QAbstractWebView *create(const QString &key) const override
    {
        return (key == QLatin1String("webview")) ? new QWebEngineWebViewPrivate() : nullptr;
    }

    void prepare() const override
    {
        QtWebEngineQuick::initialize();
    }
};

QT_END_NAMESPACE

#include "qwebenginewebviewplugin.moc"
