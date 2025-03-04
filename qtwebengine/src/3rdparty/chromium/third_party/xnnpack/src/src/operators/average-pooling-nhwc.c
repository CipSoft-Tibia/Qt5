// Copyright (c) Facebook, Inc. and its affiliates.
// All rights reserved.
//
// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fp16.h>

#include <xnnpack.h>
#include <xnnpack/allocator.h>
#include <xnnpack/config.h>
#include <xnnpack/operator.h>
#include <xnnpack/operator-type.h>
#include <xnnpack/operator-utils.h>
#include <xnnpack/common.h>
#include <xnnpack/log.h>
#include <xnnpack/math.h>
#include <xnnpack/microparams-init.h>
#include <xnnpack/params.h>
#include <xnnpack/indirection.h>


static inline size_t compute_output_dimension_with_tf_same_padding(
    size_t input_dimension,
    size_t stride_dimension)
{
  return divide_round_up(input_dimension, stride_dimension);
}

enum xnn_status xnn_create_average_pooling2d_nhwc_qu8(
    uint32_t input_padding_top,
    uint32_t input_padding_right,
    uint32_t input_padding_bottom,
    uint32_t input_padding_left,
    uint32_t pooling_height,
    uint32_t pooling_width,
    uint32_t stride_height,
    uint32_t stride_width,
    size_t channels,
    size_t input_pixel_stride,
    size_t output_pixel_stride,
    uint8_t input_zero_point,
    float input_scale,
    uint8_t output_zero_point,
    float output_scale,
    uint8_t output_min,
    uint8_t output_max,
    uint32_t flags,
    xnn_operator_t* average_pooling_op_out)
{
  xnn_operator_t average_pooling_op = NULL;
  enum xnn_status status = xnn_status_uninitialized;

  if ((xnn_params.init_flags & XNN_INIT_FLAG_XNNPACK) == 0) {
    xnn_log_error("failed to create %s operator: XNNPACK is not initialized",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8));
    goto error;
  }

  status = xnn_status_invalid_parameter;

  const uint32_t pooling_size = pooling_height * pooling_width;
  if (pooling_size == 0) {
    xnn_log_error(
      "failed to create %s operator with %" PRIu32 "x%" PRIu32 " pooling size: "
      "pooling size dimensions must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), pooling_width, pooling_height);
    goto error;
  }

  if (pooling_size == 1) {
    xnn_log_error(
      "failed to create %s operator with 1 pooling element: 1x1 pooling is meaningless",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8));
    goto error;
  }

  if (stride_height == 0 || stride_width == 0) {
    xnn_log_error(
      "failed to create %s operator with %" PRIu32 "x%" PRIu32 " stride: stride dimensions must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), stride_width, stride_height);
    goto error;
  }

  if (stride_height > pooling_height) {
    xnn_log_error(
      "failed to define %s operator with %" PRIu32 " stride height: must be less than pooling height %" PRIu32,
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), stride_height, pooling_height);
    return xnn_status_invalid_parameter;
  }

  if (stride_width > pooling_width) {
    xnn_log_error(
      "failed to define %s operator with %" PRIu32 " stride width: must be less than pooling width %" PRIu32,
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), stride_width, pooling_width);
    return xnn_status_invalid_parameter;
  }

  if (channels == 0) {
    xnn_log_error(
      "failed to create %s operator with %zu channels: number of channels must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), channels);
    goto error;
  }

  if (input_pixel_stride < channels) {
    xnn_log_error(
      "failed to create %s operator with input pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), input_pixel_stride, channels);
    goto error;
  }

  if (output_pixel_stride < channels) {
    xnn_log_error(
      "failed to create %s operator with output pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), output_pixel_stride, channels);
    goto error;
  }

  if (input_scale <= 0.0f || !isnormal(input_scale)) {
    xnn_log_error(
      "failed to create %s operator with %.7g input scale: scale must be finite, normalized, and positive",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), input_scale);
    goto error;
  }

  if (output_scale <= 0.0f || !isnormal(output_scale)) {
    xnn_log_error(
      "failed to create %s operator with %.7g output scale: scale must be finite, normalized, and positive",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), output_scale);
    goto error;
  }

  if (output_min >= output_max) {
    xnn_log_error(
      "failed to create %s operator with [%" PRIu8 ", %" PRIu8 "] output range: range min must be below range max",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8), output_min, output_max);
    goto error;
  }

  const bool any_padding = (input_padding_left | input_padding_top | input_padding_right | input_padding_bottom) != 0;
  if ((flags & XNN_FLAG_TENSORFLOW_SAME_PADDING) != 0) {
    if (any_padding) {
      xnn_log_error(
        "failed to create %s operator with %" PRIu32 "+%" PRIu32 "x%" PRIu32 "+%" PRIu32" padding: "
        "TensorFlow SAME padding can't be combined with explicit padding specification",
        xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8),
        input_padding_top, input_padding_left, input_padding_bottom, input_padding_right);
      goto error;
    }
  }

  status = xnn_status_unsupported_parameter;

  const float input_output_scale = input_scale / output_scale;
  if (input_output_scale < 0x1.0p-8f || input_output_scale >= 0x1.0p+8f) {
    xnn_log_error(
      "failed to create %s operator with %.7g input scale and %.7g output scale: "
      "input-to-output scale ratio (%.7f) must be in [2**-8, 2**8) range",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8),
      input_scale, output_scale, input_output_scale);
    goto error;
  }

  if (pooling_size >= 16777216) {
    xnn_log_error(
      "failed to create %s operator with %"PRIu32" (%" PRIu32 "x%" PRIu32 ") pooling elements: "
      "the number of elements in the pooling area must be below 2**24",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8),
      pooling_size, pooling_width, pooling_height);
    goto error;
  }

  status = xnn_status_out_of_memory;

  average_pooling_op = xnn_allocate_zero_simd_memory(sizeof(struct xnn_operator));
  if (average_pooling_op == NULL) {
    xnn_log_error(
      "failed to allocate %zu bytes for %s operator descriptor",
      sizeof(struct xnn_operator), xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8));
    goto error;
  }

  const size_t zero_bytes = channels * sizeof(uint8_t) + XNN_EXTRA_BYTES;
  void* zero_buffer = xnn_allocate_simd_memory(zero_bytes);
  if (zero_buffer == NULL) {
    xnn_log_error(
      "failed to allocate %zu bytes for %s operator zero padding",
      zero_bytes, xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8));
    goto error;
  }
  memset(zero_buffer, input_zero_point, channels * sizeof(uint8_t));
  average_pooling_op->zero_buffer = zero_buffer;

  average_pooling_op->padding_top = input_padding_top;
  average_pooling_op->padding_right = input_padding_right;
  average_pooling_op->padding_bottom = input_padding_bottom;
  average_pooling_op->padding_left = input_padding_left;

  average_pooling_op->kernel_height = pooling_height;
  average_pooling_op->kernel_width = pooling_width;
  average_pooling_op->stride_height = stride_height;
  average_pooling_op->stride_width = stride_width;
  average_pooling_op->dilation_height = 1;
  average_pooling_op->dilation_width = 1;
  average_pooling_op->channels = channels;
  average_pooling_op->input_pixel_stride = input_pixel_stride;
  average_pooling_op->output_pixel_stride = output_pixel_stride;

  average_pooling_op->input_zero_point = (int32_t) (uint32_t) input_zero_point;
  average_pooling_op->input_scale = input_scale;
  average_pooling_op->output_scale = output_scale;

  const struct xnn_avgpool_config* avgpool_config = xnn_init_qu8_avgpool_config();
  assert(avgpool_config != NULL);
  average_pooling_op->avgpool_config = avgpool_config;

  const struct xnn_gavgpool_config* gavgpool_config = xnn_init_qu8_gavgpool_config();
  assert(gavgpool_config != NULL);
  average_pooling_op->gavgpool_config = gavgpool_config;

  // Number of rows read in the AVGPOOL micro-kernel.
  const size_t avgpool_nrows =
    round_up(doz(pooling_size, avgpool_config->primary_tile), avgpool_config->incremental_tile) + avgpool_config->primary_tile;
  const float requantization_scale = input_scale / (output_scale * (float) pooling_size);
  avgpool_config->init.qu8(&average_pooling_op->params.qu8_avgpool,
    (int32_t) -((uint32_t) input_zero_point * (uint32_t) avgpool_nrows),
    requantization_scale, output_zero_point, output_min, output_max);
  gavgpool_config->init.qu8(&average_pooling_op->params.qu8_gavgpool,
    0 /* bias */, requantization_scale, output_zero_point, output_min, output_max);

  average_pooling_op->type = xnn_operator_type_average_pooling_nhwc_qu8;
  average_pooling_op->ukernel.type = xnn_microkernel_type_average_pooling;
  average_pooling_op->flags = flags;

  *average_pooling_op_out = average_pooling_op;
  return xnn_status_success;

error:
  xnn_delete_operator(average_pooling_op);
  return status;
}

enum xnn_status xnn_create_average_pooling2d_nhwc_f16(
    uint32_t input_padding_top,
    uint32_t input_padding_right,
    uint32_t input_padding_bottom,
    uint32_t input_padding_left,
    uint32_t pooling_height,
    uint32_t pooling_width,
    uint32_t stride_height,
    uint32_t stride_width,
    size_t channels,
    size_t input_pixel_stride,
    size_t output_pixel_stride,
    float output_min,
    float output_max,
    uint32_t flags,
    xnn_operator_t* average_pooling_op_out)
{
  xnn_operator_t average_pooling_op = NULL;
  enum xnn_status status = xnn_status_uninitialized;

  if ((xnn_params.init_flags & XNN_INIT_FLAG_XNNPACK) == 0) {
    xnn_log_error("failed to create %s operator: XNNPACK is not initialized",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    goto error;
  }

  status = xnn_status_invalid_parameter;

  const uint32_t pooling_size = pooling_height * pooling_width;
  if (pooling_size == 0) {
    xnn_log_error(
      "failed to create %s operator with %" PRIu32 "x%" PRIu32 " pooling size: "
      "pooling size dimensions must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16), pooling_width, pooling_height);
    goto error;
  }

  if (pooling_size == 1) {
    xnn_log_error(
      "failed to create %s operator with 1 pooling element: 1x1 pooling is meaningless",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    goto error;
  }

  if (stride_height == 0 || stride_width == 0) {
    xnn_log_error(
      "failed to create %s operator with %" PRIu32 "x%" PRIu32 " stride: stride dimensions must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16), stride_width, stride_height);
    goto error;
  }

  if (stride_height > pooling_height) {
    xnn_log_error(
      "failed to define %s operator with %" PRIu32 " stride height: must be less than pooling height %" PRIu32,
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16), stride_height, pooling_height);
    return xnn_status_invalid_parameter;
  }

  if (stride_width > pooling_width) {
    xnn_log_error(
      "failed to define %s operator with %" PRIu32 " stride width: must be less than pooling width %" PRIu32,
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16), stride_width, pooling_width);
    return xnn_status_invalid_parameter;
  }

  if (channels == 0) {
    xnn_log_error(
      "failed to create %s operator with %zu channels: number of channels must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16), channels);
    goto error;
  }

  if (input_pixel_stride < channels) {
    xnn_log_error(
      "failed to create %s operator with input pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16), input_pixel_stride, channels);
    goto error;
  }

  if (output_pixel_stride < channels) {
    xnn_log_error(
      "failed to create %s operator with output pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16), output_pixel_stride, channels);
    goto error;
  }

  if (isnan(output_min)) {
    xnn_log_error(
      "failed to create %s operator with NaN output lower bound: lower bound must be non-NaN",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    goto error;
  }

  if (isnan(output_max)) {
    xnn_log_error(
      "failed to create %s operator with NaN output upper bound: upper bound must be non-NaN",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    goto error;
  }

  const uint16_t fp16_output_min = fp16_ieee_from_fp32_value(output_min);
  const uint16_t fp16_output_max = fp16_ieee_from_fp32_value(output_max);
  const float rounded_output_min = fp16_ieee_to_fp32_value(fp16_output_min);
  const float rounded_output_max = fp16_ieee_to_fp32_value(fp16_output_max);
  if (rounded_output_min >= rounded_output_max) {
    xnn_log_error(
      "failed to create %s operator with [%.7g, %.7g] output range: lower bound must be below upper bound",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16), rounded_output_min, rounded_output_max);
    goto error;
  }

  const bool any_padding = (input_padding_left | input_padding_top | input_padding_right | input_padding_bottom) != 0;
  if ((flags & XNN_FLAG_TENSORFLOW_SAME_PADDING) != 0) {
    if (any_padding) {
      xnn_log_error(
        "failed to create %s operator with %" PRIu32 "+%" PRIu32 "x%" PRIu32 "+%" PRIu32" padding: "
        "TensorFlow SAME padding can't be combined with explicit padding specification",
        xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16),
        input_padding_top, input_padding_left, input_padding_bottom, input_padding_right);
      goto error;
    }
  }

  status = xnn_status_out_of_memory;

  average_pooling_op = xnn_allocate_zero_simd_memory(sizeof(struct xnn_operator));
  if (average_pooling_op == NULL) {
    xnn_log_error(
      "failed to allocate %zu bytes for %s operator descriptor",
      sizeof(struct xnn_operator), xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    goto error;
  }

  const size_t zero_bytes = channels * sizeof(uint16_t) + XNN_EXTRA_BYTES;
  void* zero_buffer = xnn_allocate_zero_simd_memory(zero_bytes);
  if (zero_buffer == NULL) {
    xnn_log_error(
      "failed to allocate %zu bytes for %s operator zero padding",
      zero_bytes, xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    goto error;
  }
  average_pooling_op->zero_buffer = zero_buffer;

  average_pooling_op->padding_top = input_padding_top;
  average_pooling_op->padding_right = input_padding_right;
  average_pooling_op->padding_bottom = input_padding_bottom;
  average_pooling_op->padding_left = input_padding_left;

  average_pooling_op->kernel_height = pooling_height;
  average_pooling_op->kernel_width = pooling_width;
  average_pooling_op->stride_height = stride_height;
  average_pooling_op->stride_width = stride_width;
  average_pooling_op->dilation_height = 1;
  average_pooling_op->dilation_width = 1;
  average_pooling_op->channels = channels;
  average_pooling_op->input_pixel_stride = input_pixel_stride;
  average_pooling_op->output_pixel_stride = output_pixel_stride;

  average_pooling_op->type = xnn_operator_type_average_pooling_nhwc_f16;

  const struct xnn_avgpool_config* avgpool_config = xnn_init_f16_avgpool_config();
  if (avgpool_config == NULL) {
    xnn_log_error("failed to create %s operator: unsupported hardware configuration",
                  xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    return xnn_status_unsupported_hardware;
  }
  average_pooling_op->avgpool_config = avgpool_config;

  const struct xnn_pavgpool_config* pavgpool_config = xnn_init_f16_pavgpool_config();
  if (pavgpool_config == NULL) {
    xnn_log_error("failed to create %s operator: unsupported hardware configuration",
                  xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    return xnn_status_unsupported_hardware;
  }
  average_pooling_op->pavgpool_config = pavgpool_config;

  const struct xnn_gavgpool_config* gavgpool_config = xnn_init_f16_gavgpool_config();
  if (gavgpool_config == NULL) {
    xnn_log_error("failed to create %s operator: unsupported hardware configuration",
                  xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16));
    return xnn_status_unsupported_hardware;
  }
  average_pooling_op->gavgpool_config = gavgpool_config;

  avgpool_config->init.f16(&average_pooling_op->params.f16_scaleminmax,
    fp16_ieee_from_fp32_value(1.0f / (float) (int32_t) pooling_size), fp16_output_min, fp16_output_max);
  const bool tf_same_padding = (flags & XNN_FLAG_TENSORFLOW_SAME_PADDING) != 0;
  if (any_padding || tf_same_padding) {
    pavgpool_config->init.f16(&average_pooling_op->params.f16_minmax, fp16_output_min, fp16_output_max);
    average_pooling_op->ukernel.type = xnn_microkernel_type_pixelwise_average_pooling;
  } else {
    average_pooling_op->ukernel.type = xnn_microkernel_type_average_pooling;
  }
  average_pooling_op->flags = flags;

  *average_pooling_op_out = average_pooling_op;
  return xnn_status_success;

error:
  xnn_delete_operator(average_pooling_op);
  return status;
}

enum xnn_status xnn_create_average_pooling2d_nhwc_f32(
    uint32_t input_padding_top,
    uint32_t input_padding_right,
    uint32_t input_padding_bottom,
    uint32_t input_padding_left,
    uint32_t pooling_height,
    uint32_t pooling_width,
    uint32_t stride_height,
    uint32_t stride_width,
    size_t channels,
    size_t input_pixel_stride,
    size_t output_pixel_stride,
    float output_min,
    float output_max,
    uint32_t flags,
    xnn_operator_t* average_pooling_op_out)
{
  xnn_operator_t average_pooling_op = NULL;
  enum xnn_status status = xnn_status_uninitialized;

  if ((xnn_params.init_flags & XNN_INIT_FLAG_XNNPACK) == 0) {
    xnn_log_error("failed to create %s operator: XNNPACK is not initialized",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    goto error;
  }

  status = xnn_status_invalid_parameter;

  const uint32_t pooling_size = pooling_height * pooling_width;
  if (pooling_size == 0) {
    xnn_log_error(
      "failed to create %s operator with %" PRIu32 "x%" PRIu32 " pooling size: "
      "pooling size dimensions must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32), pooling_width, pooling_height);
    goto error;
  }

  if (pooling_size == 1) {
    xnn_log_error(
      "failed to create %s operator with 1 pooling element: 1x1 pooling is meaningless",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    goto error;
  }

  if (stride_height == 0 || stride_width == 0) {
    xnn_log_error(
      "failed to create %s operator with %" PRIu32 "x%" PRIu32 " stride: stride dimensions must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32), stride_width, stride_height);
    goto error;
  }

  if (stride_height > pooling_height) {
    xnn_log_error(
      "failed to define %s operator with %" PRIu32 " stride height: must be less than pooling height %" PRIu32,
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32), stride_height, pooling_height);
    return xnn_status_invalid_parameter;
  }

  if (stride_width > pooling_width) {
    xnn_log_error(
      "failed to define %s operator with %" PRIu32 " stride width: must be less than pooling width %" PRIu32,
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32), stride_width, pooling_width);
    return xnn_status_invalid_parameter;
  }

  if (channels == 0) {
    xnn_log_error(
      "failed to create %s operator with %zu channels: number of channels must be non-zero",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32), channels);
    goto error;
  }

  if (input_pixel_stride < channels) {
    xnn_log_error(
      "failed to create %s operator with input pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32), input_pixel_stride, channels);
    goto error;
  }

  if (output_pixel_stride < channels) {
    xnn_log_error(
      "failed to create %s operator with output pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32), output_pixel_stride, channels);
    goto error;
  }

  if (isnan(output_min)) {
    xnn_log_error(
      "failed to create %s operator with NaN output lower bound: lower bound must be non-NaN",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    goto error;
  }

  if (isnan(output_max)) {
    xnn_log_error(
      "failed to create %s operator with NaN output upper bound: upper bound must be non-NaN",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    goto error;
  }

  if (output_min >= output_max) {
    xnn_log_error(
      "failed to create %s operator with [%.7g, %.7g] output range: lower bound must be below upper bound",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32), output_min, output_max);
    goto error;
  }

  const bool any_padding = (input_padding_left | input_padding_top | input_padding_right | input_padding_bottom) != 0;
  if ((flags & XNN_FLAG_TENSORFLOW_SAME_PADDING) != 0) {
    if (any_padding) {
      xnn_log_error(
        "failed to create %s operator with %" PRIu32 "+%" PRIu32 "x%" PRIu32 "+%" PRIu32" padding: "
        "TensorFlow SAME padding can't be combined with explicit padding specification",
        xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32),
        input_padding_top, input_padding_left, input_padding_bottom, input_padding_right);
      goto error;
    }
  }

  status = xnn_status_out_of_memory;

  average_pooling_op = xnn_allocate_zero_simd_memory(sizeof(struct xnn_operator));
  if (average_pooling_op == NULL) {
    xnn_log_error(
      "failed to allocate %zu bytes for %s operator descriptor",
      sizeof(struct xnn_operator), xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    goto error;
  }

  const size_t zero_bytes = channels * sizeof(float) + XNN_EXTRA_BYTES;
  void* zero_buffer = xnn_allocate_zero_simd_memory(zero_bytes);
  if (zero_buffer == NULL) {
    xnn_log_error(
      "failed to allocate %zu bytes for %s operator zero padding",
      zero_bytes, xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    goto error;
  }
  average_pooling_op->zero_buffer = zero_buffer;

  average_pooling_op->padding_top = input_padding_top;
  average_pooling_op->padding_right = input_padding_right;
  average_pooling_op->padding_bottom = input_padding_bottom;
  average_pooling_op->padding_left = input_padding_left;

  average_pooling_op->kernel_height = pooling_height;
  average_pooling_op->kernel_width = pooling_width;
  average_pooling_op->stride_height = stride_height;
  average_pooling_op->stride_width = stride_width;
  average_pooling_op->dilation_height = 1;
  average_pooling_op->dilation_width = 1;
  average_pooling_op->channels = channels;
  average_pooling_op->input_pixel_stride = input_pixel_stride;
  average_pooling_op->output_pixel_stride = output_pixel_stride;

  average_pooling_op->type = xnn_operator_type_average_pooling_nhwc_f32;

  const struct xnn_avgpool_config* avgpool_config = xnn_init_f32_avgpool_config();
  if (avgpool_config == NULL) {
    xnn_log_error("failed to create %s operator: unsupported hardware configuration",
                  xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    return xnn_status_unsupported_hardware;
  }
  average_pooling_op->avgpool_config = avgpool_config;

  const struct xnn_pavgpool_config* pavgpool_config = xnn_init_f32_pavgpool_config();
  if (pavgpool_config == NULL) {
    xnn_log_error("failed to create %s operator: unsupported hardware configuration",
                  xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    return xnn_status_unsupported_hardware;
  }
  average_pooling_op->pavgpool_config = pavgpool_config;

  const struct xnn_gavgpool_config* gavgpool_config = xnn_init_f32_gavgpool_config();
  if (gavgpool_config == NULL) {
    xnn_log_error("failed to create %s operator: unsupported hardware configuration",
                  xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32));
    return xnn_status_unsupported_hardware;
  }
  average_pooling_op->gavgpool_config = gavgpool_config;

  avgpool_config->init.f32(&average_pooling_op->params.f32_scaleminmax,
    1.0f / (float) (int32_t) pooling_size, output_min, output_max);
  const bool tf_same_padding = (flags & XNN_FLAG_TENSORFLOW_SAME_PADDING) != 0;
  if (any_padding || tf_same_padding) {
    pavgpool_config->init.f32(&average_pooling_op->params.f32_minmax, output_min, output_max);
    average_pooling_op->ukernel.type = xnn_microkernel_type_pixelwise_average_pooling;
  } else {
    average_pooling_op->ukernel.type = xnn_microkernel_type_average_pooling;
  }
  average_pooling_op->flags = flags;

  *average_pooling_op_out = average_pooling_op;
  return xnn_status_success;

error:
  xnn_delete_operator(average_pooling_op);
  return status;
}

static enum xnn_status setup_average_pooling2d(
  xnn_operator_t average_pooling_op,
  size_t batch_size,
  size_t input_height,
  size_t input_width,
  const void* input,
  void* output,
  uint32_t log2_data_element_size,
  uint32_t log2_weight_element_size,
  uint32_t log2_accumulator_element_size,
  xnn_indirection_init_pavgpool2d_fn indirection_init_pavgpool2d,
  const struct xnn_avgpool_config avgpool[ XNN_MIN_ELEMENTS(1)],
  const struct xnn_pavgpool_config pavgpool[ 1],
  const struct xnn_gavgpool_config gavgpool[ XNN_MIN_ELEMENTS(1)],
  const void* params,
  size_t params_size,
  const void* global_params,
  size_t global_params_size,
  size_t num_threads,
  bool is_pixelwise)
{
  assert(!is_pixelwise || pavgpool != NULL && indirection_init_pavgpool2d != NULL);

  average_pooling_op->state = xnn_run_state_invalid;

  if ((xnn_params.init_flags & XNN_INIT_FLAG_XNNPACK) == 0) {
    xnn_log_error("failed to setup %s operator: XNNPACK is not initialized",
      xnn_operator_type_to_string(average_pooling_op->type));
    return xnn_status_uninitialized;
  }

  if (input_width == 0 || input_height == 0) {
    xnn_log_error(
      "failed to setup %s operator with %zux%zu input: input dimensions must be non-zero",
      xnn_operator_type_to_string(average_pooling_op->type), input_width, input_height);
    return xnn_status_invalid_parameter;
  }

  if (batch_size == 0) {
    average_pooling_op->state = xnn_run_state_skip;
    return xnn_status_success;
  }

  average_pooling_op->input_height = input_height;
  average_pooling_op->input_width = input_width;
  average_pooling_op->input = input;

  const bool tf_same_padding = (average_pooling_op->flags & XNN_FLAG_TENSORFLOW_SAME_PADDING) != 0;
  if (tf_same_padding) {
    average_pooling_op->output_height = compute_output_dimension_with_tf_same_padding(
        input_height, average_pooling_op->stride_height);
    average_pooling_op->output_width = compute_output_dimension_with_tf_same_padding(
        input_width, average_pooling_op->stride_width);

    const uint32_t kernel_height = average_pooling_op->kernel_height;
    const uint32_t kernel_width = average_pooling_op->kernel_width;
    const uint32_t total_padding_height =
      (average_pooling_op->output_height - 1) * average_pooling_op->stride_height + kernel_height - input_height;
    const uint32_t total_padding_width =
      (average_pooling_op->output_width - 1) * average_pooling_op->stride_width + kernel_width - input_width;
    average_pooling_op->padding_top = total_padding_height / 2;
    average_pooling_op->padding_left = total_padding_width / 2;
    average_pooling_op->padding_bottom = total_padding_height - average_pooling_op->padding_top;
    average_pooling_op->padding_right = total_padding_width - average_pooling_op->padding_left;
  } else {
    average_pooling_op->output_height = xnn_compute_convolution_output_dimension(
        average_pooling_op->padding_top + input_height + average_pooling_op->padding_bottom,
        average_pooling_op->kernel_height,
        1,
        average_pooling_op->stride_height);
    average_pooling_op->output_width = xnn_compute_convolution_output_dimension(
        average_pooling_op->padding_left + input_width + average_pooling_op->padding_right,
        average_pooling_op->kernel_width,
        1,
        average_pooling_op->stride_width);
  }
  average_pooling_op->output = output;

  const size_t output_height = average_pooling_op->output_height;
  const size_t output_width = average_pooling_op->output_width;
  const size_t padded_input_width = average_pooling_op->padding_left + input_width + average_pooling_op->padding_right;
  const size_t padded_input_height = average_pooling_op->padding_top + input_height + average_pooling_op->padding_bottom;
  if (padded_input_width == average_pooling_op->kernel_width && padded_input_height == average_pooling_op->kernel_height) {
    // Global average pooling
    const size_t input_elements = input_height * input_width;
    const size_t input_stride_in_bytes = average_pooling_op->input_pixel_stride << log2_data_element_size;
    const size_t channels = average_pooling_op->channels;
    average_pooling_op->context.global_average_pooling_nwc = (struct global_average_pooling_nwc_context) {
        .input = input,
        .zero = average_pooling_op->zero_buffer,
        .input_pixel_stride = input_stride_in_bytes,
        .input_batch_stride = input_stride_in_bytes * input_elements,
        .input_elements = input_elements,
        .channels = channels,
        .output = output,
        .output_batch_stride = average_pooling_op->output_pixel_stride << log2_data_element_size,
    };
    memcpy(&average_pooling_op->context.global_average_pooling_nwc.params, global_params, global_params_size);
    average_pooling_op->compute[0].type = xnn_parallelization_type_1d;
    average_pooling_op->compute[0].range[0] = batch_size;

    if (input_elements <= gavgpool->row_tile) {
      average_pooling_op->compute[0].task_1d = (pthreadpool_task_1d_t) xnn_compute_global_average_pooling_nwc_unipass;
      average_pooling_op->context.global_average_pooling_nwc.unipass_ukernel = gavgpool->unipass;
    } else {
      average_pooling_op->context.global_average_pooling_nwc.buffer_size =
        (channels + (XNN_MAX_SIMD_SIZE >> log2_data_element_size)) << log2_accumulator_element_size;
      average_pooling_op->compute[0].task_1d = (pthreadpool_task_1d_t) xnn_compute_global_average_pooling_nwc_multipass;
      average_pooling_op->context.global_average_pooling_nwc.multipass_ukernel = gavgpool->multipass;
    }
  } else {
    // Non-global average pooling
    const size_t pooling_height = average_pooling_op->kernel_height;
    const size_t pooling_width = average_pooling_op->kernel_width;
    const size_t pooling_size = pooling_height * pooling_width;

    const uint32_t primary_tile = is_pixelwise ? pavgpool->primary_tile : avgpool->primary_tile;

    const size_t step_width = min(average_pooling_op->stride_width, pooling_width);
    const size_t step_height = pooling_size + (output_width - 1) * step_width * pooling_height;

    const size_t last_input_height = average_pooling_op->last_input_height;
    const size_t last_input_width = average_pooling_op->last_input_width;
    if (input_height != last_input_height || input_width != last_input_width) {
      // Micro-kernel may read up to (primary_tile - 1) elements after the end of indirection buffer.
      const size_t indirection_buffer_size = sizeof(void*) * ((primary_tile - 1) + output_height * step_height);

      const void** indirection_buffer =
        (const void**) xnn_reallocate_memory(average_pooling_op->indirection_buffer, indirection_buffer_size);
      if (indirection_buffer == NULL) {
        xnn_log_error("failed to allocate %zu bytes for %s operator indirection buffer",
          indirection_buffer_size, xnn_operator_type_to_string(average_pooling_op->type));
        return xnn_status_out_of_memory;
      }
      average_pooling_op->indirection_buffer = indirection_buffer;
      xnn_log_debug("allocated %zu bytes for indirection buffer in %s operator",
        indirection_buffer_size, xnn_operator_type_to_string(average_pooling_op->type));

      xnn_indirection_init_dwconv2d(average_pooling_op, step_height, step_width, primary_tile, log2_data_element_size);

      average_pooling_op->last_input = input;
      average_pooling_op->last_input_height = input_height;
      average_pooling_op->last_input_width = input_width;
    }

    const size_t channels = average_pooling_op->channels;

    const size_t indirect_input_height_stride = step_height * sizeof(void*);
    const size_t output_width_stride = average_pooling_op->output_pixel_stride << log2_data_element_size;
    const size_t output_height_stride = output_width * output_width_stride;

    if (is_pixelwise) {
      assert(indirection_init_pavgpool2d != NULL);

      if (input_height != last_input_height || input_width != last_input_width) {
        const size_t pixelwise_buffer_size = (output_height * output_width) << log2_weight_element_size;
        void* pixelwise_buffer = xnn_reallocate_memory(average_pooling_op->pixelwise_buffer, pixelwise_buffer_size);
        if (pixelwise_buffer == NULL) {
          xnn_log_error("failed to allocate %zu bytes for %s operator pixelwise buffer",
            pixelwise_buffer_size, xnn_operator_type_to_string(average_pooling_op->type));
          return xnn_status_out_of_memory;
        }
        average_pooling_op->pixelwise_buffer = pixelwise_buffer;
        xnn_log_debug("allocated %zu bytes for pixelwise buffer in %s operator",
          pixelwise_buffer_size, xnn_operator_type_to_string(average_pooling_op->type));

        indirection_init_pavgpool2d(
          input_height, input_width,
          output_height, output_width,
          average_pooling_op->kernel_height, average_pooling_op->kernel_width,
          average_pooling_op->stride_height, average_pooling_op->stride_width,
          average_pooling_op->padding_top, average_pooling_op->padding_left,
          pixelwise_buffer);
      }

      const uint32_t incremental_tile = pavgpool->incremental_tile;
      const size_t multipass_adjustment =
        pooling_size > primary_tile ? round_up(pooling_size - primary_tile, incremental_tile) + primary_tile - incremental_tile : 0;
      average_pooling_op->context.pixelwise_average_pooling = (struct pixelwise_average_pooling_context) {
        .indirect_input = average_pooling_op->indirection_buffer,
        .indirect_input_height_stride = indirect_input_height_stride,
        .input_batch_stride = input_height * input_width * average_pooling_op->input_pixel_stride << log2_data_element_size,
        .input_offset = (size_t) ((uintptr_t) input - (uintptr_t) average_pooling_op->last_input),
        .pixelwise_buffer = average_pooling_op->pixelwise_buffer,
        .pixelwise_buffer_height_stride = output_width << log2_data_element_size,
        .output = output,
        .output_batch_stride = output_height * output_height_stride,
        .output_height_stride = output_height_stride,
        .output_width = output_width,
        .pooling_size = pooling_size,
        .channels = channels,
        .zero = average_pooling_op->zero_buffer,
        .input_increment = (pooling_height * step_width - multipass_adjustment) * sizeof(void*),
        .output_increment = output_width_stride - (channels << log2_data_element_size),
      };
      memcpy(&average_pooling_op->context.pixelwise_average_pooling.params, params, params_size);
      if (pooling_size <= primary_tile) {
        average_pooling_op->context.pixelwise_average_pooling.unipass_ukernel = pavgpool->unipass;
        average_pooling_op->compute[0].task_2d = (pthreadpool_task_2d_t) xnn_compute_pixelwise_average_pooling_unipass;
      } else {
        average_pooling_op->context.pixelwise_average_pooling.buffer_size =
          (channels + (XNN_MAX_SIMD_SIZE >> log2_data_element_size)) << log2_accumulator_element_size;
        average_pooling_op->context.pixelwise_average_pooling.multipass_ukernel = pavgpool->multipass;
        average_pooling_op->compute[0].task_2d = (pthreadpool_task_2d_t) xnn_compute_pixelwise_average_pooling_multipass;
      }
    } else {
      const uint32_t incremental_tile = avgpool->incremental_tile;
      const size_t multipass_adjustment =
        pooling_size > primary_tile ? round_up(pooling_size - primary_tile, incremental_tile) + primary_tile - incremental_tile : 0;
      average_pooling_op->context.average_pooling = (struct average_pooling_context) {
        .indirect_input = average_pooling_op->indirection_buffer,
        .indirect_input_height_stride = indirect_input_height_stride,
        .input_offset = (size_t) ((uintptr_t) input - (uintptr_t) average_pooling_op->last_input),
        .input_batch_stride = input_height * input_width * average_pooling_op->input_pixel_stride << log2_data_element_size,
        .output = output,
        .output_batch_stride = output_height * output_height_stride,
        .output_height_stride = output_height_stride,
        .output_width = output_width,
        .pooling_size = pooling_size,
        .channels = channels,
        .zero = average_pooling_op->zero_buffer,
        .input_increment = (pooling_height * step_width - multipass_adjustment) * sizeof(void*),
        .output_increment = output_width_stride - (channels << log2_data_element_size),
        .params.f32 = average_pooling_op->params.f32_scaleminmax,
      };
      memcpy(&average_pooling_op->context.average_pooling.params, params, params_size);
      if (pooling_size <= primary_tile) {
        average_pooling_op->context.average_pooling.unipass_ukernel = avgpool->unipass;
        average_pooling_op->compute[0].task_2d = (pthreadpool_task_2d_t) xnn_compute_average_pooling_unipass;
      } else {
        average_pooling_op->context.average_pooling.buffer_size =
          (channels + (XNN_MAX_SIMD_SIZE >> log2_data_element_size)) << log2_accumulator_element_size;
        average_pooling_op->context.average_pooling.multipass_ukernel = avgpool->multipass;
        average_pooling_op->compute[0].task_2d = (pthreadpool_task_2d_t) xnn_compute_average_pooling_multipass;
      }
    }
    average_pooling_op->compute[0].type = xnn_parallelization_type_2d;
    average_pooling_op->compute[0].range[0] = batch_size;
    average_pooling_op->compute[0].range[1] = output_height;
  }
  average_pooling_op->state = xnn_run_state_ready;

  return xnn_status_success;
}

enum xnn_status xnn_setup_average_pooling2d_nhwc_qu8(
    xnn_operator_t average_pooling_op,
    size_t batch_size,
    size_t input_height,
    size_t input_width,
    const uint8_t* input,
    uint8_t* output,
    pthreadpool_t threadpool)
{
  if (average_pooling_op->type != xnn_operator_type_average_pooling_nhwc_qu8) {
    xnn_log_error("failed to setup operator: operator type mismatch (expected %s, got %s)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_qu8),
      xnn_operator_type_to_string(average_pooling_op->type));
    return xnn_status_invalid_parameter;
  }

  assert(average_pooling_op->ukernel.type == xnn_microkernel_type_average_pooling);

  // Number of rows read in the GAVGPOOL micro-kernel.
  const size_t input_size = input_height * input_width;
  const size_t pooling_size = average_pooling_op->kernel_height * average_pooling_op->kernel_width;
  const size_t gavgpool_nrows = round_up(input_size, average_pooling_op->gavgpool_config->row_tile);
  average_pooling_op->gavgpool_config->update.qu8(
    &average_pooling_op->params.qu8_gavgpool,
    -(average_pooling_op->input_zero_point * (int32_t) gavgpool_nrows),
    average_pooling_op->input_scale / (average_pooling_op->output_scale * (float) pooling_size));

  return setup_average_pooling2d(
    average_pooling_op,
    batch_size, input_height, input_width,
    input, output,
    /*log2_data_element_size=*/XNN_LOG2_SIZEOF_UINT8_T,
    /*log2_weight_element_size=*/XNN_LOG2_SIZEOF_UINT8_T,
    /*log2_accumulator_element_size=*/XNN_LOG2_SIZEOF_INT32_T,
    NULL /* indirection_init_pavgpool2d */,
    average_pooling_op->avgpool_config,
    NULL /* no PAVGPOOL micro-kernel */,
    average_pooling_op->gavgpool_config,
    &average_pooling_op->params.qu8_avgpool,
    sizeof(average_pooling_op->params.qu8_avgpool),
    &average_pooling_op->params.qu8_gavgpool,
    sizeof(average_pooling_op->params.qu8_gavgpool),
    pthreadpool_get_threads_count(threadpool),
    false /* pixelwise not supported */);
}

enum xnn_status xnn_setup_average_pooling2d_nhwc_f16(
    xnn_operator_t average_pooling_op,
    size_t batch_size,
    size_t input_height,
    size_t input_width,
    const void* input,
    void* output,
    pthreadpool_t threadpool)
{
  if (average_pooling_op->type != xnn_operator_type_average_pooling_nhwc_f16) {
    xnn_log_error("failed to setup operator: operator type mismatch (expected %s, got %s)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f16),
      xnn_operator_type_to_string(average_pooling_op->type));
    return xnn_status_invalid_parameter;
  }

  assert(average_pooling_op->ukernel.type == xnn_microkernel_type_average_pooling ||
         average_pooling_op->ukernel.type == xnn_microkernel_type_pixelwise_average_pooling);

  const void* pooling_params = &average_pooling_op->params.f16_scaleminmax;
  size_t pooling_params_size = sizeof(average_pooling_op->params.f16_scaleminmax);
  const bool is_pixelwise = average_pooling_op->ukernel.type == xnn_microkernel_type_pixelwise_average_pooling;
  if (is_pixelwise) {
    const size_t input_size = input_height * input_width;
    average_pooling_op->gavgpool_config->update.f16(
      &average_pooling_op->params.f16_scaleminmax, fp16_ieee_from_fp32_value(1.0f / (float) (int32_t) input_size));
    pooling_params = &average_pooling_op->params.f16_minmax;
    pooling_params_size = sizeof(average_pooling_op->params.f16_minmax);
  }

  return setup_average_pooling2d(
    average_pooling_op,
    batch_size, input_height, input_width,
    input, output,
    /*log2_data_element_size=*/XNN_LOG2_SIZEOF_HALF,
    /*log2_weight_element_size=*/XNN_LOG2_SIZEOF_HALF,
    /*log2_accumulator_element_size=*/XNN_LOG2_SIZEOF_HALF,
    (xnn_indirection_init_pavgpool2d_fn) xnn_indirection_init_pavgpool2d_f16,
    average_pooling_op->avgpool_config, average_pooling_op->pavgpool_config, average_pooling_op->gavgpool_config,
    pooling_params, pooling_params_size,
    &average_pooling_op->params.f16_scaleminmax, sizeof(average_pooling_op->params.f16_scaleminmax),
    pthreadpool_get_threads_count(threadpool),
    is_pixelwise);
}

enum xnn_status xnn_setup_average_pooling2d_nhwc_f32(
    xnn_operator_t average_pooling_op,
    size_t batch_size,
    size_t input_height,
    size_t input_width,
    const float* input,
    float* output,
    pthreadpool_t threadpool)
{
  if (average_pooling_op->type != xnn_operator_type_average_pooling_nhwc_f32) {
    xnn_log_error("failed to setup operator: operator type mismatch (expected %s, got %s)",
      xnn_operator_type_to_string(xnn_operator_type_average_pooling_nhwc_f32),
      xnn_operator_type_to_string(average_pooling_op->type));
    return xnn_status_invalid_parameter;
  }

  assert(average_pooling_op->ukernel.type == xnn_microkernel_type_average_pooling ||
         average_pooling_op->ukernel.type == xnn_microkernel_type_pixelwise_average_pooling);

  const void* pooling_params = &average_pooling_op->params.f32_scaleminmax;
  size_t pooling_params_size = sizeof(average_pooling_op->params.f32_scaleminmax);
  const bool is_pixelwise = average_pooling_op->ukernel.type == xnn_microkernel_type_pixelwise_average_pooling;
  if (is_pixelwise) {
    const size_t input_size = input_height * input_width;
    average_pooling_op->gavgpool_config->update.f32(
      &average_pooling_op->params.f32_scaleminmax, 1.0f / (float) (int32_t) input_size);
    pooling_params = &average_pooling_op->params.f32_minmax;
    pooling_params_size = sizeof(average_pooling_op->params.f32_minmax);
  }

  return setup_average_pooling2d(
    average_pooling_op,
    batch_size, input_height, input_width,
    input, output,
    /*log2_data_element_size=*/XNN_LOG2_SIZEOF_FLOAT,
    /*log2_weight_element_size=*/XNN_LOG2_SIZEOF_FLOAT,
    /*log2_accumulator_element_size=*/XNN_LOG2_SIZEOF_FLOAT,
    (xnn_indirection_init_pavgpool2d_fn) xnn_indirection_init_pavgpool2d_f32,
    average_pooling_op->avgpool_config, average_pooling_op->pavgpool_config, average_pooling_op->gavgpool_config,
    pooling_params, pooling_params_size,
    &average_pooling_op->params.f32_scaleminmax, sizeof(average_pooling_op->params.f32_scaleminmax),
    pthreadpool_get_threads_count(threadpool),
    is_pixelwise);
}
