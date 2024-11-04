// Copyright 2022 The Dawn Authors
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

#include "dawn/native/opengl/PhysicalDeviceGL.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "dawn/common/GPUInfo.h"
#include "dawn/native/Instance.h"
#include "dawn/native/opengl/ContextEGL.h"
#include "dawn/native/opengl/DeviceGL.h"

namespace dawn::native::opengl {

namespace {

struct Vendor {
    const char* vendorName;
    uint32_t vendorId;
};

const Vendor kVendors[] = {{"ATI", gpu_info::kVendorID_AMD},
                           {"ARM", gpu_info::kVendorID_ARM},
                           {"Imagination", gpu_info::kVendorID_ImgTec},
                           {"Intel", gpu_info::kVendorID_Intel},
                           {"NVIDIA", gpu_info::kVendorID_Nvidia},
                           {"Qualcomm", gpu_info::kVendorID_Qualcomm}};

uint32_t GetVendorIdFromVendors(const char* vendor) {
    uint32_t vendorId = 0;
    for (const auto& it : kVendors) {
        // Matching vendor name with vendor string
        if (strstr(vendor, it.vendorName) != nullptr) {
            vendorId = it.vendorId;
            break;
        }
    }
    return vendorId;
}

uint32_t GetDeviceIdFromRender(std::string_view render) {
    uint32_t deviceId = 0;
    size_t pos = render.find("(0x");
    if (pos == std::string_view::npos) {
        pos = render.find("(0X");
    }
    if (pos == std::string_view::npos) {
        return deviceId;
    }
    render.remove_prefix(pos + 3);

    // The first character after the prefix must be hexadecimal, otherwise an invalid argument
    // exception is thrown.
    if (!render.empty() && std::isxdigit(static_cast<unsigned char>(*render.data()))) {
        deviceId = static_cast<uint32_t>(std::stoul(render.data(), nullptr, 16));
    }

    return deviceId;
}

}  // anonymous namespace

// static
ResultOrError<Ref<PhysicalDevice>> PhysicalDevice::Create(InstanceBase* instance,
                                                          wgpu::BackendType backendType,
                                                          void* (*getProc)(const char*),
                                                          EGLDisplay display) {
    EGLFunctions egl;
    egl.Init(getProc);

    EGLenum api = backendType == wgpu::BackendType::OpenGLES ? EGL_OPENGL_ES_API : EGL_OPENGL_API;

    if (display == EGL_NO_DISPLAY) {
        display = egl.GetCurrentDisplay();
    }

    if (display == EGL_NO_DISPLAY) {
        display = egl.GetDisplay(EGL_DEFAULT_DISPLAY);
    }

    std::unique_ptr<ContextEGL> context;
    DAWN_TRY_ASSIGN(context, ContextEGL::Create(egl, api, display, false));

    EGLContext prevDrawSurface = egl.GetCurrentSurface(EGL_DRAW);
    EGLContext prevReadSurface = egl.GetCurrentSurface(EGL_READ);
    EGLContext prevContext = egl.GetCurrentContext();

    context->MakeCurrent();

    Ref<PhysicalDevice> physicalDevice =
        AcquireRef(new PhysicalDevice(instance, backendType, display));
    DAWN_TRY(physicalDevice->InitializeGLFunctions(getProc));
    DAWN_TRY(physicalDevice->Initialize());

    egl.MakeCurrent(display, prevDrawSurface, prevReadSurface, prevContext);
    return physicalDevice;
}

PhysicalDevice::PhysicalDevice(InstanceBase* instance,
                               wgpu::BackendType backendType,
                               EGLDisplay display)
    : PhysicalDeviceBase(instance, backendType), mDisplay(display) {}

MaybeError PhysicalDevice::InitializeGLFunctions(void* (*getProc)(const char*)) {
    // Use getProc to populate the dispatch table
    mEGLFunctions.Init(getProc);
    return mFunctions.Initialize(getProc);
}

bool PhysicalDevice::SupportsExternalImages() const {
    // Via dawn::native::opengl::WrapExternalEGLImage
    return GetBackendType() == wgpu::BackendType::OpenGLES;
}

MaybeError PhysicalDevice::InitializeImpl() {
    if (mFunctions.GetVersion().IsES()) {
        ASSERT(GetBackendType() == wgpu::BackendType::OpenGLES);
    } else {
        ASSERT(GetBackendType() == wgpu::BackendType::OpenGL);
    }

    mName = reinterpret_cast<const char*>(mFunctions.GetString(GL_RENDERER));

    // Workaroud to find vendor id from vendor name
    const char* vendor = reinterpret_cast<const char*>(mFunctions.GetString(GL_VENDOR));
    mVendorId = GetVendorIdFromVendors(vendor);
    // Workaround to find device id from ANGLE render string
    if (mName.find("ANGLE") == 0) {
        mDeviceId = GetDeviceIdFromRender(mName);
    }

    mDriverDescription = std::string("OpenGL version ") +
                         reinterpret_cast<const char*>(mFunctions.GetString(GL_VERSION));

    if (mName.find("SwiftShader") != std::string::npos) {
        mAdapterType = wgpu::AdapterType::CPU;
    }

    return {};
}

void PhysicalDevice::InitializeSupportedFeaturesImpl() {
    // TextureCompressionBC
    {
        // BC1, BC2 and BC3 are not supported in OpenGL or OpenGL ES core features.
        bool supportsS3TC =
            mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_s3tc") ||
            (mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_dxt1") &&
             mFunctions.IsGLExtensionSupported("GL_ANGLE_texture_compression_dxt3") &&
             mFunctions.IsGLExtensionSupported("GL_ANGLE_texture_compression_dxt5"));

        // COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT and
        // COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT requires both GL_EXT_texture_sRGB and
        // GL_EXT_texture_compression_s3tc on desktop OpenGL drivers.
        // (https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_sRGB.txt)
        bool supportsTextureSRGB = mFunctions.IsGLExtensionSupported("GL_EXT_texture_sRGB");

        // GL_EXT_texture_compression_s3tc_srgb is an extension in OpenGL ES.
        // NVidia GLES drivers don't support this extension, but they do support
        // GL_NV_sRGB_formats. (Note that GL_EXT_texture_sRGB does not exist on ES.
        // GL_EXT_sRGB does (core in ES 3.0), but it does not automatically provide S3TC
        // SRGB support even if S3TC is supported; see
        // https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_sRGB.txt.)
        bool supportsS3TCSRGB =
            mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_s3tc_srgb") ||
            mFunctions.IsGLExtensionSupported("GL_NV_sRGB_formats");

        // BC4 and BC5
        bool supportsRGTC = mFunctions.IsAtLeastGL(3, 0) ||
                            mFunctions.IsGLExtensionSupported("GL_ARB_texture_compression_rgtc") ||
                            mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_rgtc");

        // BC6 and BC7
        bool supportsBPTC = mFunctions.IsAtLeastGL(4, 2) ||
                            mFunctions.IsGLExtensionSupported("GL_ARB_texture_compression_bptc") ||
                            mFunctions.IsGLExtensionSupported("GL_EXT_texture_compression_bptc");

        if (supportsS3TC && (supportsTextureSRGB || supportsS3TCSRGB) && supportsRGTC &&
            supportsBPTC) {
            EnableFeature(dawn::native::Feature::TextureCompressionBC);
        }
    }
    if (mName.find("ANGLE") != std::string::npos) {
        EnableFeature(dawn::native::Feature::ANGLETextureSharing);
    }

    // Non-zero baseInstance requires at least desktop OpenGL 4.2, and it is not supported in
    // OpenGL ES OpenGL:
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawElementsIndirect.xhtml
    // OpenGL ES:
    // https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDrawElementsIndirect.xhtml
    if (mFunctions.IsAtLeastGL(4, 2)) {
        EnableFeature(Feature::IndirectFirstInstance);
    }

    // ShaderF16
    if (mFunctions.IsGLExtensionSupported("GL_AMD_gpu_shader_half_float")) {
        EnableFeature(Feature::ShaderF16);
    }

    // DualSourceBlending
    if (mFunctions.IsGLExtensionSupported("GL_EXT_blend_func_extended") ||
        mFunctions.IsAtLeastGL(3, 3)) {
        EnableFeature(Feature::DualSourceBlending);
    }

    // Norm16TextureFormats
    if (mFunctions.IsGLExtensionSupported("GL_EXT_texture_norm16")) {
        EnableFeature(Feature::Norm16TextureFormats);
    }
}

namespace {
GLint Get(const OpenGLFunctions& gl, GLenum pname) {
    GLint value;
    gl.GetIntegerv(pname, &value);
    return value;
}

GLint GetIndexed(const OpenGLFunctions& gl, GLenum pname, GLuint index) {
    GLint value;
    gl.GetIntegeri_v(pname, index, &value);
    return value;
}
}  // namespace

MaybeError PhysicalDevice::InitializeSupportedLimitsImpl(CombinedLimits* limits) {
    const OpenGLFunctions& gl = mFunctions;
    GetDefaultLimits(&limits->v1);

    limits->v1.maxTextureDimension1D = limits->v1.maxTextureDimension2D =
        Get(gl, GL_MAX_TEXTURE_SIZE);
    limits->v1.maxTextureDimension3D = Get(gl, GL_MAX_3D_TEXTURE_SIZE);
    limits->v1.maxTextureArrayLayers = Get(gl, GL_MAX_ARRAY_TEXTURE_LAYERS);

    // Since we flatten bindings, leave maxBindGroups and maxBindingsPerBindGroup at the default.

    limits->v1.maxDynamicUniformBuffersPerPipelineLayout = Get(gl, GL_MAX_UNIFORM_BUFFER_BINDINGS);
    limits->v1.maxDynamicStorageBuffersPerPipelineLayout =
        Get(gl, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS);
    limits->v1.maxSampledTexturesPerShaderStage =
        std::min(Get(gl, GL_MAX_TEXTURE_IMAGE_UNITS), Get(gl, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS));
    limits->v1.maxSamplersPerShaderStage = Get(gl, GL_MAX_TEXTURE_IMAGE_UNITS);
    limits->v1.maxStorageBuffersPerShaderStage = Get(gl, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS);
    // TODO(crbug.com/dawn/1834): Note that OpenGLES allows an implementation to have zero vertex
    // image uniforms, so this isn't technically correct for vertex shaders.
    limits->v1.maxStorageTexturesPerShaderStage = Get(gl, GL_MAX_FRAGMENT_IMAGE_UNIFORMS);

    limits->v1.maxUniformBuffersPerShaderStage = Get(gl, GL_MAX_UNIFORM_BUFFER_BINDINGS);
    limits->v1.maxUniformBufferBindingSize = Get(gl, GL_MAX_UNIFORM_BLOCK_SIZE);
    limits->v1.maxStorageBufferBindingSize = Get(gl, GL_MAX_SHADER_STORAGE_BLOCK_SIZE);

    limits->v1.minUniformBufferOffsetAlignment = Get(gl, GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    limits->v1.minStorageBufferOffsetAlignment = Get(gl, GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
    limits->v1.maxVertexBuffers = Get(gl, GL_MAX_VERTEX_ATTRIB_BINDINGS);
    limits->v1.maxBufferSize = kAssumedMaxBufferSize;
    GLint maxVertexAttribs = Get(gl, GL_MAX_VERTEX_ATTRIBS);
    limits->v1.maxVertexAttributes = limits->v1.maxVertexBuffers * maxVertexAttribs;
    limits->v1.maxVertexBufferArrayStride = Get(gl, GL_MAX_VERTEX_ATTRIB_STRIDE);
    limits->v1.maxInterStageShaderComponents = Get(gl, GL_MAX_VARYING_COMPONENTS);
    limits->v1.maxInterStageShaderVariables = Get(gl, GL_MAX_VARYING_VECTORS);
    limits->v1.maxColorAttachments =
        std::min(Get(gl, GL_MAX_COLOR_ATTACHMENTS), Get(gl, GL_MAX_DRAW_BUFFERS));

    // TODO(crbug.com/dawn/1834): determine if GL has an equivalent value here.
    //    limits->v1.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;

    limits->v1.maxComputeWorkgroupStorageSize = Get(gl, GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
    limits->v1.maxComputeInvocationsPerWorkgroup = Get(gl, GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
    limits->v1.maxComputeWorkgroupSizeX = GetIndexed(gl, GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0);
    limits->v1.maxComputeWorkgroupSizeY = GetIndexed(gl, GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1);
    limits->v1.maxComputeWorkgroupSizeZ = GetIndexed(gl, GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2);
    GLint v[3];
    v[0] = GetIndexed(gl, GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0);
    v[1] = GetIndexed(gl, GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1);
    v[2] = GetIndexed(gl, GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2);
    limits->v1.maxComputeWorkgroupsPerDimension = std::min(v[0], std::min(v[1], v[2]));
    return {};
}

void PhysicalDevice::SetupBackendAdapterToggles(TogglesState* adpterToggles) const {}

void PhysicalDevice::SetupBackendDeviceToggles(TogglesState* deviceToggles) const {
    const OpenGLFunctions& gl = mFunctions;

    bool supportsBaseVertex = gl.IsAtLeastGLES(3, 2) || gl.IsAtLeastGL(3, 2);

    bool supportsBaseInstance = gl.IsAtLeastGLES(3, 2) || gl.IsAtLeastGL(4, 2);

    // TODO(crbug.com/dawn/582): Use OES_draw_buffers_indexed where available.
    bool supportsIndexedDrawBuffers = gl.IsAtLeastGLES(3, 2) || gl.IsAtLeastGL(3, 0);

    bool supportsSnormRead =
        gl.IsAtLeastGL(4, 4) || gl.IsGLExtensionSupported("GL_EXT_render_snorm");

    bool supportsDepthRead = gl.IsAtLeastGL(3, 0) || gl.IsGLExtensionSupported("GL_NV_read_depth");

    bool supportsStencilRead =
        gl.IsAtLeastGL(3, 0) || gl.IsGLExtensionSupported("GL_NV_read_stencil");

    bool supportsDepthStencilRead =
        gl.IsAtLeastGL(3, 0) || gl.IsGLExtensionSupported("GL_NV_read_depth_stencil");

    // Desktop GL supports BGRA textures via swizzling in the driver; ES requires an extension.
    bool supportsBGRARead =
        gl.GetVersion().IsDesktop() || gl.IsGLExtensionSupported("GL_EXT_read_format_bgra");

    bool supportsSampleVariables = gl.IsAtLeastGL(4, 0) || gl.IsAtLeastGLES(3, 2) ||
                                   gl.IsGLExtensionSupported("GL_OES_sample_variables");

    // TODO(crbug.com/dawn/343): We can support the extension variants, but need to load the EXT
    // procs without the extension suffix.
    // We'll also need emulation of shader builtins gl_BaseVertex and gl_BaseInstance.

    // supportsBaseVertex |=
    //     (gl.IsAtLeastGLES(2, 0) &&
    //      (gl.IsGLExtensionSupported("OES_draw_elements_base_vertex") ||
    //       gl.IsGLExtensionSupported("EXT_draw_elements_base_vertex"))) ||
    //     (gl.IsAtLeastGL(3, 1) && gl.IsGLExtensionSupported("ARB_draw_elements_base_vertex"));

    // supportsBaseInstance |=
    //     (gl.IsAtLeastGLES(3, 1) && gl.IsGLExtensionSupported("EXT_base_instance")) ||
    //     (gl.IsAtLeastGL(3, 1) && gl.IsGLExtensionSupported("ARB_base_instance"));

    if (gl.IsAtLeastGLES(3, 1) && gl.IsGLExtensionSupported("GL_ANGLE_base_vertex_base_instance")) {
        supportsBaseVertex = true;
        supportsBaseInstance = true;
    }

    // TODO(crbug.com/dawn/343): Investigate emulation.
    deviceToggles->Default(Toggle::DisableBaseVertex, !supportsBaseVertex);
    deviceToggles->Default(Toggle::DisableBaseInstance, !supportsBaseInstance);
    deviceToggles->Default(Toggle::DisableIndexedDrawBuffers, !supportsIndexedDrawBuffers);
    deviceToggles->Default(Toggle::DisableDepthRead, !supportsDepthRead);
    deviceToggles->Default(Toggle::DisableStencilRead, !supportsStencilRead);
    deviceToggles->Default(Toggle::DisableDepthStencilRead, !supportsDepthStencilRead);
    deviceToggles->Default(Toggle::DisableSampleVariables, !supportsSampleVariables);
    deviceToggles->Default(Toggle::FlushBeforeClientWaitSync, gl.GetVersion().IsES());
    // For OpenGL ES, we must use a placeholder fragment shader for vertex-only render pipeline.
    deviceToggles->Default(Toggle::UsePlaceholderFragmentInVertexOnlyPipeline,
                           gl.GetVersion().IsES());
    // For OpenGL/OpenGL ES, use compute shader blit to emulate depth16unorm texture to buffer
    // copies.
    deviceToggles->Default(Toggle::UseBlitForDepth16UnormTextureToBufferCopy, true);

    // For OpenGL ES, use compute shader blit to emulate depth32float texture to buffer copies.
    deviceToggles->Default(Toggle::UseBlitForDepth32FloatTextureToBufferCopy,
                           gl.GetVersion().IsES());

    // For OpenGL ES, use compute shader blit to emulate stencil texture to buffer copies.
    deviceToggles->Default(Toggle::UseBlitForStencilTextureToBufferCopy, gl.GetVersion().IsES());

    // For OpenGL ES, use compute shader blit to emulate snorm texture to buffer copies.
    deviceToggles->Default(Toggle::UseBlitForSnormTextureToBufferCopy,
                           gl.GetVersion().IsES() || !supportsSnormRead);

    // For OpenGL ES, use compute shader blit to emulate bgra8unorm texture to buffer copies.
    deviceToggles->Default(Toggle::UseBlitForBGRA8UnormTextureToBufferCopy, !supportsBGRARead);
}

ResultOrError<Ref<DeviceBase>> PhysicalDevice::CreateDeviceImpl(AdapterBase* adapter,
                                                                const DeviceDescriptor* descriptor,
                                                                const TogglesState& deviceToggles) {
    EGLenum api =
        GetBackendType() == wgpu::BackendType::OpenGL ? EGL_OPENGL_API : EGL_OPENGL_ES_API;
    std::unique_ptr<Device::Context> context;
    bool useANGLETextureSharing = false;
    for (size_t i = 0; i < descriptor->requiredFeatureCount; ++i) {
        if (descriptor->requiredFeatures[i] == wgpu::FeatureName::ANGLETextureSharing) {
            useANGLETextureSharing = true;
        }
    }

    DAWN_TRY_ASSIGN(context,
                    ContextEGL::Create(mEGLFunctions, api, mDisplay, useANGLETextureSharing));
    return Device::Create(adapter, descriptor, mFunctions, std::move(context), deviceToggles);
}

bool PhysicalDevice::SupportsFeatureLevel(FeatureLevel featureLevel) const {
    return featureLevel == FeatureLevel::Compatibility;
}

MaybeError PhysicalDevice::ValidateFeatureSupportedWithTogglesImpl(
    wgpu::FeatureName feature,
    const TogglesState& toggles) const {
    return {};
}
}  // namespace dawn::native::opengl
