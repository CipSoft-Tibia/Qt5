/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/rtp_rtcp/source/rtp_format.h"

#include <utility>

#include "modules/rtp_rtcp/source/rtp_format_h264.h"
#include "modules/rtp_rtcp/source/rtp_format_video_generic.h"
#include "modules/rtp_rtcp/source/rtp_format_vp8.h"
#include "modules/rtp_rtcp/source/rtp_format_vp9.h"

namespace webrtc {
RtpPacketizer* RtpPacketizer::Create(VideoCodecType type,
                                     size_t max_payload_len,
                                     size_t last_packet_reduction_len,
                                     const RTPVideoHeader* rtp_video_header,
                                     FrameType frame_type) {
  switch (type) {
    case kVideoCodecH264:
      RTC_CHECK(rtp_video_header);
      return new RtpPacketizerH264(max_payload_len, last_packet_reduction_len,
                                   rtp_video_header->h264().packetization_mode);
    case kVideoCodecVP8:
      RTC_CHECK(rtp_video_header);
      return new RtpPacketizerVp8(rtp_video_header->vp8(), max_payload_len,
                                  last_packet_reduction_len);
    case kVideoCodecVP9:
      RTC_CHECK(rtp_video_header);
      return new RtpPacketizerVp9(rtp_video_header->vp9(), max_payload_len,
                                  last_packet_reduction_len);
    case kVideoCodecGeneric:
      return new RtpPacketizerGeneric(frame_type, max_payload_len,
                                      last_packet_reduction_len);
    default:
      RTC_NOTREACHED();
  }
  return nullptr;
}

RtpDepacketizer* RtpDepacketizer::Create(VideoCodecType type) {
  switch (type) {
    case kVideoCodecH264:
      return new RtpDepacketizerH264();
    case kVideoCodecVP8:
      return new RtpDepacketizerVp8();
    case kVideoCodecVP9:
      return new RtpDepacketizerVp9();
    case kVideoCodecGeneric:
      return new RtpDepacketizerGeneric();
    default:
      RTC_NOTREACHED();
  }
  return nullptr;
}
}  // namespace webrtc
