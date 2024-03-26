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

#include "dawn/native/CommandBufferStateTracker.h"

#include <optional>
#include <type_traits>
#include <utility>

#include "dawn/common/Assert.h"
#include "dawn/common/BitSetIterator.h"
#include "dawn/common/StackContainer.h"
#include "dawn/native/BindGroup.h"
#include "dawn/native/ComputePassEncoder.h"
#include "dawn/native/ComputePipeline.h"
#include "dawn/native/Forward.h"
#include "dawn/native/ObjectType_autogen.h"
#include "dawn/native/PipelineLayout.h"
#include "dawn/native/RenderPipeline.h"

// TODO(dawn:563): None of the error messages in this file include the buffer objects they are
// validating against. It would be nice to improve that, but difficult to do without incurring
// additional tracking costs.

namespace dawn::native {

namespace {
// Returns nullopt if all buffers in unverifiedBufferSizes are at least large enough to satisfy the
// minimum listed in pipelineMinBufferSizes, or the index of the first detected failing buffer
// otherwise.
std::optional<uint32_t> FindFirstUndersizedBuffer(
    const ityp::span<uint32_t, uint64_t> unverifiedBufferSizes,
    const std::vector<uint64_t>& pipelineMinBufferSizes) {
    ASSERT(unverifiedBufferSizes.size() == pipelineMinBufferSizes.size());

    for (uint32_t i = 0; i < unverifiedBufferSizes.size(); ++i) {
        if (unverifiedBufferSizes[i] < pipelineMinBufferSizes[i]) {
            return i;
        }
    }

    return std::nullopt;
}

struct BufferBindingAliasingResult {
    struct Entry {
        BindGroupIndex bindGroupIndex;
        BindingIndex bindingIndex;

        // Adjusted offset with dynamic offset
        uint64_t offset;
        uint64_t size;
    };
    Entry e0;
    Entry e1;
};

// TODO(dawn:1642): Find storage texture binding aliasing as well.
template <typename Return>
Return FindStorageBufferBindingAliasing(
    const PipelineLayoutBase* pipelineLayout,
    const ityp::array<BindGroupIndex, BindGroupBase*, kMaxBindGroups>& bindGroups,
    const ityp::array<BindGroupIndex, std::vector<uint32_t>, kMaxBindGroups> dynamicOffsets) {
    // Reduce the bindings array first to only preserve storage buffer bindings that could
    // potentially have ranges overlap.
    // There can at most be 8 storage buffer bindings per shader stage.
    StackVector<BufferBinding, 8> bindingsToCheck;

    StackVector<std::pair<BindGroupIndex, BindingIndex>, 8> bindingIndices;

    for (BindGroupIndex groupIndex : IterateBitSet(pipelineLayout->GetBindGroupLayoutsMask())) {
        BindGroupLayoutBase* bgl = bindGroups[groupIndex]->GetLayout();

        for (BindingIndex bindingIndex{0}; bindingIndex < bgl->GetBufferCount(); ++bindingIndex) {
            const BindingInfo& bindingInfo = bgl->GetBindingInfo(bindingIndex);
            // Buffer bindings are sorted to have smallest of bindingIndex.
            ASSERT(bindingInfo.bindingType == BindingInfoType::Buffer);

            // BindGroup validation already guarantees the buffer usage includes
            // wgpu::BufferUsage::Storage
            if (bindingInfo.buffer.type != wgpu::BufferBindingType::Storage) {
                continue;
            }

            const BufferBinding bufferBinding =
                bindGroups[groupIndex]->GetBindingAsBufferBinding(bindingIndex);

            if (bufferBinding.size == 0) {
                continue;
            }

            uint64_t adjustedOffset = bufferBinding.offset;
            // Apply dynamic offset if any.
            if (bindingInfo.buffer.hasDynamicOffset) {
                // SetBindGroup validation already guarantees offsets and sizes don't overflow.
                adjustedOffset += dynamicOffsets[groupIndex][static_cast<uint32_t>(bindingIndex)];
            }

            bindingsToCheck->push_back(BufferBinding{
                bufferBinding.buffer,
                adjustedOffset,
                bufferBinding.size,
            });

            if constexpr (std::is_same_v<Return, std::optional<BufferBindingAliasingResult>>) {
                bindingIndices->emplace_back(groupIndex, bindingIndex);
            }
        }
    }

    // Iterate through each bindings to find if any writable storage bindings aliasing exists.
    // Given that maxStorageBuffersPerShaderStage is 8,
    // it doesn't seem too bad to do a nested loop check.
    // TODO(dawn:1642): Maybe do algorithm optimization from O(N^2) to O(N*logN).
    for (size_t i = 0; i < bindingsToCheck->size(); i++) {
        const auto& bufferBinding0 = bindingsToCheck[i];

        for (size_t j = i + 1; j < bindingsToCheck->size(); j++) {
            const auto& bufferBinding1 = bindingsToCheck[j];

            if (bufferBinding0.buffer != bufferBinding1.buffer) {
                continue;
            }

            if (bufferBinding0.offset <= bufferBinding1.offset + bufferBinding1.size - 1 &&
                bufferBinding1.offset <= bufferBinding0.offset + bufferBinding0.size - 1) {
                if constexpr (std::is_same_v<Return, bool>) {
                    return true;
                } else if constexpr (std::is_same_v<Return,
                                                    std::optional<BufferBindingAliasingResult>>) {
                    return BufferBindingAliasingResult{
                        {bindingIndices[i].first, bindingIndices[i].second, bufferBinding0.offset,
                         bufferBinding0.size},
                        {bindingIndices[j].first, bindingIndices[j].second, bufferBinding1.offset,
                         bufferBinding1.size},
                    };
                }
            }
        }
    }

    if constexpr (std::is_same_v<Return, bool>) {
        return false;
    } else if constexpr (std::is_same_v<Return, std::optional<BufferBindingAliasingResult>>) {
        return std::nullopt;
    }
    UNREACHABLE();
}

}  // namespace

enum ValidationAspect {
    VALIDATION_ASPECT_PIPELINE,
    VALIDATION_ASPECT_BIND_GROUPS,
    VALIDATION_ASPECT_VERTEX_BUFFERS,
    VALIDATION_ASPECT_INDEX_BUFFER,

    VALIDATION_ASPECT_COUNT
};
static_assert(VALIDATION_ASPECT_COUNT == CommandBufferStateTracker::kNumAspects);

static constexpr CommandBufferStateTracker::ValidationAspects kDispatchAspects =
    1 << VALIDATION_ASPECT_PIPELINE | 1 << VALIDATION_ASPECT_BIND_GROUPS;

static constexpr CommandBufferStateTracker::ValidationAspects kDrawAspects =
    1 << VALIDATION_ASPECT_PIPELINE | 1 << VALIDATION_ASPECT_BIND_GROUPS |
    1 << VALIDATION_ASPECT_VERTEX_BUFFERS;

static constexpr CommandBufferStateTracker::ValidationAspects kDrawIndexedAspects =
    1 << VALIDATION_ASPECT_PIPELINE | 1 << VALIDATION_ASPECT_BIND_GROUPS |
    1 << VALIDATION_ASPECT_VERTEX_BUFFERS | 1 << VALIDATION_ASPECT_INDEX_BUFFER;

static constexpr CommandBufferStateTracker::ValidationAspects kLazyAspects =
    1 << VALIDATION_ASPECT_BIND_GROUPS | 1 << VALIDATION_ASPECT_VERTEX_BUFFERS |
    1 << VALIDATION_ASPECT_INDEX_BUFFER;

CommandBufferStateTracker::CommandBufferStateTracker() = default;

CommandBufferStateTracker::CommandBufferStateTracker(const CommandBufferStateTracker&) = default;

CommandBufferStateTracker::CommandBufferStateTracker(CommandBufferStateTracker&&) = default;

CommandBufferStateTracker::~CommandBufferStateTracker() = default;

CommandBufferStateTracker& CommandBufferStateTracker::operator=(const CommandBufferStateTracker&) =
    default;

CommandBufferStateTracker& CommandBufferStateTracker::operator=(CommandBufferStateTracker&&) =
    default;

MaybeError CommandBufferStateTracker::ValidateCanDispatch() {
    return ValidateOperation(kDispatchAspects);
}

MaybeError CommandBufferStateTracker::ValidateCanDraw() {
    return ValidateOperation(kDrawAspects);
}

MaybeError CommandBufferStateTracker::ValidateCanDrawIndexed() {
    return ValidateOperation(kDrawIndexedAspects);
}

MaybeError CommandBufferStateTracker::ValidateBufferInRangeForVertexBuffer(uint32_t vertexCount,
                                                                           uint32_t firstVertex) {
    uint64_t strideCount = static_cast<uint64_t>(firstVertex) + vertexCount;

    if (strideCount == 0) {
        // All vertex step mode buffers are always in range if stride count is zero
        return {};
    }

    RenderPipelineBase* lastRenderPipeline = GetRenderPipeline();

    const ityp::bitset<VertexBufferSlot, kMaxVertexBuffers>& vertexBufferSlotsUsedAsVertexBuffer =
        lastRenderPipeline->GetVertexBufferSlotsUsedAsVertexBuffer();

    for (auto usedSlotVertex : IterateBitSet(vertexBufferSlotsUsedAsVertexBuffer)) {
        const VertexBufferInfo& vertexBuffer = lastRenderPipeline->GetVertexBuffer(usedSlotVertex);
        uint64_t arrayStride = vertexBuffer.arrayStride;
        uint64_t bufferSize = mVertexBufferSizes[usedSlotVertex];

        if (arrayStride == 0) {
            DAWN_INVALID_IF(vertexBuffer.usedBytesInStride > bufferSize,
                            "Bound vertex buffer size (%u) at slot %u with an arrayStride of 0 "
                            "is smaller than the required size for all attributes (%u)",
                            bufferSize, static_cast<uint8_t>(usedSlotVertex),
                            vertexBuffer.usedBytesInStride);
        } else {
            DAWN_ASSERT(strideCount != 0u);
            uint64_t requiredSize = (strideCount - 1u) * arrayStride + vertexBuffer.lastStride;
            // firstVertex and vertexCount are in uint32_t,
            // arrayStride must not be larger than kMaxVertexBufferArrayStride, which is
            // currently 2048, and vertexBuffer.lastStride = max(attribute.offset +
            // sizeof(attribute.format)) with attribute.offset being no larger than
            // kMaxVertexBufferArrayStride, so by doing checks in uint64_t we avoid
            // overflows.
            DAWN_INVALID_IF(
                requiredSize > bufferSize,
                "Vertex range (first: %u, count: %u) requires a larger buffer (%u) than "
                "the "
                "bound buffer size (%u) of the vertex buffer at slot %u with stride %u.",
                firstVertex, vertexCount, requiredSize, bufferSize,
                static_cast<uint8_t>(usedSlotVertex), arrayStride);
        }
    }

    return {};
}

MaybeError CommandBufferStateTracker::ValidateBufferInRangeForInstanceBuffer(
    uint32_t instanceCount,
    uint32_t firstInstance) {
    uint64_t strideCount = static_cast<uint64_t>(firstInstance) + instanceCount;

    if (strideCount == 0) {
        // All instance step mode buffers are always in range if stride count is zero
        return {};
    }

    RenderPipelineBase* lastRenderPipeline = GetRenderPipeline();

    const ityp::bitset<VertexBufferSlot, kMaxVertexBuffers>& vertexBufferSlotsUsedAsInstanceBuffer =
        lastRenderPipeline->GetVertexBufferSlotsUsedAsInstanceBuffer();

    for (auto usedSlotInstance : IterateBitSet(vertexBufferSlotsUsedAsInstanceBuffer)) {
        const VertexBufferInfo& vertexBuffer =
            lastRenderPipeline->GetVertexBuffer(usedSlotInstance);
        uint64_t arrayStride = vertexBuffer.arrayStride;
        uint64_t bufferSize = mVertexBufferSizes[usedSlotInstance];
        if (arrayStride == 0) {
            DAWN_INVALID_IF(vertexBuffer.usedBytesInStride > bufferSize,
                            "Bound vertex buffer size (%u) at slot %u with an arrayStride of 0 "
                            "is smaller than the required size for all attributes (%u)",
                            bufferSize, static_cast<uint8_t>(usedSlotInstance),
                            vertexBuffer.usedBytesInStride);
        } else {
            DAWN_ASSERT(strideCount != 0u);
            uint64_t requiredSize = (strideCount - 1u) * arrayStride + vertexBuffer.lastStride;
            // firstInstance and instanceCount are in uint32_t,
            // arrayStride must not be larger than kMaxVertexBufferArrayStride, which is
            // currently 2048, and vertexBuffer.lastStride = max(attribute.offset +
            // sizeof(attribute.format)) with attribute.offset being no larger than
            // kMaxVertexBufferArrayStride, so by doing checks in uint64_t we avoid
            // overflows.
            DAWN_INVALID_IF(
                requiredSize > bufferSize,
                "Instance range (first: %u, count: %u) requires a larger buffer (%u) than "
                "the "
                "bound buffer size (%u) of the vertex buffer at slot %u with stride %u.",
                firstInstance, instanceCount, requiredSize, bufferSize,
                static_cast<uint8_t>(usedSlotInstance), arrayStride);
        }
    }

    return {};
}

MaybeError CommandBufferStateTracker::ValidateIndexBufferInRange(uint32_t indexCount,
                                                                 uint32_t firstIndex) {
    // Validate the range of index buffer
    // firstIndex and indexCount are in uint32_t, while IndexFormatSize is 2 (for
    // wgpu::IndexFormat::Uint16) or 4 (for wgpu::IndexFormat::Uint32), so by doing checks in
    // uint64_t we avoid overflows.
    DAWN_INVALID_IF(
        (static_cast<uint64_t>(firstIndex) + indexCount) * IndexFormatSize(mIndexFormat) >
            mIndexBufferSize,
        "Index range (first: %u, count: %u, format: %s) does not fit in index buffer size "
        "(%u).",
        firstIndex, indexCount, mIndexFormat, mIndexBufferSize);
    return {};
}

MaybeError CommandBufferStateTracker::ValidateOperation(ValidationAspects requiredAspects) {
    // Fast return-true path if everything is good
    ValidationAspects missingAspects = requiredAspects & ~mAspects;
    if (missingAspects.none()) {
        return {};
    }

    // Generate an error immediately if a non-lazy aspect is missing as computing lazy aspects
    // requires the pipeline to be set.
    DAWN_TRY(CheckMissingAspects(missingAspects & ~kLazyAspects));

    RecomputeLazyAspects(missingAspects);

    DAWN_TRY(CheckMissingAspects(requiredAspects & ~mAspects));

    return {};
}

void CommandBufferStateTracker::RecomputeLazyAspects(ValidationAspects aspects) {
    ASSERT(mAspects[VALIDATION_ASPECT_PIPELINE]);
    ASSERT((aspects & ~kLazyAspects).none());

    if (aspects[VALIDATION_ASPECT_BIND_GROUPS]) {
        bool matches = true;

        for (BindGroupIndex i : IterateBitSet(mLastPipelineLayout->GetBindGroupLayoutsMask())) {
            if (mBindgroups[i] == nullptr ||
                mLastPipelineLayout->GetBindGroupLayout(i) != mBindgroups[i]->GetLayout() ||
                FindFirstUndersizedBuffer(mBindgroups[i]->GetUnverifiedBufferSizes(),
                                          (*mMinBufferSizes)[i])
                    .has_value()) {
                matches = false;
                break;
            }
        }

        if (matches) {
            // Continue checking if there is writable storage buffer binding aliasing or not
            if (FindStorageBufferBindingAliasing<bool>(mLastPipelineLayout, mBindgroups,
                                                       mDynamicOffsets)) {
                matches = false;
            }
        }

        if (matches) {
            mAspects.set(VALIDATION_ASPECT_BIND_GROUPS);
        }
    }

    if (aspects[VALIDATION_ASPECT_VERTEX_BUFFERS]) {
        RenderPipelineBase* lastRenderPipeline = GetRenderPipeline();

        const ityp::bitset<VertexBufferSlot, kMaxVertexBuffers>& requiredVertexBuffers =
            lastRenderPipeline->GetVertexBufferSlotsUsed();
        if (IsSubset(requiredVertexBuffers, mVertexBufferSlotsUsed)) {
            mAspects.set(VALIDATION_ASPECT_VERTEX_BUFFERS);
        }
    }

    if (aspects[VALIDATION_ASPECT_INDEX_BUFFER] && mIndexBufferSet) {
        RenderPipelineBase* lastRenderPipeline = GetRenderPipeline();
        if (!IsStripPrimitiveTopology(lastRenderPipeline->GetPrimitiveTopology()) ||
            mIndexFormat == lastRenderPipeline->GetStripIndexFormat()) {
            mAspects.set(VALIDATION_ASPECT_INDEX_BUFFER);
        }
    }
}

MaybeError CommandBufferStateTracker::CheckMissingAspects(ValidationAspects aspects) {
    if (!aspects.any()) {
        return {};
    }

    DAWN_INVALID_IF(aspects[VALIDATION_ASPECT_PIPELINE], "No pipeline set.");

    if (DAWN_UNLIKELY(aspects[VALIDATION_ASPECT_INDEX_BUFFER])) {
        DAWN_INVALID_IF(!mIndexBufferSet, "Index buffer was not set.");

        RenderPipelineBase* lastRenderPipeline = GetRenderPipeline();
        wgpu::IndexFormat pipelineIndexFormat = lastRenderPipeline->GetStripIndexFormat();

        if (IsStripPrimitiveTopology(lastRenderPipeline->GetPrimitiveTopology())) {
            DAWN_INVALID_IF(
                pipelineIndexFormat == wgpu::IndexFormat::Undefined,
                "%s has a strip primitive topology (%s) but a strip index format of %s, which "
                "prevents it for being used for indexed draw calls.",
                lastRenderPipeline, lastRenderPipeline->GetPrimitiveTopology(),
                pipelineIndexFormat);

            DAWN_INVALID_IF(
                mIndexFormat != pipelineIndexFormat,
                "Strip index format (%s) of %s does not match index buffer format (%s).",
                pipelineIndexFormat, lastRenderPipeline, mIndexFormat);
        }

        // The chunk of code above should be similar to the one in |RecomputeLazyAspects|.
        // It returns the first invalid state found. We shouldn't be able to reach this line
        // because to have invalid aspects one of the above conditions must have failed earlier.
        // If this is reached, make sure lazy aspects and the error checks above are consistent.
        UNREACHABLE();
        return DAWN_VALIDATION_ERROR("Index buffer is invalid.");
    }

    if (aspects[VALIDATION_ASPECT_VERTEX_BUFFERS]) {
        const ityp::bitset<VertexBufferSlot, kMaxVertexBuffers> missingVertexBuffers =
            GetRenderPipeline()->GetVertexBufferSlotsUsed() & ~mVertexBufferSlotsUsed;
        ASSERT(missingVertexBuffers.any());

        VertexBufferSlot firstMissing = ityp::Sub(GetHighestBitIndexPlusOne(missingVertexBuffers),
                                                  VertexBufferSlot(uint8_t(1)));
        return DAWN_VALIDATION_ERROR("Vertex buffer slot %u required by %s was not set.",
                                     uint8_t(firstMissing), GetRenderPipeline());
    }

    if (DAWN_UNLIKELY(aspects[VALIDATION_ASPECT_BIND_GROUPS])) {
        for (BindGroupIndex i : IterateBitSet(mLastPipelineLayout->GetBindGroupLayoutsMask())) {
            ASSERT(HasPipeline());

            DAWN_INVALID_IF(mBindgroups[i] == nullptr, "No bind group set at group index %u.",
                            static_cast<uint32_t>(i));

            BindGroupLayoutBase* requiredBGL = mLastPipelineLayout->GetBindGroupLayout(i);
            BindGroupLayoutBase* currentBGL = mBindgroups[i]->GetLayout();

            DAWN_INVALID_IF(
                requiredBGL->GetPipelineCompatibilityToken() != PipelineCompatibilityToken(0) &&
                    currentBGL->GetPipelineCompatibilityToken() !=
                        requiredBGL->GetPipelineCompatibilityToken(),
                "The current pipeline (%s) was created with a default layout, and is not "
                "compatible with the %s set at group index %u which uses a %s that was not "
                "created by the pipeline. Either use the bind group layout returned by calling "
                "getBindGroupLayout(%u) on the pipeline when creating the bind group, or "
                "provide an explicit pipeline layout when creating the pipeline.",
                mLastPipeline, mBindgroups[i], static_cast<uint32_t>(i), currentBGL,
                static_cast<uint32_t>(i));

            DAWN_INVALID_IF(
                requiredBGL->GetPipelineCompatibilityToken() == PipelineCompatibilityToken(0) &&
                    currentBGL->GetPipelineCompatibilityToken() != PipelineCompatibilityToken(0),
                "%s set at group index %u uses a %s which was created as part of the default "
                "layout "
                "for a different pipeline than the current one (%s), and as a result is not "
                "compatible. Use an explicit bind group layout when creating bind groups and "
                "an explicit pipeline layout when creating pipelines to share bind groups "
                "between pipelines.",
                mBindgroups[i], static_cast<uint32_t>(i), currentBGL, mLastPipeline);

            DAWN_INVALID_IF(
                mLastPipelineLayout->GetBindGroupLayout(i) != mBindgroups[i]->GetLayout(),
                "Bind group layout %s of pipeline layout %s does not match layout %s of bind "
                "group %s set at group index %u.",
                requiredBGL, mLastPipelineLayout, currentBGL, mBindgroups[i],
                static_cast<uint32_t>(i));

            // TODO(dawn:563): Report which buffer bindings are failing. This requires the ability
            // to look up the binding index from the packed index.
            std::optional<uint32_t> packedIndex = FindFirstUndersizedBuffer(
                mBindgroups[i]->GetUnverifiedBufferSizes(), (*mMinBufferSizes)[i]);
            if (packedIndex.has_value()) {
                uint64_t bufferSize =
                    mBindgroups[i]->GetUnverifiedBufferSizes()[packedIndex.value()];
                uint64_t minBufferSize = (*mMinBufferSizes)[i][packedIndex.value()];
                return DAWN_VALIDATION_ERROR(
                    "Binding sizes are too small for %s set at group index %u. A bound buffer "
                    "contained %u bytes, but the current pipeline (%s) requires a buffer which is "
                    "at least %u bytes. (Note that uniform buffer bindings must be a multiple of "
                    "16 bytes, and as a result may be larger than the associated data in the "
                    "shader source.)",
                    mBindgroups[i], static_cast<uint32_t>(i), bufferSize, mLastPipeline,
                    minBufferSize);
            }
        }

        auto result = FindStorageBufferBindingAliasing<std::optional<BufferBindingAliasingResult>>(
            mLastPipelineLayout, mBindgroups, mDynamicOffsets);

        if (result) {
            return DAWN_VALIDATION_ERROR(
                "Writable storage buffer binding found between bind group index %u, binding index "
                "%u, and bind group index %u, binding index %u, with overlapping ranges (offset: "
                "%u, size: %u) and (offset: %u, size: %u).",
                static_cast<uint32_t>(result->e0.bindGroupIndex),
                static_cast<uint32_t>(result->e0.bindingIndex),
                static_cast<uint32_t>(result->e1.bindGroupIndex),
                static_cast<uint32_t>(result->e1.bindingIndex), result->e0.offset, result->e0.size,
                result->e1.offset, result->e1.size);
        }

        // The chunk of code above should be similar to the one in |RecomputeLazyAspects|.
        // It returns the first invalid state found. We shouldn't be able to reach this line
        // because to have invalid aspects one of the above conditions must have failed earlier.
        // If this is reached, make sure lazy aspects and the error checks above are consistent.
        UNREACHABLE();
        return DAWN_VALIDATION_ERROR("Bind groups are invalid.");
    }

    UNREACHABLE();
}

void CommandBufferStateTracker::SetComputePipeline(ComputePipelineBase* pipeline) {
    SetPipelineCommon(pipeline);
}

void CommandBufferStateTracker::SetRenderPipeline(RenderPipelineBase* pipeline) {
    SetPipelineCommon(pipeline);
}

void CommandBufferStateTracker::SetBindGroup(BindGroupIndex index,
                                             BindGroupBase* bindgroup,
                                             uint32_t dynamicOffsetCount,
                                             const uint32_t* dynamicOffsets) {
    mBindgroups[index] = bindgroup;
    mDynamicOffsets[index].assign(dynamicOffsets, dynamicOffsets + dynamicOffsetCount);
    mAspects.reset(VALIDATION_ASPECT_BIND_GROUPS);
}

void CommandBufferStateTracker::SetIndexBuffer(wgpu::IndexFormat format, uint64_t size) {
    mIndexBufferSet = true;
    mIndexFormat = format;
    mIndexBufferSize = size;
}

void CommandBufferStateTracker::SetVertexBuffer(VertexBufferSlot slot, uint64_t size) {
    mVertexBufferSlotsUsed.set(slot);
    mVertexBufferSizes[slot] = size;
}

void CommandBufferStateTracker::SetPipelineCommon(PipelineBase* pipeline) {
    mLastPipeline = pipeline;
    mLastPipelineLayout = pipeline != nullptr ? pipeline->GetLayout() : nullptr;
    mMinBufferSizes = pipeline != nullptr ? &pipeline->GetMinBufferSizes() : nullptr;

    mAspects.set(VALIDATION_ASPECT_PIPELINE);

    // Reset lazy aspects so they get recomputed on the next operation.
    mAspects &= ~kLazyAspects;
}

BindGroupBase* CommandBufferStateTracker::GetBindGroup(BindGroupIndex index) const {
    return mBindgroups[index];
}

const std::vector<uint32_t>& CommandBufferStateTracker::GetDynamicOffsets(
    BindGroupIndex index) const {
    return mDynamicOffsets[index];
}

bool CommandBufferStateTracker::HasPipeline() const {
    return mLastPipeline != nullptr;
}

RenderPipelineBase* CommandBufferStateTracker::GetRenderPipeline() const {
    ASSERT(HasPipeline() && mLastPipeline->GetType() == ObjectType::RenderPipeline);
    return static_cast<RenderPipelineBase*>(mLastPipeline);
}

ComputePipelineBase* CommandBufferStateTracker::GetComputePipeline() const {
    ASSERT(HasPipeline() && mLastPipeline->GetType() == ObjectType::ComputePipeline);
    return static_cast<ComputePipelineBase*>(mLastPipeline);
}

PipelineLayoutBase* CommandBufferStateTracker::GetPipelineLayout() const {
    return mLastPipelineLayout;
}

wgpu::IndexFormat CommandBufferStateTracker::GetIndexFormat() const {
    return mIndexFormat;
}

uint64_t CommandBufferStateTracker::GetIndexBufferSize() const {
    return mIndexBufferSize;
}

}  // namespace dawn::native
