// Copyright 2019 The Dawn Authors
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

#ifndef SRC_DAWN_NATIVE_D3D12_D3D12INFO_H_
#define SRC_DAWN_NATIVE_D3D12_D3D12INFO_H_

#include "dawn/native/Error.h"
#include "dawn/native/PerStage.h"
#include "dawn/native/d3d12/d3d12_platform.h"

namespace dawn::native::d3d12 {

class PhysicalDevice;

struct D3D12DeviceInfo {
    bool isUMA;
    uint32_t resourceHeapTier;
    bool supportsRenderPass;
    bool supportsShaderF16;
    // shaderModel indicates the maximum supported shader model, for example, the value 62
    // indicates that current driver supports the maximum shader model is shader model 6.2.
    uint32_t shaderModel;
    PerStage<std::wstring> shaderProfiles;
    bool supportsSharedResourceCapabilityTier1;
    bool supportsDP4a;
    bool supportsCastingFullyTypedFormat;
    uint32_t programmableSamplePositionsTier;
    bool supportsRootSignatureVersion1_1;
    bool use64KBAlignedMSAATexture;
    bool supportsHeapFlagCreateNotZeroed;
    // Whether the device support wave intrinsics
    bool supportsWaveOps;
    uint32_t waveLaneCountMin;
    // Currently the WaveLaneCountMax queried from D3D12 API is not reliable and the meaning is
    // unclear. Reference:
    // https://github.com/Microsoft/DirectXShaderCompiler/wiki/Wave-Intrinsics#:~:text=UINT%20WaveLaneCountMax
    uint32_t waveLaneCountMax;
};

ResultOrError<D3D12DeviceInfo> GatherDeviceInfo(const PhysicalDevice& physicalDevice);
}  // namespace dawn::native::d3d12

#endif  // SRC_DAWN_NATIVE_D3D12_D3D12INFO_H_
