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

#include "dawn/native/opengl/PipelineGL.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <string>

#include "dawn/common/BitSetIterator.h"
#include "dawn/native/BindGroupLayoutInternal.h"
#include "dawn/native/Device.h"
#include "dawn/native/Pipeline.h"
#include "dawn/native/opengl/BufferGL.h"
#include "dawn/native/opengl/Forward.h"
#include "dawn/native/opengl/OpenGLFunctions.h"
#include "dawn/native/opengl/PipelineLayoutGL.h"
#include "dawn/native/opengl/SamplerGL.h"
#include "dawn/native/opengl/ShaderModuleGL.h"
#include "dawn/native/opengl/TextureGL.h"

namespace dawn::native::opengl {

PipelineGL::PipelineGL() : mProgram(0) {}

PipelineGL::~PipelineGL() = default;

MaybeError PipelineGL::InitializeBase(const OpenGLFunctions& gl,
                                      const PipelineLayout* layout,
                                      const PerStage<ProgrammableStage>& stages) {
    mProgram = gl.CreateProgram();

    // Compute the set of active stages.
    wgpu::ShaderStage activeStages = wgpu::ShaderStage::None;
    for (SingleShaderStage stage : IterateStages(kAllStages)) {
        if (stages[stage].module != nullptr) {
            activeStages |= StageBit(stage);
        }
    }

    // Create an OpenGL shader for each stage and gather the list of combined samplers.
    PerStage<CombinedSamplerInfo> combinedSamplers;
    bool needsPlaceholderSampler = false;
    std::vector<GLuint> glShaders;
    for (SingleShaderStage stage : IterateStages(activeStages)) {
        const ShaderModule* module = ToBackend(stages[stage].module.Get());
        GLuint shader;
        DAWN_TRY_ASSIGN(shader, module->CompileShader(
                                    gl, stages[stage], stage, &combinedSamplers[stage], layout,
                                    &needsPlaceholderSampler, &mNeedsTextureBuiltinUniformBuffer,
                                    &mBindingPointEmulatedBuiltins));
        gl.AttachShader(mProgram, shader);
        glShaders.push_back(shader);
    }

    if (needsPlaceholderSampler) {
        SamplerDescriptor desc = {};
        ASSERT(desc.minFilter == wgpu::FilterMode::Nearest);
        ASSERT(desc.magFilter == wgpu::FilterMode::Nearest);
        ASSERT(desc.mipmapFilter == wgpu::MipmapFilterMode::Nearest);
        Ref<SamplerBase> sampler;
        DAWN_TRY_ASSIGN(sampler, layout->GetDevice()->GetOrCreateSampler(&desc));
        mPlaceholderSampler = ToBackend(std::move(sampler));
    }

    if (!mBindingPointEmulatedBuiltins.empty()) {
        BufferDescriptor desc = {};
        desc.size = mBindingPointEmulatedBuiltins.size() * sizeof(uint32_t);
        desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        Ref<BufferBase> buffer;
        DAWN_TRY_ASSIGN(buffer, layout->GetDevice()->CreateBuffer(&desc));
        mTextureBuiltinsBuffer = ToBackend(std::move(buffer));
    }

    // Link all the shaders together.
    gl.LinkProgram(mProgram);

    GLint linkStatus = GL_FALSE;
    gl.GetProgramiv(mProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint infoLogLength = 0;
        gl.GetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 1) {
            std::vector<char> buffer(infoLogLength);
            gl.GetProgramInfoLog(mProgram, infoLogLength, nullptr, &buffer[0]);
            return DAWN_VALIDATION_ERROR("Program link failed:\n%s", buffer.data());
        }
    }

    // Compute links between stages for combined samplers, then bind them to texture units
    gl.UseProgram(mProgram);
    const auto& indices = layout->GetBindingIndexInfo();

    std::set<CombinedSampler> combinedSamplersSet;
    for (SingleShaderStage stage : IterateStages(activeStages)) {
        for (const CombinedSampler& combined : combinedSamplers[stage]) {
            combinedSamplersSet.insert(combined);
        }
    }

    mUnitsForSamplers.resize(layout->GetNumSamplers());
    mUnitsForTextures.resize(layout->GetNumSampledTextures());

    GLuint textureUnit = layout->GetTextureUnitsUsed();
    for (const auto& combined : combinedSamplersSet) {
        const std::string& name = combined.GetName();
        GLint location = gl.GetUniformLocation(mProgram, name.c_str());

        if (location == -1) {
            continue;
        }

        gl.Uniform1i(location, textureUnit);

        bool shouldUseFiltering;
        {
            const BindGroupLayoutInternalBase* bgl =
                layout->GetBindGroupLayout(combined.textureLocation.group);
            BindingIndex bindingIndex = bgl->GetBindingIndex(combined.textureLocation.binding);

            GLuint textureIndex = indices[combined.textureLocation.group][bindingIndex];
            mUnitsForTextures[textureIndex].push_back(textureUnit);

            shouldUseFiltering = bgl->GetBindingInfo(bindingIndex).texture.sampleType ==
                                 wgpu::TextureSampleType::Float;
        }
        {
            if (combined.usePlaceholderSampler) {
                mPlaceholderSamplerUnits.push_back(textureUnit);
            } else {
                const BindGroupLayoutInternalBase* bgl =
                    layout->GetBindGroupLayout(combined.samplerLocation.group);
                BindingIndex bindingIndex = bgl->GetBindingIndex(combined.samplerLocation.binding);

                GLuint samplerIndex = indices[combined.samplerLocation.group][bindingIndex];
                mUnitsForSamplers[samplerIndex].push_back({textureUnit, shouldUseFiltering});
            }
        }

        textureUnit++;
    }

    for (GLuint glShader : glShaders) {
        gl.DetachShader(mProgram, glShader);
        gl.DeleteShader(glShader);
    }

    mInternalUniformBufferBinding = layout->GetInternalUniformBinding();

    return {};
}

void PipelineGL::DeleteProgram(const OpenGLFunctions& gl) {
    gl.DeleteProgram(mProgram);
}

const std::vector<PipelineGL::SamplerUnit>& PipelineGL::GetTextureUnitsForSampler(
    GLuint index) const {
    ASSERT(index < mUnitsForSamplers.size());
    return mUnitsForSamplers[index];
}

const std::vector<GLuint>& PipelineGL::GetTextureUnitsForTextureView(GLuint index) const {
    ASSERT(index < mUnitsForTextures.size());
    return mUnitsForTextures[index];
}

GLuint PipelineGL::GetProgramHandle() const {
    return mProgram;
}

void PipelineGL::ApplyNow(const OpenGLFunctions& gl) {
    gl.UseProgram(mProgram);
    for (GLuint unit : mPlaceholderSamplerUnits) {
        ASSERT(mPlaceholderSampler.Get() != nullptr);
        gl.BindSampler(unit, mPlaceholderSampler->GetNonFilteringHandle());
    }

    if (mTextureBuiltinsBuffer.Get() != nullptr) {
        gl.BindBufferBase(GL_UNIFORM_BUFFER, mInternalUniformBufferBinding,
                          mTextureBuiltinsBuffer->GetHandle());
    }
}

const Buffer* PipelineGL::GetInternalUniformBuffer() const {
    return mTextureBuiltinsBuffer.Get();
}

const tint::TextureBuiltinsFromUniformOptions::BindingPointToFieldAndOffset&
PipelineGL::GetBindingPointBuiltinDataInfo() const {
    return mBindingPointEmulatedBuiltins;
}

}  // namespace dawn::native::opengl
