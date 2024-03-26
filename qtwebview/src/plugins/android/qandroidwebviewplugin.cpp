// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidwebview_p.h"
#include <private/qwebviewplugin_p.h>

QT_BEGIN_NAMESPACE

class QAndroidWebViewPlugin : public QWebViewPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWebViewPluginInterface_iid FILE "android.json")

public:
    QAbstractWebView *create(const QString &key) const override
    {
        return (key == QLatin1String("webview")) ? new QAndroidWebViewPrivate() : nullptr;
    }
};

QT_END_NAMESPACE

#include "qandroidwebviewplugin.moc"
