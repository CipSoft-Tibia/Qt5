// Copyright 2023 The Tint Authors.
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

#include "src/tint/lang/wgsl/ast/transform/manager.h"
#include "src/tint/lang/wgsl/ast/transform/transform.h"
#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"

/// If set to 1 then the transform::Manager will dump the WGSL of the program
/// before and after each transform. Helpful for debugging bad output.
#define TINT_PRINT_PROGRAM_FOR_EACH_TRANSFORM 0

#if TINT_PRINT_PROGRAM_FOR_EACH_TRANSFORM
#include <iostream>
#define TINT_IF_PRINT_PROGRAM(x) x
#else  // TINT_PRINT_PROGRAM_FOR_EACH_TRANSFORM
#define TINT_IF_PRINT_PROGRAM(x)
#endif  // TINT_PRINT_PROGRAM_FOR_EACH_TRANSFORM

namespace tint::ast::transform {

Manager::Manager() = default;
Manager::~Manager() = default;

Program Manager::Run(const Program* program, const DataMap& inputs, DataMap& outputs) const {
#if TINT_PRINT_PROGRAM_FOR_EACH_TRANSFORM
    auto print_program = [&](const char* msg, const Transform* transform) {
        auto wgsl = Program::printer(program);
        std::cout << "=========================================================" << std::endl;
        std::cout << "== " << msg << " "
                  << (transform ? transform->TypeInfo().name : "transform manager") << ":"
                  << std::endl;
        std::cout << "=========================================================" << std::endl;
        std::cout << wgsl << std::endl;
        if (!program->IsValid()) {
            std::cout << "-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --" << std::endl;
            std::cout << program->Diagnostics().str() << std::endl;
        }
        std::cout << "=========================================================" << std::endl
                  << std::endl;
    };
#endif

    std::optional<Program> output;

    TINT_IF_PRINT_PROGRAM(print_program("Input of", nullptr));

    for (const auto& transform : transforms_) {
        if (auto result = transform->Apply(program, inputs, outputs)) {
            output.emplace(std::move(result.value()));
            program = &output.value();

            if (!program->IsValid()) {
                TINT_IF_PRINT_PROGRAM(print_program("Invalid output of", transform.get()));
                break;
            }

            TINT_IF_PRINT_PROGRAM(print_program("Output of", transform.get()));
        } else {
            TINT_IF_PRINT_PROGRAM(std::cout << "Skipped " << transform->TypeInfo().name
                                            << std::endl);
        }
    }

    TINT_IF_PRINT_PROGRAM(print_program("Final output of", nullptr));

    if (!output) {
        ProgramBuilder b;
        program::CloneContext ctx{&b, program, /* auto_clone_symbols */ true};
        ctx.Clone();
        output = resolver::Resolve(b);
    }
    return std::move(output.value());
}

}  // namespace tint::ast::transform
