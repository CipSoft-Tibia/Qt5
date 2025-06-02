// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR
// GPL-2.0-only OR GPL-3.0-only

#include "build/build_config.h"
#include "gpu/vulkan/buildflags.h"
#include "ui/base/ozone_buildflags.h"

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_display.h"
#include "ui/gl/gl_surface.h"
#endif

#if BUILDFLAG(IS_LINUX) && BUILDFLAG(IS_OZONE_X11)
#include "ui/gl/gl_surface_glx.h"
#endif

#include "ui/base/dragdrop/os_exchange_data_provider_factory.h"
#include "ui/base/pointer/pointer_device.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(ENABLE_VULKAN)
#include "gpu/vulkan/init/vulkan_factory.h"
#endif

#if BUILDFLAG(IS_LINUX) && BUILDFLAG(IS_OZONE_X11)
void* GetQtXDisplay() {
  return nullptr;
}

namespace gl {
bool GLSurfaceGLX::IsCreateContextRobustnessSupported() {
  return false;
}
bool GLSurfaceGLX::IsRobustnessVideoMemoryPurgeSupported() {
  return false;
}
bool GLSurfaceGLX::IsCreateContextProfileSupported() {
  return false;
}
bool GLSurfaceGLX::IsCreateContextES2ProfileSupported() {
  return false;
}
bool GLSurfaceGLX::IsCreateContextSupported() {
  return false;
}
} // namespace gl
#endif  // if BUILDFLAG(IS_LINUX) && BUILDFLAG(IS_OZONE_X11)

#if BUILDFLAG(IS_WIN)
namespace gl {
class GLShareGroup;
class GLContextAttribs;
namespace init {

scoped_refptr<GLContext> CreateGLContext(GLShareGroup* share_group,
                                         GLSurface* compatible_surface,
                                         const GLContextAttribs& attribs) {
  return nullptr;
}
GLDisplay* InitializeGLOneOffPlatform(GpuPreference preference) {
  return nullptr;
}
bool usingSoftwareDynamicGL() {
  return false;
}
scoped_refptr<GLSurface> CreateOffscreenGLSurface(GLDisplay* display,
                                                  const gfx::Size& size) {
  return nullptr;
}
scoped_refptr<GLSurface> CreateViewGLSurface(GLDisplay* display,
                                             gfx::AcceleratedWidget window) {
  return nullptr;
}

}  // namespace init
}  // namespace gl
#endif  // BUILDFLAG(IS_WIN)

namespace ui {
class OzonePlatform;
OzonePlatform* CreateOzonePlatformQt() {
  return nullptr;
}
std::unique_ptr<OSExchangeDataProvider>
OSExchangeDataProviderFactory::CreateProvider() {
  return nullptr;
}
bool ResourceBundle::LocaleDataPakExists(const std::string& locale) {
  return false;
}
std::string ResourceBundle::LoadLocaleResources(const std::string& pref_locale,
                                                bool crash_on_failure) {
  return std::string();
}
void ResourceBundle::LoadCommonResources() {}
int GetAvailablePointerTypes() {
  return POINTER_TYPE_NONE;
}
int GetAvailableHoverTypes() {
  return HOVER_TYPE_NONE;
}
gfx::Image& ResourceBundle::GetNativeImageNamed(int resource_id) {
  return GetImageNamed(resource_id);
}

namespace gfx {
class ClientNativePixmapFactory;
}
gfx::ClientNativePixmapFactory* CreateClientNativePixmapFactoryQt() {
  return nullptr;
}
}  // namespace ui

#if BUILDFLAG(ENABLE_VULKAN)
namespace gpu {
std::unique_ptr<VulkanImplementation> CreateVulkanImplementation(
    bool use_swiftshader,
    bool allow_protected_memory) {
  return nullptr;
}
}
#endif  // BUILDFLAG(ENABLE_VULKAN)
