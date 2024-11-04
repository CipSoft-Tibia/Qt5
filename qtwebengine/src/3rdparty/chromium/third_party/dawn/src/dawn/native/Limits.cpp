// Copyright 2021 The Dawn Authors
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

#include "dawn/native/Limits.h"

#include <algorithm>
#include <array>

#include "dawn/common/Assert.h"
#include "dawn/common/Constants.h"
#include "dawn/common/Math.h"

// clang-format off
// TODO(crbug.com/dawn/685):
// For now, only expose these tiers until metrics can determine better ones.
#define LIMITS_WORKGROUP_STORAGE_SIZE(X)                                  \
    X(Maximum, maxComputeWorkgroupStorageSize, 16384, 32768, 49152, 65536)

// Tiers for limits related to workgroup size.
// TODO(crbug.com/dawn/685): Define these better. For now, use two tiers where one
// is available on nearly all desktop platforms.
#define LIMITS_WORKGROUP_SIZE(X)                                                   \
    X(Maximum,           maxComputeInvocationsPerWorkgroup,       256,       1024) \
    X(Maximum,                    maxComputeWorkgroupSizeX,       256,       1024) \
    X(Maximum,                    maxComputeWorkgroupSizeY,       256,       1024) \
    X(Maximum,                    maxComputeWorkgroupSizeZ,        64,         64) \
    X(Maximum,            maxComputeWorkgroupsPerDimension,     65535,      65535)

// Tiers are 128MB, 1GB, 2GB-4, 4GB-4.
#define LIMITS_STORAGE_BUFFER_BINDING_SIZE(X)                                             \
    X(Maximum, maxStorageBufferBindingSize, 134217728, 1073741824, 2147483644, 4294967292)

// Tiers are 256MB, 1GB, 2GB, 4GB.
#define LIMITS_MAX_BUFFER_SIZE(X)                                             \
    X(Maximum, maxBufferSize, 0x10000000, 0x40000000, 0x80000000, 0x100000000)

// Tiers for limits related to resource bindings.
// TODO(crbug.com/dawn/685): Define these better. For now, use two tiers where one
// offers slightly better than default limits.
#define LIMITS_RESOURCE_BINDINGS(X)                                                \
    X(Maximum,   maxDynamicUniformBuffersPerPipelineLayout,         8,         10) \
    X(Maximum,   maxDynamicStorageBuffersPerPipelineLayout,         4,          8) \
    X(Maximum,            maxSampledTexturesPerShaderStage,        16,         16) \
    X(Maximum,                   maxSamplersPerShaderStage,        16,         16) \
    X(Maximum,             maxStorageBuffersPerShaderStage,         8,          8) \
    X(Maximum,            maxStorageTexturesPerShaderStage,         4,          8) \
    X(Maximum,             maxUniformBuffersPerShaderStage,        12,         12)

// TODO(crbug.com/dawn/685):
// These limits don't have tiers yet. Define two tiers with the same values since the macros
// in this file expect more than one tier.
#define LIMITS_OTHER(X)                                                            \
    X(Maximum,                       maxTextureDimension1D,      8192,       8192) \
    X(Maximum,                       maxTextureDimension2D,      8192,       8192) \
    X(Maximum,                       maxTextureDimension3D,      2048,       2048) \
    X(Maximum,                       maxTextureArrayLayers,       256,        256) \
    X(Maximum,                               maxBindGroups,         4,          4) \
    X(Maximum,              maxBindGroupsPlusVertexBuffers,        24,         24) \
    X(Maximum,                     maxBindingsPerBindGroup,      1000,       1000) \
    X(Maximum,                 maxUniformBufferBindingSize,     65536,      65536) \
    X(Alignment,           minUniformBufferOffsetAlignment,       256,        256) \
    X(Alignment,           minStorageBufferOffsetAlignment,       256,        256) \
    X(Maximum,                            maxVertexBuffers,         8,          8) \
    X(Maximum,                         maxVertexAttributes,        16,         16) \
    X(Maximum,                  maxVertexBufferArrayStride,      2048,       2048) \
    X(Maximum,               maxInterStageShaderComponents,        60,         60) \
    X(Maximum,               maxInterStageShaderVariables,         16,         16) \
    X(Maximum,                         maxColorAttachments,         8,          8) \
    X(Maximum,            maxColorAttachmentBytesPerSample,        32,         32)
// clang-format on

#define LIMITS_EACH_GROUP(X)              \
    X(LIMITS_WORKGROUP_STORAGE_SIZE)      \
    X(LIMITS_WORKGROUP_SIZE)              \
    X(LIMITS_STORAGE_BUFFER_BINDING_SIZE) \
    X(LIMITS_MAX_BUFFER_SIZE)             \
    X(LIMITS_RESOURCE_BINDINGS)           \
    X(LIMITS_OTHER)

#define LIMITS(X)                         \
    LIMITS_WORKGROUP_STORAGE_SIZE(X)      \
    LIMITS_WORKGROUP_SIZE(X)              \
    LIMITS_STORAGE_BUFFER_BINDING_SIZE(X) \
    LIMITS_MAX_BUFFER_SIZE(X)             \
    LIMITS_RESOURCE_BINDINGS(X)           \
    LIMITS_OTHER(X)

namespace dawn::native {
namespace {
template <uint32_t A, uint32_t B>
constexpr void StaticAssertSame() {
    static_assert(A == B, "Mismatching tier count in limit group.");
}

template <uint32_t I, uint32_t... Is>
constexpr uint32_t ReduceSameValue(std::integer_sequence<uint32_t, I, Is...>) {
    int unused[] = {0, (StaticAssertSame<I, Is>(), 0)...};
    DAWN_UNUSED(unused);
    return I;
}

enum class LimitClass {
    Alignment,
    Maximum,
};

template <LimitClass C>
struct CheckLimit;

template <>
struct CheckLimit<LimitClass::Alignment> {
    template <typename T>
    static bool IsBetter(T lhs, T rhs) {
        return lhs < rhs;
    }

    template <typename T>
    static MaybeError Validate(T supported, T required) {
        DAWN_INVALID_IF(IsBetter(required, supported),
                        "Required limit (%u) is lower than the supported limit (%u).", required,
                        supported);
        DAWN_INVALID_IF(!IsPowerOfTwo(required), "Required limit (%u) is not a power of two.",
                        required);
        return {};
    }
};

template <>
struct CheckLimit<LimitClass::Maximum> {
    template <typename T>
    static bool IsBetter(T lhs, T rhs) {
        return lhs > rhs;
    }

    template <typename T>
    static MaybeError Validate(T supported, T required) {
        DAWN_INVALID_IF(IsBetter(required, supported),
                        "Required limit (%u) is greater than the supported limit (%u).", required,
                        supported);
        return {};
    }
};

template <typename T>
bool IsLimitUndefined(T value) {
    static_assert(sizeof(T) != sizeof(T), "IsLimitUndefined not implemented for this type");
    return false;
}

template <>
bool IsLimitUndefined<uint32_t>(uint32_t value) {
    return value == wgpu::kLimitU32Undefined;
}

template <>
bool IsLimitUndefined<uint64_t>(uint64_t value) {
    return value == wgpu::kLimitU64Undefined;
}

}  // namespace

void GetDefaultLimits(Limits* limits) {
    ASSERT(limits != nullptr);
#define X(Better, limitName, base, ...) limits->limitName = base;
    LIMITS(X)
#undef X
}

Limits ReifyDefaultLimits(const Limits& limits) {
    Limits out;
#define X(Class, limitName, base, ...)                                                         \
    if (IsLimitUndefined(limits.limitName) ||                                                  \
        CheckLimit<LimitClass::Class>::IsBetter(static_cast<decltype(limits.limitName)>(base), \
                                                limits.limitName)) {                           \
        /* If the limit is undefined or the default is better, use the default */              \
        out.limitName = base;                                                                  \
    } else {                                                                                   \
        out.limitName = limits.limitName;                                                      \
    }
    LIMITS(X)
#undef X
    return out;
}

MaybeError ValidateLimits(const Limits& supportedLimits, const Limits& requiredLimits) {
#define X(Class, limitName, ...)                                                            \
    if (!IsLimitUndefined(requiredLimits.limitName)) {                                      \
        DAWN_TRY_CONTEXT(CheckLimit<LimitClass::Class>::Validate(supportedLimits.limitName, \
                                                                 requiredLimits.limitName), \
                         "validating " #limitName);                                         \
    }
    LIMITS(X)
#undef X
    return {};
}

Limits ApplyLimitTiers(Limits limits) {
#define X_TIER_COUNT(Better, limitName, ...) , std::integer_sequence<uint64_t, __VA_ARGS__>{}.size()
#define GET_TIER_COUNT(LIMIT_GROUP) \
    ReduceSameValue(std::integer_sequence<uint32_t LIMIT_GROUP(X_TIER_COUNT)>{})

#define X_EACH_GROUP(LIMIT_GROUP)                                    \
    {                                                                \
        constexpr uint32_t kTierCount = GET_TIER_COUNT(LIMIT_GROUP); \
        for (uint32_t i = kTierCount; i != 0; --i) {                 \
            LIMIT_GROUP(X_CHECK_BETTER_AND_CLAMP)                    \
            /* Limits fit in tier and have been clamped. Break. */   \
            break;                                                   \
        }                                                            \
    }

#define X_CHECK_BETTER_AND_CLAMP(Class, limitName, ...)                                   \
    {                                                                                     \
        constexpr std::array<decltype(Limits::limitName), kTierCount> tiers{__VA_ARGS__}; \
        decltype(Limits::limitName) tierValue = tiers[i - 1];                             \
        if (CheckLimit<LimitClass::Class>::IsBetter(tierValue, limits.limitName)) {       \
            /* The tier is better. Go to the next tier. */                                \
            continue;                                                                     \
        } else if (tierValue != limits.limitName) {                                       \
            /* Better than the tier. Degrade |limits| to the tier. */                     \
            limits.limitName = tiers[i - 1];                                              \
        }                                                                                 \
    }

    LIMITS_EACH_GROUP(X_EACH_GROUP)

    // After tiering all limit values, enforce additional restriction by calling NormalizeLimits.
    // Since maxStorageBufferBindingSize and maxBufferSize tiers are not exactly aligned, it is
    // possible that tiered maxStorageBufferBindingSize is larger than tiered maxBufferSize. For
    // example, on a hypothetical device with both maxStorageBufferBindingSize and maxBufferSize
    // being 4GB-1, the tiered maxStorageBufferBindingSize would be 4GB-4 while the tiered
    // maxBufferSize being 2GB. NormalizeLimits will clamp the maxStorageBufferBindingSize to
    // maxBufferSize in such cases, although the result may or may not be one of predefined
    // maxStorageBufferBindingSize tiers.
    NormalizeLimits(&limits);

#undef X_CHECK_BETTER_AND_CLAMP
#undef X_EACH_GROUP
#undef GET_TIER_COUNT
#undef X_TIER_COUNT
    return limits;
}

#define DAWN_INTERNAL_LIMITS_MEMBER_ASSIGNMENT(type, name) \
    { result.name = limits.name; }
#define DAWN_INTERNAL_LIMITS_FOREACH_MEMBER_ASSIGNMENT(MEMBERS) \
    MEMBERS(DAWN_INTERNAL_LIMITS_MEMBER_ASSIGNMENT)
LimitsForCompilationRequest LimitsForCompilationRequest::Create(const Limits& limits) {
    LimitsForCompilationRequest result;
    DAWN_INTERNAL_LIMITS_FOREACH_MEMBER_ASSIGNMENT(LIMITS_FOR_COMPILATION_REQUEST_MEMBERS)
    return result;
}
#undef DAWN_INTERNAL_LIMITS_FOREACH_MEMBER_ASSIGNMENT
#undef DAWN_INTERNAL_LIMITS_MEMBER_ASSIGNMENT

template <>
void stream::Stream<LimitsForCompilationRequest>::Write(Sink* s,
                                                        const LimitsForCompilationRequest& t) {
    t.VisitAll([&](const auto&... members) { StreamIn(s, members...); });
}

void NormalizeLimits(Limits* limits) {
    // Enforce internal Dawn constants for some limits to ensure they don't go over fixed-size
    // arrays in Dawn's internal code.
    limits->maxVertexBufferArrayStride =
        std::min(limits->maxVertexBufferArrayStride, kMaxVertexBufferArrayStride);
    limits->maxColorAttachments =
        std::min(limits->maxColorAttachments, uint32_t(kMaxColorAttachments));
    limits->maxBindGroups = std::min(limits->maxBindGroups, kMaxBindGroups);
    limits->maxBindGroupsPlusVertexBuffers =
        std::min(limits->maxBindGroupsPlusVertexBuffers, kMaxBindGroupsPlusVertexBuffers);
    limits->maxVertexAttributes =
        std::min(limits->maxVertexAttributes, uint32_t(kMaxVertexAttributes));
    limits->maxVertexBuffers = std::min(limits->maxVertexBuffers, uint32_t(kMaxVertexBuffers));
    limits->maxInterStageShaderComponents =
        std::min(limits->maxInterStageShaderComponents, kMaxInterStageShaderComponents);
    limits->maxSampledTexturesPerShaderStage =
        std::min(limits->maxSampledTexturesPerShaderStage, kMaxSampledTexturesPerShaderStage);
    limits->maxSamplersPerShaderStage =
        std::min(limits->maxSamplersPerShaderStage, kMaxSamplersPerShaderStage);
    limits->maxStorageBuffersPerShaderStage =
        std::min(limits->maxStorageBuffersPerShaderStage, kMaxStorageBuffersPerShaderStage);
    limits->maxStorageTexturesPerShaderStage =
        std::min(limits->maxStorageTexturesPerShaderStage, kMaxStorageTexturesPerShaderStage);
    limits->maxUniformBuffersPerShaderStage =
        std::min(limits->maxUniformBuffersPerShaderStage, kMaxUniformBuffersPerShaderStage);

    // Additional enforcement for dependent limits.
    limits->maxStorageBufferBindingSize =
        std::min(limits->maxStorageBufferBindingSize, limits->maxBufferSize);
    limits->maxUniformBufferBindingSize =
        std::min(limits->maxUniformBufferBindingSize, limits->maxBufferSize);
}

}  // namespace dawn::native
