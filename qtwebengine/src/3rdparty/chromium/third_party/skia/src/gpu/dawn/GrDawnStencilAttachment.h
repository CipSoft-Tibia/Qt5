/*
* Copyright 2019 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#ifndef GrDawnStencil_DEFINED
#define GrDawnStencil_DEFINED

#include "src/gpu/GrStencilAttachment.h"

#include "dawn/webgpu_cpp.h"

class GrDawnGpu;

class GrDawnStencilAttachment : public GrStencilAttachment {
public:
    static GrDawnStencilAttachment* Create(GrDawnGpu* gpu, SkISize dimensions, int sampleCnt);

    ~GrDawnStencilAttachment() override;
    wgpu::TextureView view() const { return fView; }
    GrBackendFormat backendFormat() const override {
        return GrBackendFormat::MakeDawn(wgpu::TextureFormat::Depth24PlusStencil8);
    }

protected:
    void onRelease() override;
    void onAbandon() override;

private:
    size_t onGpuMemorySize() const override;

    GrDawnStencilAttachment(GrDawnGpu* gpu, SkISize dimensions, int bits, int samples,
                            wgpu::Texture texture, wgpu::TextureView view);

    GrDawnGpu* getDawnGpu() const;

    wgpu::Texture fTexture;
    wgpu::TextureView fView;

    using INHERITED = GrStencilAttachment;
};

#endif
