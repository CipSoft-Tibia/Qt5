// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/delegated_ink_point_renderer_base.h"

#include "base/trace_event/trace_event.h"
#include "components/viz/common/delegated_ink_metadata.h"

namespace viz {

DelegatedInkPointRendererBase::DelegatedInkPointRendererBase() = default;
DelegatedInkPointRendererBase::~DelegatedInkPointRendererBase() = default;

void DelegatedInkPointRendererBase::InitMessagePipeline(
    mojo::PendingReceiver<mojom::DelegatedInkPointRenderer> receiver) {
  receiver_.Bind(std::move(receiver));
}

void DelegatedInkPointRendererBase::DrawDelegatedInkTrail() {
  if (!metadata_)
    return;

  DrawDelegatedInkTrailInternal();

  // Always reset |metadata_| regardless of the outcome of
  // DrawDelegatedInkPathInternal() so that the trail is never incorrectly
  // drawn if the aggregated frame did not contain delegated ink metadata.
  metadata_.reset();
}

void DelegatedInkPointRendererBase::FilterPoints() {
  if (points_.size() == 0)
    return;

  auto first_valid_point = points_.find(metadata_->timestamp());
  // It is possible that this results in |points_| being empty. This occurs when
  // the points being forwarded from the browser process lose the race against
  // the ink metadata arriving in Display, including the point that matches the
  // metadata. There may still be old points in |points_| allowing execution to
  // get here, but none of them match the metadata point, so they are all
  // erased.
  points_.erase(points_.begin(), first_valid_point);

  TRACE_EVENT_INSTANT1("viz", "Filtered points for delegated ink trail",
                       TRACE_EVENT_SCOPE_THREAD, "points", points_.size());
}

void DelegatedInkPointRendererBase::StoreDelegatedInkPoint(
    const DelegatedInkPoint& point) {
  TRACE_EVENT_INSTANT1("viz",
                       "DelegatedInkPointRendererImpl::StoreDelegatedInkPoint",
                       TRACE_EVENT_SCOPE_THREAD, "point", point.ToString());

  // Fail-safe to prevent storing excessive points if they are being sent but
  // never filtered and used, like if the renderer has stalled during a long
  // running script.
  if (points_.size() == kMaximumDelegatedInkPointsStored)
    points_.erase(points_.begin());

  points_.insert({point.timestamp(), point.point()});
}

}  // namespace viz
