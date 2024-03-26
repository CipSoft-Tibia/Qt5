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

#ifndef SRC_DAWN_NATIVE_VULKAN_EXTERNAL_SEMAPHORE_SERVICEIMPLEMENTATIONFD_H_
#define SRC_DAWN_NATIVE_VULKAN_EXTERNAL_SEMAPHORE_SERVICEIMPLEMENTATIONFD_H_

#include <memory>

namespace dawn::native::vulkan {
class Device;
struct VulkanDeviceInfo;
struct VulkanFunctions;
}  // namespace dawn::native::vulkan

namespace dawn::native::vulkan::external_semaphore {
class ServiceImplementation;

bool CheckFDSupport(const VulkanDeviceInfo& deviceInfo,
                    VkPhysicalDevice physicalDevice,
                    const VulkanFunctions& fn);
std::unique_ptr<ServiceImplementation> CreateFDService(Device* device);

}  // namespace dawn::native::vulkan::external_semaphore

#endif  // SRC_DAWN_NATIVE_VULKAN_EXTERNAL_SEMAPHORE_SERVICEIMPLEMENTATIONFD_H_
