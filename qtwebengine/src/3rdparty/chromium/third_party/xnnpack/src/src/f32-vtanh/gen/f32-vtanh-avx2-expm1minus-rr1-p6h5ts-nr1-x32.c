// Auto-generated file. Do not edit!
//   Template: src/f32-vtanh/avx2-expm1minus.c.in
//   Generator: tools/xngen
//
// Copyright 2023 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <immintrin.h>

#include <xnnpack/common.h>
#include <xnnpack/microparams.h>
#include <xnnpack/vunary.h>


void xnn_f32_vtanh_ukernel__avx2_expm1minus_rr1_p6h5ts_nr1_x32(
    size_t batch,
    const float* input,
    float* output,
    const union xnn_f32_tanh_params params[restrict XNN_MIN_ELEMENTS(1)]) XNN_OOB_READS
{
  assert(batch != 0);
  assert(batch % sizeof(float) == 0);
  assert(input != NULL);
  assert(output != NULL);

  const __m256 vsign_mask = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.sign_mask);
  const __m256 vsat_cutoff = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.sat_cutoff);
  const __m256 vlog2e = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.log2e);
  const __m256 vmagic_bias = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.magic_bias);
  const __m256 vminus_ln2 = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.minus_ln2);
  const __m256 vc6 = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.c6);
  const __m256 vc5 = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.c5);
  const __m256 vc4 = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.c4);
  const __m256 vc3 = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.c3);
  const __m256 vc2 = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.c2);
  const __m256 vtwo = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.two);
  const __m256 vminus_one = _mm256_load_ps(params->avx_expm1minus_rr1_p6h5.minus_one);

  for (; batch >= 32 * sizeof(float); batch -= 32 * sizeof(float)) {
    const __m256 vx0 = _mm256_loadu_ps(input);
    const __m256 vx1 = _mm256_loadu_ps(input + 8);
    const __m256 vx2 = _mm256_loadu_ps(input + 16);
    const __m256 vx3 = _mm256_loadu_ps(input + 24);
    input += 32;

    const __m256 vz0 = _mm256_or_ps(vx0, vsign_mask);
    const __m256 vz1 = _mm256_or_ps(vx1, vsign_mask);
    const __m256 vz2 = _mm256_or_ps(vx2, vsign_mask);
    const __m256 vz3 = _mm256_or_ps(vx3, vsign_mask);

    const __m256 vinvsignx0 = _mm256_xor_ps(vx0, vz0);
    const __m256 vm0 = _mm256_cmp_ps(vz0, vsat_cutoff, _CMP_LE_OS);
    const __m256 vinvsignx1 = _mm256_xor_ps(vx1, vz1);
    const __m256 vm1 = _mm256_cmp_ps(vz1, vsat_cutoff, _CMP_LE_OS);
    const __m256 vinvsignx2 = _mm256_xor_ps(vx2, vz2);
    const __m256 vm2 = _mm256_cmp_ps(vz2, vsat_cutoff, _CMP_LE_OS);
    const __m256 vinvsignx3 = _mm256_xor_ps(vx3, vz3);
    const __m256 vm3 = _mm256_cmp_ps(vz3, vsat_cutoff, _CMP_LE_OS);

    __m256 vn0 = _mm256_fmadd_ps(vz0, vlog2e, vmagic_bias);
    __m256 vn1 = _mm256_fmadd_ps(vz1, vlog2e, vmagic_bias);
    __m256 vn2 = _mm256_fmadd_ps(vz2, vlog2e, vmagic_bias);
    __m256 vn3 = _mm256_fmadd_ps(vz3, vlog2e, vmagic_bias);

    const __m256 vs0 = _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_castps_si256(vn0), 23));
    const __m256 vs1 = _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_castps_si256(vn1), 23));
    const __m256 vs2 = _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_castps_si256(vn2), 23));
    const __m256 vs3 = _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_castps_si256(vn3), 23));

    vn0 = _mm256_sub_ps(vn0, vmagic_bias);
    vn1 = _mm256_sub_ps(vn1, vmagic_bias);
    vn2 = _mm256_sub_ps(vn2, vmagic_bias);
    vn3 = _mm256_sub_ps(vn3, vmagic_bias);

    const __m256 vt0 = _mm256_fmadd_ps(vn0, vminus_ln2, vz0);
    const __m256 vt1 = _mm256_fmadd_ps(vn1, vminus_ln2, vz1);
    const __m256 vt2 = _mm256_fmadd_ps(vn2, vminus_ln2, vz2);
    const __m256 vt3 = _mm256_fmadd_ps(vn3, vminus_ln2, vz3);

    __m256 vp0 = vc6;
    __m256 vp1 = vc6;
    __m256 vp2 = vc6;
    __m256 vp3 = vc6;
    vp0 = _mm256_fmadd_ps(vp0, vt0, vc5);
    vp1 = _mm256_fmadd_ps(vp1, vt1, vc5);
    vp2 = _mm256_fmadd_ps(vp2, vt2, vc5);
    vp3 = _mm256_fmadd_ps(vp3, vt3, vc5);
    vp0 = _mm256_fmadd_ps(vp0, vt0, vc4);
    vp1 = _mm256_fmadd_ps(vp1, vt1, vc4);
    vp2 = _mm256_fmadd_ps(vp2, vt2, vc4);
    vp3 = _mm256_fmadd_ps(vp3, vt3, vc4);
    vp0 = _mm256_fmadd_ps(vp0, vt0, vc3);
    vp1 = _mm256_fmadd_ps(vp1, vt1, vc3);
    vp2 = _mm256_fmadd_ps(vp2, vt2, vc3);
    vp3 = _mm256_fmadd_ps(vp3, vt3, vc3);
    vp0 = _mm256_fmadd_ps(vp0, vt0, vc2);
    vp1 = _mm256_fmadd_ps(vp1, vt1, vc2);
    vp2 = _mm256_fmadd_ps(vp2, vt2, vc2);
    vp3 = _mm256_fmadd_ps(vp3, vt3, vc2);
    vp0 = _mm256_fmadd_ps(vp0, vt0, vtwo);
    vp1 = _mm256_fmadd_ps(vp1, vt1, vtwo);
    vp2 = _mm256_fmadd_ps(vp2, vt2, vtwo);
    vp3 = _mm256_fmadd_ps(vp3, vt3, vtwo);

    const __m256 vts0 = _mm256_mul_ps(vt0, vs0);
    const __m256 vsmo0 = _mm256_add_ps(vs0, vminus_one);
    const __m256 vts1 = _mm256_mul_ps(vt1, vs1);
    const __m256 vsmo1 = _mm256_add_ps(vs1, vminus_one);
    const __m256 vts2 = _mm256_mul_ps(vt2, vs2);
    const __m256 vsmo2 = _mm256_add_ps(vs2, vminus_one);
    const __m256 vts3 = _mm256_mul_ps(vt3, vs3);
    const __m256 vsmo3 = _mm256_add_ps(vs3, vminus_one);
    const __m256 vemo0 = _mm256_fmadd_ps(vp0, vts0, vsmo0);
    const __m256 vemo1 = _mm256_fmadd_ps(vp1, vts1, vsmo1);
    const __m256 vemo2 = _mm256_fmadd_ps(vp2, vts2, vsmo2);
    const __m256 vemo3 = _mm256_fmadd_ps(vp3, vts3, vsmo3);
    const __m256 vepo0 = _mm256_add_ps(vemo0, vtwo);
    const __m256 vepo1 = _mm256_add_ps(vemo1, vtwo);
    const __m256 vepo2 = _mm256_add_ps(vemo2, vtwo);
    const __m256 vepo3 = _mm256_add_ps(vemo3, vtwo);

    __m256 vrepo0 = _mm256_rcp_ps(vepo0);
    __m256 vrepo1 = _mm256_rcp_ps(vepo1);
    __m256 vrepo2 = _mm256_rcp_ps(vepo2);
    __m256 vrepo3 = _mm256_rcp_ps(vepo3);
    const __m256 verepo0 = _mm256_fnmsub_ps(vrepo0, vepo0, vminus_one);
    const __m256 verepo1 = _mm256_fnmsub_ps(vrepo1, vepo1, vminus_one);
    const __m256 verepo2 = _mm256_fnmsub_ps(vrepo2, vepo2, vminus_one);
    const __m256 verepo3 = _mm256_fnmsub_ps(vrepo3, vepo3, vminus_one);
    vrepo0 = _mm256_fmadd_ps(verepo0, vrepo0, vrepo0);
    vrepo1 = _mm256_fmadd_ps(verepo1, vrepo1, vrepo1);
    vrepo2 = _mm256_fmadd_ps(verepo2, vrepo2, vrepo2);
    vrepo3 = _mm256_fmadd_ps(verepo3, vrepo3, vrepo3);

    __m256 vy0 = _mm256_mul_ps(vemo0, vrepo0);
    __m256 vy1 = _mm256_mul_ps(vemo1, vrepo1);
    __m256 vy2 = _mm256_mul_ps(vemo2, vrepo2);
    __m256 vy3 = _mm256_mul_ps(vemo3, vrepo3);


    vy0 = _mm256_blendv_ps(vy0, vminus_one, vm0);
    vy1 = _mm256_blendv_ps(vy1, vminus_one, vm1);
    vy2 = _mm256_blendv_ps(vy2, vminus_one, vm2);
    vy3 = _mm256_blendv_ps(vy3, vminus_one, vm3);
    vy0 = _mm256_xor_ps(vy0, vinvsignx0);
    vy1 = _mm256_xor_ps(vy1, vinvsignx1);
    vy2 = _mm256_xor_ps(vy2, vinvsignx2);
    vy3 = _mm256_xor_ps(vy3, vinvsignx3);

    _mm256_storeu_ps(output, vy0);
    _mm256_storeu_ps(output + 8, vy1);
    _mm256_storeu_ps(output + 16, vy2);
    _mm256_storeu_ps(output + 24, vy3);
    output += 32;
  }
  for (; batch >= 8 * sizeof(float); batch -= 8 * sizeof(float)) {
    const __m256 vx = _mm256_loadu_ps(input);
    input += 8;

    const __m256 vz = _mm256_or_ps(vx, vsign_mask);

    const __m256 vinvsignx = _mm256_xor_ps(vx, vz);
    const __m256 vm = _mm256_cmp_ps(vz, vsat_cutoff, _CMP_LE_OS);

    __m256 vn = _mm256_fmadd_ps(vz, vlog2e, vmagic_bias);

    const __m256 vs = _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_castps_si256(vn), 23));

    vn = _mm256_sub_ps(vn, vmagic_bias);

    const __m256 vt = _mm256_fmadd_ps(vn, vminus_ln2, vz);

    __m256 vp = vc6;
    vp = _mm256_fmadd_ps(vp, vt, vc5);
    vp = _mm256_fmadd_ps(vp, vt, vc4);
    vp = _mm256_fmadd_ps(vp, vt, vc3);
    vp = _mm256_fmadd_ps(vp, vt, vc2);
    vp = _mm256_fmadd_ps(vp, vt, vtwo);

    const __m256 vts = _mm256_mul_ps(vt, vs);
    const __m256 vsmo = _mm256_add_ps(vs, vminus_one);
    const __m256 vemo = _mm256_fmadd_ps(vp, vts, vsmo);
    const __m256 vepo = _mm256_add_ps(vemo, vtwo);

    __m256 vrepo = _mm256_rcp_ps(vepo);
    const __m256 verepo = _mm256_fnmsub_ps(vrepo, vepo, vminus_one);
    vrepo = _mm256_fmadd_ps(verepo, vrepo, vrepo);

    __m256 vy = _mm256_mul_ps(vemo, vrepo);


    vy = _mm256_blendv_ps(vy, vminus_one, vm);
    vy = _mm256_xor_ps(vy, vinvsignx);

    _mm256_storeu_ps(output, vy);
    output += 8;
  }
  if XNN_UNLIKELY(batch != 0) {
    assert(batch >= 1 * sizeof(float));
    assert(batch <= 7 * sizeof(float));
    const __m256i vmask = _mm256_loadu_si256((const __m256i*) ((uintptr_t) &params->avx_expm1minus_rr1_p6h5.mask_table[7] - batch));

    const __m256 vx = _mm256_maskload_ps(input, vmask);

    const __m256 vz = _mm256_or_ps(vx, vsign_mask);

    const __m256 vinvsignx = _mm256_xor_ps(vx, vz);
    const __m256 vm = _mm256_cmp_ps(vz, vsat_cutoff, _CMP_LE_OS);

    __m256 vn = _mm256_fmadd_ps(vz, vlog2e, vmagic_bias);

    const __m256 vs = _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_castps_si256(vn), 23));

    vn = _mm256_sub_ps(vn, vmagic_bias);

    const __m256 vt = _mm256_fmadd_ps(vn, vminus_ln2, vz);

    __m256 vp = vc6;
    vp = _mm256_fmadd_ps(vp, vt, vc5);
    vp = _mm256_fmadd_ps(vp, vt, vc4);
    vp = _mm256_fmadd_ps(vp, vt, vc3);
    vp = _mm256_fmadd_ps(vp, vt, vc2);
    vp = _mm256_fmadd_ps(vp, vt, vtwo);

    const __m256 vts = _mm256_mul_ps(vt, vs);
    const __m256 vsmo = _mm256_add_ps(vs, vminus_one);
    const __m256 vemo = _mm256_fmadd_ps(vp, vts, vsmo);
    const __m256 vepo = _mm256_add_ps(vemo, vtwo);

    __m256 vrepo = _mm256_rcp_ps(vepo);
    const __m256 verepo = _mm256_fnmsub_ps(vrepo, vepo, vminus_one);
    vrepo = _mm256_fmadd_ps(verepo, vrepo, vrepo);

    __m256 vy = _mm256_mul_ps(vemo, vrepo);


    vy = _mm256_blendv_ps(vy, vminus_one, vm);
    vy = _mm256_xor_ps(vy, vinvsignx);

    __m128 vy_lo = _mm256_castps256_ps128(vy);
    if (batch & (4 * sizeof(float))) {
      _mm_storeu_ps(output, vy_lo);
      vy_lo = _mm256_extractf128_ps(vy, 1);
      output += 4;
    }
    if (batch & (2 * sizeof(float))) {
      _mm_storel_pi((__m64*) output, vy_lo);
      vy_lo = _mm_movehl_ps(vy_lo, vy_lo);
      output += 2;
    }
    if (batch & (1 * sizeof(float))) {
      _mm_store_ss(output, vy_lo);
    }
  }
}
