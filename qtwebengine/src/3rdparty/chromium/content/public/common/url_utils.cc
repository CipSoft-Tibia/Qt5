// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/url_utils.h"

#include <set>
#include <string>

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/feature_list.h"
#include "base/strings/string_piece.h"
#include "build/build_config.h"
#include "content/common/url_schemes.h"
#include "content/public/common/content_features.h"
#include "content/public/common/url_constants.h"
#include "third_party/blink/public/common/chrome_debug_urls.h"
#include "url/gurl.h"
#include "url/url_util.h"
#include "url/url_util_qt.h"

namespace content {

bool HasWebUIScheme(const GURL& url) {
  return HasWebUIOrigin(url::Origin::Create(url));
}

bool HasWebUIOrigin(const url::Origin& origin) {
  return origin.scheme() == content::kChromeUIScheme ||
         origin.scheme() == content::kChromeUIUntrustedScheme ||
         origin.scheme() == content::kChromeDevToolsScheme;
}

bool IsSavableURL(const GURL& url) {
  for (auto& scheme : GetSavableSchemes()) {
    if (url.SchemeIs(scheme))
      return true;
  }
  return false;
}

bool IsURLHandledByNetworkStack(const GURL& url) {
  // Javascript URLs, srcdoc, schemes that don't load data should not send a
  // request to the network stack.
  if (url.SchemeIs(url::kJavaScriptScheme) || url.is_empty() ||
      url.IsAboutSrcdoc()) {
    return false;
  }

  for (const auto& scheme : url::GetEmptyDocumentSchemes()) {
    if (url.SchemeIs(scheme))
      return false;
  }

  // Renderer debug URLs (e.g. chrome://kill) are handled in the renderer
  // process directly and should not be sent to the network stack.
  if (blink::IsRendererDebugURL(url))
    return false;

  // For your information, even though a "data:" url doesn't generate actual
  // network requests, it is handled by the network stack and so must return
  // true. The reason is that a few "data:" urls can't be handled locally. For
  // instance:
  // - the ones that result in downloads.
  // - the ones that are invalid. An error page must be served instead.
  // - the ones that have an unsupported MIME type.
  // - the ones that target the top-level frame on Android.

  return true;
}

bool IsSafeRedirectTarget(const GURL& from_url, const GURL& to_url) {
  static const auto kUnsafeSchemes =
      base::MakeFixedFlatSet<base::StringPiece>({
        url::kAboutScheme,
            url::kJavaScriptScheme, url::kBlobScheme,
#if !defined(CHROMECAST_BUILD)
        url::kDataScheme,
#endif
#if BUILDFLAG(IS_ANDROID)
        url::kContentScheme,
#endif
      });
  if (from_url.is_empty())
    return false;
  if (base::Contains(url::GetLocalSchemes(), to_url.scheme_piece())) {
#if defined(TOOLKIT_QT)
    if (auto *cs = url::CustomScheme::FindScheme(from_url.scheme_piece())) {
      if (cs->flags & (url::CustomScheme::Local | url::CustomScheme::LocalAccessAllowed))
        return true;
    }
#endif
    return base::Contains(url::GetLocalSchemes(), from_url.scheme_piece());
  }
#if defined(TOOLKIT_QT)
  if (from_url.IsCustom())
    return true;
#endif
  if (HasWebUIScheme(to_url))
    return false;
  if (kUnsafeSchemes.contains(to_url.scheme_piece()))
    return false;
  if (to_url.SchemeIsFileSystem())
    return from_url.SchemeIsFileSystem();
  return true;
}

}  // namespace content
