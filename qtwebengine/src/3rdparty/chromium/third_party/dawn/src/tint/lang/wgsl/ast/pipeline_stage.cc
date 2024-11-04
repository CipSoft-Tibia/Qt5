// Copyright 2020 The Tint Authors.
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

#include "src/tint/lang/wgsl/ast/pipeline_stage.h"

namespace tint::ast {

std::string_view ToString(PipelineStage stage) {
    switch (stage) {
        case PipelineStage::kNone:
            return "none";
        case PipelineStage::kVertex:
            return "vertex";
        case PipelineStage::kFragment:
            return "fragment";
        case PipelineStage::kCompute:
            return "compute";
    }
    return "<unknown>";
}

}  // namespace tint::ast
