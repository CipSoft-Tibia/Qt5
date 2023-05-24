// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINRTWEBVIEW_P_H
#define QWINRTWEBVIEW_P_H

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

#include <private/qabstractwebview_p.h>

namespace ABI {
    namespace Windows {
        namespace UI {
            namespace Xaml {
                namespace Controls {
                    struct IWebView;
                    struct IWebViewNavigationStartingEventArgs;
                    struct IWebViewNavigationCompletedEventArgs;
                    struct IWebViewUnviewableContentIdentifiedEventArgs;
                }
            }
        }
    }
}

QT_BEGIN_NAMESPACE

struct WinRTWebView;
class QWinRTWebViewPrivate : public QAbstractWebView
{
    Q_OBJECT
public:
    explicit QWinRTWebViewPrivate(QObject *parent = nullptr);
    ~QWinRTWebViewPrivate() override;

    QString httpUserAgent() const override;
    void setHttpUserAgent(const QString &userAgent) override;
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

protected:
    void runJavaScriptPrivate(const QString &script, int callbackId) override;

private:
    HRESULT onNavigationStarted(ABI::Windows::UI::Xaml::Controls::IWebView *, ABI::Windows::UI::Xaml::Controls::IWebViewNavigationStartingEventArgs *);
    HRESULT onNavigationCompleted(ABI::Windows::UI::Xaml::Controls::IWebView *, ABI::Windows::UI::Xaml::Controls::IWebViewNavigationCompletedEventArgs *);
    HRESULT onUnviewableContent(ABI::Windows::UI::Xaml::Controls::IWebView *,
                             ABI::Windows::UI::Xaml::Controls::IWebViewUnviewableContentIdentifiedEventArgs *);
    QScopedPointer<WinRTWebView> d;
};

QT_END_NAMESPACE

#endif // QWINRTWEBVIEW_P_H
