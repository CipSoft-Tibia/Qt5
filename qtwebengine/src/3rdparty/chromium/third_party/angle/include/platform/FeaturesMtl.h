//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FeaturesMtl.h: Optional features for the Metal renderer.
//

#ifndef ANGLE_PLATFORM_FEATURESMTL_H_
#define ANGLE_PLATFORM_FEATURESMTL_H_

#include "platform/Feature.h"

namespace angle
{

struct FeaturesMtl : FeatureSetBase
{
    // BaseVertex/Instanced draw support:
    Feature hasBaseVertexInstancedDraw = {
        "has_base_vertex_instanced_draw", FeatureCategory::MetalFeatures,
        "The renderer supports base vertex instanced draw", &members};

    // Support depth texture filtering
    Feature hasDepthTextureFiltering = {
        "has_depth_texture_filtering", FeatureCategory::MetalFeatures,
        "The renderer supports depth texture's filtering other than nearest", &members};

    // Non-uniform compute shader dispatch support, i.e. Group size is not necessarily to be fixed:
    Feature hasNonUniformDispatch = {
        "has_non_uniform_dispatch", FeatureCategory::MetalFeatures,
        "The renderer supports non uniform compute shader dispatch's group size", &members};

    // fragment stencil output support
    Feature hasStencilOutput = {"has_shader_stencil_output", FeatureCategory::MetalFeatures,
                                "The renderer supports stencil output from fragment shader",
                                &members};

    // Texture swizzle support:
    Feature hasTextureSwizzle = {"has_texture_swizzle", FeatureCategory::MetalFeatures,
                                 "The renderer supports texture swizzle", &members};

    Feature hasDepthAutoResolve = {
        "has_msaa_depth_auto_resolve", FeatureCategory::MetalFeatures,
        "The renderer supports MSAA depth auto resolve at the end of render pass", &members};

    Feature hasStencilAutoResolve = {
        "has_msaa_stencil_auto_resolve", FeatureCategory::MetalFeatures,
        "The renderer supports MSAA stencil auto resolve at the end of render pass", &members};

    // On macos, separate depth & stencil buffers are not supproted. However, on iOS devices,
    // they are supproted:
    Feature allowSeparatedDepthStencilBuffers = {
        "allow_separate_depth_stencil_buffers", FeatureCategory::MetalFeatures,
        "Some Apple platforms such as iOS allows separate depth & stencil buffers, "
        "whereas others such as macOS don't",
        &members};

    Feature allowMultisampleStoreAndResolve = {
        "allow_msaa_store_and_resolve", FeatureCategory::MetalFeatures,
        "The renderer supports MSAA store and resolve in the same pass", &members};

    Feature allowGenMultipleMipsPerPass = {
        "gen_multiple_mips_per_pass", FeatureCategory::MetalFeatures,
        "The renderer supports generating multiple mipmaps per pass", &members};
};

}  // namespace angle

#endif  // ANGLE_PLATFORM_FEATURESMTL_H_
