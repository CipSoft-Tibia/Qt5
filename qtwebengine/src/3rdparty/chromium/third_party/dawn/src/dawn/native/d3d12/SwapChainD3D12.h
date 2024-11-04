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

#ifndef SRC_DAWN_NATIVE_D3D12_SWAPCHAIND3D12_H_
#define SRC_DAWN_NATIVE_D3D12_SWAPCHAIND3D12_H_

#include <vector>

#include "dawn/native/d3d/SwapChainD3D.h"

#include "dawn/native/IntegerTypes.h"
#include "dawn/native/d3d12/d3d12_platform.h"

namespace dawn::native::d3d12 {

class Device;
class Texture;

class SwapChain final : public d3d::SwapChain {
  public:
    static ResultOrError<Ref<SwapChain>> Create(Device* device,
                                                Surface* surface,
                                                SwapChainBase* previousSwapChain,
                                                const SwapChainDescriptor* descriptor);

  private:
    using Base = d3d::SwapChain;
    using Base::Base;
    ~SwapChain() override;

    // SwapChainBase implementation
    MaybeError PresentImpl() override;
    ResultOrError<Ref<TextureBase>> GetCurrentTextureImpl() override;
    void DetachFromSurfaceImpl() override;

    // d3d::SwapChain implementation
    IUnknown* GetD3DDeviceForCreatingSwapChain() override;
    void ReuseBuffers(SwapChainBase* previousSwapChain) override;
    // Does the swapchain initialization step of gathering the buffers.
    MaybeError CollectSwapChainBuffers() override;
    // Calls DetachFromSurface but also synchronously waits until all references to the
    // swapchain and buffers are removed, as that's a constraint for some DXGI operations.
    MaybeError DetachAndWaitForDeallocation() override;

    std::vector<ComPtr<ID3D12Resource>> mBuffers;
    std::vector<ExecutionSerial> mBufferLastUsedSerials;
    uint32_t mCurrentBuffer = 0;

    Ref<Texture> mApiTexture;
};

}  // namespace dawn::native::d3d12

#endif  // SRC_DAWN_NATIVE_D3D12_SWAPCHAIND3D12_H_
