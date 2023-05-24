// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_X11_NATIVE_PIXMAP_GLX_BINDING_H_
#define UI_OZONE_PLATFORM_X11_NATIVE_PIXMAP_GLX_BINDING_H_

#include "ui/gfx/native_pixmap.h"
#include "ui/gl/gl_image_glx_native_pixmap.h"
#include "ui/ozone/public/native_pixmap_gl_binding.h"

#include <memory>

namespace ui {

// A binding maintained between GLImageGLXNativePixmap and GL Textures in Ozone.
// This binding is used for ChromeOS-on-Linux and for Linux/Ozone/X11 with
// Drm/Kms.
class NativePixmapGLXBinding : public NativePixmapGLBinding {
 public:
  NativePixmapGLXBinding(scoped_refptr<gl::GLImageGLXNativePixmap> gl_image);
  ~NativePixmapGLXBinding() override;

  static std::unique_ptr<NativePixmapGLBinding> Create(
      scoped_refptr<gfx::NativePixmap> pixmap,
      gfx::BufferFormat plane_format,
      gfx::BufferPlane plane,
      gfx::Size plane_size,
      GLenum target,
      GLuint texture_id);

  // NativePixmapGLBinding:
  GLuint GetInternalFormat() override;
  GLenum GetDataType() override;

 private:
  // Invokes NativePixmapGLBinding::BindTexture, passing |gl_image_|.
  bool BindTexture(GLenum target, GLuint texture_id);

  scoped_refptr<gl::GLImageGLXNativePixmap> gl_image_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_X11C_NATIVE_PIXMAP_GLX_BINDING_H_
