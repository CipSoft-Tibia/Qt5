// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRERENDER_BROWSER_PRERENDER_UTIL_H_
#define COMPONENTS_PRERENDER_BROWSER_PRERENDER_UTIL_H_

#include "services/metrics/public/cpp/ukm_source_id.h"

class GURL;

namespace content {
class NavigationHandle;
}

namespace prerender {

class PrerenderManager;

// Indicates whether the URL provided is a GWS origin.
bool IsGoogleOriginURL(const GURL& origin_url);

// Report a URL was canceled due to trying to handle an external URL.
void ReportPrerenderExternalURL();

// Report a URL was canceled due to unsupported prerender scheme.
void ReportUnsupportedPrerenderScheme(const GURL& url);

// Records the metrics for the nostate prefetch to an event with UKM source ID
// |source_id|.
void RecordNoStatePrefetchMetrics(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id,
    prerender::PrerenderManager* prerender_manager);

}  // namespace prerender

#endif  // COMPONENTS_PRERENDER_BROWSER_PRERENDER_UTIL_H_
