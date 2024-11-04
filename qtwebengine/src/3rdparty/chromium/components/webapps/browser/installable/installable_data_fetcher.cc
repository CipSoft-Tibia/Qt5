// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/webapps/browser/installable/installable_data_fetcher.h"

#include "base/functional/callback.h"
#include "components/webapps/browser/installable/installable_metrics.h"
#include "components/webapps/common/constants.h"
#include "content/public/browser/manifest_icon_downloader.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/manifest/manifest_util.h"

namespace webapps {

namespace {

// Minimum dimension size in pixels for screenshots.
const int kMinimumScreenshotSizeInPx = 320;

// Maximum dimension size in pixels for screenshots.
const int kMaximumScreenshotSizeInPx = 3840;

// Maximum number of screenshots allowed, the rest will be ignored.
const int kMaximumNumOfScreenshots = 8;

}  // namespace

InstallableDataFetcher::InstallableDataFetcher(
    content::WebContents* web_contents,
    content::ServiceWorkerContext* service_worker_context,
    InstallablePageData& data)
    : web_contents_(web_contents->GetWeakPtr()),
      service_worker_context_(service_worker_context),
      page_data_(data) {}

InstallableDataFetcher::~InstallableDataFetcher() = default;

void InstallableDataFetcher::FetchManifest(FetcherCallback finish_callback) {
  if (page_data_->manifest->fetched) {
    std::move(finish_callback).Run(page_data_->manifest->error);
    return;
  }

  // This uses DidFinishNavigation to abort when the primary page changes.
  // Therefore this should always be the correct page.
  web_contents()->GetPrimaryPage().GetManifest(base::BindOnce(
      &InstallableDataFetcher::OnDidGetManifest, weak_ptr_factory_.GetWeakPtr(),
      std::move(finish_callback)));
}

void InstallableDataFetcher::OnDidGetManifest(
    FetcherCallback finish_callback,
    const GURL& manifest_url,
    blink::mojom::ManifestPtr manifest) {
  if (manifest_url.is_empty()) {
    page_data_->manifest->error = NO_MANIFEST;
  } else if (blink::IsEmptyManifest(manifest)) {
    page_data_->manifest->error = MANIFEST_EMPTY;
  }

  page_data_->manifest->url = manifest_url;
  page_data_->manifest->manifest = std::move(manifest);
  page_data_->manifest->fetched = true;

  std::move(finish_callback).Run(page_data_->manifest->error);
}

void InstallableDataFetcher::FetchWebPageMetadata(
    FetcherCallback finish_callback) {
  if (page_data_->web_page_metadata->fetched) {
    std::move(finish_callback).Run(page_data_->web_page_metadata->error);
    return;
  }

  // Send a message to the renderer to retrieve information about the page.
  mojo::AssociatedRemote<mojom::WebPageMetadataAgent> metadata_agent;
  web_contents()
      ->GetPrimaryMainFrame()
      ->GetRemoteAssociatedInterfaces()
      ->GetInterface(&metadata_agent);
  // Bind the InterfacePtr into the callback so that it's kept alive until
  // there's either a connection error or a response.
  auto* web_page_metadata_proxy = metadata_agent.get();
  web_page_metadata_proxy->GetWebPageMetadata(
      base::BindOnce(&InstallableDataFetcher::OnDidGetWebPageMetadata,
                     weak_ptr_factory_.GetWeakPtr(), std::move(metadata_agent),
                     std::move(finish_callback)));
}

void InstallableDataFetcher::OnDidGetWebPageMetadata(
    mojo::AssociatedRemote<mojom::WebPageMetadataAgent> metadata_agent,
    FetcherCallback finish_callback,
    mojom::WebPageMetadataPtr web_page_metadata) {
  page_data_->web_page_metadata->metadata = std::move(web_page_metadata);
  page_data_->web_page_metadata->fetched = true;
  std::move(finish_callback).Run(NO_ERROR_DETECTED);
}

void InstallableDataFetcher::CheckServiceWorker(
    FetcherCallback finish_callback,
    base::OnceClosure pause_callback,
    bool wait_for_worker) {
  if (page_data_->worker->fetched &&
      page_data_->worker->error != NO_MATCHING_SERVICE_WORKER) {
    std::move(finish_callback).Run(page_data_->worker->error);
    return;
  }

  if (blink::IsEmptyManifest(*page_data_->manifest->manifest)) {
    std::move(finish_callback).Run(MANIFEST_DEPENDENT_TASK_NOT_RUN);
    return;
  }

  if (!service_worker_context_) {
    return;
  }

  // Check to see if there is a service worker for the manifest's scope.
  service_worker_context_->CheckHasServiceWorker(
      page_data_->GetManifest().scope,
      blink::StorageKey::CreateFirstParty(
          url::Origin::Create(page_data_->GetManifest().scope)),
      base::BindOnce(&InstallableDataFetcher::OnDidCheckHasServiceWorker,
                     weak_ptr_factory_.GetWeakPtr(), std::move(finish_callback),
                     std::move(pause_callback), wait_for_worker,
                     base::TimeTicks::Now()));
}

void InstallableDataFetcher::OnDidCheckHasServiceWorker(
    FetcherCallback finish_callback,
    base::OnceClosure pause_callback,
    bool wait_for_worker,
    base::TimeTicks check_service_worker_start_time,
    content::ServiceWorkerCapability capability) {
  switch (capability) {
    case content::ServiceWorkerCapability::SERVICE_WORKER_WITH_FETCH_HANDLER:
      page_data_->worker->has_worker = true;
      page_data_->worker->error = NO_ERROR_DETECTED;
      break;
    case content::ServiceWorkerCapability::SERVICE_WORKER_NO_FETCH_HANDLER:
      page_data_->worker->has_worker = false;
      page_data_->worker->error = NOT_OFFLINE_CAPABLE;
      break;
    case content::ServiceWorkerCapability::NO_SERVICE_WORKER:
      if (wait_for_worker) {
        std::move(pause_callback).Run();
        return;
      } else {
        page_data_->worker->has_worker = false;
        page_data_->worker->error = NO_MATCHING_SERVICE_WORKER;
      }
      break;
  }

  InstallableMetrics::RecordCheckServiceWorkerTime(
      base::TimeTicks::Now() - check_service_worker_start_time);
  InstallableMetrics::RecordCheckServiceWorkerStatus(
      InstallableMetrics::ConvertFromServiceWorkerCapability(capability));

  page_data_->worker->fetched = true;
  if (finish_callback) {
    std::move(finish_callback).Run(page_data_->worker->error);
  }
}

void InstallableDataFetcher::CheckAndFetchBestPrimaryIcon(
    FetcherCallback finish_callback,
    bool prefer_maskable,
    bool fetch_favicon) {
  if (blink::IsEmptyManifest(*page_data_->manifest->manifest)) {
    std::move(finish_callback).Run(MANIFEST_DEPENDENT_TASK_NOT_RUN);
    return;
  }
  if (page_data_->primary_icon->fetched) {
    std::move(finish_callback).Run(page_data_->primary_icon->error);
    return;
  }
  icon_fetcher_ = std::make_unique<InstallableIconFetcher>(
      web_contents(), *page_data_, page_data_->GetManifest().icons,
      prefer_maskable, fetch_favicon, std::move(finish_callback));
}

void InstallableDataFetcher::CheckAndFetchScreenshots(
    FetcherCallback finish_callback) {
  if (page_data_->is_screenshots_fetch_complete) {
    std::move(finish_callback).Run(NO_ERROR_DETECTED);
    return;
  }

  screenshots_downloading_ = 0;
  screenshot_complete_ = std::move(finish_callback);

  int num_of_screenshots = 0;
  for (const auto& url : page_data_->GetManifest().screenshots) {
#if BUILDFLAG(IS_ANDROID)
    if (url->form_factor ==
        blink::mojom::ManifestScreenshot::FormFactor::kWide) {
      continue;
    }
#else
    if (url->form_factor !=
        blink::mojom::ManifestScreenshot::FormFactor::kWide) {
      continue;
    }
#endif  // BUILDFLAG(IS_ANDROID)

    if (++num_of_screenshots > kMaximumNumOfScreenshots) {
      break;
    }

    // A screenshot URL that's in the map is already taken care of.
    if (downloaded_screenshots_.count(url->image.src) > 0) {
      continue;
    }

    int ideal_size_in_px = url->image.sizes.empty()
                               ? kMinimumScreenshotSizeInPx
                               : std::max(url->image.sizes[0].width(),
                                          url->image.sizes[0].height());
    // Do not pass in a maximum icon size so that screenshots larger than
    // kMaximumScreenshotSizeInPx are not downscaled to the maximum size by
    // `ManifestIconDownloader::Download`. Screenshots with size larger than
    // kMaximumScreenshotSizeInPx get filtered out by OnScreenshotFetched.
    bool can_download = content::ManifestIconDownloader::Download(
        web_contents(), url->image.src, ideal_size_in_px,
        kMinimumScreenshotSizeInPx,
        /*maximum_icon_size_in_px=*/0,
        base::BindOnce(&InstallableDataFetcher::OnScreenshotFetched,
                       weak_ptr_factory_.GetWeakPtr(), url->image.src),
        /*square_only=*/false);
    if (can_download) {
      ++screenshots_downloading_;
    }
  }

  if (!screenshots_downloading_) {
    page_data_->is_screenshots_fetch_complete = true;
    std::move(screenshot_complete_).Run(NO_ERROR_DETECTED);
  }
}

void InstallableDataFetcher::OnScreenshotFetched(GURL screenshot_url,
                                                 const SkBitmap& bitmap) {
  DCHECK_GT(screenshots_downloading_, 0);

  if (!web_contents()) {
    return;
  }

  if (!bitmap.drawsNothing()) {
    downloaded_screenshots_[screenshot_url] = bitmap;
  }

  if (--screenshots_downloading_ == 0) {
    // Now that all images have finished downloading, populate screenshots in
    // the order they are declared in the manifest.
    int num_of_screenshots = 0;
    for (const auto& url : page_data_->GetManifest().screenshots) {
      if (++num_of_screenshots > kMaximumNumOfScreenshots) {
        break;
      }

      auto iter = downloaded_screenshots_.find(url->image.src);
      if (iter == downloaded_screenshots_.end()) {
        continue;
      }

      auto screenshot = iter->second;
      if (screenshot.dimensions().width() > kMaximumScreenshotSizeInPx ||
          screenshot.dimensions().height() > kMaximumScreenshotSizeInPx) {
        continue;
      }

      // Screenshots must have the same aspect ratio. Cross-multiplying
      // dimensions checks portrait vs landscape mode (1:2 vs 2:1 for instance).
      if (page_data_->screenshots.size() &&
          screenshot.dimensions().width() *
                  page_data_->screenshots[0].image.dimensions().height() !=
              screenshot.dimensions().height() *
                  page_data_->screenshots[0].image.dimensions().width()) {
        continue;
      }

      std::pair<int, int> dimensions =
          std::minmax(screenshot.width(), screenshot.height());
      if (dimensions.second > dimensions.first * kMaximumScreenshotRatio) {
        continue;
      }

      page_data_->screenshots.emplace_back(std::move(screenshot), url->label);
    }

    downloaded_screenshots_.clear();
    page_data_->is_screenshots_fetch_complete = true;
    std::move(screenshot_complete_).Run(NO_ERROR_DETECTED);
  }
}

}  // namespace webapps
