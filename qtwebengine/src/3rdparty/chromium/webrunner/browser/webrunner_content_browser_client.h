// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBRUNNER_BROWSER_WEBRUNNER_CONTENT_BROWSER_CLIENT_H_
#define WEBRUNNER_BROWSER_WEBRUNNER_CONTENT_BROWSER_CLIENT_H_

#include <lib/zx/channel.h>

#include "base/macros.h"
#include "content/public/browser/content_browser_client.h"

namespace webrunner {

class WebRunnerContentBrowserClient : public content::ContentBrowserClient {
 public:
  explicit WebRunnerContentBrowserClient(zx::channel context_channel);
  ~WebRunnerContentBrowserClient() override;

  // ContentBrowserClient overrides.
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;

 private:
  zx::channel context_channel_;

  DISALLOW_COPY_AND_ASSIGN(WebRunnerContentBrowserClient);
};

}  // namespace webrunner

#endif  // WEBRUNNER_BROWSER_WEBRUNNER_CONTENT_BROWSER_CLIENT_H_
