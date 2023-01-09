/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WEB_CONTENTS_VIEW_QT_H
#define WEB_CONTENTS_VIEW_QT_H

#include "content/browser/renderer_host/render_view_host_delegate_view.h"
#include "content/browser/web_contents/web_contents_view.h"

#include "api/qtwebenginecoreglobal_p.h"
#include "web_contents_adapter_client.h"

namespace content {
class WebContents;
}

namespace QtWebEngineCore {
class WebContentsAdapterClient;

class WebContentsViewQt
    : public content::WebContentsView
    , public content::RenderViewHostDelegateView
{
public:
    static inline WebContentsViewQt *from(WebContentsView *view) { return static_cast<WebContentsViewQt*>(view); }

    WebContentsViewQt(content::WebContents *webContents);

    void setFactoryClient(WebContentsAdapterClient* client);
    void setClient(WebContentsAdapterClient* client);
    WebContentsAdapterClient *client() { return m_client; }

    // content::WebContentsView overrides:
    content::RenderWidgetHostViewBase *CreateViewForWidget(content::RenderWidgetHost *render_widget_host) override;

    void CreateView(gfx::NativeView context) override;

    content::RenderWidgetHostViewBase *CreateViewForChildWidget(content::RenderWidgetHost* render_widget_host) override;

    void SetPageTitle(const base::string16& title) override { }

    void RenderViewReady() override { }

    void RenderViewHostChanged(content::RenderViewHost*, content::RenderViewHost*) override { }

    void SetOverscrollControllerEnabled(bool enabled) override { }

    gfx::NativeView GetNativeView() const override;

    gfx::NativeView GetContentNativeView() const override { return nullptr; }

    gfx::NativeWindow GetTopLevelNativeWindow() const override { return nullptr; }

    gfx::Rect GetContainerBounds() const override;

    void Focus() override;

    void SetInitialFocus() override;

    void StoreFocus() override { }

    void RestoreFocus() override { }

    content::DropData* GetDropData() const override { QT_NOT_YET_IMPLEMENTED return nullptr; }

    gfx::Rect GetViewBounds() const override { return gfx::Rect(); }

    void FocusThroughTabTraversal(bool reverse) override;

#if defined(OS_MAC)
    bool CloseTabAfterEventTrackingIfNeeded() override { QT_NOT_YET_IMPLEMENTED return false; }
#endif // defined(OS_MAC)

    // content::RenderViewHostDelegateView overrides:
    void StartDragging(const content::DropData& drop_data, blink::DragOperationsMask allowed_ops,
                       const gfx::ImageSkia& image, const gfx::Vector2d& image_offset,
                       const blink::mojom::DragEventSourceInfo &event_info,
                       content::RenderWidgetHostImpl *source_rwh) override;

    void UpdateDragCursor(blink::DragOperation dragOperation) override;

    void ShowContextMenu(content::RenderFrameHost *, const content::ContextMenuParams &params) override;

    void GotFocus(content::RenderWidgetHostImpl *render_widget_host) override;
    void LostFocus(content::RenderWidgetHostImpl *render_widget_host) override;
    void TakeFocus(bool reverse) override;

    static WebEngineContextMenuData buildContextMenuData(const content::ContextMenuParams &params);

private:
    content::WebContents *m_webContents;
    WebContentsAdapterClient *m_client;
    WebContentsAdapterClient *m_factoryClient;
};

} // namespace QtWebEngineCore

#endif // WEB_CONTENTS_VIEW_QT_H
