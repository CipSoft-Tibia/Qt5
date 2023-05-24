// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_IMAGE_GLX_H_
#define UI_GL_GL_IMAGE_GLX_H_

#include <stdint.h>

#include "ui/gfx/geometry/size.h"
#include "ui/gfx/x/glx.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_image.h"

namespace gl {

class GL_EXPORT GLImageGLX : public GLImage {
 public:
  GLImageGLX(const gfx::Size& size, gfx::BufferFormat format);

  GLImageGLX(const GLImageGLX&) = delete;
  GLImageGLX& operator=(const GLImageGLX&) = delete;

  bool Initialize(x11::Pixmap pixmap);

  // Overridden from GLImage:
  gfx::Size GetSize() override;
  unsigned GetInternalFormat();
  unsigned GetDataType();
  bool BindTexImage(unsigned target);
  void ReleaseTexImage(unsigned target);
  bool CopyTexImage(unsigned target);

 protected:
  ~GLImageGLX() override;

  gfx::BufferFormat format() const { return format_; }

 private:
  uint32_t glx_pixmap_;
  const gfx::Size size_;
  gfx::BufferFormat format_;
};

}  // namespace gl

#endif  // UI_GL_GL_IMAGE_GLX_H_
