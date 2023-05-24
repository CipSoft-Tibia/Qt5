// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface_wgl.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_display.h"
#include "ui/gl/gl_display_manager.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_wgl_api_implementation.h"
#include "ui/gl/init/gl_initializer.h"

namespace gl {

namespace {
const PIXELFORMATDESCRIPTOR kPixelFormatDescriptor = {
  sizeof(kPixelFormatDescriptor),    // Size of structure.
  1,                       // Default version.
  PFD_DRAW_TO_WINDOW |     // Window drawing support.
  PFD_SUPPORT_OPENGL |     // OpenGL support.
  PFD_DOUBLEBUFFER,        // Double buffering support (not stereo).
  PFD_TYPE_RGBA,           // RGBA color mode (not indexed).
  24,                      // 24 bit color mode.
  0, 0, 0, 0, 0, 0,        // Don't set RGB bits & shifts.
  8, 0,                    // 8 bit alpha
  0,                       // No accumulation buffer.
  0, 0, 0, 0,              // Ignore accumulation bits.
  0,                       // no z-buffer.
  0,                       // no stencil buffer.
  0,                       // No aux buffer.
  PFD_MAIN_PLANE,          // Main drawing plane (not overlay).
  0,                       // Reserved.
  0, 0, 0,                 // Layer masks ignored.
};
}  // namespace

// static
bool GLSurfaceWGL::initialized_ = false;

GLSurfaceWGL::GLSurfaceWGL() {
  display_ =
      GLDisplayManagerWGL::GetInstance()->GetDisplay(GpuPreference::kDefault);
}

GLSurfaceWGL::~GLSurfaceWGL() {
}

GLDisplay* GLSurfaceWGL::GetGLDisplay() {
  return display_;
}

// static
bool GLSurfaceWGL::InitializeOneOff() {
  if (initialized_)
    return true;

   auto wgl_display = GLDisplayManagerWGL::GetInstance()->GetDisplay(GpuPreference::kDefault);
   if (!wgl_display->Init(init::usingSoftwareDynamicGL()))
     return false;

  initialized_ = true;
  return true;
}

// static
bool GLSurfaceWGL::InitializeExtensionSettingsOneOff() {
  if (!initialized_)
    return false;
  g_driver_wgl.InitializeExtensionBindings();
  return true;
}

void GLSurfaceWGL::InitializeOneOffForTesting() {
  NOTREACHED();
}

HDC GLSurfaceWGL::GetDisplayDC() {
  return GLDisplayManagerWGL::GetInstance()->GetDisplay(GpuPreference::kDefault)->device_context();
}

NativeViewGLSurfaceWGL::NativeViewGLSurfaceWGL(gfx::AcceleratedWidget window)
    : window_(window), child_window_(NULL), device_context_(NULL) {
  DCHECK(window);
}

NativeViewGLSurfaceWGL::~NativeViewGLSurfaceWGL() {
  Destroy();
}

bool NativeViewGLSurfaceWGL::Initialize(GLSurfaceFormat format) {
  DCHECK(!device_context_);

  RECT rect;
  if (!GetClientRect(window_, &rect)) {
    LOG(ERROR) << "GetClientRect failed.\n";
    Destroy();
    return false;
  }

  // Create a child window. WGL has problems using a window handle owned by
  // another process.
  child_window_ = CreateWindowEx(
      WS_EX_NOPARENTNOTIFY,
      reinterpret_cast<wchar_t*>(display_->window_class()), L"",
      WS_CHILDWINDOW | WS_DISABLED | WS_VISIBLE, 0, 0, rect.right - rect.left,
      rect.bottom - rect.top, window_, NULL, NULL, NULL);
  if (!child_window_) {
    LOG(ERROR) << "CreateWindow failed.\n";
    Destroy();
    return false;
  }

  // The GL context will render to this window.
  device_context_ = GetDC(child_window_);
  if (!device_context_) {
    LOG(ERROR) << "Unable to get device context for window.";
    Destroy();
    return false;
  }

  if (!SetPixelFormat(device_context_, display_->pixel_format(),
                      &kPixelFormatDescriptor)) {
    LOG(ERROR) << "Unable to set the pixel format for GL context.";
    Destroy();
    return false;
  }

  format_ = format;

  return true;
}

void NativeViewGLSurfaceWGL::Destroy() {
  if (child_window_ && device_context_)
    ReleaseDC(child_window_, device_context_);

  if (child_window_)
    DestroyWindow(child_window_);

  child_window_ = NULL;
  device_context_ = NULL;
}

bool NativeViewGLSurfaceWGL::Resize(const gfx::Size& size,
                                    float scale_factor,
                                    const gfx::ColorSpace& color_space,
                                    bool has_alpha) {
  RECT rect;
  if (!GetClientRect(window_, &rect)) {
    LOG(ERROR) << "Failed to get parent window size.";
    return false;
  }
  DCHECK(size.width() == (rect.right - rect.left) &&
         size.height() == (rect.bottom - rect.top));
  if (!MoveWindow(child_window_, 0, 0, size.width(), size.height(), FALSE)) {
    LOG(ERROR) << "Failed to resize child window.";
    return false;
  }
  return true;
}

bool NativeViewGLSurfaceWGL::Recreate() {
  Destroy();
  if (!Initialize(format_)) {
    LOG(ERROR) << "Failed to create surface.";
    return false;
  }
  return true;
}

bool NativeViewGLSurfaceWGL::IsOffscreen() {
  return false;
}

gfx::SwapResult NativeViewGLSurfaceWGL::SwapBuffers(
    PresentationCallback callback, gfx::FrameData data) {
  // TODO(penghuang): Provide presentation feedback. https://crbug.com/776877
  TRACE_EVENT2("gpu", "NativeViewGLSurfaceWGL:RealSwapBuffers",
      "width", GetSize().width(),
      "height", GetSize().height());

  // Resize the child window to match the parent before swapping. Do not repaint
  // it as it moves.
  RECT rect;
  if (!GetClientRect(window_, &rect))
    return gfx::SwapResult::SWAP_FAILED;
  if (!MoveWindow(child_window_,
                  0,
                  0,
                  rect.right - rect.left,
                  rect.bottom - rect.top,
                  FALSE)) {
    return gfx::SwapResult::SWAP_FAILED;
  }

  DCHECK(device_context_);
  if (::SwapBuffers(device_context_) == TRUE) {
    // TODO(penghuang): Provide more accurate values for presentation feedback.
    constexpr int64_t kRefreshIntervalInMicroseconds =
        base::Time::kMicrosecondsPerSecond / 60;
    std::move(callback).Run(gfx::PresentationFeedback(
        base::TimeTicks::Now(),
        base::Microseconds(kRefreshIntervalInMicroseconds),
        0 /* flags */));
    return gfx::SwapResult::SWAP_ACK;
  } else {
    std::move(callback).Run(gfx::PresentationFeedback::Failure());
    return gfx::SwapResult::SWAP_FAILED;
  }
}

gfx::Size NativeViewGLSurfaceWGL::GetSize() {
  RECT rect;
  BOOL result = GetClientRect(child_window_, &rect);
  DCHECK(result);
  return gfx::Size(rect.right - rect.left, rect.bottom - rect.top);
}

void* NativeViewGLSurfaceWGL::GetHandle() {
  return device_context_;
}

GLSurfaceFormat NativeViewGLSurfaceWGL::GetFormat() {
  return GLSurfaceFormat();
}

void NativeViewGLSurfaceWGL::SetVSyncEnabled(bool enabled) {
  DCHECK(GLContext::GetCurrent() && GLContext::GetCurrent()->IsCurrent(this));
  if (g_driver_wgl.ext.b_WGL_EXT_swap_control) {
    wglSwapIntervalEXT(enabled ? 1 : 0);
  } else {
    LOG(WARNING) << "Could not disable vsync: driver does not "
                    "support WGL_EXT_swap_control";
  }
}

PbufferGLSurfaceWGL::PbufferGLSurfaceWGL(const gfx::Size& size)
    : size_(size),
      device_context_(NULL),
      pbuffer_(NULL) {
  // Some implementations of Pbuffer do not support having a 0 size. For such
  // cases use a (1, 1) surface.
  if (size_.GetArea() == 0)
    size_.SetSize(1, 1);
}

PbufferGLSurfaceWGL::~PbufferGLSurfaceWGL() {
  Destroy();
}

bool PbufferGLSurfaceWGL::Initialize(GLSurfaceFormat format) {
  DCHECK(!device_context_);

  if (!g_driver_wgl.fn.wglCreatePbufferARBFn) {
    LOG(ERROR) << "wglCreatePbufferARB not available.";
    Destroy();
    return false;
  }

  const int kNoAttributes[] = { 0 };
  pbuffer_ = wglCreatePbufferARB(display_->device_context(),
                                 display_->pixel_format(), size_.width(),
                                 size_.height(), kNoAttributes);

  if (!pbuffer_) {
    LOG(ERROR) << "Unable to create pbuffer.";
    Destroy();
    return false;
  }

  device_context_ = wglGetPbufferDCARB(static_cast<HPBUFFERARB>(pbuffer_));
  if (!device_context_) {
    LOG(ERROR) << "Unable to get pbuffer device context.";
    Destroy();
    return false;
  }

  return true;
}

void PbufferGLSurfaceWGL::Destroy() {
  if (pbuffer_ && device_context_)
    wglReleasePbufferDCARB(static_cast<HPBUFFERARB>(pbuffer_), device_context_);

  device_context_ = NULL;

  if (pbuffer_) {
    wglDestroyPbufferARB(static_cast<HPBUFFERARB>(pbuffer_));
    pbuffer_ = NULL;
  }
}

bool PbufferGLSurfaceWGL::IsOffscreen() {
  return true;
}

gfx::SwapResult PbufferGLSurfaceWGL::SwapBuffers(
    PresentationCallback callback, gfx::FrameData data) {
  NOTREACHED() << "Attempted to call SwapBuffers on a pbuffer.";
  return gfx::SwapResult::SWAP_FAILED;
}

gfx::Size PbufferGLSurfaceWGL::GetSize() {
  return size_;
}

void* PbufferGLSurfaceWGL::GetHandle() {
  return device_context_;
}

GLSurfaceFormat PbufferGLSurfaceWGL::GetFormat() {
  return GLSurfaceFormat();
}

}  // namespace gl
