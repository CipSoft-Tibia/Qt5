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

#ifndef SRC_DAWN_NATIVE_WEBGPU_ABSL_FORMAT_H_
#define SRC_DAWN_NATIVE_WEBGPU_ABSL_FORMAT_H_

#include "absl/strings/str_format.h"
#include "dawn/native/dawn_platform.h"
#include "dawn/native/webgpu_absl_format_autogen.h"

namespace dawn::ityp {
template <typename Index, typename Value>
class span;
}

namespace dawn::native {

//
// Structs
//

struct Color;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString>
AbslFormatConvert(const Color* value, const absl::FormatConversionSpec& spec, absl::FormatSink* s);

struct Extent2D;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const Extent2D* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

struct Extent3D;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const Extent3D* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

struct Origin2D;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const Origin2D* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

struct Origin3D;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const Origin3D* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

struct BindingInfo;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const BindingInfo& value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

struct ImageCopyTexture;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const ImageCopyTexture* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

struct TextureDataLayout;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const TextureDataLayout* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

//
// Objects
//

class DeviceBase;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const DeviceBase* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

class ApiObjectBase;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const ApiObjectBase* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

class AttachmentState;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const AttachmentState* value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

//
// Enums
//

enum class Aspect : uint8_t;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString>
AbslFormatConvert(Aspect value, const absl::FormatConversionSpec& spec, absl::FormatSink* s);

enum class BindingInfoType;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    BindingInfoType value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

enum class SampleTypeBit : uint8_t;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString>
AbslFormatConvert(SampleTypeBit value, const absl::FormatConversionSpec& spec, absl::FormatSink* s);

enum class SingleShaderStage;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    SingleShaderStage value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

enum class VertexFormatBaseType;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    VertexFormatBaseType value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

enum class InterStageComponentType;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    InterStageComponentType value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

enum class InterpolationType;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    InterpolationType value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

enum class InterpolationSampling;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    InterpolationSampling value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

enum class TextureComponentType;
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    TextureComponentType value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s);

template <typename I, typename T>
absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const ityp::span<I, T>& values,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s) {
    s->Append("[");
    bool first = true;
    for (const auto& v : values) {
        if (!first) {
            s->Append(absl::StrFormat(", %s", v));
        } else {
            s->Append(absl::StrFormat("%s", v));
        }
        first = false;
    }
    s->Append("]");
    return {true};
}

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_WEBGPU_ABSL_FORMAT_H_
