// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_ACCESSIBILITY_FAKE_SERVICE_CLIENT_H_
#define SERVICES_ACCESSIBILITY_FAKE_SERVICE_CLIENT_H_

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/accessibility/buildflags.h"
#include "services/accessibility/public/mojom/accessibility_service.mojom.h"
#include "services/accessibility/public/mojom/automation.mojom.h"

#if BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
#include "services/accessibility/public/mojom/tts.mojom.h"
#include "services/accessibility/public/mojom/user_interface.mojom.h"
#endif  // BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)

namespace ax {

// A fake AccessibilityServiceClient and AutomationClient for use in tests.
// This allows tests to mock out the OS side of the mojom pipes.
// TODO(b/262637071) This can be extended to allow for passing events into
// the service once the mojom is landed.
// TODO(b/262637071): This should be split for OS vs Browser ATP.
class FakeServiceClient : public mojom::AccessibilityServiceClient,
#if BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
                          public mojom::Tts,
                          public mojom::UserInterface,
#endif
                          public mojom::AutomationClient {
 public:
  // |service| may be null if it won't be used in the test.
  explicit FakeServiceClient(mojom::AccessibilityService* service);
  FakeServiceClient(const FakeServiceClient& other) = delete;
  FakeServiceClient& operator=(const FakeServiceClient&) = delete;
  ~FakeServiceClient() override;

  // ax::mojom::AccessibilityServiceClient:
  void BindAutomation(
      mojo::PendingAssociatedRemote<ax::mojom::Automation> automation,
      mojo::PendingReceiver<ax::mojom::AutomationClient> automation_client)
      override;
#if BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
  void BindTts(mojo::PendingReceiver<ax::mojom::Tts> tts_receiver) override;
  void BindUserInterface(
      mojo::PendingReceiver<ax::mojom::UserInterface> ux_receiver) override;

  // ax::mojom::Tts:
  void Speak(const std::string& utterance,
             ax::mojom::TtsOptionsPtr options,
             SpeakCallback callback) override;
  void Stop() override;
  void Pause() override;
  void Resume() override;
  void IsSpeaking(IsSpeakingCallback callback) override;
  void GetVoices(GetVoicesCallback callback) override;

  // ax::mojom::UserInterface:
  void SetFocusRings(std::vector<ax::mojom::FocusRingInfoPtr> focus_rings,
                     ax::mojom::AssistiveTechnologyType at_type) override;
#endif  // BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)

  // Methods for testing.
  void BindAccessibilityServiceClientForTest();
  bool AccessibilityServiceClientIsBound() const;
  void SetAutomationBoundClosure(base::OnceClosure closure);
  bool AutomationIsBound() const;

#if BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
  void SetTtsSpeakCallback(
      base::RepeatingCallback<void(const std::string&, mojom::TtsOptionsPtr)>
          callback);
  void SendTtsUtteranceEvent(mojom::TtsEventPtr tts_event);

  bool UserInterfaceIsBound() const;
  void SetFocusRingsCallback(base::RepeatingCallback<void()> callback);
  const std::vector<ax::mojom::FocusRingInfoPtr>& GetFocusRingsForType(
      mojom::AssistiveTechnologyType type) const;
#endif  // BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
  base::WeakPtr<FakeServiceClient> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  raw_ptr<mojom::AccessibilityService, DanglingUntriaged> service_;
  base::OnceClosure automation_bound_closure_;

  mojo::AssociatedRemoteSet<mojom::Automation> automation_remotes_;
  mojo::ReceiverSet<mojom::AutomationClient> automation_client_receivers_;
#if BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
  base::RepeatingCallback<void(const std::string&, mojom::TtsOptionsPtr)>
      tts_speak_callback_;
  mojo::ReceiverSet<mojom::Tts> tts_receivers_;
  mojo::Remote<ax::mojom::TtsUtteranceClient> tts_utterance_client_;

  base::RepeatingCallback<void()> focus_rings_callback_;
  mojo::ReceiverSet<mojom::UserInterface> ux_receivers_;
  std::map<mojom::AssistiveTechnologyType,
           std::vector<ax::mojom::FocusRingInfoPtr>>
      focus_rings_for_type_;
#endif  // BUILDFLAG(SUPPORTS_OS_ACCESSIBILITY_SERVICE)
  mojo::Receiver<mojom::AccessibilityServiceClient> a11y_client_receiver_{this};

  base::WeakPtrFactory<FakeServiceClient> weak_ptr_factory_{this};
};

}  // namespace ax

#endif  // SERVICES_ACCESSIBILITY_FAKE_SERVICE_CLIENT_H_
