// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/init/gl_initializer.h"

#include <dwmapi.h>

#include "base/at_exit.h"
#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_event.h"
#include "base/win/windows_version.h"
#include "ui/gl/direct_composition_support.h"
#include "ui/gl/gl_angle_util_win.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_display.h"
#include "ui/gl/gl_egl_api_implementation.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_utils.h"
#include "ui/gl/gl_surface_wgl.h"
#include "ui/gl/gl_wgl_api_implementation.h"
#include "ui/gl/init/gl_display_initializer.h"
#include "ui/gl/vsync_provider_win.h"

namespace gl {
namespace init {

namespace {

const wchar_t kD3DCompiler[] = L"D3DCompiler_47.dll";

bool LoadD3DXLibrary(const base::FilePath& module_path,
                     const base::FilePath::StringType& name) {
  base::NativeLibrary library =
      base::LoadNativeLibrary(module_path.Append(name), nullptr);
  if (!library) {
    library = base::LoadNativeLibrary(base::FilePath(name), nullptr);
    if (!library) {
      DVLOG(1) << name << " not found.";
      return false;
    }
  }
  return true;
}

bool InitializeStaticEGLInternalFromLibrary(GLImplementation implementation) {
#if BUILDFLAG(USE_STATIC_ANGLE)
  NOTREACHED();
#endif

  base::FilePath module_path;
  if (!base::PathService::Get(base::DIR_MODULE, &module_path))
    return false;

  // Attempt to load the D3DX shader compiler using an absolute path. This is to
  // ensure that we load the versions of these DLLs that we ship. If that fails,
  // load the OS version.
  LoadD3DXLibrary(module_path, kD3DCompiler);

  base::FilePath gles_path = module_path;

  // Load libglesv2.dll before libegl.dll because the latter is dependent on
  // the former and if there is another version of libglesv2.dll in the dll
  // search path, it will get loaded instead.
  base::NativeLibrary gles_library =
      base::LoadNativeLibrary(gles_path.Append(L"libglesv2.dll"), nullptr);
  if (!gles_library) {
    DVLOG(1) << "libglesv2.dll not found";
    return false;
  }

  // When using EGL, first try eglGetProcAddress and then Windows
  // GetProcAddress on both the EGL and GLES2 DLLs.
  base::NativeLibrary egl_library =
      base::LoadNativeLibrary(gles_path.Append(L"libegl.dll"), nullptr);
  if (!egl_library) {
    DVLOG(1) << "libegl.dll not found.";
    base::UnloadNativeLibrary(gles_library);
    return false;
  }

  GLGetProcAddressProc get_proc_address =
      reinterpret_cast<GLGetProcAddressProc>(
          base::GetFunctionPointerFromNativeLibrary(egl_library,
                                                    "eglGetProcAddress"));
  if (!get_proc_address) {
    LOG(ERROR) << "eglGetProcAddress not found.";
    base::UnloadNativeLibrary(egl_library);
    base::UnloadNativeLibrary(gles_library);
    return false;
  }

  SetGLGetProcAddressProc(get_proc_address);
  AddGLNativeLibrary(egl_library);
  AddGLNativeLibrary(gles_library);

  return true;
}

bool InitializeStaticEGLInternal(GLImplementationParts implementation) {
#if BUILDFLAG(USE_STATIC_ANGLE)
  if (implementation.gl == kGLImplementationEGLANGLE) {
    // Use ANGLE if it is requested and it is statically linked
    if (!InitializeStaticANGLEEGL())
      return false;
  } else if (!InitializeStaticEGLInternalFromLibrary(implementation.gl)) {
    return false;
  }
#else
  if (!InitializeStaticEGLInternalFromLibrary(implementation.gl)) {
    return false;
  }
#endif  // !BUILDFLAG(USE_STATIC_ANGLE)

  SetGLImplementationParts(implementation);
  InitializeStaticGLBindingsGL();
  InitializeStaticGLBindingsEGL();

  return true;
}

bool InitializeStaticWGLInternal() {
#ifdef TOOLKIT_QT
  const wchar_t *libraryName = L"opengl32.dll";
  if (usingSoftwareDynamicGL())
      libraryName = L"opengl32sw.dll";

  base::NativeLibrary library =
      base::LoadNativeLibrary(base::FilePath(libraryName), nullptr);
  if (!library) {
    DVLOG(1) << libraryName << " not found";
    return false;
  }
#else
  base::NativeLibrary library =
      base::LoadNativeLibrary(base::FilePath(L"opengl32.dll"), nullptr);
  if (!library) {
    DVLOG(1) << "opengl32.dll not found";
    return false;
  }
#endif

  GLGetProcAddressProc get_proc_address =
      reinterpret_cast<GLGetProcAddressProc>(
          base::GetFunctionPointerFromNativeLibrary(library,
                                                    "wglGetProcAddress"));
  if (!get_proc_address) {
    LOG(ERROR) << "wglGetProcAddress not found.";
    base::UnloadNativeLibrary(library);
    return false;
  }

  SetGLGetProcAddressProc(get_proc_address);
  AddGLNativeLibrary(library);
  SetGLImplementation(kGLImplementationDesktopGL);

  // Initialize GL surface and get some functions needed for the context
  // creation below.
  if (!GLSurfaceWGL::InitializeOneOff()) {
    LOG(ERROR) << "GLSurfaceWGL::InitializeOneOff failed.";
    return false;
  }
  wglCreateContextProc wglCreateContextFn =
      reinterpret_cast<wglCreateContextProc>(
          GetGLProcAddress("wglCreateContext"));
  wglDeleteContextProc wglDeleteContextFn =
      reinterpret_cast<wglDeleteContextProc>(
          GetGLProcAddress("wglDeleteContext"));
  wglMakeCurrentProc wglMakeCurrentFn =
      reinterpret_cast<wglMakeCurrentProc>(GetGLProcAddress("wglMakeCurrent"));

  // Create a temporary GL context to bind to entry points. This is needed
  // because wglGetProcAddress is specified to return nullptr for all queries
  // if a context is not current in MSDN documentation, and the static
  // bindings may contain functions that need to be queried with
  // wglGetProcAddress. OpenGL wiki further warns that other error values
  // than nullptr could also be returned from wglGetProcAddress on some
  // implementations, so we need to clear the WGL bindings and reinitialize
  // them after the context creation.
  HGLRC gl_context = wglCreateContextFn(GLSurfaceWGL::GetDisplayDC());
  if (!gl_context) {
    LOG(ERROR) << "Failed to create temporary context.";
    return false;
  }
  if (!wglMakeCurrentFn(GLSurfaceWGL::GetDisplayDC(), gl_context)) {
    LOG(ERROR) << "Failed to make temporary GL context current.";
    wglDeleteContextFn(gl_context);
    return false;
  }

  InitializeStaticGLBindingsGL();
  InitializeStaticGLBindingsWGL();

  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(gl_context);

  return true;
}

}  // namespace

#if !defined(TOOLKIT_QT)
GLDisplay* InitializeGLOneOffPlatform(gl::GpuPreference gpu_preference) {
  VSyncProviderWin::InitializeOneOff();

  GLDisplayEGL* display = GetDisplayEGL(gpu_preference);
  switch (GetGLImplementation()) {
    case kGLImplementationDesktopGL:
      if (!GLSurfaceWGL::InitializeOneOff()) {
        LOG(ERROR) << "GLSurfaceWGL::InitializeOneOff failed.";
        return false;
      }
      break;
    case kGLImplementationEGLANGLE: {
      if (!InitializeDisplay(display, EGLDisplayPlatform(GetDC(nullptr)))) {
        LOG(ERROR) << "GLDisplayEGL::Initialize failed.";
        return nullptr;
      }
      if (auto d3d11_device = QueryD3D11DeviceObjectFromANGLE()) {
        InitializeDirectComposition(std::move(d3d11_device));
      }
      break;
    }
    case kGLImplementationMockGL:
    case kGLImplementationStubGL:
      break;
    default:
      NOTREACHED();
  }
  return display;
}
#endif

bool InitializeStaticGLBindings(GLImplementationParts implementation) {
  // Prevent reinitialization with a different implementation. Once the gpu
  // unit tests have initialized with kGLImplementationMock, we don't want to
  // later switch to another GL implementation.
  DCHECK_EQ(kGLImplementationNone, GetGLImplementation());

  // Allow the main thread or another to initialize these bindings
  // after instituting restrictions on I/O. Going forward they will
  // likely be used in the browser process on most platforms. The
  // one-time initialization cost is small, between 2 and 5 ms.
  base::ScopedAllowBlocking allow_blocking;

  switch (implementation.gl) {
    case kGLImplementationEGLANGLE:
      return InitializeStaticEGLInternal(implementation);
    case kGLImplementationDesktopGL:
      return InitializeStaticWGLInternal();
    case kGLImplementationMockGL:
    case kGLImplementationStubGL:
      SetGLImplementationParts(implementation);
      InitializeStaticGLBindingsGL();
      return true;
    default:
      NOTREACHED();
  }

  return false;
}

void ShutdownGLPlatform(GLDisplay* display) {
  ShutdownDirectComposition();
  if (display)
    display->Shutdown();
  ClearBindingsEGL();
  ClearBindingsGL();
  ClearBindingsWGL();
}

}  // namespace init
}  // namespace gl
