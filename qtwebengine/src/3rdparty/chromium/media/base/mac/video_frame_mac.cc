// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/mac/video_frame_mac.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>

#include "base/logging.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gfx/mac/io_surface.h"

namespace media {

namespace {

// Maximum number of planes supported by this implementation.
const int kMaxPlanes = 3;

// CVPixelBuffer release callback. See |GetCvPixelBufferRepresentation()|.
void CvPixelBufferReleaseCallback(void* frame_ref,
                                  const void* data,
                                  size_t size,
                                  size_t num_planes,
                                  const void* planes[]) {
  free(const_cast<void*>(data));
  reinterpret_cast<const VideoFrame*>(frame_ref)->Release();
}

// Current list of acceptable CVPixelFormat mappings. If we start supporting
// RGB frame encoding we'll need to extend this list.
bool IsAcceptableCvPixelFormat(VideoPixelFormat format, OSType cv_format) {
  if (format == PIXEL_FORMAT_I420) {
    return cv_format == kCVPixelFormatType_420YpCbCr8Planar ||
           cv_format == kCVPixelFormatType_420YpCbCr8PlanarFullRange;
  }
  if (format == PIXEL_FORMAT_NV12) {
    return cv_format == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange ||
           cv_format == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
  }
  if (format == PIXEL_FORMAT_NV12A) {
    return cv_format == kCVPixelFormatType_420YpCbCr8VideoRange_8A_TriPlanar;
  }
  return false;
}

}  // namespace

MEDIA_EXPORT base::ScopedCFTypeRef<CVPixelBufferRef>
WrapVideoFrameInCVPixelBuffer(scoped_refptr<VideoFrame> frame) {
  base::ScopedCFTypeRef<CVPixelBufferRef> pixel_buffer;
  if (!frame)
    return pixel_buffer;

  const gfx::Rect& visible_rect = frame->visible_rect();
  bool crop_needed = visible_rect != gfx::Rect(frame->coded_size());

  if (!crop_needed) {
    // If the frame is backed by a pixel buffer, just return that buffer.
    if (frame->CvPixelBuffer()) {
      pixel_buffer.reset(frame->CvPixelBuffer(), base::scoped_policy::RETAIN);
      if (!IsAcceptableCvPixelFormat(
              frame->format(), CVPixelBufferGetPixelFormatType(pixel_buffer))) {
        DLOG(ERROR) << "Dropping CVPixelBuffer w/ incorrect format.";
        pixel_buffer.reset();
      }
      return pixel_buffer;
    }

    // If the frame has a GMB, yank out its IOSurface if possible.
    if (frame->HasGpuMemoryBuffer()) {
      auto handle = frame->GetGpuMemoryBuffer()->CloneHandle();
      if (handle.type == gfx::GpuMemoryBufferType::IO_SURFACE_BUFFER) {
        gfx::ScopedIOSurface io_surface = handle.io_surface;
        if (io_surface) {
          CVReturn cv_return = CVPixelBufferCreateWithIOSurface(
              nullptr, io_surface, nullptr, pixel_buffer.InitializeInto());
          if (cv_return != kCVReturnSuccess) {
            DLOG(ERROR) << "CVPixelBufferCreateWithIOSurface failed: "
                        << cv_return;
            pixel_buffer.reset();
          }
          if (!IsAcceptableCvPixelFormat(
                  frame->format(),
                  CVPixelBufferGetPixelFormatType(pixel_buffer))) {
            DLOG(ERROR) << "Dropping CVPixelBuffer w/ incorrect format.";
            pixel_buffer.reset();
          }
          return pixel_buffer;
        }
      }
    }
  }

  // If the frame is backed by a GPU buffer, but needs cropping, map it and
  // and handle like a software frame. There is no memcpy here.
  if (frame->HasGpuMemoryBuffer())
    frame = ConvertToMemoryMappedFrame(std::move(frame));
  if (!frame)
    return pixel_buffer;

  VLOG(3) << "Returning RAM based CVPixelBuffer.";

  // VideoFrame only supports YUV formats and most of them are 'YVU' ordered,
  // which CVPixelBuffer does not support. This means we effectively can only
  // represent I420 and NV12 frames. In addition, VideoFrame does not carry
  // colorimetric information, so this function assumes standard video range
  // and ITU Rec 709 primaries.
  const VideoPixelFormat video_frame_format = frame->format();
  OSType cv_format;
  if (video_frame_format == PIXEL_FORMAT_I420) {
    cv_format = kCVPixelFormatType_420YpCbCr8Planar;
  } else if (video_frame_format == PIXEL_FORMAT_NV12) {
    cv_format = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
  } else if (video_frame_format == PIXEL_FORMAT_NV12A) {
    cv_format = kCVPixelFormatType_420YpCbCr8VideoRange_8A_TriPlanar;
  } else {
    DLOG(ERROR) << "Unsupported frame format: " << video_frame_format;
    return pixel_buffer;
  }

  DCHECK(IsAcceptableCvPixelFormat(video_frame_format, cv_format));

  int num_planes = VideoFrame::NumPlanes(video_frame_format);
  DCHECK_LE(num_planes, kMaxPlanes);

  // Build arrays for each plane's data pointer, dimensions and byte alignment.
  void* plane_ptrs[kMaxPlanes];
  size_t plane_widths[kMaxPlanes];
  size_t plane_heights[kMaxPlanes];
  size_t plane_bytes_per_row[kMaxPlanes];
  for (int plane_i = 0; plane_i < num_planes; ++plane_i) {
    plane_ptrs[plane_i] = const_cast<uint8_t*>(frame->visible_data(plane_i));
    gfx::Size plane_size =
        VideoFrame::PlaneSize(video_frame_format, plane_i, visible_rect.size());
    plane_widths[plane_i] = plane_size.width();
    plane_heights[plane_i] = plane_size.height();
    plane_bytes_per_row[plane_i] = frame->stride(plane_i);
  }

  // CVPixelBufferCreateWithPlanarBytes needs a dummy plane descriptor or the
  // release callback will not execute. The descriptor is freed in the callback.
  void* descriptor =
      calloc(1, std::max(sizeof(CVPlanarPixelBufferInfo_YCbCrPlanar),
                         sizeof(CVPlanarPixelBufferInfo_YCbCrBiPlanar)));

  // Wrap the frame's data in a CVPixelBuffer. Because this is a C API, we can't
  // give it a smart pointer to the frame, so instead pass a raw pointer and
  // increment the frame's reference count manually.
  CVReturn result = CVPixelBufferCreateWithPlanarBytes(
      kCFAllocatorDefault, visible_rect.width(), visible_rect.height(),
      cv_format, descriptor, 0, num_planes, plane_ptrs, plane_widths,
      plane_heights, plane_bytes_per_row, &CvPixelBufferReleaseCallback,
      frame.get(), nullptr, pixel_buffer.InitializeInto());
  if (result != kCVReturnSuccess) {
    DLOG(ERROR) << " CVPixelBufferCreateWithPlanarBytes failed: " << result;
    return base::ScopedCFTypeRef<CVPixelBufferRef>(nullptr);
  }

  // The CVPixelBuffer now references the data of the frame, so increment its
  // reference count manually. The release callback set on the pixel buffer will
  // release the frame.
  frame->AddRef();

  // Apply required colorimetric attachments.
  CVBufferSetAttachment(pixel_buffer, kCVImageBufferColorPrimariesKey,
                        kCVImageBufferColorPrimaries_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);
  CVBufferSetAttachment(pixel_buffer, kCVImageBufferTransferFunctionKey,
                        kCVImageBufferTransferFunction_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);
  CVBufferSetAttachment(pixel_buffer, kCVImageBufferYCbCrMatrixKey,
                        kCVImageBufferYCbCrMatrix_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);

  return pixel_buffer;
}

}  // namespace media
