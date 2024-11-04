// Copyright 2023 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn/native/d3d/PlatformFunctions.h"

#include <comdef.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <utility>

namespace dawn::native::d3d {
namespace {
// Extract Version from "10.0.{Version}.0" if possible, otherwise return 0.
uint32_t GetWindowsSDKVersionFromDirectoryName(const char* directoryName) {
    constexpr char kPrefix[] = "10.0.";
    constexpr char kPostfix[] = ".0";

    constexpr uint32_t kPrefixLen = sizeof(kPrefix) - 1;
    constexpr uint32_t kPostfixLen = sizeof(kPostfix) - 1;
    const uint32_t directoryNameLen = strlen(directoryName);

    if (directoryNameLen < kPrefixLen + kPostfixLen + 1) {
        return 0;
    }

    // Check if directoryName starts with "10.0.".
    if (strncmp(directoryName, kPrefix, kPrefixLen) != 0) {
        return 0;
    }

    // Check if directoryName ends with ".0".
    if (strncmp(directoryName + (directoryNameLen - kPostfixLen), kPostfix, kPostfixLen) != 0) {
        return 0;
    }

    // Extract Version from "10.0.{Version}.0" and convert Version into an integer.
    return atoi(directoryName + kPrefixLen);
}

class ScopedFileHandle final {
  public:
    explicit ScopedFileHandle(HANDLE handle) : mHandle(handle) {}
    ~ScopedFileHandle() {
        if (mHandle != INVALID_HANDLE_VALUE) {
            ASSERT(FindClose(mHandle));
        }
    }
    HANDLE GetHandle() const { return mHandle; }

  private:
    HANDLE mHandle;
};

std::string GetWindowsSDKBasePath() {
    const char* kDefaultWindowsSDKPath = "C:\\Program Files (x86)\\Windows Kits\\10\\bin\\*";
    WIN32_FIND_DATAA fileData;
    ScopedFileHandle handle(FindFirstFileA(kDefaultWindowsSDKPath, &fileData));
    if (handle.GetHandle() == INVALID_HANDLE_VALUE) {
        return "";
    }

    uint32_t highestWindowsSDKVersion = 0;
    do {
        if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            continue;
        }

        highestWindowsSDKVersion = std::max(
            highestWindowsSDKVersion, GetWindowsSDKVersionFromDirectoryName(fileData.cFileName));
    } while (FindNextFileA(handle.GetHandle(), &fileData));

    if (highestWindowsSDKVersion == 0) {
        return "";
    }

    // Currently we only support using DXC on x64.
    std::ostringstream ostream;
    ostream << "C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0." << highestWindowsSDKVersion
            << ".0\\x64\\";

    return ostream.str();
}
}  // anonymous namespace

PlatformFunctions::PlatformFunctions() = default;
PlatformFunctions::~PlatformFunctions() = default;

MaybeError PlatformFunctions::LoadFunctions() {
    DAWN_TRY(LoadDXGI());
    LoadDXCLibraries();
    DAWN_TRY(LoadFXCompiler());
    return {};
}

MaybeError PlatformFunctions::LoadDXGI() {
#if DAWN_PLATFORM_IS(WINUWP)
#if defined(_DEBUG)
    // DXGIGetDebugInterface1 is tagged as a development-only capability
    // which implies that linking to this function will cause
    // the application to fail Windows store certification
    // But we need it when debuging using VS Graphics Diagnostics or PIX
    // So we only link to it in debug build
    dxgiGetDebugInterface1 = &DXGIGetDebugInterface1;
#endif
    createDxgiFactory2 = &CreateDXGIFactory2;
#else
    std::string error;
    if (!mDXGILib.Open("dxgi.dll", &error) ||
        !mDXGILib.GetProc(&dxgiGetDebugInterface1, "DXGIGetDebugInterface1", &error) ||
        !mDXGILib.GetProc(&createDxgiFactory2, "CreateDXGIFactory2", &error)) {
        return DAWN_INTERNAL_ERROR(error.c_str());
    }
#endif

    return {};
}

void PlatformFunctions::LoadDXCLibraries() {
    // TODO(dawn:766)
    // Statically linked with dxcompiler.lib in UWP
    // currently linked with dxcompiler.lib making CoreApp unable to activate
    // LoadDXIL and LoadDXCompiler will fail in UWP, but LoadFunctions() can still be
    // successfully executed.

    const std::string& windowsSDKBasePath = GetWindowsSDKBasePath();

    LoadDXIL(windowsSDKBasePath);
    LoadDXCompiler(windowsSDKBasePath);
}

void PlatformFunctions::LoadDXIL(const std::string& baseWindowsSDKPath) {
    constexpr char kDxilDLLName[] = "dxil.dll";
    const std::array kDxilDLLPaths{
#ifdef DAWN_USE_BUILT_DXC
        std::string{kDxilDLLName},
#else
        std::string{kDxilDLLName},
        baseWindowsSDKPath + kDxilDLLName,
#endif
    };

    for (const std::string& dxilDLLPath : kDxilDLLPaths) {
        if (mDXILLib.Open(dxilDLLPath, nullptr)) {
            return;
        }
    }
    ASSERT(!mDXILLib.Valid());
}

void PlatformFunctions::LoadDXCompiler(const std::string& baseWindowsSDKPath) {
    // DXIL must be loaded before DXC, otherwise shader signing is unavailable
    if (!mDXILLib.Valid()) {
        return;
    }

    constexpr char kDxCompilerDLLName[] = "dxcompiler.dll";
    const std::array kDxCompilerDLLPaths{
#ifdef DAWN_USE_BUILT_DXC
        std::string{kDxCompilerDLLName},
#else
        std::string{kDxCompilerDLLName}, baseWindowsSDKPath + kDxCompilerDLLName
#endif
    };

    DynamicLib dxCompilerLib;
    for (const std::string& dllName : kDxCompilerDLLPaths) {
        if (dxCompilerLib.Open(dllName, nullptr)) {
            break;
        }
    }

    if (dxCompilerLib.Valid() &&
        dxCompilerLib.GetProc(&dxcCreateInstance, "DxcCreateInstance", nullptr)) {
        mDXCompilerLib = std::move(dxCompilerLib);
    } else {
        mDXILLib.Close();
    }
}

MaybeError PlatformFunctions::LoadFXCompiler() {
#if DAWN_PLATFORM_IS(WINUWP)
    d3dCompile = &D3DCompile;
    d3dDisassemble = &D3DDisassemble;
#else
    std::string error;
    if (!mFXCompilerLib.Open("d3dcompiler_47.dll", &error) ||
        !mFXCompilerLib.GetProc(&d3dCompile, "D3DCompile", &error) ||
        !mFXCompilerLib.GetProc(&d3dDisassemble, "D3DDisassemble", &error)) {
        return DAWN_INTERNAL_ERROR(error.c_str());
    }
#endif
    return {};
}

// Use Backend::IsDXCAvaliable if possible, which also check the DXC is no older than a given
// version
bool PlatformFunctions::IsDXCBinaryAvailable() const {
    return mDXILLib.Valid() && mDXCompilerLib.Valid();
}

}  // namespace dawn::native::d3d
