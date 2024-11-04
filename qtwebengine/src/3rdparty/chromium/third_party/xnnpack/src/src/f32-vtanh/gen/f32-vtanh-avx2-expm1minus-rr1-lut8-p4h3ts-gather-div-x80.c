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


// Table of exp2(k / 8) values decremented (as integer) by (k << 20), k = 0..7
extern XNN_INTERNAL const uint32_t xnn_table_exp2minus_k_over_8[8];

void xnn_f32_vtanh_ukernel__avx2_expm1minus_rr1_lut8_p4h3ts_gather_div_x80(
    size_t batch,
    const float* input,
    float* output,
    const union xnn_f32_tanh_params params[restrict XNN_MIN_ELEMENTS(1)]) XNN_OOB_READS
{
  assert(batch != 0);
  assert(batch % sizeof(float) == 0);
  assert(input != NULL);
  assert(output != NULL);

  const __m256 vsign_mask = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.sign_mask);
  const __m256 vsat_cutoff = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.sat_cutoff);
  const __m256 vlog2e = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.log2e);
  const __m256 vmagic_bias = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.magic_bias);
  const __m256i vindex_mask = _mm256_load_si256((const __m256i*) params->avx_expm1minus_rr1_lut8_p4h3.index_mask);
  const __m256 vminus_ln2 = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.minus_ln2);
  const __m256 vc4 = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.c4);
  const __m256 vc3 = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.c3);
  const __m256 vc2 = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.c2);
  const __m256 vtwo = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.two);
  const __m256 vminus_one = _mm256_load_ps(params->avx_expm1minus_rr1_lut8_p4h3.minus_one);

  for (; batch >= 80 * sizeof(float); batch -= 80 * sizeof(float)) {
    const __m256 vx0 = _mm256_loadu_ps(input);
    const __m256 vx1 = _mm256_loadu_ps(input + 8);
    const __m256 vx2 = _mm256_loadu_ps(input + 16);
    const __m256 vx3 = _mm256_loadu_ps(input + 24);
    const __m256 vx4 = _mm256_loadu_ps(input + 32);
    const __m256 vx5 = _mm256_loadu_ps(input + 40);
    const __m256 vx6 = _mm256_loadu_ps(input + 48);
    const __m256 vx7 = _mm256_loadu_ps(input + 56);
    const __m256 vx8 = _mm256_loadu_ps(input + 64);
    const __m256 vx9 = _mm256_loadu_ps(input + 72);
    input += 80;

    __m256 vz0 = _mm256_or_ps(vx0, vsign_mask);
    __m256 vz1 = _mm256_or_ps(vx1, vsign_mask);
    __m256 vz2 = _mm256_or_ps(vx2, vsign_mask);
    __m256 vz3 = _mm256_or_ps(vx3, vsign_mask);
    __m256 vz4 = _mm256_or_ps(vx4, vsign_mask);
    __m256 vz5 = _mm256_or_ps(vx5, vsign_mask);
    __m256 vz6 = _mm256_or_ps(vx6, vsign_mask);
    __m256 vz7 = _mm256_or_ps(vx7, vsign_mask);
    __m256 vz8 = _mm256_or_ps(vx8, vsign_mask);
    __m256 vz9 = _mm256_or_ps(vx9, vsign_mask);

    const __m256 vinvsignx0 = _mm256_xor_ps(vx0, vz0);
    vz0 = _mm256_max_ps(vsat_cutoff, vz0);
    const __m256 vinvsignx1 = _mm256_xor_ps(vx1, vz1);
    vz1 = _mm256_max_ps(vsat_cutoff, vz1);
    const __m256 vinvsignx2 = _mm256_xor_ps(vx2, vz2);
    vz2 = _mm256_max_ps(vsat_cutoff, vz2);
    const __m256 vinvsignx3 = _mm256_xor_ps(vx3, vz3);
    vz3 = _mm256_max_ps(vsat_cutoff, vz3);
    const __m256 vinvsignx4 = _mm256_xor_ps(vx4, vz4);
    vz4 = _mm256_max_ps(vsat_cutoff, vz4);
    const __m256 vinvsignx5 = _mm256_xor_ps(vx5, vz5);
    vz5 = _mm256_max_ps(vsat_cutoff, vz5);
    const __m256 vinvsignx6 = _mm256_xor_ps(vx6, vz6);
    vz6 = _mm256_max_ps(vsat_cutoff, vz6);
    const __m256 vinvsignx7 = _mm256_xor_ps(vx7, vz7);
    vz7 = _mm256_max_ps(vsat_cutoff, vz7);
    const __m256 vinvsignx8 = _mm256_xor_ps(vx8, vz8);
    vz8 = _mm256_max_ps(vsat_cutoff, vz8);
    const __m256 vinvsignx9 = _mm256_xor_ps(vx9, vz9);
    vz9 = _mm256_max_ps(vsat_cutoff, vz9);

    __m256 vn0 = _mm256_fmadd_ps(vz0, vlog2e, vmagic_bias);
    __m256 vn1 = _mm256_fmadd_ps(vz1, vlog2e, vmagic_bias);
    __m256 vn2 = _mm256_fmadd_ps(vz2, vlog2e, vmagic_bias);
    __m256 vn3 = _mm256_fmadd_ps(vz3, vlog2e, vmagic_bias);
    __m256 vn4 = _mm256_fmadd_ps(vz4, vlog2e, vmagic_bias);
    __m256 vn5 = _mm256_fmadd_ps(vz5, vlog2e, vmagic_bias);
    __m256 vn6 = _mm256_fmadd_ps(vz6, vlog2e, vmagic_bias);
    __m256 vn7 = _mm256_fmadd_ps(vz7, vlog2e, vmagic_bias);
    __m256 vn8 = _mm256_fmadd_ps(vz8, vlog2e, vmagic_bias);
    __m256 vn9 = _mm256_fmadd_ps(vz9, vlog2e, vmagic_bias);

    const __m256i ve0 = _mm256_slli_epi32(_mm256_castps_si256(vn0), 20);
    const __m256i ve1 = _mm256_slli_epi32(_mm256_castps_si256(vn1), 20);
    const __m256i ve2 = _mm256_slli_epi32(_mm256_castps_si256(vn2), 20);
    const __m256i ve3 = _mm256_slli_epi32(_mm256_castps_si256(vn3), 20);
    const __m256i ve4 = _mm256_slli_epi32(_mm256_castps_si256(vn4), 20);
    const __m256i ve5 = _mm256_slli_epi32(_mm256_castps_si256(vn5), 20);
    const __m256i ve6 = _mm256_slli_epi32(_mm256_castps_si256(vn6), 20);
    const __m256i ve7 = _mm256_slli_epi32(_mm256_castps_si256(vn7), 20);
    const __m256i ve8 = _mm256_slli_epi32(_mm256_castps_si256(vn8), 20);
    const __m256i ve9 = _mm256_slli_epi32(_mm256_castps_si256(vn9), 20);

    const __m256i vidx0 = _mm256_and_si256(_mm256_castps_si256(vn0), vindex_mask);
    const __m256i vidx1 = _mm256_and_si256(_mm256_castps_si256(vn1), vindex_mask);
    const __m256i vidx2 = _mm256_and_si256(_mm256_castps_si256(vn2), vindex_mask);
    const __m256i vidx3 = _mm256_and_si256(_mm256_castps_si256(vn3), vindex_mask);
    const __m256i vidx4 = _mm256_and_si256(_mm256_castps_si256(vn4), vindex_mask);
    const __m256i vidx5 = _mm256_and_si256(_mm256_castps_si256(vn5), vindex_mask);
    const __m256i vidx6 = _mm256_and_si256(_mm256_castps_si256(vn6), vindex_mask);
    const __m256i vidx7 = _mm256_and_si256(_mm256_castps_si256(vn7), vindex_mask);
    const __m256i vidx8 = _mm256_and_si256(_mm256_castps_si256(vn8), vindex_mask);
    const __m256i vidx9 = _mm256_and_si256(_mm256_castps_si256(vn9), vindex_mask);
    const __m256i vl0 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx0, sizeof(uint32_t));
    const __m256i vl1 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx1, sizeof(uint32_t));
    const __m256i vl2 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx2, sizeof(uint32_t));
    const __m256i vl3 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx3, sizeof(uint32_t));
    const __m256i vl4 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx4, sizeof(uint32_t));
    const __m256i vl5 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx5, sizeof(uint32_t));
    const __m256i vl6 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx6, sizeof(uint32_t));
    const __m256i vl7 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx7, sizeof(uint32_t));
    const __m256i vl8 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx8, sizeof(uint32_t));
    const __m256i vl9 = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx9, sizeof(uint32_t));

    const __m256 vs0 = _mm256_castsi256_ps(_mm256_add_epi32(vl0, ve0));
    const __m256 vs1 = _mm256_castsi256_ps(_mm256_add_epi32(vl1, ve1));
    const __m256 vs2 = _mm256_castsi256_ps(_mm256_add_epi32(vl2, ve2));
    const __m256 vs3 = _mm256_castsi256_ps(_mm256_add_epi32(vl3, ve3));
    const __m256 vs4 = _mm256_castsi256_ps(_mm256_add_epi32(vl4, ve4));
    const __m256 vs5 = _mm256_castsi256_ps(_mm256_add_epi32(vl5, ve5));
    const __m256 vs6 = _mm256_castsi256_ps(_mm256_add_epi32(vl6, ve6));
    const __m256 vs7 = _mm256_castsi256_ps(_mm256_add_epi32(vl7, ve7));
    const __m256 vs8 = _mm256_castsi256_ps(_mm256_add_epi32(vl8, ve8));
    const __m256 vs9 = _mm256_castsi256_ps(_mm256_add_epi32(vl9, ve9));

    vn0 = _mm256_sub_ps(vn0, vmagic_bias);
    vn1 = _mm256_sub_ps(vn1, vmagic_bias);
    vn2 = _mm256_sub_ps(vn2, vmagic_bias);
    vn3 = _mm256_sub_ps(vn3, vmagic_bias);
    vn4 = _mm256_sub_ps(vn4, vmagic_bias);
    vn5 = _mm256_sub_ps(vn5, vmagic_bias);
    vn6 = _mm256_sub_ps(vn6, vmagic_bias);
    vn7 = _mm256_sub_ps(vn7, vmagic_bias);
    vn8 = _mm256_sub_ps(vn8, vmagic_bias);
    vn9 = _mm256_sub_ps(vn9, vmagic_bias);

    const __m256 vt0 = _mm256_fmadd_ps(vn0, vminus_ln2, vz0);
    const __m256 vt1 = _mm256_fmadd_ps(vn1, vminus_ln2, vz1);
    const __m256 vt2 = _mm256_fmadd_ps(vn2, vminus_ln2, vz2);
    const __m256 vt3 = _mm256_fmadd_ps(vn3, vminus_ln2, vz3);
    const __m256 vt4 = _mm256_fmadd_ps(vn4, vminus_ln2, vz4);
    const __m256 vt5 = _mm256_fmadd_ps(vn5, vminus_ln2, vz5);
    const __m256 vt6 = _mm256_fmadd_ps(vn6, vminus_ln2, vz6);
    const __m256 vt7 = _mm256_fmadd_ps(vn7, vminus_ln2, vz7);
    const __m256 vt8 = _mm256_fmadd_ps(vn8, vminus_ln2, vz8);
    const __m256 vt9 = _mm256_fmadd_ps(vn9, vminus_ln2, vz9);

    __m256 vp0 = vc4;
    __m256 vp1 = vc4;
    __m256 vp2 = vc4;
    __m256 vp3 = vc4;
    __m256 vp4 = vc4;
    __m256 vp5 = vc4;
    __m256 vp6 = vc4;
    __m256 vp7 = vc4;
    __m256 vp8 = vc4;
    __m256 vp9 = vc4;
    vp0 = _mm256_fmadd_ps(vp0, vt0, vc3);
    vp1 = _mm256_fmadd_ps(vp1, vt1, vc3);
    vp2 = _mm256_fmadd_ps(vp2, vt2, vc3);
    vp3 = _mm256_fmadd_ps(vp3, vt3, vc3);
    vp4 = _mm256_fmadd_ps(vp4, vt4, vc3);
    vp5 = _mm256_fmadd_ps(vp5, vt5, vc3);
    vp6 = _mm256_fmadd_ps(vp6, vt6, vc3);
    vp7 = _mm256_fmadd_ps(vp7, vt7, vc3);
    vp8 = _mm256_fmadd_ps(vp8, vt8, vc3);
    vp9 = _mm256_fmadd_ps(vp9, vt9, vc3);
    vp0 = _mm256_fmadd_ps(vp0, vt0, vc2);
    vp1 = _mm256_fmadd_ps(vp1, vt1, vc2);
    vp2 = _mm256_fmadd_ps(vp2, vt2, vc2);
    vp3 = _mm256_fmadd_ps(vp3, vt3, vc2);
    vp4 = _mm256_fmadd_ps(vp4, vt4, vc2);
    vp5 = _mm256_fmadd_ps(vp5, vt5, vc2);
    vp6 = _mm256_fmadd_ps(vp6, vt6, vc2);
    vp7 = _mm256_fmadd_ps(vp7, vt7, vc2);
    vp8 = _mm256_fmadd_ps(vp8, vt8, vc2);
    vp9 = _mm256_fmadd_ps(vp9, vt9, vc2);
    vp0 = _mm256_fmadd_ps(vp0, vt0, vtwo);
    vp1 = _mm256_fmadd_ps(vp1, vt1, vtwo);
    vp2 = _mm256_fmadd_ps(vp2, vt2, vtwo);
    vp3 = _mm256_fmadd_ps(vp3, vt3, vtwo);
    vp4 = _mm256_fmadd_ps(vp4, vt4, vtwo);
    vp5 = _mm256_fmadd_ps(vp5, vt5, vtwo);
    vp6 = _mm256_fmadd_ps(vp6, vt6, vtwo);
    vp7 = _mm256_fmadd_ps(vp7, vt7, vtwo);
    vp8 = _mm256_fmadd_ps(vp8, vt8, vtwo);
    vp9 = _mm256_fmadd_ps(vp9, vt9, vtwo);

    const __m256 vts0 = _mm256_mul_ps(vt0, vs0);
    const __m256 vsmo0 = _mm256_add_ps(vs0, vminus_one);
    const __m256 vts1 = _mm256_mul_ps(vt1, vs1);
    const __m256 vsmo1 = _mm256_add_ps(vs1, vminus_one);
    const __m256 vts2 = _mm256_mul_ps(vt2, vs2);
    const __m256 vsmo2 = _mm256_add_ps(vs2, vminus_one);
    const __m256 vts3 = _mm256_mul_ps(vt3, vs3);
    const __m256 vsmo3 = _mm256_add_ps(vs3, vminus_one);
    const __m256 vts4 = _mm256_mul_ps(vt4, vs4);
    const __m256 vsmo4 = _mm256_add_ps(vs4, vminus_one);
    const __m256 vts5 = _mm256_mul_ps(vt5, vs5);
    const __m256 vsmo5 = _mm256_add_ps(vs5, vminus_one);
    const __m256 vts6 = _mm256_mul_ps(vt6, vs6);
    const __m256 vsmo6 = _mm256_add_ps(vs6, vminus_one);
    const __m256 vts7 = _mm256_mul_ps(vt7, vs7);
    const __m256 vsmo7 = _mm256_add_ps(vs7, vminus_one);
    const __m256 vts8 = _mm256_mul_ps(vt8, vs8);
    const __m256 vsmo8 = _mm256_add_ps(vs8, vminus_one);
    const __m256 vts9 = _mm256_mul_ps(vt9, vs9);
    const __m256 vsmo9 = _mm256_add_ps(vs9, vminus_one);
    const __m256 vemo0 = _mm256_fmadd_ps(vp0, vts0, vsmo0);
    const __m256 vemo1 = _mm256_fmadd_ps(vp1, vts1, vsmo1);
    const __m256 vemo2 = _mm256_fmadd_ps(vp2, vts2, vsmo2);
    const __m256 vemo3 = _mm256_fmadd_ps(vp3, vts3, vsmo3);
    const __m256 vemo4 = _mm256_fmadd_ps(vp4, vts4, vsmo4);
    const __m256 vemo5 = _mm256_fmadd_ps(vp5, vts5, vsmo5);
    const __m256 vemo6 = _mm256_fmadd_ps(vp6, vts6, vsmo6);
    const __m256 vemo7 = _mm256_fmadd_ps(vp7, vts7, vsmo7);
    const __m256 vemo8 = _mm256_fmadd_ps(vp8, vts8, vsmo8);
    const __m256 vemo9 = _mm256_fmadd_ps(vp9, vts9, vsmo9);
    const __m256 vepo0 = _mm256_add_ps(vemo0, vtwo);
    const __m256 vepo1 = _mm256_add_ps(vemo1, vtwo);
    const __m256 vepo2 = _mm256_add_ps(vemo2, vtwo);
    const __m256 vepo3 = _mm256_add_ps(vemo3, vtwo);
    const __m256 vepo4 = _mm256_add_ps(vemo4, vtwo);
    const __m256 vepo5 = _mm256_add_ps(vemo5, vtwo);
    const __m256 vepo6 = _mm256_add_ps(vemo6, vtwo);
    const __m256 vepo7 = _mm256_add_ps(vemo7, vtwo);
    const __m256 vepo8 = _mm256_add_ps(vemo8, vtwo);
    const __m256 vepo9 = _mm256_add_ps(vemo9, vtwo);

    __m256 vy0 = _mm256_div_ps(vemo0, vepo0);
    __m256 vy1 = _mm256_div_ps(vemo1, vepo1);
    __m256 vy2 = _mm256_div_ps(vemo2, vepo2);
    __m256 vy3 = _mm256_div_ps(vemo3, vepo3);
    __m256 vy4 = _mm256_div_ps(vemo4, vepo4);
    __m256 vy5 = _mm256_div_ps(vemo5, vepo5);
    __m256 vy6 = _mm256_div_ps(vemo6, vepo6);
    __m256 vy7 = _mm256_div_ps(vemo7, vepo7);
    __m256 vy8 = _mm256_div_ps(vemo8, vepo8);
    __m256 vy9 = _mm256_div_ps(vemo9, vepo9);

    vy0 = _mm256_xor_ps(vy0, vinvsignx0);
    vy1 = _mm256_xor_ps(vy1, vinvsignx1);
    vy2 = _mm256_xor_ps(vy2, vinvsignx2);
    vy3 = _mm256_xor_ps(vy3, vinvsignx3);
    vy4 = _mm256_xor_ps(vy4, vinvsignx4);
    vy5 = _mm256_xor_ps(vy5, vinvsignx5);
    vy6 = _mm256_xor_ps(vy6, vinvsignx6);
    vy7 = _mm256_xor_ps(vy7, vinvsignx7);
    vy8 = _mm256_xor_ps(vy8, vinvsignx8);
    vy9 = _mm256_xor_ps(vy9, vinvsignx9);

    _mm256_storeu_ps(output, vy0);
    _mm256_storeu_ps(output + 8, vy1);
    _mm256_storeu_ps(output + 16, vy2);
    _mm256_storeu_ps(output + 24, vy3);
    _mm256_storeu_ps(output + 32, vy4);
    _mm256_storeu_ps(output + 40, vy5);
    _mm256_storeu_ps(output + 48, vy6);
    _mm256_storeu_ps(output + 56, vy7);
    _mm256_storeu_ps(output + 64, vy8);
    _mm256_storeu_ps(output + 72, vy9);
    output += 80;
  }
  for (; batch >= 8 * sizeof(float); batch -= 8 * sizeof(float)) {
    const __m256 vx = _mm256_loadu_ps(input);
    input += 8;

    __m256 vz = _mm256_or_ps(vx, vsign_mask);

    const __m256 vinvsignx = _mm256_xor_ps(vx, vz);
    vz = _mm256_max_ps(vsat_cutoff, vz);

    __m256 vn = _mm256_fmadd_ps(vz, vlog2e, vmagic_bias);

    const __m256i ve = _mm256_slli_epi32(_mm256_castps_si256(vn), 20);

    const __m256i vidx = _mm256_and_si256(_mm256_castps_si256(vn), vindex_mask);
    const __m256i vl = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx, sizeof(uint32_t));

    const __m256 vs = _mm256_castsi256_ps(_mm256_add_epi32(vl, ve));

    vn = _mm256_sub_ps(vn, vmagic_bias);

    const __m256 vt = _mm256_fmadd_ps(vn, vminus_ln2, vz);

    __m256 vp = vc4;
    vp = _mm256_fmadd_ps(vp, vt, vc3);
    vp = _mm256_fmadd_ps(vp, vt, vc2);
    vp = _mm256_fmadd_ps(vp, vt, vtwo);

    const __m256 vts = _mm256_mul_ps(vt, vs);
    const __m256 vsmo = _mm256_add_ps(vs, vminus_one);
    const __m256 vemo = _mm256_fmadd_ps(vp, vts, vsmo);
    const __m256 vepo = _mm256_add_ps(vemo, vtwo);

    __m256 vy = _mm256_div_ps(vemo, vepo);

    vy = _mm256_xor_ps(vy, vinvsignx);

    _mm256_storeu_ps(output, vy);
    output += 8;
  }
  if XNN_UNLIKELY(batch != 0) {
    assert(batch >= 1 * sizeof(float));
    assert(batch <= 7 * sizeof(float));
    const __m256i vmask = _mm256_loadu_si256((const __m256i*) ((uintptr_t) &params->avx_expm1minus_rr1_lut8_p4h3.mask_table[7] - batch));

    const __m256 vx = _mm256_maskload_ps(input, vmask);

    __m256 vz = _mm256_or_ps(vx, vsign_mask);

    const __m256 vinvsignx = _mm256_xor_ps(vx, vz);
    vz = _mm256_max_ps(vsat_cutoff, vz);

    __m256 vn = _mm256_fmadd_ps(vz, vlog2e, vmagic_bias);

    const __m256i ve = _mm256_slli_epi32(_mm256_castps_si256(vn), 20);

    const __m256i vidx = _mm256_and_si256(_mm256_castps_si256(vn), vindex_mask);
    const __m256i vl = _mm256_i32gather_epi32((const int*) xnn_table_exp2minus_k_over_8, vidx, sizeof(uint32_t));

    const __m256 vs = _mm256_castsi256_ps(_mm256_add_epi32(vl, ve));

    vn = _mm256_sub_ps(vn, vmagic_bias);

    const __m256 vt = _mm256_fmadd_ps(vn, vminus_ln2, vz);

    __m256 vp = vc4;
    vp = _mm256_fmadd_ps(vp, vt, vc3);
    vp = _mm256_fmadd_ps(vp, vt, vc2);
    vp = _mm256_fmadd_ps(vp, vt, vtwo);

    const __m256 vts = _mm256_mul_ps(vt, vs);
    const __m256 vsmo = _mm256_add_ps(vs, vminus_one);
    const __m256 vemo = _mm256_fmadd_ps(vp, vts, vsmo);
    const __m256 vepo = _mm256_add_ps(vemo, vtwo);

    __m256 vy = _mm256_div_ps(vemo, vepo);

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
