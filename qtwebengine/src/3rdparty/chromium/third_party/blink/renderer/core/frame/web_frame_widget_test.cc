// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"

#include "base/run_loop.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "third_party/blink/renderer/core/frame/web_view_frame_widget.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"

namespace blink {

class WebFrameWidgetSimTest : public SimTest {};

// Tests that if a WebView is auto-resized, the associated
// WebViewFrameWidget requests a new viz::LocalSurfaceId to be allocated on the
// impl thread.
TEST_F(WebFrameWidgetSimTest, AutoResizeAllocatedLocalSurfaceId) {
  viz::ParentLocalSurfaceIdAllocator allocator;

  // Enable auto-resize.
  blink::VisualProperties visual_properties;
  visual_properties.auto_resize_enabled = true;
  visual_properties.min_size_for_auto_resize = gfx::Size(100, 100);
  visual_properties.max_size_for_auto_resize = gfx::Size(200, 200);
  allocator.GenerateId();
  visual_properties.local_surface_id = allocator.GetCurrentLocalSurfaceId();
  WebView().MainFrameWidget()->ApplyVisualProperties(visual_properties);
  WebView().MainFrameViewWidget()->UpdateSurfaceAndScreenInfo(
      visual_properties.local_surface_id.value(),
      visual_properties.compositor_viewport_pixel_rect,
      visual_properties.screen_info);

  EXPECT_EQ(allocator.GetCurrentLocalSurfaceId(),
            WebView().MainFrameViewWidget()->LocalSurfaceIdFromParent());
  EXPECT_FALSE(WebView()
                   .MainFrameViewWidget()
                   ->LayerTreeHost()
                   ->new_local_surface_id_request_for_testing());

  constexpr gfx::Size size(200, 200);
  static_cast<WebViewFrameWidget*>(WebView().MainFrameViewWidget())
      ->DidAutoResize(size);
  EXPECT_EQ(allocator.GetCurrentLocalSurfaceId(),
            WebView().MainFrameViewWidget()->LocalSurfaceIdFromParent());
  EXPECT_TRUE(WebView()
                  .MainFrameViewWidget()
                  ->LayerTreeHost()
                  ->new_local_surface_id_request_for_testing());
}

TEST_F(WebFrameWidgetSimTest, FrameSinkIdHitTestAPI) {
  SimRequest request("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");
  request.Complete(
      R"HTML(
      <style>
      html, body {
        margin :0px;
        padding: 0px;
      }
      </style>

      <div style='background: green; padding: 100px; margin: 0px;'>
        <iframe style='width: 200px; height: 100px;'
          srcdoc='<body style="margin : 0px; height : 100px; width : 200px;">
          </body>'>
        </iframe>
      </div>

      )HTML");

  gfx::PointF point;
  viz::FrameSinkId main_frame_sink_id =
      WebView().MainFrameViewWidget()->GetFrameSinkIdAtPoint(
          gfx::PointF(10.43, 10.74), &point);
  EXPECT_EQ(WebView().MainFrameViewWidget()->Client()->GetFrameSinkId(),
            main_frame_sink_id);
  EXPECT_EQ(gfx::PointF(10.43, 10.74), point);

  // Targeting a child frame should also return the FrameSinkId for the main
  // widget.
  viz::FrameSinkId frame_sink_id =
      WebView().MainFrameViewWidget()->GetFrameSinkIdAtPoint(
          gfx::PointF(150.27, 150.25), &point);
  EXPECT_EQ(main_frame_sink_id, frame_sink_id);
  EXPECT_EQ(gfx::PointF(150.27, 150.25), point);
}

// Tests that page scale is propagated to all remote frames controlled
// by a widget.
TEST_F(WebFrameWidgetSimTest, PropagateScaleToRemoteFrames) {
  SimRequest request("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");
  request.Complete(
      R"HTML(
      <iframe style='width: 200px; height: 100px;'
        srcdoc='<iframe srcdoc="plain text"></iframe>'>
        </iframe>

      )HTML");
  base::RunLoop().RunUntilIdle();
  class PageScaleRemoteFrameClient
      : public frame_test_helpers::TestWebRemoteFrameClient {
   public:
    void PageScaleFactorChanged(float page_scale_factor,
                                bool is_pinch_gesture_active) override {
      page_scale_factor_changed_ = true;
    }
    bool page_scale_factor_changed_ = false;
  };

  PageScaleRemoteFrameClient page_scale_remote_frame_client;
  EXPECT_TRUE(WebView().MainFrame()->FirstChild());
  {
    WebFrame* grandchild = WebView().MainFrame()->FirstChild()->FirstChild();
    EXPECT_TRUE(grandchild);
    EXPECT_TRUE(grandchild->IsWebLocalFrame());
    grandchild->Swap(
        frame_test_helpers::CreateRemote(&page_scale_remote_frame_client));
  }
  auto* widget = WebView().MainFrameViewWidget();
  widget->SetPageScaleStateAndLimits(1.3f, true, 1.0f, 3.0f);
  EXPECT_TRUE(page_scale_remote_frame_client.page_scale_factor_changed_);
  WebView().MainFrame()->FirstChild()->FirstChild()->Detach();
}

}  // namespace blink
