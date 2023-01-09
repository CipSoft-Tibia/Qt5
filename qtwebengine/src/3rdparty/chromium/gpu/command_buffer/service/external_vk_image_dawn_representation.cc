// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/external_vk_image_dawn_representation.h"

#include <dawn_native/VulkanBackend.h>

#include <utility>
#include <vector>

#include "base/posix/eintr_wrapper.h"
#include "gpu/vulkan/vulkan_image.h"

namespace gpu {

ExternalVkImageDawnRepresentation::ExternalVkImageDawnRepresentation(
    SharedImageManager* manager,
    SharedImageBacking* backing,
    MemoryTypeTracker* tracker,
    WGPUDevice device,
    WGPUTextureFormat wgpu_format,
    base::ScopedFD memory_fd)
    : SharedImageRepresentationDawn(manager, backing, tracker),
      device_(device),
      wgpu_format_(wgpu_format),
      memory_fd_(std::move(memory_fd)),
      dawn_procs_(dawn_native::GetProcs()) {
  DCHECK(device_);

  // Keep a reference to the device so that it stays valid (it might become
  // lost in which case operations will be noops).
  dawn_procs_.deviceReference(device_);
}

ExternalVkImageDawnRepresentation::~ExternalVkImageDawnRepresentation() {
  EndAccess();
  dawn_procs_.deviceRelease(device_);
}

WGPUTexture ExternalVkImageDawnRepresentation::BeginAccess(
    WGPUTextureUsage usage) {
  DCHECK(begin_access_semaphores_.empty());
  if (!backing_impl()->BeginAccess(false, &begin_access_semaphores_,
                                   false /* is_gl */)) {
    return nullptr;
  }

  WGPUTextureDescriptor texture_descriptor = {};
  texture_descriptor.nextInChain = nullptr;
  texture_descriptor.format = wgpu_format_;
  texture_descriptor.usage = usage;
  texture_descriptor.dimension = WGPUTextureDimension_2D;
  texture_descriptor.size = {size().width(), size().height(), 1};
  texture_descriptor.mipLevelCount = 1;
  texture_descriptor.sampleCount = 1;

  dawn_native::vulkan::ExternalImageDescriptorOpaqueFD descriptor = {};
  descriptor.cTextureDescriptor = &texture_descriptor;
  descriptor.isCleared = IsCleared();
  descriptor.allocationSize = backing_impl()->image()->device_size();
  descriptor.memoryTypeIndex = backing_impl()->image()->memory_type_index();
  descriptor.memoryFD = dup(memory_fd_.get());

  // TODO(http://crbug.com/dawn/200): We may not be obeying all of the rules
  // specified by Vulkan for external queue transfer barriers. Investigate this.

  for (auto& external_semaphore : begin_access_semaphores_) {
    descriptor.waitFDs.push_back(
        external_semaphore.handle().TakeHandle().release());
  }

  texture_ = dawn_native::vulkan::WrapVulkanImage(device_, &descriptor);

  if (texture_) {
    // Keep a reference to the texture so that it stays valid (its content
    // might be destroyed).
    dawn_procs_.textureReference(texture_);
  }

  return texture_;
}

void ExternalVkImageDawnRepresentation::EndAccess() {
  if (!texture_) {
    return;
  }

  // Grab the signal semaphore from dawn
  int signal_semaphore_fd =
      dawn_native::vulkan::ExportSignalSemaphoreOpaqueFD(device_, texture_);

  if (dawn_native::IsTextureSubresourceInitialized(texture_, 0, 1, 0, 1)) {
    SetCleared();
  }

  // Wrap file descriptor in a handle
  SemaphoreHandle handle(VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
                         base::ScopedFD(signal_semaphore_fd));

  auto semaphore = ExternalSemaphore::CreateFromHandle(
      backing_impl()->context_provider(), std::move(handle));

  backing_impl()->EndAccess(false, std::move(semaphore), false /* is_gl */);

  // Destroy the texture, signaling the semaphore in dawn
  dawn_procs_.textureDestroy(texture_);
  dawn_procs_.textureRelease(texture_);
  texture_ = nullptr;

  // We have done with |begin_access_semaphores_|. They should have been waited.
  // So add them to pending semaphores for reusing or relaeasing.
  backing_impl()->AddSemaphoresToPendingListOrRelease(
      std::move(begin_access_semaphores_));
  begin_access_semaphores_.clear();
}

}  // namespace gpu
