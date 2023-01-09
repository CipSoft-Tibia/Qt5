/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "ozone_platform_qt.h"

#if defined(USE_OZONE)
#include "ui/base/buildflags.h"
#include "ui/base/cursor/ozone/bitmap_cursor_factory_ozone.h"
#include "ui/base/ime/input_method.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/events/ozone/layout/stub/stub_keyboard_layout_engine.h"
#include "ui/ozone/common/stub_client_native_pixmap_factory.h"
#include "ui/ozone/common/stub_overlay_manager.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/public/input_controller.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/platform_screen.h"
#include "ui/ozone/public/system_input_injector.h"
#include "ui/platform_window/platform_window_delegate.h"
#include "ui/platform_window/platform_window_init_properties.h"

#include "surface_factory_qt.h"
#include "platform_window_qt.h"

#if BUILDFLAG(USE_XKBCOMMON) && defined(USE_X11)
#include "ui/events/ozone/layout/xkb/xkb_evdev_codes.h"
#include "ui/events/ozone/layout/xkb/xkb_keyboard_layout_engine.h"

#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

extern void *GetQtXDisplay();
#endif // BUILDFLAG(USE_XKBCOMMON) && defined(USE_X11)

namespace ui {

namespace {

class OzonePlatformQt : public OzonePlatform {
public:
    OzonePlatformQt();
    ~OzonePlatformQt() override;

    ui::SurfaceFactoryOzone* GetSurfaceFactoryOzone() override;
    ui::CursorFactory* GetCursorFactory() override;
    GpuPlatformSupportHost* GetGpuPlatformSupportHost() override;
    std::unique_ptr<PlatformWindow> CreatePlatformWindow(PlatformWindowDelegate* delegate, PlatformWindowInitProperties properties) override;
    std::unique_ptr<display::NativeDisplayDelegate> CreateNativeDisplayDelegate() override;
    ui::InputController* GetInputController() override;
    std::unique_ptr<ui::SystemInputInjector> CreateSystemInputInjector() override;
    ui::OverlayManagerOzone* GetOverlayManager() override;
    std::unique_ptr<InputMethod> CreateInputMethod(internal::InputMethodDelegate *delegate, gfx::AcceleratedWidget widget) override;
    std::unique_ptr<ui::PlatformScreen> CreateScreen() override { return nullptr; }
private:
    void InitializeUI(const ui::OzonePlatform::InitParams &) override;
    void InitializeGPU(const ui::OzonePlatform::InitParams &) override;

    std::unique_ptr<QtWebEngineCore::SurfaceFactoryQt> surface_factory_ozone_;
    std::unique_ptr<CursorFactory> cursor_factory_ozone_;

    std::unique_ptr<GpuPlatformSupportHost> gpu_platform_support_host_;
    std::unique_ptr<InputController> input_controller_;
    std::unique_ptr<OverlayManagerOzone> overlay_manager_;

#if BUILDFLAG(USE_XKBCOMMON) && defined(USE_X11)
    XkbEvdevCodes m_xkbEvdevCodeConverter;
#endif
    std::unique_ptr<KeyboardLayoutEngine> m_keyboardLayoutEngine;

    DISALLOW_COPY_AND_ASSIGN(OzonePlatformQt);
};


OzonePlatformQt::OzonePlatformQt() {}

OzonePlatformQt::~OzonePlatformQt() {}

ui::SurfaceFactoryOzone* OzonePlatformQt::GetSurfaceFactoryOzone()
{
    return surface_factory_ozone_.get();
}

ui::CursorFactory* OzonePlatformQt::GetCursorFactory()
{
    return cursor_factory_ozone_.get();
}

GpuPlatformSupportHost* OzonePlatformQt::GetGpuPlatformSupportHost()
{
    return gpu_platform_support_host_.get();
}

std::unique_ptr<PlatformWindow> OzonePlatformQt::CreatePlatformWindow(PlatformWindowDelegate* delegate, PlatformWindowInitProperties properties)
{
    return base::WrapUnique(new PlatformWindowQt(delegate, properties.bounds));
}

ui::InputController* OzonePlatformQt::GetInputController()
{
    return input_controller_.get();
}

std::unique_ptr<ui::SystemInputInjector> OzonePlatformQt::CreateSystemInputInjector()
{
    return nullptr;  // no input injection support.
}

ui::OverlayManagerOzone* OzonePlatformQt::GetOverlayManager()
{
    return overlay_manager_.get();
}

std::unique_ptr<display::NativeDisplayDelegate> OzonePlatformQt::CreateNativeDisplayDelegate()
{
    NOTREACHED();
    return nullptr;
}

#if BUILDFLAG(USE_XKBCOMMON) && defined(USE_X11)
static std::string getCurrentKeyboardLayout()
{
    Display *dpy = static_cast<Display *>(GetQtXDisplay());
    if (dpy == nullptr)
        return std::string();

    XkbStateRec state;
    if (XkbGetState(dpy, XkbUseCoreKbd, &state) != 0)
        return std::string();

    XkbRF_VarDefsRec vdr {}; // zero initialize it
    struct Cleanup {
        XkbRF_VarDefsRec &vdr;
        Cleanup(XkbRF_VarDefsRec &vdr) : vdr(vdr) { }
        ~Cleanup() {
            free (vdr.model);
            free (vdr.layout);
            free (vdr.variant);
            free (vdr.options);
        }
    } cleanup(vdr);
    if (XkbRF_GetNamesProp(dpy, nullptr, &vdr) == 0)
        return std::string();

    char *layout = strtok(vdr.layout, ",");
    for (int i = 0; i < state.group; i++) {
        layout = strtok(nullptr, ",");
        if (layout == nullptr)
            return std::string();
    }

    char *variant = strtok(vdr.variant, ",");
    if (!variant)
        return layout;

    for (int i = 0; i < state.group; i++) {
        variant = strtok(nullptr, ",");
        if (variant == nullptr)
            return layout;
    }

    std::string layoutWithVariant = layout;
    layoutWithVariant = layoutWithVariant.append("-");
    layoutWithVariant = layoutWithVariant.append(variant);
    return layoutWithVariant;
}
#endif // BUILDFLAG(USE_XKBCOMMON) && defined(USE_X11)

void OzonePlatformQt::InitializeUI(const ui::OzonePlatform::InitParams &)
{
    overlay_manager_.reset(new StubOverlayManager());
    cursor_factory_ozone_.reset(new BitmapCursorFactoryOzone());
    gpu_platform_support_host_.reset(ui::CreateStubGpuPlatformSupportHost());
    input_controller_ = CreateStubInputController();

#if BUILDFLAG(USE_XKBCOMMON) && defined(USE_X11)
    std::string layout = getCurrentKeyboardLayout();
    if (layout.empty()) {
        m_keyboardLayoutEngine = std::make_unique<StubKeyboardLayoutEngine>();
    } else {
        m_keyboardLayoutEngine = std::make_unique<XkbKeyboardLayoutEngine>(m_xkbEvdevCodeConverter);
        m_keyboardLayoutEngine->SetCurrentLayoutByName(layout);
    }
#else
    m_keyboardLayoutEngine = std::make_unique<StubKeyboardLayoutEngine>();
#endif // BUILDFLAG(USE_XKBCOMMON) && defined(USE_X11)

    KeyboardLayoutEngineManager::SetKeyboardLayoutEngine(m_keyboardLayoutEngine.get());
}

void OzonePlatformQt::InitializeGPU(const ui::OzonePlatform::InitParams &)
{
    surface_factory_ozone_.reset(new QtWebEngineCore::SurfaceFactoryQt());
}

std::unique_ptr<InputMethod> OzonePlatformQt::CreateInputMethod(internal::InputMethodDelegate *, gfx::AcceleratedWidget)
{
    NOTREACHED();
    return nullptr;
}

} // namespace

OzonePlatform* CreateOzonePlatformQt() { return new OzonePlatformQt; }

gfx::ClientNativePixmapFactory* CreateClientNativePixmapFactoryQt()
{
    return CreateStubClientNativePixmapFactory();
}

}  // namespace ui

#endif

