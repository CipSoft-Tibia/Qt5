// Copyright 2023 The Dawn Authors
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

#ifndef SRC_DAWN_NATIVE_D3D11_BINDGROUPD3D11_H_
#define SRC_DAWN_NATIVE_D3D11_BINDGROUPD3D11_H_

#include "dawn/common/PlacementAllocated.h"
#include "dawn/native/BindGroup.h"

namespace dawn::native::d3d11 {

class Device;

class BindGroup final : public BindGroupBase, public PlacementAllocated {
  public:
    static Ref<BindGroup> Create(Device* device, const BindGroupDescriptor* descriptor);

  private:
    friend SlabAllocator<BindGroup>;

    BindGroup(Device* device, const BindGroupDescriptor* descriptor);
    ~BindGroup() override;

    void DestroyImpl() override;
};

}  // namespace dawn::native::d3d11

#endif  // SRC_DAWN_NATIVE_D3D11_BINDGROUPD3D11_H_
