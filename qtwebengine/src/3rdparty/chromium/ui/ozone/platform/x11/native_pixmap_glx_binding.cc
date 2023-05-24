// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/x11/native_pixmap_glx_binding.h"

#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "ui/gl/gl_image_glx_native_pixmap.h"
#include "ui/gl/scoped_binders.h"

#include <GL/gl.h>

namespace ui {

NativePixmapGLXBinding::NativePixmapGLXBinding(scoped_refptr<gl::GLImageGLXNativePixmap> gl_image)
    : gl_image_(gl_image)
{}
NativePixmapGLXBinding::~NativePixmapGLXBinding() = default;

// static
std::unique_ptr<NativePixmapGLBinding> NativePixmapGLXBinding::Create(
    scoped_refptr<gfx::NativePixmap> pixmap,
    gfx::BufferFormat plane_format,
    gfx::BufferPlane plane,
    gfx::Size plane_size,
    GLenum target,
    GLuint texture_id) {
  auto gl_image = base::MakeRefCounted<gl::GLImageGLXNativePixmap>(
      plane_size, plane_format, plane);

  // Initialize the image using glXCreatePixmap.
  if (!gl_image->Initialize(std::move(pixmap))) {
    LOG(ERROR) << "Unable to initialize GL image from pixmap";
    return nullptr;
  }

  auto binding = std::make_unique<NativePixmapGLXBinding>(std::move(gl_image));
  if (!binding->BindTexture(target, texture_id)) {
    return nullptr;
  }

  return binding;
}

bool NativePixmapGLXBinding::BindTexture(GLenum target, GLuint texture_id) {
  gl::ScopedTextureBinder binder(base::strict_cast<unsigned int>(target),
                                 base::strict_cast<unsigned int>(texture_id));

  if (!gl_image_->BindTexImage(base::strict_cast<unsigned>(target))) {
      LOG(ERROR) << "Unable to bind GL image to target = " << target;
      return false;
  }

  return true;
}

GLuint NativePixmapGLXBinding::GetInternalFormat() {
  return gl_image_->GetInternalFormat();
}

GLenum NativePixmapGLXBinding::GetDataType() {
  return gl_image_->GetDataType();
}


}  // namespace ui
