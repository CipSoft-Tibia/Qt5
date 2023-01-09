// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/sms/user_consent_handler.h"
#include "base/callback.h"
#include "content/browser/sms/sms_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"

using blink::mojom::SmsStatus;

namespace content {

NoopUserConsentHandler::~NoopUserConsentHandler() = default;

void NoopUserConsentHandler::RequestUserConsent(
    const std::string& one_time_code,
    CompletionCallback on_complete) {
  std::move(on_complete).Run(SmsStatus::kSuccess);
}

bool NoopUserConsentHandler::is_active() const {
  return false;
}
bool NoopUserConsentHandler::is_async() const {
  return false;
}

PromptBasedUserConsentHandler::PromptBasedUserConsentHandler(
    RenderFrameHost* frame_host,
    const url::Origin& origin)
    : frame_host_{frame_host}, origin_{origin} {}
PromptBasedUserConsentHandler::~PromptBasedUserConsentHandler() = default;

void PromptBasedUserConsentHandler::RequestUserConsent(
    const std::string& one_time_code,
    CompletionCallback on_complete) {
  WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host_);
  if (!web_contents->GetDelegate()) {
    std::move(on_complete).Run(SmsStatus::kCancelled);
    return;
  }

  on_complete_ = std::move(on_complete);
  is_prompt_open_ = true;
  web_contents->GetDelegate()->CreateSmsPrompt(
      frame_host_, origin_, one_time_code,
      base::BindOnce(&PromptBasedUserConsentHandler::OnConfirm,
                     weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&PromptBasedUserConsentHandler::OnCancel,
                     weak_ptr_factory_.GetWeakPtr()));
}
bool PromptBasedUserConsentHandler::is_active() const {
  return is_prompt_open_;
}
bool PromptBasedUserConsentHandler::is_async() const {
  return true;
}

void PromptBasedUserConsentHandler::OnConfirm() {
  is_prompt_open_ = false;
  std::move(on_complete_).Run(SmsStatus::kSuccess);
}

void PromptBasedUserConsentHandler::OnCancel() {
  is_prompt_open_ = false;
  std::move(on_complete_).Run(SmsStatus::kCancelled);
}

}  // namespace content