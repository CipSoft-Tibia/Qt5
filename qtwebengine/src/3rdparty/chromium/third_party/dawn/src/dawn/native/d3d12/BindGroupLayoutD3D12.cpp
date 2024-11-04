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

#include "dawn/native/d3d12/BindGroupLayoutD3D12.h"

#include <utility>

#include "dawn/common/BitSetIterator.h"
#include "dawn/native/d3d12/DeviceD3D12.h"
#include "dawn/native/d3d12/SamplerHeapCacheD3D12.h"
#include "dawn/native/d3d12/StagingDescriptorAllocatorD3D12.h"

namespace dawn::native::d3d12 {
namespace {
D3D12_DESCRIPTOR_RANGE_TYPE WGPUBindingInfoToDescriptorRangeType(const BindingInfo& bindingInfo) {
    switch (bindingInfo.bindingType) {
        case BindingInfoType::Buffer:
            switch (bindingInfo.buffer.type) {
                case wgpu::BufferBindingType::Uniform:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                case wgpu::BufferBindingType::Storage:
                case kInternalStorageBufferBinding:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                case wgpu::BufferBindingType::ReadOnlyStorage:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                case wgpu::BufferBindingType::Undefined:
                    UNREACHABLE();
            }

        case BindingInfoType::Sampler:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

        case BindingInfoType::Texture:
        case BindingInfoType::ExternalTexture:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        case BindingInfoType::StorageTexture:
            switch (bindingInfo.storageTexture.access) {
                case wgpu::StorageTextureAccess::WriteOnly:
                case wgpu::StorageTextureAccess::ReadWrite:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                case wgpu::StorageTextureAccess::ReadOnly:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                case wgpu::StorageTextureAccess::Undefined:
                    UNREACHABLE();
            }
    }
}
}  // anonymous namespace

// static
Ref<BindGroupLayout> BindGroupLayout::Create(Device* device,
                                             const BindGroupLayoutDescriptor* descriptor) {
    return AcquireRef(new BindGroupLayout(device, descriptor));
}

BindGroupLayout::BindGroupLayout(Device* device, const BindGroupLayoutDescriptor* descriptor)
    : BindGroupLayoutInternalBase(device, descriptor),
      mDescriptorHeapOffsets(GetBindingCount()),
      mShaderRegisters(GetBindingCount()),
      mCbvUavSrvDescriptorCount(0),
      mSamplerDescriptorCount(0),
      mBindGroupAllocator(MakeFrontendBindGroupAllocator<BindGroup>(4096)) {
    for (BindingIndex bindingIndex{0}; bindingIndex < GetBindingCount(); ++bindingIndex) {
        const BindingInfo& bindingInfo = GetBindingInfo(bindingIndex);

        D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType =
            WGPUBindingInfoToDescriptorRangeType(bindingInfo);
        mShaderRegisters[bindingIndex] = uint32_t(bindingInfo.binding);

        // For dynamic resources, Dawn uses root descriptor in D3D12 backend. So there is no
        // need to allocate the descriptor from descriptor heap or create descriptor ranges.
        if (bindingIndex < GetDynamicBufferCount()) {
            continue;
        }
        ASSERT(!bindingInfo.buffer.hasDynamicOffset);

        mDescriptorHeapOffsets[bindingIndex] =
            descriptorRangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
                ? mSamplerDescriptorCount++
                : mCbvUavSrvDescriptorCount++;

        D3D12_DESCRIPTOR_RANGE1 range;
        range.RangeType = descriptorRangeType;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = GetShaderRegister(bindingIndex);
        range.RegisterSpace = kRegisterSpacePlaceholder;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // In Dawn we always use the descriptors as static ones, which means the descriptors in a
        // descriptor heap pointed to by a root descriptor table have been initialized by the time
        // the descriptor table is set on a command list (during recording), and the descriptors
        // cannot be changed until the command list has finished executing for the last time, so we
        // don't need to set DESCRIPTORS_VOLATILE for any binding types.
        switch (bindingInfo.bindingType) {
            // Sampler descriptor ranges don't support DATA_* flags at all since samplers do not
            // point to data.
            case BindingInfoType::Sampler:
                range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
                break;

            // In Dawn it's allowed to do state transitions on the buffers or textures after binding
            // them on the current command list, which indicates a change to its data (or possibly
            // resource metadata), so we cannot bind them as DATA_STATIC.
            // We cannot bind them as DATA_STATIC_WHILE_SET_AT_EXECUTE either because it is required
            // to be rebound to the command list before the next (this) Draw/Dispatch call, while
            // currently we may not rebind these resources if the current bind group is not changed.
            case BindingInfoType::Buffer:
                range.Flags =
                    D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS |
                    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
                break;
            case BindingInfoType::Texture:
            case BindingInfoType::StorageTexture:
                range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
                break;

            // ExternalTexture bindings are decayed in the frontend and backends shouldn't need to
            // handle them.
            case BindingInfoType::ExternalTexture:
            default:
                UNREACHABLE();
                break;
        }
        std::vector<D3D12_DESCRIPTOR_RANGE1>& descriptorRanges =
            descriptorRangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER ? mSamplerDescriptorRanges
                                                                       : mCbvUavSrvDescriptorRanges;

        // Try to join this range with the previous one, if the current range is a continuation
        // of the previous. This is possible because the binding infos in the base type are
        // sorted.
        if (descriptorRanges.size() >= 2) {
            D3D12_DESCRIPTOR_RANGE1& previous = descriptorRanges.back();
            if (previous.RangeType == range.RangeType &&
                previous.BaseShaderRegister + previous.NumDescriptors == range.BaseShaderRegister) {
                previous.NumDescriptors += range.NumDescriptors;
                continue;
            }
        }

        descriptorRanges.push_back(range);
    }

    mViewAllocator = device->GetViewStagingDescriptorAllocator(GetCbvUavSrvDescriptorCount());
    mSamplerAllocator = device->GetSamplerStagingDescriptorAllocator(GetSamplerDescriptorCount());
}

ResultOrError<Ref<BindGroup>> BindGroupLayout::AllocateBindGroup(
    Device* device,
    const BindGroupDescriptor* descriptor) {
    uint32_t viewSizeIncrement = 0;
    CPUDescriptorHeapAllocation viewAllocation;
    if (GetCbvUavSrvDescriptorCount() > 0) {
        ASSERT(mViewAllocator != nullptr);
        DAWN_TRY((*mViewAllocator).Use([&](auto viewAllocator) -> MaybeError {
            DAWN_TRY_ASSIGN(viewAllocation, viewAllocator->AllocateCPUDescriptors());
            viewSizeIncrement = viewAllocator->GetSizeIncrement();
            return {};
        }));
    }

    Ref<BindGroup> bindGroup = AcquireRef<BindGroup>(
        mBindGroupAllocator->Allocate(device, descriptor, viewSizeIncrement, viewAllocation));

    if (GetSamplerDescriptorCount() > 0) {
        ASSERT(mSamplerAllocator != nullptr);
        Ref<SamplerHeapCacheEntry> samplerHeapCacheEntry;
        DAWN_TRY_ASSIGN(samplerHeapCacheEntry, device->GetSamplerHeapCache()->GetOrCreate(
                                                   bindGroup.Get(), *mSamplerAllocator));
        bindGroup->SetSamplerAllocationEntry(std::move(samplerHeapCacheEntry));
    }

    return bindGroup;
}

void BindGroupLayout::DeallocateBindGroup(BindGroup* bindGroup,
                                          CPUDescriptorHeapAllocation* viewAllocation) {
    if (viewAllocation->IsValid()) {
        (*mViewAllocator)->Deallocate(viewAllocation);
    }

    mBindGroupAllocator->Deallocate(bindGroup);
}

ityp::span<BindingIndex, const uint32_t> BindGroupLayout::GetDescriptorHeapOffsets() const {
    return {mDescriptorHeapOffsets.data(), mDescriptorHeapOffsets.size()};
}

uint32_t BindGroupLayout::GetShaderRegister(BindingIndex bindingIndex) const {
    return mShaderRegisters[bindingIndex];
}

uint32_t BindGroupLayout::GetCbvUavSrvDescriptorCount() const {
    return mCbvUavSrvDescriptorCount;
}

uint32_t BindGroupLayout::GetSamplerDescriptorCount() const {
    return mSamplerDescriptorCount;
}

const std::vector<D3D12_DESCRIPTOR_RANGE1>& BindGroupLayout::GetCbvUavSrvDescriptorRanges() const {
    return mCbvUavSrvDescriptorRanges;
}

const std::vector<D3D12_DESCRIPTOR_RANGE1>& BindGroupLayout::GetSamplerDescriptorRanges() const {
    return mSamplerDescriptorRanges;
}

}  // namespace dawn::native::d3d12
