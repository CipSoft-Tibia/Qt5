// Copyright 2017 The Dawn Authors
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

#ifndef SRC_DAWN_NATIVE_NULL_DEVICENULL_H_
#define SRC_DAWN_NATIVE_NULL_DEVICENULL_H_

#include <memory>
#include <vector>

#include "dawn/native/BindGroup.h"
#include "dawn/native/BindGroupLayoutInternal.h"
#include "dawn/native/Buffer.h"
#include "dawn/native/CommandBuffer.h"
#include "dawn/native/CommandEncoder.h"
#include "dawn/native/ComputePipeline.h"
#include "dawn/native/Device.h"
#include "dawn/native/PhysicalDevice.h"
#include "dawn/native/PipelineLayout.h"
#include "dawn/native/QuerySet.h"
#include "dawn/native/Queue.h"
#include "dawn/native/RenderPipeline.h"
#include "dawn/native/RingBufferAllocator.h"
#include "dawn/native/Sampler.h"
#include "dawn/native/ShaderModule.h"
#include "dawn/native/SwapChain.h"
#include "dawn/native/Texture.h"
#include "dawn/native/ToBackend.h"
#include "dawn/native/dawn_platform.h"

namespace dawn::native::null {

class BindGroup;
class BindGroupLayout;
class Buffer;
class CommandBuffer;
class ComputePipeline;
class Device;
class PhysicalDevice;
using PipelineLayout = PipelineLayoutBase;
class QuerySet;
class Queue;
class RenderPipeline;
using Sampler = SamplerBase;
class ShaderModule;
class SwapChain;
class Texture;
using TextureView = TextureViewBase;

struct NullBackendTraits {
    using BindGroupType = BindGroup;
    using BindGroupLayoutType = BindGroupLayout;
    using BufferType = Buffer;
    using CommandBufferType = CommandBuffer;
    using ComputePipelineType = ComputePipeline;
    using DeviceType = Device;
    using PhysicalDeviceType = PhysicalDevice;
    using PipelineLayoutType = PipelineLayout;
    using QuerySetType = QuerySet;
    using QueueType = Queue;
    using RenderPipelineType = RenderPipeline;
    using SamplerType = Sampler;
    using ShaderModuleType = ShaderModule;
    using SwapChainType = SwapChain;
    using TextureType = Texture;
    using TextureViewType = TextureView;
};

template <typename T>
auto ToBackend(T&& common) -> decltype(ToBackendBase<NullBackendTraits>(common)) {
    return ToBackendBase<NullBackendTraits>(common);
}

struct PendingOperation {
    virtual ~PendingOperation() = default;
    virtual void Execute() = 0;
};

class Device final : public DeviceBase {
  public:
    static ResultOrError<Ref<Device>> Create(AdapterBase* adapter,
                                             const DeviceDescriptor* descriptor,
                                             const TogglesState& deviceToggles);
    ~Device() override;

    MaybeError Initialize(const DeviceDescriptor* descriptor);

    ResultOrError<Ref<CommandBufferBase>> CreateCommandBuffer(
        CommandEncoder* encoder,
        const CommandBufferDescriptor* descriptor) override;

    MaybeError TickImpl() override;

    void AddPendingOperation(std::unique_ptr<PendingOperation> operation);
    MaybeError SubmitPendingOperations();
    void ForgetPendingOperations();

    MaybeError CopyFromStagingToBufferImpl(BufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;
    MaybeError CopyFromStagingToTextureImpl(const BufferBase* source,
                                            const TextureDataLayout& src,
                                            const TextureCopy& dst,
                                            const Extent3D& copySizePixels) override;

    MaybeError IncrementMemoryUsage(uint64_t bytes);
    void DecrementMemoryUsage(uint64_t bytes);

    uint32_t GetOptimalBytesPerRowAlignment() const override;
    uint64_t GetOptimalBufferToTextureCopyOffsetAlignment() const override;

    float GetTimestampPeriodInNS() const override;

    bool IsResolveTextureBlitWithDrawSupported() const override;

  private:
    using DeviceBase::DeviceBase;

    ResultOrError<Ref<BindGroupBase>> CreateBindGroupImpl(
        const BindGroupDescriptor* descriptor) override;
    ResultOrError<Ref<BindGroupLayoutInternalBase>> CreateBindGroupLayoutImpl(
        const BindGroupLayoutDescriptor* descriptor) override;
    ResultOrError<Ref<BufferBase>> CreateBufferImpl(const BufferDescriptor* descriptor) override;
    Ref<ComputePipelineBase> CreateUninitializedComputePipelineImpl(
        const ComputePipelineDescriptor* descriptor) override;
    ResultOrError<Ref<PipelineLayoutBase>> CreatePipelineLayoutImpl(
        const PipelineLayoutDescriptor* descriptor) override;
    ResultOrError<Ref<QuerySetBase>> CreateQuerySetImpl(
        const QuerySetDescriptor* descriptor) override;
    Ref<RenderPipelineBase> CreateUninitializedRenderPipelineImpl(
        const RenderPipelineDescriptor* descriptor) override;
    ResultOrError<Ref<SamplerBase>> CreateSamplerImpl(const SamplerDescriptor* descriptor) override;
    ResultOrError<Ref<ShaderModuleBase>> CreateShaderModuleImpl(
        const ShaderModuleDescriptor* descriptor,
        ShaderModuleParseResult* parseResult,
        OwnedCompilationMessages* compilationMessages) override;
    ResultOrError<Ref<SwapChainBase>> CreateSwapChainImpl(
        Surface* surface,
        SwapChainBase* previousSwapChain,
        const SwapChainDescriptor* descriptor) override;
    ResultOrError<Ref<TextureBase>> CreateTextureImpl(const TextureDescriptor* descriptor) override;
    ResultOrError<Ref<TextureViewBase>> CreateTextureViewImpl(
        TextureBase* texture,
        const TextureViewDescriptor* descriptor) override;

    ResultOrError<wgpu::TextureUsage> GetSupportedSurfaceUsageImpl(
        const Surface* surface) const override;

    void DestroyImpl() override;

    std::vector<std::unique_ptr<PendingOperation>> mPendingOperations;

    static constexpr uint64_t kMaxMemoryUsage = 512 * 1024 * 1024;
    size_t mMemoryUsage = 0;
};

class PhysicalDevice : public PhysicalDeviceBase {
  public:
    // Create null adapter without providing toggles state for testing, only inherit instance's
    // toggles state
    explicit PhysicalDevice(InstanceBase* instance);
    ~PhysicalDevice() override;

    // PhysicalDeviceBase Implementation
    bool SupportsExternalImages() const override;

    bool SupportsFeatureLevel(FeatureLevel featureLevel) const override;

    // Used for the tests that intend to use an adapter without all features enabled.
    using PhysicalDeviceBase::SetSupportedFeaturesForTesting;

  private:
    MaybeError InitializeImpl() override;
    void InitializeSupportedFeaturesImpl() override;
    MaybeError InitializeSupportedLimitsImpl(CombinedLimits* limits) override;

    MaybeError ValidateFeatureSupportedWithTogglesImpl(wgpu::FeatureName feature,
                                                       const TogglesState& toggles) const override;

    void SetupBackendAdapterToggles(TogglesState* adapterToggles) const override;
    void SetupBackendDeviceToggles(TogglesState* deviceToggles) const override;
    ResultOrError<Ref<DeviceBase>> CreateDeviceImpl(AdapterBase* adapter,
                                                    const DeviceDescriptor* descriptor,
                                                    const TogglesState& deviceToggles) override;
};

// Helper class so |BindGroup| can allocate memory for its binding data,
// before calling the BindGroupBase base class constructor.
class BindGroupDataHolder {
  protected:
    explicit BindGroupDataHolder(size_t size);
    ~BindGroupDataHolder();

    void* mBindingDataAllocation;
};

// We don't have the complexity of placement-allocation of bind group data in
// the Null backend. This class, keeps the binding data in a separate allocation for simplicity.
class BindGroup final : private BindGroupDataHolder, public BindGroupBase {
  public:
    BindGroup(DeviceBase* device, const BindGroupDescriptor* descriptor);

  private:
    ~BindGroup() override = default;
};

class BindGroupLayout final : public BindGroupLayoutInternalBase {
  public:
    BindGroupLayout(DeviceBase* device, const BindGroupLayoutDescriptor* descriptor);

  private:
    ~BindGroupLayout() override = default;
};

class Buffer final : public BufferBase {
  public:
    Buffer(Device* device, const BufferDescriptor* descriptor);

    void CopyFromStaging(BufferBase* staging,
                         uint64_t sourceOffset,
                         uint64_t destinationOffset,
                         uint64_t size);

    void DoWriteBuffer(uint64_t bufferOffset, const void* data, size_t size);

  private:
    MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) override;
    void UnmapImpl() override;
    void DestroyImpl() override;
    bool IsCPUWritableAtCreation() const override;
    MaybeError MapAtCreationImpl() override;
    void* GetMappedPointer() override;

    std::unique_ptr<uint8_t[]> mBackingData;
};

class CommandBuffer final : public CommandBufferBase {
  public:
    CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor);
};

class QuerySet final : public QuerySetBase {
  public:
    QuerySet(Device* device, const QuerySetDescriptor* descriptor);
};

class Queue final : public QueueBase {
  public:
    Queue(Device* device, const QueueDescriptor* descriptor);

  private:
    ~Queue() override;
    MaybeError SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) override;
    MaybeError WriteBufferImpl(BufferBase* buffer,
                               uint64_t bufferOffset,
                               const void* data,
                               size_t size) override;
    ResultOrError<ExecutionSerial> CheckAndUpdateCompletedSerials() override;
    void ForceEventualFlushOfCommands() override;
    bool HasPendingCommands() const override;
    MaybeError WaitForIdleForDestruction() override;
};

class ComputePipeline final : public ComputePipelineBase {
  public:
    using ComputePipelineBase::ComputePipelineBase;

    MaybeError Initialize() override;
};

class RenderPipeline final : public RenderPipelineBase {
  public:
    using RenderPipelineBase::RenderPipelineBase;

    MaybeError Initialize() override;
};

class ShaderModule final : public ShaderModuleBase {
  public:
    using ShaderModuleBase::ShaderModuleBase;

    MaybeError Initialize(ShaderModuleParseResult* parseResult,
                          OwnedCompilationMessages* compilationMessages);
};

class SwapChain final : public SwapChainBase {
  public:
    static ResultOrError<Ref<SwapChain>> Create(Device* device,
                                                Surface* surface,
                                                SwapChainBase* previousSwapChain,
                                                const SwapChainDescriptor* descriptor);
    ~SwapChain() override;

  private:
    using SwapChainBase::SwapChainBase;
    MaybeError Initialize(SwapChainBase* previousSwapChain);

    Ref<Texture> mTexture;

    MaybeError PresentImpl() override;
    ResultOrError<Ref<TextureBase>> GetCurrentTextureImpl() override;
    void DetachFromSurfaceImpl() override;
};

class Texture : public TextureBase {
  public:
    Texture(DeviceBase* device, const TextureDescriptor* descriptor);
};

}  // namespace dawn::native::null

#endif  // SRC_DAWN_NATIVE_NULL_DEVICENULL_H_
