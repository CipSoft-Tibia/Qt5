// Copyright 2020 The Dawn Authors
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

#include "dawn/native/d3d12/SamplerHeapCacheD3D12.h"

#include <utility>

#include "dawn/common/Assert.h"
#include "dawn/common/HashUtils.h"
#include "dawn/native/d3d12/BindGroupD3D12.h"
#include "dawn/native/d3d12/BindGroupLayoutD3D12.h"
#include "dawn/native/d3d12/DeviceD3D12.h"
#include "dawn/native/d3d12/Forward.h"
#include "dawn/native/d3d12/SamplerD3D12.h"
#include "dawn/native/d3d12/ShaderVisibleDescriptorAllocatorD3D12.h"
#include "dawn/native/d3d12/StagingDescriptorAllocatorD3D12.h"

namespace dawn::native::d3d12 {

SamplerHeapCacheEntry::SamplerHeapCacheEntry(MutexProtected<StagingDescriptorAllocator>& allocator,
                                             std::vector<Sampler*> samplers)
    : mSamplers(std::move(samplers)), mAllocator(allocator) {}

SamplerHeapCacheEntry::SamplerHeapCacheEntry(SamplerHeapCache* cache,
                                             MutexProtected<StagingDescriptorAllocator>& allocator,
                                             std::vector<Sampler*> samplers,
                                             CPUDescriptorHeapAllocation allocation)
    : mCPUAllocation(std::move(allocation)),
      mSamplers(std::move(samplers)),
      mAllocator(allocator),
      mCache(cache) {
    ASSERT(mCache != nullptr);
    ASSERT(mCPUAllocation.IsValid());
    ASSERT(!mSamplers.empty());
}

std::vector<Sampler*>&& SamplerHeapCacheEntry::AcquireSamplers() {
    return std::move(mSamplers);
}

SamplerHeapCacheEntry::~SamplerHeapCacheEntry() {
    // If this is a blueprint then the CPU allocation cannot exist and has no entry to remove.
    if (mCPUAllocation.IsValid()) {
        mCache->RemoveCacheEntry(this);
        mAllocator->Deallocate(&mCPUAllocation);
    }

    ASSERT(!mCPUAllocation.IsValid());
}

bool SamplerHeapCacheEntry::Populate(Device* device,
                                     MutexProtected<ShaderVisibleDescriptorAllocator>& allocator) {
    if (allocator->IsAllocationStillValid(mGPUAllocation)) {
        return true;
    }

    ASSERT(!mSamplers.empty());

    // Attempt to allocate descriptors for the currently bound shader-visible heaps.
    // If either failed, return early to re-allocate and switch the heaps.
    const uint32_t descriptorCount = mSamplers.size();
    D3D12_CPU_DESCRIPTOR_HANDLE baseCPUDescriptor;
    if (!allocator->AllocateGPUDescriptors(descriptorCount, device->GetPendingCommandSerial(),
                                           &baseCPUDescriptor, &mGPUAllocation)) {
        return false;
    }

    // CPU bindgroups are sparsely allocated across CPU heaps. Instead of doing
    // simple copies per bindgroup, a single non-simple copy could be issued.
    // TODO(dawn:155): Consider doing this optimization.
    device->GetD3D12Device()->CopyDescriptorsSimple(descriptorCount, baseCPUDescriptor,
                                                    mCPUAllocation.GetBaseDescriptor(),
                                                    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE SamplerHeapCacheEntry::GetBaseDescriptor() const {
    return mGPUAllocation.GetBaseDescriptor();
}

ResultOrError<Ref<SamplerHeapCacheEntry>> SamplerHeapCache::GetOrCreate(
    const BindGroup* group,
    MutexProtected<StagingDescriptorAllocator>& samplerAllocator) {
    const BindGroupLayout* bgl = ToBackend(group->GetLayout());

    // If a previously created bindgroup used the same samplers, the backing sampler heap
    // allocation can be reused. The packed list of samplers acts as the key to lookup the
    // allocation in a cache.
    // TODO(dawn:155): Avoid re-allocating the vector each lookup.
    std::vector<Sampler*> samplers;
    samplers.reserve(bgl->GetSamplerDescriptorCount());

    for (BindingIndex bindingIndex = bgl->GetDynamicBufferCount();
         bindingIndex < bgl->GetBindingCount(); ++bindingIndex) {
        const BindingInfo& bindingInfo = bgl->GetBindingInfo(bindingIndex);
        if (bindingInfo.bindingType == BindingInfoType::Sampler) {
            samplers.push_back(ToBackend(group->GetBindingAsSampler(bindingIndex)));
        }
    }

    // Check the cache if there exists a sampler heap allocation that corresponds to the
    // samplers.
    SamplerHeapCacheEntry blueprint(samplerAllocator, std::move(samplers));
    auto iter = mCache.find(&blueprint);
    if (iter != mCache.end()) {
        return Ref<SamplerHeapCacheEntry>(*iter);
    }

    // Steal the sampler vector back from the blueprint to avoid creating a new copy for the
    // real entry below.
    samplers = std::move(blueprint.AcquireSamplers());

    uint32_t samplerSizeIncrement = 0;
    CPUDescriptorHeapAllocation allocation;
    DAWN_TRY(samplerAllocator.Use([&](auto allocator) -> MaybeError {
        DAWN_TRY_ASSIGN(allocation, allocator->AllocateCPUDescriptors());
        samplerSizeIncrement = allocator->GetSizeIncrement();
        return {};
    }));

    ID3D12Device* d3d12Device = mDevice->GetD3D12Device();

    for (uint32_t i = 0; i < samplers.size(); ++i) {
        const auto& samplerDesc = samplers[i]->GetSamplerDescriptor();
        d3d12Device->CreateSampler(&samplerDesc, allocation.OffsetFrom(samplerSizeIncrement, i));
    }

    Ref<SamplerHeapCacheEntry> entry = AcquireRef(new SamplerHeapCacheEntry(
        this, samplerAllocator, std::move(samplers), std::move(allocation)));
    mCache.insert(entry.Get());
    return std::move(entry);
}

SamplerHeapCache::SamplerHeapCache(Device* device) : mDevice(device) {}

SamplerHeapCache::~SamplerHeapCache() {
    ASSERT(mCache.empty());
}

void SamplerHeapCache::RemoveCacheEntry(SamplerHeapCacheEntry* entry) {
    ASSERT(entry->GetRefCountForTesting() == 0);
    size_t removedCount = mCache.erase(entry);
    ASSERT(removedCount == 1);
}

size_t SamplerHeapCacheEntry::HashFunc::operator()(const SamplerHeapCacheEntry* entry) const {
    size_t hash = 0;
    for (const Sampler* sampler : entry->mSamplers) {
        HashCombine(&hash, sampler);
    }
    return hash;
}

bool SamplerHeapCacheEntry::EqualityFunc::operator()(const SamplerHeapCacheEntry* a,
                                                     const SamplerHeapCacheEntry* b) const {
    return a->mSamplers == b->mSamplers;
}
}  // namespace dawn::native::d3d12
