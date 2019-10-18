// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver_session.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "cast/common/public/message_port.h"
#include "cast/streaming/environment.h"
#include "cast/streaming/message_fields.h"
#include "cast/streaming/offer_messages.h"
#include "cast/streaming/receiver.h"
#include "util/json/json_helpers.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

// Using statements for constructor readability.
using Preferences = ReceiverSession::Preferences;
using ConfiguredReceivers = ReceiverSession::ConfiguredReceivers;

namespace {

template <typename Stream, typename Codec>
const Stream* SelectStream(const std::vector<Codec>& preferred_codecs,
                           const std::vector<Stream>& offered_streams) {
  for (auto codec : preferred_codecs) {
    const std::string codec_name = CodecToString(codec);
    for (const Stream& offered_stream : offered_streams) {
      if (offered_stream.stream.codec_name == codec_name) {
        OSP_DVLOG << "Selected " << codec_name << " as codec for streaming";
        return &offered_stream;
      }
    }
  }
  return nullptr;
}

// Helper method that creates an invalid Answer response.
Json::Value CreateInvalidAnswerMessage(Error error) {
  Json::Value message_root;
  message_root[kMessageType] = kMessageTypeAnswer;
  message_root[kResult] = kResultError;
  message_root[kErrorMessageBody][kErrorCode] = static_cast<int>(error.code());
  message_root[kErrorMessageBody][kErrorDescription] = error.message();

  return message_root;
}

// Helper method that creates a valid Answer response.
Json::Value CreateAnswerMessage(const Answer& answer) {
  OSP_DCHECK(answer.IsValid());
  Json::Value message_root;
  message_root[kMessageType] = kMessageTypeAnswer;
  message_root[kAnswerMessageBody] = answer.ToJson();
  message_root[kResult] = kResultOk;
  return message_root;
}

DisplayResolution ToDisplayResolution(const Resolution& resolution) {
  return DisplayResolution{resolution.width, resolution.height};
}

}  // namespace

ReceiverSession::Client::~Client() = default;

Preferences::Preferences() = default;
Preferences::Preferences(std::vector<VideoCodec> video_codecs,
                         std::vector<AudioCodec> audio_codecs)
    : Preferences(video_codecs, audio_codecs, nullptr, nullptr) {}

Preferences::Preferences(std::vector<VideoCodec> video_codecs,
                         std::vector<AudioCodec> audio_codecs,
                         std::unique_ptr<Constraints> constraints,
                         std::unique_ptr<DisplayDescription> description)
    : video_codecs(std::move(video_codecs)),
      audio_codecs(std::move(audio_codecs)),
      constraints(std::move(constraints)),
      display_description(std::move(description)) {}

Preferences::Preferences(Preferences&&) noexcept = default;
Preferences& Preferences::operator=(Preferences&&) noexcept = default;

ReceiverSession::ReceiverSession(Client* const client,
                                 Environment* environment,
                                 MessagePort* message_port,
                                 Preferences preferences)
    : client_(client),
      environment_(environment),
      message_port_(message_port),
      preferences_(std::move(preferences)),
      packet_router_(environment_) {
  OSP_DCHECK(client_);
  OSP_DCHECK(message_port_);
  OSP_DCHECK(environment_);

  message_port_->SetClient(this, kDefaultStreamingReceiverSenderId);
}

ReceiverSession::~ReceiverSession() {
  ResetReceivers(Client::kEndOfSession);
  message_port_->ResetClient();
}

void ReceiverSession::OnMessage(const std::string& sender_id,
                                const std::string& message_namespace,
                                const std::string& message) {
  ErrorOr<Json::Value> message_json = json::Parse(message);

  if (!message_json) {
    client_->OnError(this, Error::Code::kJsonParseError);
    OSP_DLOG_WARN << "Received an invalid message: " << message;
    return;
  }
  OSP_DVLOG << "Received a message: " << message;

  int sequence_number;
  if (!json::ParseAndValidateInt(message_json.value()[kSequenceNumber],
                                 &sequence_number)) {
    OSP_DLOG_WARN << "Invalid message sequence number";
    return;
  }

  std::string key;
  if (!json::ParseAndValidateString(message_json.value()[kKeyType], &key)) {
    OSP_DLOG_WARN << "Invalid message key";
    return;
  }

  Message parsed_message{sender_id.data(), message_namespace.data(),
                         sequence_number};
  if (key == kMessageTypeOffer) {
    parsed_message.body = std::move(message_json.value()[kOfferMessageBody]);
    OnOffer(&parsed_message);
  }
}

void ReceiverSession::OnError(Error error) {
  OSP_DLOG_WARN << "ReceiverSession message port error: " << error;
}

void ReceiverSession::OnOffer(Message* message) {
  ErrorOr<Offer> offer = Offer::Parse(std::move(message->body));
  if (!offer) {
    client_->OnError(this, offer.error());
    OSP_DLOG_WARN << "Could not parse offer" << offer.error();
    message->body = CreateInvalidAnswerMessage(
        Error(Error::Code::kParseError, "Failed to parse malformed OFFER"));
    SendMessage(message);
    return;
  }

  const AudioStream* selected_audio_stream = nullptr;
  if (!offer.value().audio_streams.empty() &&
      !preferences_.audio_codecs.empty()) {
    selected_audio_stream =
        SelectStream(preferences_.audio_codecs, offer.value().audio_streams);
  }

  const VideoStream* selected_video_stream = nullptr;
  if (!offer.value().video_streams.empty() &&
      !preferences_.video_codecs.empty()) {
    selected_video_stream =
        SelectStream(preferences_.video_codecs, offer.value().video_streams);
  }

  if (!selected_audio_stream && !selected_video_stream) {
    message->body = CreateInvalidAnswerMessage(
        Error(Error::Code::kParseError, "No selected streams"));
    OSP_DLOG_WARN << "Failed to select any streams from OFFER";
    SendMessage(message);
    return;
  }

  const Answer answer =
      ConstructAnswer(message, selected_audio_stream, selected_video_stream);
  if (!answer.IsValid()) {
    message->body = CreateInvalidAnswerMessage(
        Error(Error::Code::kParseError, "Invalid answer message"));
    OSP_DLOG_WARN << "Failed to construct an ANSWER message";
    SendMessage(message);
    return;
  }

  // Only spawn receivers if we know we have a valid answer message.
  ConfiguredReceivers receivers =
      SpawnReceivers(selected_audio_stream, selected_video_stream);
  // If the answer message is invalid, there is no point in setting up a
  // negotiation because the sender won't be able to connect to it.
  client_->OnNegotiated(this, std::move(receivers));
  message->body = CreateAnswerMessage(answer);
  SendMessage(message);
}

std::unique_ptr<Receiver> ReceiverSession::ConstructReceiver(
    const Stream& stream) {
  SessionConfig config = {stream.ssrc,         stream.ssrc + 1,
                          stream.rtp_timebase, stream.channels,
                          stream.target_delay, stream.aes_key,
                          stream.aes_iv_mask};
  return std::make_unique<Receiver>(environment_, &packet_router_,
                                    std::move(config));
}

ConfiguredReceivers ReceiverSession::SpawnReceivers(const AudioStream* audio,
                                                    const VideoStream* video) {
  OSP_DCHECK(audio || video);
  ResetReceivers(Client::kRenegotiated);

  AudioCaptureConfig audio_config;
  absl::optional<ConfiguredReceiver<AudioStream>> deprecated_audio;
  if (audio) {
    current_audio_receiver_ = ConstructReceiver(audio->stream);
    audio_config = AudioCaptureConfig{
        StringToAudioCodec(audio->stream.codec_name), audio->stream.channels,
        audio->bit_rate, audio->stream.rtp_timebase,
        audio->stream.target_delay};
    deprecated_audio.emplace(ConfiguredReceiver<AudioStream>{
        current_audio_receiver_.get(), current_audio_receiver_->config(),
        *audio});
  }

  VideoCaptureConfig video_config;
  absl::optional<ConfiguredReceiver<VideoStream>> deprecated_video;
  if (video) {
    current_video_receiver_ = ConstructReceiver(video->stream);
    std::vector<DisplayResolution> display_resolutions;
    std::transform(video->resolutions.begin(), video->resolutions.end(),
                   std::back_inserter(display_resolutions),
                   ToDisplayResolution);
    video_config =
        VideoCaptureConfig{StringToVideoCodec(video->stream.codec_name),
                           FrameRate{video->max_frame_rate.numerator,
                                     video->max_frame_rate.denominator},
                           video->max_bit_rate, std::move(display_resolutions),
                           video->stream.target_delay};
    deprecated_video.emplace(ConfiguredReceiver<VideoStream>{
        current_video_receiver_.get(), current_video_receiver_->config(),
        *video});
  }

  return ConfiguredReceivers{
      current_audio_receiver_.get(), std::move(audio_config),
      current_video_receiver_.get(), std::move(video_config),

      // TODO(crbug.com/1132109): Remove deprecated ConfiguredReceiver fields
      // after downstream migration
      std::move(deprecated_audio), std::move(deprecated_video)};
}

void ReceiverSession::ResetReceivers(Client::ReceiversDestroyingReason reason) {
  if (current_video_receiver_ || current_audio_receiver_) {
    client_->OnReceiversDestroying(this, reason);
    current_audio_receiver_.reset();
    current_video_receiver_.reset();
  }
}

Answer ReceiverSession::ConstructAnswer(
    Message* message,
    const AudioStream* selected_audio_stream,
    const VideoStream* selected_video_stream) {
  OSP_DCHECK(selected_audio_stream || selected_video_stream);

  std::vector<int> stream_indexes;
  std::vector<Ssrc> stream_ssrcs;
  if (selected_audio_stream) {
    stream_indexes.push_back(selected_audio_stream->stream.index);
    stream_ssrcs.push_back(selected_audio_stream->stream.ssrc + 1);
  }

  if (selected_video_stream) {
    stream_indexes.push_back(selected_video_stream->stream.index);
    stream_ssrcs.push_back(selected_video_stream->stream.ssrc + 1);
  }

  absl::optional<Constraints> constraints;
  if (preferences_.constraints) {
    constraints = absl::optional<Constraints>(*preferences_.constraints);
  }

  absl::optional<DisplayDescription> display;
  if (preferences_.display_description) {
    display =
        absl::optional<DisplayDescription>(*preferences_.display_description);
  }

  return Answer{environment_->GetBoundLocalEndpoint().port,
                std::move(stream_indexes),
                std::move(stream_ssrcs),
                std::move(constraints),
                std::move(display),
                std::vector<int>{},  // receiver_rtcp_event_log
                std::vector<int>{},  // receiver_rtcp_dscp
                supports_wifi_status_reporting_};
}

void ReceiverSession::SendMessage(Message* message) {
  // All messages have the sequence number embedded.
  message->body[kSequenceNumber] = message->sequence_number;

  auto body_or_error = json::Stringify(message->body);
  if (body_or_error.is_value()) {
    OSP_DVLOG << "Sending message: SENDER[" << message->sender_id
              << "], NAMESPACE[" << message->message_namespace << "], BODY:\n"
              << body_or_error.value();
    message_port_->PostMessage(message->sender_id, message->message_namespace,
                               body_or_error.value());
  } else {
    OSP_DLOG_WARN << "Sending message failed with error:\n"
                  << body_or_error.error();
    client_->OnError(this, body_or_error.error());
  }
}

}  // namespace cast
}  // namespace openscreen
