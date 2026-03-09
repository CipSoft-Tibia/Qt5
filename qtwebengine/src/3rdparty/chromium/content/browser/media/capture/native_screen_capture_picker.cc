// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/native_screen_capture_picker.h"

#include "content/common/buildflags.h"

#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_SCREEN_CAPTURE)
#include "content/browser/media/capture/native_screen_capture_picker_mac.h"
#endif // BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_SCREEN_CAPTURE)

namespace content {

std::unique_ptr<NativeScreenCapturePicker>
MaybeCreateNativeScreenCapturePicker() {
#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_SCREEN_CAPTURE)
  return CreateNativeScreenCapturePickerMac();
#else
  return nullptr;
#endif // BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_SCREEN_CAPTURE)
}

}  // namespace content
