// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_GLX_H_
#define UI_GL_GL_SURFACE_GLX_H_

namespace gl {

class GL_EXPORT GLSurfaceGLX {
 public:
  // Implemented in src/core/ozone/gl_surface_glx_qt.cpp:
  static bool IsCreateContextSupported();
  static bool IsCreateContextRobustnessSupported();
  static bool IsRobustnessVideoMemoryPurgeSupported();
  static bool IsCreateContextProfileSupported();
  static bool IsCreateContextES2ProfileSupported();
};

}  // namespace gl

#endif  // UI_GL_GL_SURFACE_GLX_H_
