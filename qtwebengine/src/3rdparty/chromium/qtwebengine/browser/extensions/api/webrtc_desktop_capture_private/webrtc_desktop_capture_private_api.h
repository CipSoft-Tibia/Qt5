/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// based on //chrome/browser/extensions/api/webrtc_desktop_capture_private/webrtc_desktop_capture_private_api.h
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBENGINE_BROWSER_EXTENSIONS_API_WEBRTC_DESKTOP_CAPTURE_PRIVATE_WEBRTC_DESKTOP_CAPTURE_PRIVATE_API_H_
#define WEBENGINE_BROWSER_EXTENSIONS_API_WEBRTC_DESKTOP_CAPTURE_PRIVATE_WEBRTC_DESKTOP_CAPTURE_PRIVATE_API_H_

#include <map>

#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/media_stream_request.h"
#include "extensions/browser/extension_function.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom-shared.h"

namespace content {
class RenderFrameHost;
}

namespace extensions {

class WebrtcDesktopCapturePrivateChooseDesktopMediaFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("webrtcDesktopCapturePrivate.chooseDesktopMedia",
                             WEBRTCDESKTOPCAPTUREPRIVATE_CHOOSEDESKTOPMEDIA)
  WebrtcDesktopCapturePrivateChooseDesktopMediaFunction();

 private:
  ~WebrtcDesktopCapturePrivateChooseDesktopMediaFunction() override;
  void ProcessAccessRequestResponse(
      content::RenderFrameHost* const main_frame,
      const GURL &origin,
      const blink::mojom::StreamDevicesSet& devicesSet,
      blink::mojom::MediaStreamRequestResult stream_request_result,
      std::unique_ptr<content::MediaStreamUI> stream_ui);

  // ExtensionFunction overrides.
  ResponseAction Run() override;

  int request_id_;
};

class DesktopCaptureRequestsRegistry {
 public:
  DesktopCaptureRequestsRegistry();
  ~DesktopCaptureRequestsRegistry();

  static DesktopCaptureRequestsRegistry* GetInstance();

  void AddRequest(int process_id,
                  int request_id,
                  WebrtcDesktopCapturePrivateChooseDesktopMediaFunction* handler);
  void RemoveRequest(int process_id, int request_id);

 private:
  friend struct base::DefaultSingletonTraits<DesktopCaptureRequestsRegistry>;

  struct RequestId {
    RequestId(int process_id, int request_id);

    // Need to use RequestId as a key in std::map<>.
    bool operator<(const RequestId& other) const;

    int process_id;
    int request_id;
  };

  using RequestsMap =
      std::map<RequestId, WebrtcDesktopCapturePrivateChooseDesktopMediaFunction*>;

  RequestsMap requests_;
};

}  // namespace extensions

#endif  // WEBENGINE_BROWSER_EXTENSIONS_API_WEBRTC_DESKTOP_CAPTURE_PRIVATE_WEBRTC_DESKTOP_CAPTURE_PRIVATE_API_H_
