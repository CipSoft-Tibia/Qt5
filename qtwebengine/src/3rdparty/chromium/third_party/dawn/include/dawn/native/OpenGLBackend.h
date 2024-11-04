// Copyright 2018 The Dawn Authors
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

#ifndef INCLUDE_DAWN_NATIVE_OPENGLBACKEND_H_
#define INCLUDE_DAWN_NATIVE_OPENGLBACKEND_H_

using EGLDisplay = void*;
using EGLImage = void*;
using GLuint = unsigned int;

#include "dawn/native/DawnNative.h"
#include "dawn/webgpu_cpp_chained_struct.h"

namespace dawn::native::opengl {

// Can be chained in WGPURequestAdapterOptions
struct DAWN_NATIVE_EXPORT RequestAdapterOptionsGetGLProc : wgpu::ChainedStruct {
    RequestAdapterOptionsGetGLProc();

    void* (*getProc)(const char*);
    EGLDisplay display;
};

struct DAWN_NATIVE_EXPORT PhysicalDeviceDiscoveryOptions
    : public PhysicalDeviceDiscoveryOptionsBase {
    explicit PhysicalDeviceDiscoveryOptions(WGPUBackendType type);

    void* (*getProc)(const char*);
    EGLDisplay display;
};

struct DAWN_NATIVE_EXPORT ExternalImageDescriptorEGLImage : ExternalImageDescriptor {
  public:
    ExternalImageDescriptorEGLImage();

    ::EGLImage image;
};

DAWN_NATIVE_EXPORT WGPUTexture
WrapExternalEGLImage(WGPUDevice device, const ExternalImageDescriptorEGLImage* descriptor);

struct DAWN_NATIVE_EXPORT ExternalImageDescriptorGLTexture : ExternalImageDescriptor {
  public:
    ExternalImageDescriptorGLTexture();

    GLuint texture;
};

DAWN_NATIVE_EXPORT WGPUTexture
WrapExternalGLTexture(WGPUDevice device, const ExternalImageDescriptorGLTexture* descriptor);

}  // namespace dawn::native::opengl

#endif  // INCLUDE_DAWN_NATIVE_OPENGLBACKEND_H_
