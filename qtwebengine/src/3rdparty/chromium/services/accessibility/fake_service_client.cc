// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/accessibility/fake_service_client.h"

#include "base/functional/callback_forward.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace ax {
FakeServiceClient::FakeServiceClient(mojom::AccessibilityService* service)
    : service_(service) {}

FakeServiceClient::~FakeServiceClient() = default;

void FakeServiceClient::BindAutomation(
    mojo::PendingAssociatedRemote<ax::mojom::Automation> automation,
    mojo::PendingReceiver<ax::mojom::AutomationClient> automation_client) {
  automation_client_receivers_.Add(this, std::move(automation_client));
  automation_remotes_.Add(std::move(automation));
  if (automation_bound_closure_) {
    std::move(automation_bound_closure_).Run();
  }
}

#if BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
void FakeServiceClient::BindTts(
    mojo::PendingReceiver<ax::mojom::Tts> tts_receiver) {
  tts_receivers_.Add(this, std::move(tts_receiver));
}

void FakeServiceClient::BindUserInterface(
    mojo::PendingReceiver<mojom::UserInterface> ux_receiver) {
  ux_receivers_.Add(this, std::move(ux_receiver));
}

void FakeServiceClient::Speak(const std::string& utterance,
                              ax::mojom::TtsOptionsPtr options,
                              SpeakCallback callback) {
  auto result = mojom::TtsSpeakResult::New();
  result->error = mojom::TtsError::kNoError;
  result->utterance_client = tts_utterance_client_.BindNewPipeAndPassReceiver();
  std::move(callback).Run(std::move(result));
  if (tts_speak_callback_) {
    tts_speak_callback_.Run(utterance, std::move(options));
  }
}

void FakeServiceClient::Stop() {
  if (!tts_utterance_client_.is_bound()) {
    return;
  }
  auto event = mojom::TtsEvent::New();
  event->type = mojom::TtsEventType::kInterrupted;
  tts_utterance_client_->OnEvent(std::move(event));
  tts_utterance_client_.reset();
}

void FakeServiceClient::Pause() {
  if (!tts_utterance_client_.is_bound()) {
    return;
  }
  auto event = mojom::TtsEvent::New();
  event->type = mojom::TtsEventType::kPause;
  tts_utterance_client_->OnEvent(std::move(event));
}

void FakeServiceClient::Resume() {
  if (!tts_utterance_client_.is_bound()) {
    return;
  }
  auto event = mojom::TtsEvent::New();
  event->type = mojom::TtsEventType::kResume;
  tts_utterance_client_->OnEvent(std::move(event));
}

void FakeServiceClient::IsSpeaking(IsSpeakingCallback callback) {
  std::move(callback).Run(tts_utterance_client_.is_bound());
}

void FakeServiceClient::GetVoices(GetVoicesCallback callback) {
  std::vector<ax::mojom::TtsVoicePtr> voices;

  // Create a voice with all event types.
  auto first_voice = ax::mojom::TtsVoice::New();
  first_voice->voice_name = "Lyra";
  first_voice->lang = "en-US", first_voice->remote = false;
  first_voice->engine_id = "us_toddler";
  first_voice->event_types = std::vector<mojom::TtsEventType>();
  for (int i = static_cast<int>(mojom::TtsEventType::kMinValue);
       i <= static_cast<int>(mojom::TtsEventType::kMaxValue); i++) {
    first_voice->event_types->emplace_back(static_cast<mojom::TtsEventType>(i));
  }

  // Create a voice with just two event types/
  auto second_voice = ax::mojom::TtsVoice::New();
  second_voice->voice_name = "Juno";
  second_voice->lang = "en-GB", second_voice->remote = true;
  second_voice->engine_id = "us_baby";
  second_voice->event_types = std::vector<mojom::TtsEventType>();
  second_voice->event_types->emplace_back(mojom::TtsEventType::kStart);
  second_voice->event_types->emplace_back(mojom::TtsEventType::kEnd);

  voices.emplace_back(std::move(first_voice));
  voices.emplace_back(std::move(second_voice));
  std::move(callback).Run(std::move(voices));
}

void FakeServiceClient::SetFocusRings(
    std::vector<mojom::FocusRingInfoPtr> focus_rings,
    mojom::AssistiveTechnologyType at_type) {
  focus_rings_for_type_[at_type] = std::move(focus_rings);
  if (focus_rings_callback_) {
    focus_rings_callback_.Run();
  }
}
#endif  // BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)

void FakeServiceClient::BindAccessibilityServiceClientForTest() {
  if (service_) {
    service_->BindAccessibilityServiceClient(
        a11y_client_receiver_.BindNewPipeAndPassRemote());
  }
}

void FakeServiceClient::SetAutomationBoundClosure(base::OnceClosure closure) {
  automation_bound_closure_ = std::move(closure);
}

bool FakeServiceClient::AutomationIsBound() const {
  return automation_client_receivers_.size() && automation_remotes_.size();
}

#if BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
void FakeServiceClient::SetTtsSpeakCallback(
    base::RepeatingCallback<void(const std::string&, mojom::TtsOptionsPtr)>
        callback) {
  tts_speak_callback_ = std::move(callback);
}

void FakeServiceClient::SendTtsUtteranceEvent(mojom::TtsEventPtr tts_event) {
  CHECK(tts_utterance_client_.is_bound());
  tts_utterance_client_->OnEvent(std::move(tts_event));
}

void FakeServiceClient::SetFocusRingsCallback(
    base::RepeatingCallback<void()> callback) {
  focus_rings_callback_ = std::move(callback);
}
bool FakeServiceClient::UserInterfaceIsBound() const {
  return ux_receivers_.size();
}

const std::vector<mojom::FocusRingInfoPtr>&
FakeServiceClient::GetFocusRingsForType(
    mojom::AssistiveTechnologyType type) const {
  return focus_rings_for_type_.at(type);
}

#endif  // BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)

bool FakeServiceClient::AccessibilityServiceClientIsBound() const {
  return a11y_client_receiver_.is_bound();
}

}  // namespace ax
