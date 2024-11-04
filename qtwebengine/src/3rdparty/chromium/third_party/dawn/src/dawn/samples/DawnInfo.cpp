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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "dawn/dawn_proc.h"
#include "dawn/native/DawnNative.h"

namespace {

// Wraps a string to about 75 characters and prints indented. Splits on whitespace instead of
// between characters in a word.
std::string WrapString(const std::string& in, const std::string& indent) {
    std::stringstream out;

    size_t last_space = 0;
    size_t start_pos = 0;
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == ' ') {
            last_space = i;
        } else if (in[i] == '\n') {
            last_space = i;
        }

        if ((i - start_pos) != 0 && ((i - start_pos) % 75) == 0) {
            out << indent << in.substr(start_pos, last_space - start_pos) << "\n";
            start_pos = last_space + 1;
            last_space = start_pos;
        }
    }
    out << indent << in.substr(start_pos, in.size() - start_pos);

    return out.str();
}

std::string AdapterTypeToString(wgpu::AdapterType type) {
    switch (type) {
        case wgpu::AdapterType::DiscreteGPU:
            return "discrete GPU";
        case wgpu::AdapterType::IntegratedGPU:
            return "integrated GPU";
        case wgpu::AdapterType::CPU:
            return "CPU";
        case wgpu::AdapterType::Unknown:
            break;
    }
    return "unknown";
}

std::string BackendTypeToString(wgpu::BackendType type) {
    switch (type) {
        case wgpu::BackendType::Null:
            return "Null";
        case wgpu::BackendType::WebGPU:
            return "WebGPU";
        case wgpu::BackendType::D3D11:
            return "D3D11";
        case wgpu::BackendType::D3D12:
            return "D3D12";
        case wgpu::BackendType::Metal:
            return "Metal";
        case wgpu::BackendType::Vulkan:
            return "Vulkan";
        case wgpu::BackendType::OpenGL:
            return "OpenGL";
        case wgpu::BackendType::OpenGLES:
            return "OpenGLES";
        case wgpu::BackendType::Undefined:
            return "Undefined";
    }
    return "unknown";
}

std::string PowerPreferenceToString(const wgpu::DawnAdapterPropertiesPowerPreference& prop) {
    switch (prop.powerPreference) {
        case wgpu::PowerPreference::LowPower:
            return "low power";
        case wgpu::PowerPreference::HighPerformance:
            return "high performance";
        case wgpu::PowerPreference::Undefined:
            return "<undefined>";
    }
    return "<unknown>";
}

std::string AsHex(uint32_t val) {
    std::stringstream hex;
    hex << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << val;
    return hex.str();
}

std::string AdapterPropertiesToString(const wgpu::AdapterProperties& props) {
    std::stringstream out;
    out << "VendorID: " << AsHex(props.vendorID) << "\n";
    out << "Vendor: " << props.vendorName << "\n";
    out << "Architecture: " << props.architecture << "\n";
    out << "DeviceID: " << AsHex(props.deviceID) << "\n";
    out << "Name: " << props.name << "\n";
    out << "Driver description: " << props.driverDescription << "\n";
    out << "Adapter Type: " << AdapterTypeToString(props.adapterType) << "\n";
    out << "Backend Type: " << BackendTypeToString(props.backendType) << "\n";

    return out.str();
}

std::string FormatNumber(uint64_t num) {
    auto s = std::to_string(num);
    std::stringstream ret;

    auto remainder = s.length() % 3;
    ret << s.substr(0, remainder);
    for (size_t i = remainder; i < s.length(); i += 3) {
        if (i > 0) {
            ret << ",";
        }
        ret << s.substr(i, 3);
    }
    return ret.str();
}

std::string LimitsToString(const wgpu::Limits& limits, const std::string& indent) {
    std::stringstream out;

    out << indent << "maxTextureDimension1D: " << FormatNumber(limits.maxTextureDimension1D)
        << "\n";
    out << indent << "maxTextureDimension2D: " << FormatNumber(limits.maxTextureDimension2D)
        << "\n";
    out << indent << "maxTextureDimension3D: " << FormatNumber(limits.maxTextureDimension3D)
        << "\n";
    out << indent << "maxTextureArrayLayers: " << FormatNumber(limits.maxTextureArrayLayers)
        << "\n";
    out << indent << "maxBindGroups: " << FormatNumber(limits.maxBindGroups) << "\n";
    out << indent << "maxDynamicUniformBuffersPerPipelineLayout: "
        << FormatNumber(limits.maxDynamicUniformBuffersPerPipelineLayout) << "\n";
    out << indent << "maxDynamicStorageBuffersPerPipelineLayout: "
        << FormatNumber(limits.maxDynamicStorageBuffersPerPipelineLayout) << "\n";
    out << indent << "maxSampledTexturesPerShaderStage: "
        << FormatNumber(limits.maxSampledTexturesPerShaderStage) << "\n";
    out << indent << "maxSamplersPerShaderStage: " << FormatNumber(limits.maxSamplersPerShaderStage)
        << "\n";
    out << indent << "maxStorageBuffersPerShaderStage: "
        << FormatNumber(limits.maxStorageBuffersPerShaderStage) << "\n";
    out << indent << "maxStorageTexturesPerShaderStage: "
        << FormatNumber(limits.maxStorageTexturesPerShaderStage) << "\n";
    out << indent << "maxUniformBuffersPerShaderStage: "
        << FormatNumber(limits.maxUniformBuffersPerShaderStage) << "\n";
    out << indent
        << "maxUniformBufferBindingSize: " << FormatNumber(limits.maxUniformBufferBindingSize)
        << "\n";
    out << indent
        << "maxStorageBufferBindingSize: " << FormatNumber(limits.maxStorageBufferBindingSize)
        << "\n";
    out << indent << "minUniformBufferOffsetAlignment: "
        << FormatNumber(limits.minUniformBufferOffsetAlignment) << "\n";
    out << indent << "minStorageBufferOffsetAlignment: "
        << FormatNumber(limits.minStorageBufferOffsetAlignment) << "\n";
    out << indent << "maxVertexBuffers: " << FormatNumber(limits.maxVertexBuffers) << "\n";
    out << indent << "maxVertexAttributes: " << FormatNumber(limits.maxVertexAttributes) << "\n";
    out << indent
        << "maxVertexBufferArrayStride: " << FormatNumber(limits.maxVertexBufferArrayStride)
        << "\n";
    out << indent
        << "maxInterStageShaderComponents: " << FormatNumber(limits.maxInterStageShaderComponents)
        << "\n";
    out << indent
        << "maxInterStageShaderVariables: " << FormatNumber(limits.maxInterStageShaderVariables)
        << "\n";
    out << indent << "maxColorAttachments: " << FormatNumber(limits.maxColorAttachments) << "\n";
    out << indent
        << "maxComputeWorkgroupStorageSize: " << FormatNumber(limits.maxComputeWorkgroupStorageSize)
        << "\n";
    out << indent << "maxComputeInvocationsPerWorkgroup: "
        << FormatNumber(limits.maxComputeInvocationsPerWorkgroup) << "\n";
    out << indent << "maxComputeWorkgroupSizeX: " << FormatNumber(limits.maxComputeWorkgroupSizeX)
        << "\n";
    out << indent << "maxComputeWorkgroupSizeY: " << FormatNumber(limits.maxComputeWorkgroupSizeY)
        << "\n";
    out << indent << "maxComputeWorkgroupSizeZ: " << FormatNumber(limits.maxComputeWorkgroupSizeZ)
        << "\n";
    out << indent << "maxComputeWorkgroupsPerDimension: "
        << FormatNumber(limits.maxComputeWorkgroupsPerDimension) << "\n";

    return out.str();
}

void DumpAdapterProperties(const wgpu::Adapter& adapter) {
    wgpu::DawnAdapterPropertiesPowerPreference power_props{};

    wgpu::AdapterProperties properties{};
    properties.nextInChain = &power_props;

    adapter.GetProperties(&properties);
    std::cout << AdapterPropertiesToString(properties);
    std::cout << "Power: " << PowerPreferenceToString(power_props) << "\n";
    std::cout << "\n";
}

void DumpAdapterFeatures(const wgpu::Adapter& adapter) {
    auto feature_count = adapter.EnumerateFeatures(nullptr);
    std::vector<wgpu::FeatureName> features(feature_count);
    adapter.EnumerateFeatures(features.data());

    std::cout << "  Features\n";
    std::cout << "  ========\n";
    for (const auto& f : features) {
        auto info = dawn::native::GetFeatureInfo(f);
        std::cout << "   * " << info.name << "\n";
        std::cout << WrapString(info.description, "      ") << "\n";
        std::cout << "      " << info.url << "\n";
    }
}

void DumpAdapterLimits(const wgpu::Adapter& adapter) {
    wgpu::SupportedLimits adapterLimits;
    if (adapter.GetLimits(&adapterLimits)) {
        std::cout << "\n";
        std::cout << "  Adapter Limits\n";
        std::cout << "  ==============\n";
        std::cout << LimitsToString(adapterLimits.limits, "    ") << "\n";
    }
}

void DumpAdapter(const wgpu::Adapter& adapter) {
    std::cout << "Adapter\n";
    std::cout << "=======\n";

    DumpAdapterProperties(adapter);
    DumpAdapterFeatures(adapter);
    DumpAdapterLimits(adapter);
}

}  // namespace

int main(int argc, const char* argv[]) {
    auto toggles = dawn::native::AllToggleInfos();
    std::sort(toggles.begin(), toggles.end(), [](const auto* a, const auto* b) {
        return std::string(a->name) < std::string(b->name);
    });

    std::cout << "Toggles\n";
    std::cout << "=======\n";
    bool first = true;
    for (const auto* info : toggles) {
        if (!first) {
            std::cout << "\n";
        }
        first = false;
        std::cout << "  Name: " << info->name << "\n";
        std::cout << WrapString(info->description, "    ") << "\n";
        std::cout << "    " << info->url << "\n";
    }
    std::cout << "\n";

    dawnProcSetProcs(&dawn::native::GetProcs());

    auto instance = std::make_unique<dawn::native::Instance>();
    std::vector<dawn::native::Adapter> adapters = instance->EnumerateAdapters();

    for (const auto& adapter : adapters) {
        DumpAdapter(wgpu::Adapter(adapter.Get()));
    }
}
