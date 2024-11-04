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

#include "dawn/native/metal/DeviceMTL.h"

#include "dawn/common/GPUInfo.h"
#include "dawn/common/Platform.h"
#include "dawn/native/Adapter.h"
#include "dawn/native/BackendConnection.h"
#include "dawn/native/ChainUtils_autogen.h"
#include "dawn/native/Commands.h"
#include "dawn/native/ErrorData.h"
#include "dawn/native/metal/BindGroupLayoutMTL.h"
#include "dawn/native/metal/BindGroupMTL.h"
#include "dawn/native/metal/BufferMTL.h"
#include "dawn/native/metal/CommandBufferMTL.h"
#include "dawn/native/metal/ComputePipelineMTL.h"
#include "dawn/native/metal/PipelineLayoutMTL.h"
#include "dawn/native/metal/QuerySetMTL.h"
#include "dawn/native/metal/QueueMTL.h"
#include "dawn/native/metal/RenderPipelineMTL.h"
#include "dawn/native/metal/SamplerMTL.h"
#include "dawn/native/metal/ShaderModuleMTL.h"
#include "dawn/native/metal/SharedFenceMTL.h"
#include "dawn/native/metal/SharedTextureMemoryMTL.h"
#include "dawn/native/metal/SwapChainMTL.h"
#include "dawn/native/metal/TextureMTL.h"
#include "dawn/native/metal/UtilsMetal.h"
#include "dawn/platform/DawnPlatform.h"
#include "dawn/platform/tracing/TraceEvent.h"

#include <type_traits>

namespace dawn::native::metal {

struct KalmanInfo {
    float filterValue;  // The estimation value
    float kalmanGain;   // The kalman gain
    float R;            // The covariance of the observation noise
    float P;            // The a posteriori estimate covariance
};

namespace {

// The time interval for each round of kalman filter
static constexpr uint64_t kFilterIntervalInMs = static_cast<uint64_t>(NSEC_PER_SEC / 10);

// A simplified kalman filter for estimating timestamp period based on measured values
float KalmanFilter(KalmanInfo* info, float measuredValue) {
    // Optimize kalman gain
    info->kalmanGain = info->P / (info->P + info->R);

    // Correct filter value
    info->filterValue =
        info->kalmanGain * measuredValue + (1.0 - info->kalmanGain) * info->filterValue;
    // Update estimate covariance
    info->P = (1.0f - info->kalmanGain) * info->P;
    return info->filterValue;
}

void API_AVAILABLE(macos(10.15), ios(14)) UpdateTimestampPeriod(id<MTLDevice> device,
                                                                KalmanInfo* info,
                                                                MTLTimestamp* cpuTimestampStart,
                                                                MTLTimestamp* gpuTimestampStart,
                                                                float* timestampPeriod) {
    // The filter value is converged to an optimal value when the kalman gain is less than
    // 0.01. At this time, the weight of the measured value is too small to change the next
    // filter value, the sampling and calculations do not need to continue anymore.
    if (info->kalmanGain < 0.01f) {
        return;
    }

    MTLTimestamp cpuTimestampEnd = 0, gpuTimestampEnd = 0;
    [device sampleTimestamps:&cpuTimestampEnd gpuTimestamp:&gpuTimestampEnd];

    // Update the timestamp start values when timestamp reset happens
    if (cpuTimestampEnd < *cpuTimestampStart || gpuTimestampEnd < *gpuTimestampStart) {
        *cpuTimestampStart = cpuTimestampEnd;
        *gpuTimestampStart = gpuTimestampEnd;
        return;
    }

    if (cpuTimestampEnd - *cpuTimestampStart >= kFilterIntervalInMs) {
        // The measured timestamp period
        float measurement = (cpuTimestampEnd - *cpuTimestampStart) /
                            static_cast<float>(gpuTimestampEnd - *gpuTimestampStart);

        // Measurement update
        *timestampPeriod = KalmanFilter(info, measurement);

        *cpuTimestampStart = cpuTimestampEnd;
        *gpuTimestampStart = gpuTimestampEnd;
    }
}

}  // namespace

// static
ResultOrError<Ref<Device>> Device::Create(AdapterBase* adapter,
                                          NSPRef<id<MTLDevice>> mtlDevice,
                                          const DeviceDescriptor* descriptor,
                                          const TogglesState& deviceToggles) {
    @autoreleasepool {
        Ref<Device> device =
            AcquireRef(new Device(adapter, std::move(mtlDevice), descriptor, deviceToggles));
        DAWN_TRY(device->Initialize(descriptor));
        return device;
    }
}

Device::Device(AdapterBase* adapter,
               NSPRef<id<MTLDevice>> mtlDevice,
               const DeviceDescriptor* descriptor,
               const TogglesState& deviceToggles)
    : DeviceBase(adapter, descriptor, deviceToggles), mMtlDevice(std::move(mtlDevice)) {
    // On macOS < 11.0, we only can check whether counter sampling is supported, and the counter
    // only can be sampled between command boundary using sampleCountersInBuffer API if it's
    // supported.
    if (@available(macOS 11.0, iOS 14.0, *)) {
        mCounterSamplingAtCommandBoundary = SupportCounterSamplingAtCommandBoundary(GetMTLDevice());
        mCounterSamplingAtStageBoundary = SupportCounterSamplingAtStageBoundary(GetMTLDevice());
    } else {
        mCounterSamplingAtCommandBoundary = true;
        mCounterSamplingAtStageBoundary = false;
    }

    mIsTimestampQueryEnabled =
        HasFeature(Feature::TimestampQuery) || HasFeature(Feature::TimestampQueryInsidePasses);
}

Device::~Device() {
    Destroy();
}

MaybeError Device::Initialize(const DeviceDescriptor* descriptor) {
    Ref<Queue> queue;
    DAWN_TRY_ASSIGN(queue, Queue::Create(this, &descriptor->defaultQueue));

    if (mIsTimestampQueryEnabled && !IsToggleEnabled(Toggle::DisableTimestampQueryConversion)) {
        // Make a best guess of timestamp period based on device vendor info, and converge it to
        // an accurate value by the following calculations.
        mTimestampPeriod = gpu_info::IsIntel(GetPhysicalDevice()->GetVendorId()) ? 83.333f : 1.0f;

        // Initialize kalman filter parameters
        mKalmanInfo = std::make_unique<KalmanInfo>();
        mKalmanInfo->filterValue = 0.0f;
        mKalmanInfo->kalmanGain = 0.5f;
        mKalmanInfo->R = 0.0001f;  // The smaller this value is, the smaller the error of measured
                                   // value is, the more we can trust the measured value.
        mKalmanInfo->P = 1.0f;

        if (@available(macOS 10.15, iOS 14.0, *)) {
            // Sample CPU timestamp and GPU timestamp for first time at device creation
            [*mMtlDevice sampleTimestamps:&mCpuTimestamp gpuTimestamp:&mGpuTimestamp];
        }
    }

    return DeviceBase::Initialize(std::move(queue));
}

ResultOrError<Ref<BindGroupBase>> Device::CreateBindGroupImpl(
    const BindGroupDescriptor* descriptor) {
    return BindGroup::Create(this, descriptor);
}
ResultOrError<Ref<BindGroupLayoutInternalBase>> Device::CreateBindGroupLayoutImpl(
    const BindGroupLayoutDescriptor* descriptor) {
    return BindGroupLayout::Create(this, descriptor);
}
ResultOrError<Ref<BufferBase>> Device::CreateBufferImpl(const BufferDescriptor* descriptor) {
    return Buffer::Create(this, descriptor);
}
ResultOrError<Ref<CommandBufferBase>> Device::CreateCommandBuffer(
    CommandEncoder* encoder,
    const CommandBufferDescriptor* descriptor) {
    return CommandBuffer::Create(encoder, descriptor);
}
Ref<ComputePipelineBase> Device::CreateUninitializedComputePipelineImpl(
    const ComputePipelineDescriptor* descriptor) {
    return ComputePipeline::CreateUninitialized(this, descriptor);
}
ResultOrError<Ref<PipelineLayoutBase>> Device::CreatePipelineLayoutImpl(
    const PipelineLayoutDescriptor* descriptor) {
    return PipelineLayout::Create(this, descriptor);
}
ResultOrError<Ref<QuerySetBase>> Device::CreateQuerySetImpl(const QuerySetDescriptor* descriptor) {
    return QuerySet::Create(this, descriptor);
}
Ref<RenderPipelineBase> Device::CreateUninitializedRenderPipelineImpl(
    const RenderPipelineDescriptor* descriptor) {
    return RenderPipeline::CreateUninitialized(this, descriptor);
}
ResultOrError<Ref<SamplerBase>> Device::CreateSamplerImpl(const SamplerDescriptor* descriptor) {
    return Sampler::Create(this, descriptor);
}
ResultOrError<Ref<ShaderModuleBase>> Device::CreateShaderModuleImpl(
    const ShaderModuleDescriptor* descriptor,
    ShaderModuleParseResult* parseResult,
    OwnedCompilationMessages* compilationMessages) {
    return ShaderModule::Create(this, descriptor, parseResult, compilationMessages);
}
ResultOrError<Ref<SwapChainBase>> Device::CreateSwapChainImpl(
    Surface* surface,
    SwapChainBase* previousSwapChain,
    const SwapChainDescriptor* descriptor) {
    return SwapChain::Create(this, surface, previousSwapChain, descriptor);
}
ResultOrError<Ref<TextureBase>> Device::CreateTextureImpl(const TextureDescriptor* descriptor) {
    return Texture::Create(this, descriptor);
}
ResultOrError<Ref<TextureViewBase>> Device::CreateTextureViewImpl(
    TextureBase* texture,
    const TextureViewDescriptor* descriptor) {
    return TextureView::Create(texture, descriptor);
}
void Device::InitializeComputePipelineAsyncImpl(Ref<ComputePipelineBase> computePipeline,
                                                WGPUCreateComputePipelineAsyncCallback callback,
                                                void* userdata) {
    ComputePipeline::InitializeAsync(std::move(computePipeline), callback, userdata);
}
void Device::InitializeRenderPipelineAsyncImpl(Ref<RenderPipelineBase> renderPipeline,
                                               WGPUCreateRenderPipelineAsyncCallback callback,
                                               void* userdata) {
    RenderPipeline::InitializeAsync(std::move(renderPipeline), callback, userdata);
}

ResultOrError<wgpu::TextureUsage> Device::GetSupportedSurfaceUsageImpl(
    const Surface* surface) const {
    wgpu::TextureUsage usages = wgpu::TextureUsage::RenderAttachment |
                                wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc |
                                wgpu::TextureUsage::CopyDst;
    return usages;
}

ResultOrError<Ref<SharedTextureMemoryBase>> Device::ImportSharedTextureMemoryImpl(
    const SharedTextureMemoryDescriptor* baseDescriptor) {
    DAWN_TRY(ValidateSingleSType(baseDescriptor->nextInChain,
                                 wgpu::SType::SharedTextureMemoryIOSurfaceDescriptor));

    const SharedTextureMemoryIOSurfaceDescriptor* descriptor = nullptr;
    FindInChain(baseDescriptor->nextInChain, &descriptor);

    DAWN_INVALID_IF(descriptor == nullptr,
                    "SharedTextureMemoryIOSurfaceDescriptor must be chained.");

    DAWN_INVALID_IF(!HasFeature(Feature::SharedTextureMemoryIOSurface), "%s is not enabled.",
                    wgpu::FeatureName::SharedTextureMemoryIOSurface);

    return SharedTextureMemory::Create(this, baseDescriptor->label, descriptor);
}

ResultOrError<Ref<SharedFenceBase>> Device::ImportSharedFenceImpl(
    const SharedFenceDescriptor* baseDescriptor) {
    DAWN_TRY(ValidateSingleSType(baseDescriptor->nextInChain,
                                 wgpu::SType::SharedFenceMTLSharedEventDescriptor));

    const SharedFenceMTLSharedEventDescriptor* descriptor = nullptr;
    FindInChain(baseDescriptor->nextInChain, &descriptor);

    DAWN_INVALID_IF(descriptor == nullptr, "SharedFenceMTLSharedEventDescriptor must be chained.");

    DAWN_INVALID_IF(!HasFeature(Feature::SharedFenceMTLSharedEvent), "%s is not enabled.",
                    wgpu::FeatureName::SharedFenceMTLSharedEvent);
    if (@available(macOS 10.14, ios 12.0, *)) {
        return SharedFence::Create(this, baseDescriptor->label, descriptor);
    }
    UNREACHABLE();
}

MaybeError Device::TickImpl() {
    DAWN_TRY(ToBackend(GetQueue())->SubmitPendingCommandBuffer());

    // Just run timestamp period calculation when timestamp feature is enabled and timestamp
    // conversion is not disabled.
    if (mIsTimestampQueryEnabled && !IsToggleEnabled(Toggle::DisableTimestampQueryConversion)) {
        if (@available(macOS 10.15, iOS 14.0, *)) {
            UpdateTimestampPeriod(GetMTLDevice(), mKalmanInfo.get(), &mCpuTimestamp, &mGpuTimestamp,
                                  &mTimestampPeriod);
        }
    }

    return {};
}

id<MTLDevice> Device::GetMTLDevice() {
    return mMtlDevice.Get();
}

CommandRecordingContext* Device::GetPendingCommandContext(Device::SubmitMode submitMode) {
    return ToBackend(GetQueue())->GetPendingCommandContext(submitMode);
}

MaybeError Device::CopyFromStagingToBufferImpl(BufferBase* source,
                                               uint64_t sourceOffset,
                                               BufferBase* destination,
                                               uint64_t destinationOffset,
                                               uint64_t size) {
    // Metal validation layers forbid  0-sized copies, assert it is skipped prior to calling
    // this function.
    ASSERT(size != 0);

    ToBackend(destination)
        ->EnsureDataInitializedAsDestination(
            GetPendingCommandContext(DeviceBase::SubmitMode::Passive), destinationOffset, size);

    id<MTLBuffer> uploadBuffer = ToBackend(source)->GetMTLBuffer();
    Buffer* buffer = ToBackend(destination);
    buffer->TrackUsage();
    [GetPendingCommandContext(DeviceBase::SubmitMode::Passive)->EnsureBlit()
           copyFromBuffer:uploadBuffer
             sourceOffset:sourceOffset
                 toBuffer:buffer->GetMTLBuffer()
        destinationOffset:destinationOffset
                     size:size];
    return {};
}

// In Metal we don't write from the CPU to the texture directly which can be done using the
// replaceRegion function, because the function requires a non-private storage mode and Dawn
// sets the private storage mode by default for all textures except IOSurfaces on macOS.
MaybeError Device::CopyFromStagingToTextureImpl(const BufferBase* source,
                                                const TextureDataLayout& dataLayout,
                                                const TextureCopy& dst,
                                                const Extent3D& copySizePixels) {
    Texture* texture = ToBackend(dst.texture.Get());
    texture->SynchronizeTextureBeforeUse(GetPendingCommandContext());
    DAWN_TRY(EnsureDestinationTextureInitialized(
        GetPendingCommandContext(DeviceBase::SubmitMode::Passive), texture, dst, copySizePixels));

    RecordCopyBufferToTexture(GetPendingCommandContext(DeviceBase::SubmitMode::Passive),
                              ToBackend(source)->GetMTLBuffer(), source->GetSize(),
                              dataLayout.offset, dataLayout.bytesPerRow, dataLayout.rowsPerImage,
                              texture, dst.mipLevel, dst.origin, dst.aspect, copySizePixels);
    return {};
}

Ref<Texture> Device::CreateTextureWrappingIOSurface(
    const ExternalImageDescriptor* descriptor,
    IOSurfaceRef ioSurface,
    std::vector<MTLSharedEventAndSignalValue> waitEvents) {
    const TextureDescriptor* textureDescriptor = FromAPI(descriptor->cTextureDescriptor);
    if (ConsumedError(ValidateIsAlive())) {
        return nullptr;
    }
    if (ConsumedError(ValidateTextureDescriptor(this, textureDescriptor,
                                                AllowMultiPlanarTextureFormat::Yes))) {
        return nullptr;
    }
    if (ConsumedError(ValidateIOSurfaceCanBeWrapped(this, textureDescriptor, ioSurface))) {
        return nullptr;
    }
    if (GetValidInternalFormat(textureDescriptor->format).IsMultiPlanar() &&
        !descriptor->isInitialized) {
        bool consumed = ConsumedError(DAWN_VALIDATION_ERROR(
            "External textures with multiplanar formats must be initialized."));
        DAWN_UNUSED(consumed);
        return nullptr;
    }

    Ref<Texture> result;
    if (ConsumedError(
            Texture::CreateFromIOSurface(this, descriptor, ioSurface, std::move(waitEvents)),
            &result)) {
        return nullptr;
    }
    return result;
}

void Device::DestroyImpl() {
    ASSERT(GetState() == State::Disconnected);

    GetQueue()->Destroy();
    mMtlDevice = nullptr;
    mMockBlitMtlBuffer = nullptr;
}

uint32_t Device::GetOptimalBytesPerRowAlignment() const {
    return 1;
}

uint64_t Device::GetOptimalBufferToTextureCopyOffsetAlignment() const {
    return 1;
}

float Device::GetTimestampPeriodInNS() const {
    return mTimestampPeriod;
}

bool Device::IsResolveTextureBlitWithDrawSupported() const {
    return true;
}

bool Device::UseCounterSamplingAtCommandBoundary() const {
    return mCounterSamplingAtCommandBoundary;
}

bool Device::UseCounterSamplingAtStageBoundary() const {
    return mCounterSamplingAtStageBoundary;
}

id<MTLBuffer> Device::GetMockBlitMtlBuffer() {
    if (mMockBlitMtlBuffer == nullptr) {
        mMockBlitMtlBuffer.Acquire(
            [GetMTLDevice() newBufferWithLength:1 options:MTLResourceStorageModePrivate]);
    }

    return mMockBlitMtlBuffer.Get();
}

}  // namespace dawn::native::metal
