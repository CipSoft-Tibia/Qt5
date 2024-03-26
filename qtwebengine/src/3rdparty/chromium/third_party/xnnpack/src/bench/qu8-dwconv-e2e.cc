// Copyright 2021 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>
#include <vector>

#include <xnnpack.h>

#include <benchmark/benchmark.h>
#include "bench/end2end.h"
#include "bench/utils.h"
#include "models/models.h"

#include <xnnpack.h>
#include <xnnpack/dwconv.h>
#include <xnnpack/microfnptr.h>
#include <xnnpack/microparams-init.h>
#include <xnnpack/params.h>


static void DWConvEnd2EndBenchmark(
  benchmark::State& state,
  models::ExecutionPlanFactory model_factory,
  xnn_qu8_dwconv_minmax_unipass_ukernel_fn dwconv,
  xnn_init_qu8_conv_minmax_params_fn init_params,
  uint8_t channel_tile, uint8_t primary_tile,
  benchmark::utils::IsaCheckFunction isa_check = nullptr)
{
  if (isa_check != nullptr && !isa_check(state)) {
    return;
  }
  if (xnn_initialize(nullptr /* allocator */) != xnn_status_success) {
    state.SkipWithError("failed to initialize XNNPACK");
    return;
  }

  // Save xnn_params.qu8.dwconv so that we can modify it for the benchmark and later restore it.
  struct dwconv_parameters saved_dwconv_params[XNN_MAX_QU8_DWCONV_UKERNELS];
  static_assert(sizeof(saved_dwconv_params) == sizeof(xnn_params.qu8.dwconv), "size of dwconv params must match");
  memcpy(saved_dwconv_params, xnn_params.qu8.dwconv, sizeof(saved_dwconv_params));

  // Override microkernels chosen in xnn_initialize
  for (size_t i = 0; i < XNN_MAX_QU8_DWCONV_UKERNELS; i++) {
    // Replace only the microkernel the matching kernel size.
    if (xnn_params.qu8.dwconv[i].primary_tile == primary_tile) {
      // Note: do not directly assign to xnn_params.qu8.dwconv[i] because it breaks older gcc.
      xnn_params.qu8.dwconv[i].minmax.unipass = xnn_dwconv_unipass_ukernel_fn(dwconv);
      xnn_params.qu8.dwconv[i].channel_tile = channel_tile;
      xnn_params.qu8.dwconv[i].primary_tile = primary_tile;
      xnn_params.qu8.dwconv[i].last_tile = 0;
      xnn_params.qu8.dwconv[i].init.qu8 = init_params;
      break;
    }
  }

  auto execution_plan = model_factory(nullptr);
  if (execution_plan.empty()) {
    state.SkipWithError("failed to create a model");
    return;
  }

  for (auto _ : state) {
    for (const std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)>& op : execution_plan) {
      xnn_status status = xnn_run_operator(op.get(), nullptr);
      if (status != xnn_status_success) {
        state.SkipWithError("failed to run a model");
        return;
      }
    }
  }

  const uint64_t cpu_frequency = benchmark::utils::GetCurrentCpuFrequency();
  if (cpu_frequency != 0) {
    state.counters["cpufreq"] = cpu_frequency;
  }

  // Restore xnn_params.qu8.dwconv to original state as defined in init.c.
  memcpy(xnn_params.qu8.dwconv, saved_dwconv_params, sizeof(saved_dwconv_params));
}


#if XNN_ARCH_ARM || XNN_ARCH_ARM64
  static void qu8_dwconv_9p8c__neon_mul8(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_rndnu_ukernel_9p8c__neon_mul8,
      xnn_init_qu8_conv_minmax_rndnu_neon_params,
      8 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckNEON);
  }
  static void qu8_dwconv_9p16c__neon_mul8(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_rndnu_ukernel_9p16c__neon_mul8,
      xnn_init_qu8_conv_minmax_rndnu_neon_params,
      16 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckNEON);
  }
  static void qu8_dwconv_9p24c__neon_mul8(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_rndnu_ukernel_9p24c__neon_mul8,
      xnn_init_qu8_conv_minmax_rndnu_neon_params,
      24 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckNEON);
  }
  static void qu8_dwconv_9p32c__neon_mul8(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_rndnu_ukernel_9p32c__neon_mul8,
      xnn_init_qu8_conv_minmax_rndnu_neon_params,
      32 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckNEON);
  }
  static void qu8_dwconv_9p8c__neon_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_rndnu_ukernel_9p8c__neon_mul16,
      xnn_init_qu8_conv_minmax_rndnu_neon_params,
      8 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckNEON);
  }
  static void qu8_dwconv_9p16c__neon_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_rndnu_ukernel_9p16c__neon_mul16,
      xnn_init_qu8_conv_minmax_rndnu_neon_params,
      16 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckNEON);
  }
  static void qu8_dwconv_9p24c__neon_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_rndnu_ukernel_9p24c__neon_mul16,
      xnn_init_qu8_conv_minmax_rndnu_neon_params,
      24 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckNEON);
  }
  static void qu8_dwconv_9p32c__neon_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_rndnu_ukernel_9p32c__neon_mul16,
      xnn_init_qu8_conv_minmax_rndnu_neon_params,
      32 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckNEON);
  }

  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__neon_mul8);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__neon_mul8);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p24c__neon_mul8);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p32c__neon_mul8);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__neon_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__neon_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p24c__neon_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p32c__neon_mul16);
#endif  // XNN_ARCH_ARM || XNN_ARCH_ARM64


#if XNN_ARCH_X86 || XNN_ARCH_X86_64
  static void qu8_dwconv_9p16c__avx512skx_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p16c__avx512skx_mul32,
      xnn_init_qu8_conv_minmax_fp32_avx512_params,
      16 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX512SKX);
  }
  static void qu8_dwconv_9p32c__avx512skx_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p32c__avx512skx_mul32,
      xnn_init_qu8_conv_minmax_fp32_avx512_params,
      32 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX512SKX);
  }
  static void qu8_dwconv_9p8c__avx2_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p8c__avx2_mul32,
      xnn_init_qu8_conv_minmax_fp32_avx2_params,
      8 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX2);
  }
  static void qu8_dwconv_9p16c__avx2_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p16c__avx2_mul32,
      xnn_init_qu8_conv_minmax_fp32_avx2_params,
      16 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX2);
  }
  static void qu8_dwconv_9p32c__avx2_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p32c__avx2_mul32,
      xnn_init_qu8_conv_minmax_fp32_avx2_params,
      32 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX2);
  }
  static void qu8_dwconv_9p8c__avx_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p8c__avx_mul16,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      8 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX);
  }
  static void qu8_dwconv_9p16c__avx_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p16c__avx_mul16,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      16 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX);
  }
  static void qu8_dwconv_9p8c__avx_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p8c__avx_mul32,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      8 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX);
  }
  static void qu8_dwconv_9p16c__avx_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p16c__avx_mul32,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      16 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckAVX);
  }
  static void qu8_dwconv_9p8c__sse41_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p8c__sse41_mul16,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      8 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckSSE41);
  }
  static void qu8_dwconv_9p16c__sse41_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p16c__sse41_mul16,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      16 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckSSE41);
  }
  static void qu8_dwconv_9p8c__sse41_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p8c__sse41_mul32,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      8 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckSSE41);
  }
  static void qu8_dwconv_9p16c__sse41_mul32(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p16c__sse41_mul32,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      16 /* channel tile */, 9 /* primary tile */, benchmark::utils::CheckSSE41);
  }
  static void qu8_dwconv_9p8c__sse2_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p8c__sse2_mul16,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      8 /* channel tile */, 9 /* primary tile */);
  }
  static void qu8_dwconv_9p16c__sse2_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p16c__sse2_mul16,
      xnn_init_qu8_conv_minmax_fp32_sse2_params,
      16 /* channel tile */, 9 /* primary tile */);
  }

  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__avx512skx_mul32);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p32c__avx512skx_mul32);

  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__avx2_mul32);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__avx2_mul32);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p32c__avx2_mul32);

  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__avx_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__avx_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__avx_mul32);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__avx_mul32);

  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__sse41_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__sse41_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__sse41_mul32);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__sse41_mul32);

  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__sse2_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__sse2_mul16);
#endif  // XNN_ARCH_X86 || XNN_ARCH_X86_64


#if XNN_ARCH_WASMSIMD || XNN_ARCH_WASMRELAXEDSIMD
  static void qu8_dwconv_9p8c__wasmsimd_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p8c__wasmsimd_mul16,
      xnn_init_qu8_conv_minmax_fp32_wasmsimd_params,
      8 /* channel tile */, 9 /* primary tile */);
  }
  static void qu8_dwconv_9p16c__wasmsimd_mul16(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p16c__wasmsimd_mul16,
      xnn_init_qu8_conv_minmax_fp32_wasmsimd_params,
      16 /* channel tile */, 9 /* primary tile */);
  }

  BENCHMARK_QU8_END2END(qu8_dwconv_9p8c__wasmsimd_mul16);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p16c__wasmsimd_mul16);
#endif  // XNN_ARCH_WASMSIMD || XNN_ARCH_WASMRELAXEDSIMD


#if XNN_ARCH_WASM || XNN_ARCH_WASMSIMD || XNN_ARCH_WASMRELAXEDSIMD
  static void qu8_dwconv_9p1c__wasm_fmagic(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p1c__wasm_fmagic,
      xnn_init_qu8_conv_minmax_fp32_scalar_fmagic_params,
      1 /* channel tile */, 9 /* primary tile */);
  }
  static void qu8_dwconv_9p2c__wasm_fmagic(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p2c__wasm_fmagic,
      xnn_init_qu8_conv_minmax_fp32_scalar_fmagic_params,
      2 /* channel tile */, 9 /* primary tile */);
  }
  static void qu8_dwconv_9p4c__wasm_fmagic(benchmark::State& state, models::ExecutionPlanFactory model) {
    DWConvEnd2EndBenchmark(state, model,
      xnn_qu8_dwconv_minmax_fp32_ukernel_9p4c__wasm_fmagic,
      xnn_init_qu8_conv_minmax_fp32_scalar_fmagic_params,
      4 /* channel tile */, 9 /* primary tile */);
  }

  BENCHMARK_QU8_END2END(qu8_dwconv_9p1c__wasm_fmagic);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p2c__wasm_fmagic);
  BENCHMARK_QU8_END2END(qu8_dwconv_9p4c__wasm_fmagic);
#endif  // XNN_ARCH_WASM || XNN_ARCH_WASMSIMD || XNN_ARCH_WASMRELAXEDSIMD


static void qu8_dwconv_9p1c__scalar_fmagic(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p1c__scalar_fmagic,
    xnn_init_qu8_conv_minmax_fp32_scalar_fmagic_params,
    1 /* channel tile */, 9 /* primary tile */);
}
static void qu8_dwconv_9p2c__scalar_fmagic(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p2c__scalar_fmagic,
    xnn_init_qu8_conv_minmax_fp32_scalar_fmagic_params,
    2 /* channel tile */, 9 /* primary tile */);
}
static void qu8_dwconv_9p4c__scalar_fmagic(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p4c__scalar_fmagic,
    xnn_init_qu8_conv_minmax_fp32_scalar_fmagic_params,
    4 /* channel tile */, 9 /* primary tile */);
}

static void qu8_dwconv_9p1c__scalar_imagic(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p1c__scalar_imagic,
    xnn_init_qu8_conv_minmax_fp32_scalar_imagic_params,
    1 /* channel tile */, 9 /* primary tile */);
}
static void qu8_dwconv_9p2c__scalar_imagic(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p2c__scalar_imagic,
    xnn_init_qu8_conv_minmax_fp32_scalar_imagic_params,
    2 /* channel tile */, 9 /* primary tile */);
}
static void qu8_dwconv_9p4c__scalar_imagic(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p4c__scalar_imagic,
    xnn_init_qu8_conv_minmax_fp32_scalar_imagic_params,
    4 /* channel tile */, 9 /* primary tile */);
}

static void qu8_dwconv_9p1c__scalar_lrintf(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p1c__scalar_lrintf,
    xnn_init_qu8_conv_minmax_fp32_scalar_lrintf_params,
    1 /* channel tile */, 9 /* primary tile */);
}
static void qu8_dwconv_9p2c__scalar_lrintf(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p2c__scalar_lrintf,
    xnn_init_qu8_conv_minmax_fp32_scalar_lrintf_params,
    2 /* channel tile */, 9 /* primary tile */);
}
static void qu8_dwconv_9p4c__scalar_lrintf(benchmark::State& state, models::ExecutionPlanFactory model) {
  DWConvEnd2EndBenchmark(state, model,
    xnn_qu8_dwconv_minmax_fp32_ukernel_9p4c__scalar_lrintf,
    xnn_init_qu8_conv_minmax_fp32_scalar_lrintf_params,
    4 /* channel tile */, 9 /* primary tile */);
}

BENCHMARK_QU8_END2END(qu8_dwconv_9p1c__scalar_fmagic);
BENCHMARK_QU8_END2END(qu8_dwconv_9p2c__scalar_fmagic);
BENCHMARK_QU8_END2END(qu8_dwconv_9p4c__scalar_fmagic);

BENCHMARK_QU8_END2END(qu8_dwconv_9p1c__scalar_imagic);
BENCHMARK_QU8_END2END(qu8_dwconv_9p2c__scalar_imagic);
BENCHMARK_QU8_END2END(qu8_dwconv_9p4c__scalar_imagic);

BENCHMARK_QU8_END2END(qu8_dwconv_9p1c__scalar_lrintf);
BENCHMARK_QU8_END2END(qu8_dwconv_9p2c__scalar_lrintf);
BENCHMARK_QU8_END2END(qu8_dwconv_9p4c__scalar_lrintf);


#ifndef XNNPACK_BENCHMARK_NO_MAIN
BENCHMARK_MAIN();
#endif
