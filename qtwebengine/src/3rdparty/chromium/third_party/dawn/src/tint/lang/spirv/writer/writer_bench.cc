// Copyright 2022 The Tint Authors.
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

// GEN_BUILD:CONDITION(tint_build_ir)

#include <string>

#include "src/tint/cmd/bench/bench.h"
#include "src/tint/lang/spirv/writer/writer.h"

namespace tint::spirv::writer {
namespace {

void RunBenchmark(benchmark::State& state, std::string input_name, Options options) {
    auto res = bench::LoadProgram(input_name);
    if (auto err = std::get_if<bench::Error>(&res)) {
        state.SkipWithError(err->msg.c_str());
        return;
    }
    auto& program = std::get<bench::ProgramAndFile>(res).program;
    for (auto _ : state) {
        auto res = Generate(&program, options);
        if (!res) {
            state.SkipWithError(res.Failure());
        }
    }
}

void GenerateSPIRV(benchmark::State& state, std::string input_name) {
    RunBenchmark(state, input_name, {});
}

void GenerateSPIRV_UseIR(benchmark::State& state, std::string input_name) {
    Options options;
    options.use_tint_ir = true;
    RunBenchmark(state, input_name, std::move(options));
}

TINT_BENCHMARK_PROGRAMS(GenerateSPIRV);
TINT_BENCHMARK_PROGRAMS(GenerateSPIRV_UseIR);

}  // namespace
}  // namespace tint::spirv::writer
