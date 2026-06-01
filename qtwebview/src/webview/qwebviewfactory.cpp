// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwebviewfactory_p.h"
#include "qwebviewplugin_p.h"
#include <private/qfactoryloader_p.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader, (QWebViewPluginInterface_iid, QLatin1String("/webview")))

static QString getPluginName()
{
    static const QString name = !qEnvironmentVariableIsEmpty("QT_WEBVIEW_PLUGIN")
                                ? QString::fromLatin1(qgetenv("QT_WEBVIEW_PLUGIN"))
                                : QStringLiteral("native");
    return name;
}

class QNullWebViewSettings : public QAbstractWebViewSettings
{
public:
    explicit QNullWebViewSettings(QObject *p) : QAbstractWebViewSettings(p) {}
    bool localStorageEnabled() const override { return false; }
    bool javaScriptEnabled() const override { return false; }
    bool localContentCanAccessFileUrls() const override { return false; }
    bool allowFileAccess() const override { return false; }
    void setLocalContentCanAccessFileUrls(bool) override {}
    void setJavaScriptEnabled(bool) override {}
    void setLocalStorageEnabled(bool) override {}
    void setAllowFileAccess(bool) override {}
};

class QNullWebView : public QAbstractWebView
{
public:
    explicit QNullWebView(QObject *p = nullptr)
        : QAbstractWebView(p)
        , m_settings(new QNullWebViewSettings(this))
    {}

    QString httpUserAgent() const override { return QString(); }
    void setHttpUserAgent(const QString &userAgent) override { Q_UNUSED(userAgent); }
    void setUrl(const QUrl &url) override { Q_UNUSED(url); }
    bool canGoBack() const override { return false; }
    bool canGoForward() const override { return false; }
    QString title() const override { return QString(); }
    int loadProgress() const override { return 0; }
    bool isLoading() const override { return false; }
    void goBack() override { }
    void goForward() override { }
    void stop() override { }
    void reload() override { }
    void loadHtml(const QString &html, const QUrl &baseUrl) override
    { Q_UNUSED(html); Q_UNUSED(baseUrl); }
    void runJavaScriptPrivate(const QString &script, int callbackId) override
    { Q_UNUSED(script); Q_UNUSED(callbackId); }
    void setCookie(const QString &domain, const QString &name, const QString &value) override
    { Q_UNUSED(domain); Q_UNUSED(name); Q_UNUSED(value); }
    void deleteCookie(const QString &domain, const QString &name) override
    { Q_UNUSED(domain); Q_UNUSED(name); }
    void deleteAllCookies() override {}
    QWindow *nativeWindow() const override { return nullptr; }

protected:
    QAbstractWebViewSettings *getSettings() const override
    {
        return m_settings;
    }

private:
    QNullWebViewSettings *m_settings = nullptr;
};

QAbstractWebView *QWebViewFactory::createWebView()
{
    QAbstractWebView *wv = nullptr;
    QWebViewPlugin *plugin = getPlugin();
    if (plugin)
        wv = plugin->create(QStringLiteral("webview"));

    if (!wv || !plugin) {
        qWarning("No WebView plug-in found!");
        wv = new QNullWebView;
    }

    return wv;
}

bool QWebViewFactory::requiresExtraInitializationSteps()
{
    const QString pluginName = getPluginName();
    const int index = pluginName.isEmpty() ? 0 : qMax<int>(0, loader->indexOf(pluginName));

    const QList<QPluginParsedMetaData> metaDataList = loader->metaData();
    if (metaDataList.isEmpty())
        return false;

    const auto &pluginMetaData = metaDataList.at(index);
    Q_ASSERT(pluginMetaData.value(QtPluginMetaDataKeys::IID) == QLatin1String(QWebViewPluginInterface_iid));
    const auto metaDataObject = pluginMetaData.value(QtPluginMetaDataKeys::MetaData).toMap();
    return metaDataObject.value(QLatin1String("RequiresInit")).toBool();
}

QWebViewPlugin *QWebViewFactory::getPlugin()
{
    // Plugin loading logic:
    // 1. Get plugin name - plugin name is either user specified or "native"
    //    - Exception: macOS, which will default to using "webengine" until the native plugin is matured.
    // 2. If neither a user specified or "default" plugin exists, then the first available is used.
    const QString pluginName = getPluginName();
    const int index = pluginName.isEmpty() ? 0 : qMax<int>(0, loader->indexOf(pluginName));
    return qobject_cast<QWebViewPlugin *>(loader->instance(index));
}

bool QWebViewFactory::loadedPluginHasKey(const QString key)
{
    const QString &pluginName = getPluginName();
    // instead of creating multimap with QFactoryLoader::KeyMap and doing a search
    // simply check if loded and key index matches
    if (pluginName.isEmpty())
        return false;
    const int loadedIndex = qMax<int>(0, loader->indexOf(pluginName));
    const int keyIndex = loader->indexOf(key);
    return keyIndex > -1 && loadedIndex == keyIndex;
}

QT_END_NAMESPACE
