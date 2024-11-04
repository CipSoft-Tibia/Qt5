// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/remote_cocoa/app_shim/immersive_mode_controller.h"

#include "base/apple/foundation_util.h"
#include "base/auto_reset.h"
#include "base/check.h"
#include "base/containers/contains.h"
#import "components/remote_cocoa/app_shim/immersive_mode_delegate_mac.h"
#import "components/remote_cocoa/app_shim/native_widget_ns_window_bridge.h"
#include "ui/gfx/geometry/rect.h"

namespace {

const double kMinHeight = 0.5;

NSView* GetNSTitlebarContainerViewFromWindow(NSWindow* window) {
  for (NSView* view in window.contentView.subviews) {
    if ([view isKindOfClass:NSClassFromString(@"NSTitlebarContainerView")]) {
      return view;
    }
  }
  return nil;
}

}  // namespace

@interface ImmersiveModeTitlebarObserver () {
  base::WeakPtr<remote_cocoa::ImmersiveModeController> _controller;
  NSView* __weak _titlebarContainerView;
}
@end

@implementation ImmersiveModeTitlebarObserver

- (instancetype)initWithController:
                    (base::WeakPtr<remote_cocoa::ImmersiveModeController>)
                        controller
             titlebarContainerView:(NSView*)titlebarContainerView {
  self = [super init];
  if (self) {
    _controller = std::move(controller);
    _titlebarContainerView = titlebarContainerView;
    [_titlebarContainerView addObserver:self
                             forKeyPath:@"frame"
                                options:NSKeyValueObservingOptionInitial |
                                        NSKeyValueObservingOptionNew
                                context:nullptr];
  }
  return self;
}

- (void)dealloc {
  [_titlebarContainerView removeObserver:self forKeyPath:@"frame"];
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id>*)change
                       context:(void*)context {
  if (!_controller || ![keyPath isEqualToString:@"frame"]) {
    return;
  }

  NSRect frame = [change[@"new"] rectValue];
  _controller->OnTitlebarFrameDidChange(frame);
}

@end

// A stub NSWindowDelegate class that will be used to map the AppKit controlled
// NSWindow to the overlay view widget's NSWindow. The delegate will be used to
// help with input routing.
@interface ImmersiveModeMapper : NSObject <ImmersiveModeDelegate>
@property(weak) NSWindow* originalHostingWindow;
@end

@implementation ImmersiveModeMapper
@synthesize originalHostingWindow = _originalHostingWindow;
@end

// Host of the overlay view.
@interface ImmersiveModeTitlebarViewController
    : NSTitlebarAccessoryViewController {
  NSView* __strong _blank_separator_view;
}
@end

@implementation ImmersiveModeTitlebarViewController

- (instancetype)init {
  if ((self = [super init])) {
    _blank_separator_view = [[NSView alloc] init];
  }
  return self;
}

- (void)viewWillAppear {
  [super viewWillAppear];

  // Sometimes AppKit incorrectly positions NSToolbarFullScreenWindow entirely
  // offscreen (particularly when this is a out-of-process app shim). Toggling
  // visibility when appearing in the right window seems to fix the positioning.
  // Only toggle the visibility if fullScreenMinHeight is not zero though, as
  // triggering the repositioning when the toolbar is set to auto hide would
  // result in it being incorrectly positioned in that case.
  if (remote_cocoa::IsNSToolbarFullScreenWindow(self.view.window) &&
      self.fullScreenMinHeight > kMinHeight && !self.hidden) {
    self.hidden = YES;
    self.hidden = NO;
  }
}

// This is a private API method that will be used on macOS 11+.
// Remove a small 1px blur between the overlay view and tabbed overlay view by
// returning a blank NSView.
- (NSView*)separatorView {
  return _blank_separator_view;
}

@end

// An NSView that will set the ImmersiveModeDelegate on the AppKit created
// window that ends up hosting this view via the
// NSTitlebarAccessoryViewController API.
@interface ImmersiveModeView : NSView
- (instancetype)initWithController:
    (base::WeakPtr<remote_cocoa::ImmersiveModeController>)controller;
@end

@implementation ImmersiveModeView {
  base::WeakPtr<remote_cocoa::ImmersiveModeController> _controller;
}

- (instancetype)initWithController:
    (base::WeakPtr<remote_cocoa::ImmersiveModeController>)controller {
  self = [super init];
  if (self) {
    _controller = std::move(controller);
  }
  return self;
}

- (void)viewWillMoveToWindow:(NSWindow*)window {
  if (_controller) {
    _controller->ImmersiveModeViewWillMoveToWindow(window);
  }
}

@end

namespace remote_cocoa {

bool IsNSToolbarFullScreenWindow(NSWindow* window) {
  // TODO(bur): Investigate other approaches to detecting
  // NSToolbarFullScreenWindow. This is a private class and the name could
  // change.
  return [window isKindOfClass:NSClassFromString(@"NSToolbarFullScreenWindow")];
}

ImmersiveModeController::ImmersiveModeController(
    NativeWidgetMacNSWindow* browser_window,
    NativeWidgetMacNSWindow* overlay_window)
    : weak_ptr_factory_(this) {
  browser_window_ = browser_window;
  overlay_window_ = overlay_window;
  overlay_window_.commandDispatchParentOverride = browser_window_;
  // A style of NSTitlebarSeparatorStyleAutomatic (default) will show a black
  // line separator when removing the NSWindowStyleMaskFullSizeContentView style
  // bit. We do not want a separator. Pre-macOS 11 there is no titlebar
  // separator.
  if (@available(macOS 11.0, *)) {
    browser_window_.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone;
  }

  // Create a new NSTitlebarAccessoryViewController that will host the
  // overlay_view_.
  immersive_mode_titlebar_view_controller_ =
      [[ImmersiveModeTitlebarViewController alloc] init];

  // Create a NSWindow delegate that will be used to map the AppKit created
  // NSWindow to the overlay view widget's NSWindow.
  immersive_mode_mapper_ = [[ImmersiveModeMapper alloc] init];
  immersive_mode_mapper_.originalHostingWindow = overlay_window_;
  immersive_mode_titlebar_view_controller_.view = [[ImmersiveModeView alloc]
      initWithController:weak_ptr_factory_.GetWeakPtr()];

  // Remove the content view from the overlay view widget's NSWindow, and hold a
  // local strong reference. This view will be re-parented into the AppKit
  // created NSWindow.
  BridgedContentView* overlay_content_view =
      base::apple::ObjCCastStrict<BridgedContentView>(
          overlay_window_.contentView);
  overlay_content_view_ = overlay_content_view;
  [overlay_content_view removeFromSuperview];

  // The original content view (top chrome) has been moved to the AppKit
  // created NSWindow. Create a new content view but reuse the original bridge
  // so that mouse drags are handled.
  overlay_window_.contentView =
      [[BridgedContentView alloc] initWithBridge:overlay_content_view.bridge
                                          bounds:gfx::Rect()];

  // The overlay window will become a child of NSToolbarFullScreenWindow and sit
  // above it in the z-order. Allow mouse events that are not handled by the
  // BridgedContentView to passthrough the overlay window to the
  // NSToolbarFullScreenWindow. This will allow the NSToolbarFullScreenWindow to
  // become key when interacting with "top chrome".
  overlay_window_.ignoresMouseEvents = YES;

  // Add the overlay view to the accessory view controller getting ready to
  // hand everything over to AppKit.
  [immersive_mode_titlebar_view_controller_.view
      addSubview:overlay_content_view];
  immersive_mode_titlebar_view_controller_.layoutAttribute =
      NSLayoutAttributeBottom;
}

ImmersiveModeController::~ImmersiveModeController() {
  // Remove the titlebar observer before moving the view.
  immersive_mode_titlebar_observer_ = nil;

  overlay_window_.commandDispatchParentOverride = nil;
  StopObservingChildWindows(overlay_window_);

  // Rollback the view shuffling from enablement.
  [overlay_content_view_ removeFromSuperview];
  overlay_window_.contentView = overlay_content_view_;
  [immersive_mode_titlebar_view_controller_ removeFromParentViewController];
  browser_window_.styleMask |= NSWindowStyleMaskFullSizeContentView;
  if (@available(macOS 11.0, *)) {
    browser_window_.titlebarSeparatorStyle = NSTitlebarSeparatorStyleAutomatic;
  }
}

void ImmersiveModeController::Enable() {
  DCHECK(!enabled_);
  enabled_ = true;
  [browser_window_ addTitlebarAccessoryViewController:
                       immersive_mode_titlebar_view_controller_];

  // Keep the overlay content view's size in sync with its parent view.
  overlay_content_view_.translatesAutoresizingMaskIntoConstraints = NO;
  [overlay_content_view_.heightAnchor
      constraintEqualToAnchor:overlay_content_view_.superview.heightAnchor]
      .active = YES;
  [overlay_content_view_.widthAnchor
      constraintEqualToAnchor:overlay_content_view_.superview.widthAnchor]
      .active = YES;
  [overlay_content_view_.centerXAnchor
      constraintEqualToAnchor:overlay_content_view_.superview.centerXAnchor]
      .active = YES;
  [overlay_content_view_.centerYAnchor
      constraintEqualToAnchor:overlay_content_view_.superview.centerYAnchor]
      .active = YES;
}

void ImmersiveModeController::FullscreenTransitionCompleted() {
  fullscreen_transition_complete_ = true;
  UpdateToolbarVisibility(last_used_style_);

  //  Establish reveal locks for windows that exist before entering fullscreen,
  //  such as permission popups and the find bar. Do this after the fullscreen
  //  transition has ended to avoid graphical flashes during the animation.
  for (NSWindow* child in overlay_window_.childWindows) {
    if (!ShouldObserveChildWindow(child)) {
      continue;
    }
    OnChildWindowAdded(child);
  }

  // Watch for child windows. When they are added the overlay view will be
  // revealed as appropriate.
  ObserveChildWindows(overlay_window_);
}

void ImmersiveModeController::OnTopViewBoundsChanged(const gfx::Rect& bounds) {
  // Set the height of the AppKit fullscreen view. The width will be
  // automatically handled by AppKit.
  NSRect frame = NSRectFromCGRect(bounds.ToCGRect());
  NSView* overlay_view = immersive_mode_titlebar_view_controller_.view;
  NSSize size = overlay_view.window.frame.size;
  if (frame.size.height != size.height) {
    size.height = frame.size.height;
    [overlay_view setFrameSize:size];
  }

  UpdateToolbarVisibility(last_used_style_);

  // If the toolbar is always visible, update the fullscreen min height.
  // Also update the fullscreen min height if the toolbar auto hides, but only
  // if the toolbar is currently revealed.
  if (last_used_style_ == mojom::ToolbarVisibilityStyle::kAlways ||
      (last_used_style_ == mojom::ToolbarVisibilityStyle::kAutohide &&
       reveal_lock_count_ > 0)) {
    immersive_mode_titlebar_view_controller_.fullScreenMinHeight =
        frame.size.height;
  }
}

void ImmersiveModeController::UpdateToolbarVisibility(
    mojom::ToolbarVisibilityStyle style) {
  // Remember the last used style for internal use of UpdateToolbarVisibility.
  last_used_style_ = style;

  // Only make changes if there are no outstanding reveal locks.
  if (reveal_lock_count_ > 0) {
    return;
  }

  switch (style) {
    case mojom::ToolbarVisibilityStyle::kAlways:
      immersive_mode_titlebar_view_controller_.fullScreenMinHeight =
          immersive_mode_titlebar_view_controller_.view.frame.size.height;

      // Top chrome is removed from the content view when the browser window
      // starts the fullscreen transition, however the request is asynchronous
      // and sometimes top chrome is still present in the content view when the
      // animation starts. This results in top chrome being painted twice during
      // the fullscreen animation, once in the content view and once in
      // `immersive_mode_titlebar_view_controller_`. Keep
      // NSWindowStyleMaskFullSizeContentView active during the fullscreen
      // transition to allow
      // `immersive_mode_titlebar_view_controller_` to be
      // displayed z-order on top of the content view. This will cover up any
      // perceived jank.
      // TODO(https://crbug.com/1375995): Handle fullscreen exit.
      if (fullscreen_transition_complete_) {
        browser_window_.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
      } else {
        browser_window_.styleMask |= NSWindowStyleMaskFullSizeContentView;
      }

      // Toggling the controller will allow the content view to resize below Top
      // Chrome.
      immersive_mode_titlebar_view_controller_.hidden = YES;
      immersive_mode_titlebar_view_controller_.hidden = NO;
      break;
    case mojom::ToolbarVisibilityStyle::kAutohide:
      immersive_mode_titlebar_view_controller_.hidden = NO;
      // Workaround for https://crbug.com/1369643
      immersive_mode_titlebar_view_controller_.fullScreenMinHeight = kMinHeight;
      browser_window_.styleMask |= NSWindowStyleMaskFullSizeContentView;
      break;
    case mojom::ToolbarVisibilityStyle::kNone:
      immersive_mode_titlebar_view_controller_.hidden = YES;
      break;
  }
}

void ImmersiveModeController::ObserveChildWindows(NSWindow* window) {
  // Watch the Widget for addition and removal of child Widgets.
  NativeWidgetMacNSWindow* widget_window =
      base::apple::ObjCCastStrict<NativeWidgetMacNSWindow>(window);
  widget_window.childWindowAddedHandler = ^(NSWindow* child) {
    OnChildWindowAdded(child);
  };
  widget_window.childWindowRemovedHandler = ^(NSWindow* child) {
    OnChildWindowRemoved(child);
  };
}

void ImmersiveModeController::StopObservingChildWindows(NSWindow* window) {
  NativeWidgetMacNSWindow* widget_window =
      base::apple::ObjCCastStrict<NativeWidgetMacNSWindow>(window);
  widget_window.childWindowAddedHandler = nil;
  widget_window.childWindowRemovedHandler = nil;
}

bool ImmersiveModeController::ShouldObserveChildWindow(NSWindow* child) {
  return true;
}

NSWindow* ImmersiveModeController::browser_window() {
  return browser_window_;
}
NSWindow* ImmersiveModeController::overlay_window() {
  return overlay_window_;
}
BridgedContentView* ImmersiveModeController::overlay_content_view() {
  return overlay_content_view_;
}

void ImmersiveModeController::OnChildWindowAdded(NSWindow* child) {
  // Skip applying the reveal lock if the window is in the process of being
  // re-ordered, as this may inadvertently trigger a recursive re-ordering.
  // This is because changing the titlebar visibility (which reveal lock does)
  // can itself initiate another window re-ordering, causing this method to be
  // re-entered.
  if (((NativeWidgetMacNSWindow*)child).isShufflingForOrdering) {
    return;
  }
  // TODO(kerenzhu): the sole purpose of `window_lock_received_` is to
  // verify that we don't lock twice for a single window.
  // We can remove it once this is verified.
  CHECK(!base::Contains(window_lock_received_, child));
  window_lock_received_.insert(child);
  RevealLock();

  // TODO(https://crbug.com/1350595): Handle a detached find bar.
}

void ImmersiveModeController::OnChildWindowRemoved(NSWindow* child) {
  // Skip applying the reveal lock if the window is in the process of being
  // re-ordered, as this may inadvertently trigger a recursive re-ordering.
  // This is because changing the titlebar visibility (which reveal lock does)
  // can itself initiate another window re-ordering, causing this method to be
  // re-entered.
  if (((NativeWidgetMacNSWindow*)child).isShufflingForOrdering) {
    return;
  }
  CHECK(base::Contains(window_lock_received_, child));
  window_lock_received_.erase(child);
  RevealUnlock();
}

void ImmersiveModeController::RevealLock() {
  reveal_lock_count_++;
  immersive_mode_titlebar_view_controller_.fullScreenMinHeight =
      immersive_mode_titlebar_view_controller_.view.frame.size.height;
}

void ImmersiveModeController::RevealUnlock() {
  // Re-hide the toolbar if appropriate.
  if (--reveal_lock_count_ < 1 &&
      immersive_mode_titlebar_view_controller_.fullScreenMinHeight > 0 &&
      last_used_style_ == mojom::ToolbarVisibilityStyle::kAutohide) {
    immersive_mode_titlebar_view_controller_.fullScreenMinHeight = 0;
  }

  // Account for last_used_style_ changing while a reveal lock was active.
  if (reveal_lock_count_ < 1) {
    UpdateToolbarVisibility(last_used_style_);
  }
  DCHECK(reveal_lock_count_ >= 0);
}

void ImmersiveModeController::ImmersiveModeViewWillMoveToWindow(
    NSWindow* window) {
  // AppKit hands this view controller over to a fullscreen transition window
  // before we finally land at the NSToolbarFullScreenWindow. Add the frame
  // observer only once we reach the NSToolbarFullScreenWindow.
  if (remote_cocoa::IsNSToolbarFullScreenWindow(window)) {
    // This window is created by AppKit. Make sure it doesn't have a delegate
    // so we can use it for out own purposes.
    DCHECK(!window.delegate);
    window.delegate = immersive_mode_mapper_;

    // Attach overlay_widget to NSToolbarFullScreen so that children are placed
    // on top of the toolbar. When exiting fullscreen, we don't re-parent the
    // overlay window back to the browser window because it seems to trigger
    // re-entrancy in AppKit and cause crash. This is safe because sub-widgets
    // will be re-parented to the browser window and therefore the overlay
    // window won't have any observable effect.
    // Also, explicitly remove the overlay window from the browser window.
    // Leaving a dangling reference to the overlay window on the browser window
    // causes odd behavior.
    [browser_window_ removeChildWindow:overlay_window()];
    [window addChildWindow:overlay_window() ordered:NSWindowAbove];

    NSView* view = GetNSTitlebarContainerViewFromWindow(window);
    DCHECK(view);
    // Create the titlebar observer. Observing can only start once the view has
    // been fully re-parented into the AppKit fullscreen window.
    immersive_mode_titlebar_observer_ = [[ImmersiveModeTitlebarObserver alloc]
           initWithController:weak_ptr_factory_.GetWeakPtr()
        titlebarContainerView:view];
  }
}

void ImmersiveModeController::OnTitlebarFrameDidChange(NSRect frame) {
  LayoutWindowWithAnchorView(overlay_window_, overlay_content_view_);
}

bool ImmersiveModeController::IsTabbed() {
  return false;
}

double ImmersiveModeController::GetOffscreenYOrigin() {
  // Get the height of the screen. Using this as the y origin will move a window
  // offscreen.
  double y = browser_window_.screen.frame.size.height;

  // Make sure to make it past the safe area insets, otherwise some portion
  // of the window may still be displayed.
  if (@available(macOS 12.0, *)) {
    y += browser_window_.screen.safeAreaInsets.top;
  }

  return y;
}

void ImmersiveModeController::LayoutWindowWithAnchorView(NSWindow* window,
                                                         NSView* anchor_view) {
  // Find the anchor view's point on screen (bottom left).
  NSPoint point_in_window = [anchor_view convertPoint:NSZeroPoint toView:nil];
  NSPoint point_on_screen =
      [anchor_view.window convertPointToScreen:point_in_window];

  // This branch is only useful on macOS 11 and greater. macOS 10.15 and
  // earlier move the window instead of clipping the view within the window.
  // This allows the overlay window to appropriately track the overlay view.
  if (@available(macOS 11.0, *)) {
    // If the anchor view is clipped move the window off screen. A clipped
    // anchor view indicates the titlebar is hidden or is in transition AND the
    // browser content view takes up the whole window
    // ("Always Show Toolbar in Full Screen" is disabled). When we are in this
    // state we don't want the window on screen, otherwise it may mask input to
    // the browser view. In all other cases will not enter this branch and the
    // window will be placed at the same coordinates as the anchor view.
    if (anchor_view.visibleRect.size.height != anchor_view.frame.size.height) {
      point_on_screen.y = GetOffscreenYOrigin();
    }
  }

  // If the toolbar is hidden (mojom::ToolbarVisibilityStyle::kNone) also move
  // the window offscreen. This applies to all versions of macOS where Chrome
  // can be run.
  if (last_used_style_ == mojom::ToolbarVisibilityStyle::kNone) {
    point_on_screen.y = GetOffscreenYOrigin();
  }

  [window setFrameOrigin:point_on_screen];
}

}  // namespace remote_cocoa
