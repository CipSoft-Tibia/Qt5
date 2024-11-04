// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/preloading/prefetch/prefetch_service.h"

#include <memory>
#include <utility>

#include "base/auto_reset.h"
#include "base/barrier_closure.h"
#include "base/feature_list.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/ranges/algorithm.h"
#include "base/timer/timer.h"
#include "content/browser/browser_context_impl.h"
#include "content/browser/devtools/render_frame_devtools_agent_host.h"
#include "content/browser/preloading/prefetch/prefetch_document_manager.h"
#include "content/browser/preloading/prefetch/prefetch_features.h"
#include "content/browser/preloading/prefetch/prefetch_match_resolver.h"
#include "content/browser/preloading/prefetch/prefetch_network_context.h"
#include "content/browser/preloading/prefetch/prefetch_origin_prober.h"
#include "content/browser/preloading/prefetch/prefetch_params.h"
#include "content/browser/preloading/prefetch/prefetch_proxy_configurator.h"
#include "content/browser/preloading/prefetch/prefetch_status.h"
#include "content/browser/preloading/prefetch/prefetch_streaming_url_loader.h"
#include "content/browser/preloading/prefetch/proxy_lookup_client_impl.h"
#include "content/browser/preloading/preloading_attempt_impl.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/frame_accept_header.h"
#include "content/public/browser/prefetch_service_delegate.h"
#include "content/public/browser/preloading.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_constants.h"
#include "net/base/isolation_info.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_partition_key_collection.h"
#include "net/cookies/site_for_cookies.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/devtools_observer_util.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "services/network/public/mojom/devtools_observer.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom-shared.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/blink/public/common/loader/referrer_utils.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace content {

namespace {

static ServiceWorkerContext* g_service_worker_context_for_testing = nullptr;

bool (*g_host_non_unique_filter)(base::StringPiece) = nullptr;

static network::mojom::URLLoaderFactory* g_url_loader_factory_for_testing =
    nullptr;

static network::mojom::NetworkContext*
    g_network_context_for_proxy_lookup_for_testing = nullptr;

bool ShouldConsiderDecoyRequestForStatus(PrefetchStatus status) {
  switch (status) {
    case PrefetchStatus::kPrefetchNotEligibleUserHasCookies:
    case PrefetchStatus::kPrefetchNotEligibleUserHasServiceWorker:
      // If the prefetch is not eligible because of cookie or a service worker,
      // then maybe send a decoy.
      return true;
    case PrefetchStatus::kPrefetchNotEligibleSchemeIsNotHttps:
    case PrefetchStatus::kPrefetchNotEligibleNonDefaultStoragePartition:
    case PrefetchStatus::kPrefetchIneligibleRetryAfter:
    case PrefetchStatus::kPrefetchProxyNotAvailable:
    case PrefetchStatus::kPrefetchNotEligibleHostIsNonUnique:
    case PrefetchStatus::kPrefetchNotEligibleDataSaverEnabled:
    case PrefetchStatus::kPrefetchNotEligibleBatterySaverEnabled:
    case PrefetchStatus::kPrefetchNotEligiblePreloadingDisabled:
    case PrefetchStatus::kPrefetchNotEligibleExistingProxy:
    case PrefetchStatus::kPrefetchNotEligibleBrowserContextOffTheRecord:
    case PrefetchStatus::
        kPrefetchNotEligibleSameSiteCrossOriginPrefetchRequiredProxy:
      // These statuses don't relate to any user state, so don't send a decoy
      // request.
      return false;
    case PrefetchStatus::kPrefetchNotUsedProbeFailed:
    case PrefetchStatus::kPrefetchNotStarted:
    case PrefetchStatus::kPrefetchNotFinishedInTime:
    case PrefetchStatus::kPrefetchFailedNetError:
    case PrefetchStatus::kPrefetchFailedNon2XX:
    case PrefetchStatus::kPrefetchFailedMIMENotSupported:
    case PrefetchStatus::kPrefetchSuccessful:
    case PrefetchStatus::kPrefetchIsPrivacyDecoy:
    case PrefetchStatus::kPrefetchIsStale:
    case PrefetchStatus::kPrefetchNotUsedCookiesChanged:
    case PrefetchStatus::kPrefetchResponseUsed:
    case PrefetchStatus::kPrefetchHeldback:
    case PrefetchStatus::kPrefetchAllowed:
    case PrefetchStatus::kPrefetchFailedInvalidRedirect:
    case PrefetchStatus::kPrefetchFailedIneligibleRedirect:
    case PrefetchStatus::kPrefetchFailedPerPageLimitExceeded:
    case PrefetchStatus::kPrefetchEvicted:
      // These statuses should not be returned by the eligibility checks, and
      // thus not be passed in here.
      NOTREACHED();
      return false;
  }
}

bool ShouldStartSpareRenderer() {
  if (!PrefetchStartsSpareRenderer()) {
    return false;
  }

  for (RenderProcessHost::iterator iter(RenderProcessHost::AllHostsIterator());
       !iter.IsAtEnd(); iter.Advance()) {
    if (iter.GetCurrentValue()->IsUnused()) {
      // There is already a spare renderer.
      return false;
    }
  }
  return true;
}

void RecordPrefetchProxyPrefetchMainframeTotalTime(
    network::mojom::URLResponseHead* head) {
  DCHECK(head);

  base::Time start = head->request_time;
  base::Time end = head->response_time;

  if (start.is_null() || end.is_null()) {
    return;
  }

  UMA_HISTOGRAM_CUSTOM_TIMES("PrefetchProxy.Prefetch.Mainframe.TotalTime",
                             end - start, base::Milliseconds(10),
                             base::Seconds(30), 100);
}

void RecordPrefetchProxyPrefetchMainframeConnectTime(
    network::mojom::URLResponseHead* head) {
  DCHECK(head);

  base::TimeTicks start = head->load_timing.connect_timing.connect_start;
  base::TimeTicks end = head->load_timing.connect_timing.connect_end;

  if (start.is_null() || end.is_null()) {
    return;
  }

  UMA_HISTOGRAM_TIMES("PrefetchProxy.Prefetch.Mainframe.ConnectTime",
                      end - start);
}

void RecordPrefetchProxyPrefetchMainframeRespCode(int response_code) {
  base::UmaHistogramSparse("PrefetchProxy.Prefetch.Mainframe.RespCode",
                           response_code);
}

void RecordPrefetchProxyPrefetchMainframeNetError(int net_error) {
  base::UmaHistogramSparse("PrefetchProxy.Prefetch.Mainframe.NetError",
                           std::abs(net_error));
}

void RecordPrefetchProxyPrefetchMainframeBodyLength(int64_t body_length) {
  UMA_HISTOGRAM_COUNTS_10M("PrefetchProxy.Prefetch.Mainframe.BodyLength",
                           body_length);
}

void RecordPrefetchProxyPrefetchMainframeCookiesToCopy(
    size_t cookie_list_size) {
  UMA_HISTOGRAM_COUNTS_100("PrefetchProxy.Prefetch.Mainframe.CookiesToCopy",
                           cookie_list_size);
}

void CookieSetHelper(base::RepeatingClosure closure,
                     net::CookieAccessResult access_result) {
  closure.Run();
}

// Returns true if the prefetch is heldback, and set the holdback status
// correspondingly.
bool CheckAndSetPrefetchHoldbackStatus(
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  if (!prefetch_container->HasPreloadingAttempt()) {
    return false;
  }

  // Normally CheckIfShouldHoldback() computes the holdback status based on
  // PreloadingConfig. In special cases, we call SetHoldbackOverride() to
  // override that processing.
  RenderFrameHostImpl* initiator_rfh = RenderFrameHostImpl::FromID(
      prefetch_container->GetReferringRenderFrameHostId());
  bool devtools_client_exist =
      initiator_rfh &&
      RenderFrameDevToolsAgentHost::GetFor(initiator_rfh) != nullptr;
  if (devtools_client_exist) {
    prefetch_container->preloading_attempt()->SetHoldbackStatus(
        PreloadingHoldbackStatus::kAllowed);
  } else if (IsContentPrefetchHoldback()) {
    // In addition to the globally-controlled preloading config, check for the
    // feature-specific holdback. We disable the feature if the user is in
    // either of those holdbacks.

    prefetch_container->preloading_attempt()->SetHoldbackStatus(
        PreloadingHoldbackStatus::kHoldback);
  }

  if (prefetch_container->preloading_attempt()->ShouldHoldback()) {
    prefetch_container->SetPrefetchStatus(PrefetchStatus::kPrefetchHeldback);
    return true;
  }
  return false;
}

BrowserContext* BrowserContextFromFrameTreeNodeId(int frame_tree_node_id) {
  WebContents* web_content =
      WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (!web_content) {
    return nullptr;
  }
  return web_content->GetBrowserContext();
}

void RecordRedirectResult(PrefetchRedirectResult result) {
  UMA_HISTOGRAM_ENUMERATION("PrefetchProxy.Redirect.Result", result);
}

void RecordRedirectNetworkContextTransition(
    bool previous_requires_isolated_network_context,
    bool redirect_requires_isolated_network_context) {
  PrefetchRedirectNetworkContextTransition transition;
  if (!previous_requires_isolated_network_context &&
      !redirect_requires_isolated_network_context) {
    transition = PrefetchRedirectNetworkContextTransition::kDefaultToDefault;
  }
  if (!previous_requires_isolated_network_context &&
      redirect_requires_isolated_network_context) {
    transition = PrefetchRedirectNetworkContextTransition::kDefaultToIsolated;
  }
  if (previous_requires_isolated_network_context &&
      !redirect_requires_isolated_network_context) {
    transition = PrefetchRedirectNetworkContextTransition::kIsolatedToDefault;
  }
  if (previous_requires_isolated_network_context &&
      redirect_requires_isolated_network_context) {
    transition = PrefetchRedirectNetworkContextTransition::kIsolatedToIsolated;
  }

  UMA_HISTOGRAM_ENUMERATION(
      "PrefetchProxy.Redirect.NetworkContextStateTransition", transition);
}

void OnIsolatedCookieCopyComplete(PrefetchContainer::Reader reader) {
  if (reader) {
    reader.OnIsolatedCookieCopyComplete();
  }
}

void BlockUntilHeadTimeoutHelper(
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  if (!prefetch_container || !prefetch_container->GetLastStreamingURLLoader()) {
    return;
  }

  // Takes the on_received_head_callback
  base::OnceClosure on_received_head_callback =
      prefetch_container->ReleaseOnReceivedHeadCallback();
  if (on_received_head_callback) {
    std::move(on_received_head_callback).Run();
  }
}

bool IsReferrerPolicySufficientlyStrict(
    const network::mojom::ReferrerPolicy& referrer_policy) {
  // https://github.com/WICG/nav-speculation/blob/main/prefetch.bs#L606
  // "", "`strict-origin-when-cross-origin`", "`strict-origin`",
  // "`same-origin`", "`no-referrer`".
  switch (referrer_policy) {
    case network::mojom::ReferrerPolicy::kDefault:
    case network::mojom::ReferrerPolicy::kStrictOriginWhenCrossOrigin:
    case network::mojom::ReferrerPolicy::kSameOrigin:
    case network::mojom::ReferrerPolicy::kStrictOrigin:
      return true;
    case network::mojom::ReferrerPolicy::kAlways:
    case network::mojom::ReferrerPolicy::kNoReferrerWhenDowngrade:
    case network::mojom::ReferrerPolicy::kNever:
    case network::mojom::ReferrerPolicy::kOrigin:
    case network::mojom::ReferrerPolicy::kOriginWhenCrossOrigin:
      return false;
  }
}

}  // namespace

// static
PrefetchService* PrefetchService::GetFromFrameTreeNodeId(
    int frame_tree_node_id) {
  BrowserContext* browser_context =
      BrowserContextFromFrameTreeNodeId(frame_tree_node_id);
  if (!browser_context) {
    return nullptr;
  }
  return BrowserContextImpl::From(browser_context)->GetPrefetchService();
}

void PrefetchService::SetFromFrameTreeNodeIdForTesting(
    int frame_tree_node_id,
    std::unique_ptr<PrefetchService> prefetch_service) {
  BrowserContext* browser_context =
      BrowserContextFromFrameTreeNodeId(frame_tree_node_id);
  CHECK(browser_context);
  return BrowserContextImpl::From(browser_context)
      ->SetPrefetchServiceForTesting(std::move(prefetch_service));  // IN-TEST
}

PrefetchService::PrefetchService(BrowserContext* browser_context)
    : browser_context_(browser_context),
      delegate_(GetContentClient()->browser()->CreatePrefetchServiceDelegate(
          browser_context_)),
      prefetch_proxy_configurator_(
          PrefetchProxyConfigurator::MaybeCreatePrefetchProxyConfigurator(
              PrefetchProxyHost(delegate_
                                    ? delegate_->GetDefaultPrefetchProxyHost()
                                    : GURL("")),
              delegate_ ? delegate_->GetAPIKey() : "")),
      origin_prober_(std::make_unique<PrefetchOriginProber>(
          browser_context_,
          PrefetchDNSCanaryCheckURL(
              delegate_ ? delegate_->GetDefaultDNSCanaryCheckURL() : GURL("")),
          PrefetchTLSCanaryCheckURL(
              delegate_ ? delegate_->GetDefaultTLSCanaryCheckURL()
                        : GURL("")))) {}

PrefetchService::~PrefetchService() = default;

PrefetchOriginProber* PrefetchService::GetPrefetchOriginProber() const {
  return origin_prober_.get();
}

void PrefetchService::PrefetchUrl(
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  DCHECK(prefetch_container);
  auto prefetch_container_key = prefetch_container->GetPrefetchContainerKey();

  if (delegate_) {
    // If pre* actions are disabled then don't prefetch.
    switch (delegate_->IsSomePreloadingEnabled()) {
      case PreloadingEligibility::kEligible:
        break;
      case PreloadingEligibility::kDataSaverEnabled:
        OnGotEligibilityResult(
            prefetch_container, false,
            PrefetchStatus::kPrefetchNotEligibleDataSaverEnabled);
        return;
      case PreloadingEligibility::kBatterySaverEnabled:
        OnGotEligibilityResult(
            prefetch_container, false,
            PrefetchStatus::kPrefetchNotEligibleBatterySaverEnabled);
        return;
      case PreloadingEligibility::kPreloadingDisabled:
        OnGotEligibilityResult(
            prefetch_container, false,
            PrefetchStatus::kPrefetchNotEligiblePreloadingDisabled);
        return;
      default:
        DVLOG(1) << *prefetch_container
                 << ": not prefetched (PrefetchServiceDelegate)";
        return;
    }

    const auto& prefetch_type = prefetch_container->GetPrefetchType();
    if (prefetch_type.IsProxyRequiredWhenCrossOrigin() &&
        !prefetch_type.IsProxyBypassedForTesting()) {
      bool allow_all_domains =
          PrefetchAllowAllDomains() ||
          (PrefetchAllowAllDomainsForExtendedPreloading() &&
           delegate_->IsExtendedPreloadingEnabled());
      if (!allow_all_domains &&
          !delegate_->IsDomainInPrefetchAllowList(
              RenderFrameHost::FromID(
                  prefetch_container->GetReferringRenderFrameHostId())
                  ->GetLastCommittedURL())) {
        DVLOG(1) << *prefetch_container
                 << ": not prefetched (not in allow list)";
        return;
      }
    }

    delegate_->OnPrefetchLikely(WebContents::FromRenderFrameHost(
        &prefetch_container->GetPrefetchDocumentManager()
             ->render_frame_host()));
  }

  RecordExistingPrefetchWithMatchingURL(prefetch_container);

  // A newly submitted prefetch could already be in |all_prefetches_| if and
  // only if:
  //   1) There was a same origin navigaition that used the same renderer.
  //   2) Both pages requested a prefetch for the same URL.
  //   3) The prefetch from the first page had at least started its network
  //      request (which would mean that it is in |owned_prefetches_| and owned
  //      by the prefetch service).
  // If this happens, then we just delete the old prefetch and add the new
  // prefetch to |all_prefetches_|.
  auto prefetch_iter = all_prefetches_.find(prefetch_container_key);
  if (prefetch_iter != all_prefetches_.end() && prefetch_iter->second) {
    ResetPrefetch(prefetch_iter->second);
  }
  all_prefetches_[prefetch_container_key] = prefetch_container;

  CheckEligibilityOfPrefetch(
      prefetch_container->GetURL(), prefetch_container,
      base::BindOnce(&PrefetchService::OnGotEligibilityResult,
                     weak_method_factory_.GetWeakPtr()));
}

void PrefetchService::CheckEligibilityOfPrefetch(
    const GURL& url,
    base::WeakPtr<PrefetchContainer> prefetch_container,
    OnEligibilityResultCallback result_callback) const {
  CHECK(prefetch_container);

  // TODO(https://crbug.com/1299059): Clean up the following checks by: 1)
  // moving each check to a separate function, and 2) requiring that failed
  // checks provide a PrefetchStatus related to the check.

  if (browser_context_->IsOffTheRecord()) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchNotEligibleBrowserContextOffTheRecord);
    return;
  }

  // While a registry-controlled domain could still resolve to a non-publicly
  // routable IP, this allows hosts which are very unlikely to work via the
  // proxy to be discarded immediately.
  bool is_host_non_unique =
      g_host_non_unique_filter ? g_host_non_unique_filter(url.HostNoBrackets())
                               : net::IsHostnameNonUnique(url.HostNoBrackets());
  if (!prefetch_container->GetPrefetchType().IsProxyBypassedForTesting() &&
      prefetch_container->IsProxyRequiredForURL(url) && is_host_non_unique) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchNotEligibleHostIsNonUnique);
    return;
  }

  // Only HTTP(S) URLs which are believed to be secure are eligible.
  // For proxied prefetches, we only want HTTPS URLs.
  // For non-proxied prefetches, other URLs (notably localhost HTTP) is also
  // acceptable. This is common during development.
  const bool is_secure_http = prefetch_container->IsProxyRequiredForURL(url)
                                  ? url.SchemeIs(url::kHttpsScheme)
                                  : (url.SchemeIsHTTPOrHTTPS() &&
                                     network::IsUrlPotentiallyTrustworthy(url));
  if (!is_secure_http) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchNotEligibleSchemeIsNotHttps);
    return;
  }

  if (prefetch_container->IsProxyRequiredForURL(url) &&
      !prefetch_container->GetPrefetchType().IsProxyBypassedForTesting() &&
      (!prefetch_proxy_configurator_ ||
       !prefetch_proxy_configurator_->IsPrefetchProxyAvailable())) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchProxyNotAvailable);
    return;
  }

  // Only the default storage partition is supported since that is where we
  // check for service workers and existing cookies.
  StoragePartition* default_storage_partition =
      browser_context_->GetDefaultStoragePartition();
  if (default_storage_partition !=
      browser_context_->GetStoragePartitionForUrl(url,
                                                  /*can_create=*/false)) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchNotEligibleNonDefaultStoragePartition);
    return;
  }

  // If we have recently received a "retry-after" for the origin, then don't
  // send new prefetches.
  if (delegate_ && !delegate_->IsOriginOutsideRetryAfterWindow(url)) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchIneligibleRetryAfter);
    return;
  }

  // This blocks same-site cross-origin prefetches that require the prefetch
  // proxy. Same-site prefetches are made using the default network context, and
  // the prefetch request cannot be configured to use the proxy in that network
  // context.
  // TODO(https://crbug.com/1439986): Allow same-site cross-origin prefetches
  // that require the prefetch proxy to be made.
  if (prefetch_container->IsProxyRequiredForURL(url) &&
      !prefetch_container
           ->IsIsolatedNetworkContextRequiredForCurrentPrefetch()) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::
                 kPrefetchNotEligibleSameSiteCrossOriginPrefetchRequiredProxy);
    return;
  }

  // We do not need to check the cookies of prefetches that do not need an
  // isolated network context.
  if (!prefetch_container
           ->IsIsolatedNetworkContextRequiredForCurrentPrefetch()) {
    std::move(result_callback).Run(prefetch_container, true, absl::nullopt);
    return;
  }

  CheckHasServiceWorker(url, prefetch_container, std::move(result_callback));
}

void PrefetchService::CheckHasServiceWorker(
    const GURL& url,
    base::WeakPtr<PrefetchContainer> prefetch_container,
    OnEligibilityResultCallback result_callback) const {
  CHECK(prefetch_container);
  // This service worker check assumes that the prefetch will only ever be
  // performed in a first-party context (main frame prefetch). At the moment
  // that is true but if it ever changes then the StorageKey will need to be
  // constructed with the top-level site to ensure correct partitioning.
  ServiceWorkerContext* service_worker_context =
      g_service_worker_context_for_testing
          ? g_service_worker_context_for_testing
          : browser_context_->GetDefaultStoragePartition()
                ->GetServiceWorkerContext();
  auto key = blink::StorageKey::CreateFirstParty(url::Origin::Create(url));
  // Check `MaybeHasRegistrationForStorageKey` first as it is much faster than
  // calling `CheckHasServiceWorker`.
  if (!service_worker_context->MaybeHasRegistrationForStorageKey(key)) {
    OnGotServiceWorkerResult(url, prefetch_container,
                             std::move(result_callback),
                             ServiceWorkerCapability::NO_SERVICE_WORKER);
    return;
  }
  service_worker_context->CheckHasServiceWorker(
      url, key,
      base::BindOnce(&PrefetchService::OnGotServiceWorkerResult,
                     weak_method_factory_.GetWeakPtr(), url, prefetch_container,
                     std::move(result_callback)));
}

void PrefetchService::OnGotServiceWorkerResult(
    const GURL& url,
    base::WeakPtr<PrefetchContainer> prefetch_container,
    OnEligibilityResultCallback result_callback,
    ServiceWorkerCapability service_worker_capability) const {
  switch (service_worker_capability) {
    case ServiceWorkerCapability::NO_SERVICE_WORKER:
    case ServiceWorkerCapability::SERVICE_WORKER_NO_FETCH_HANDLER:
      break;
    case ServiceWorkerCapability::SERVICE_WORKER_WITH_FETCH_HANDLER:
      std::move(result_callback)
          .Run(prefetch_container, false,
               PrefetchStatus::kPrefetchNotEligibleUserHasServiceWorker);
      return;
  }
  StoragePartition* default_storage_partition =
      browser_context_->GetDefaultStoragePartition();
  CHECK(default_storage_partition);
  net::CookieOptions options = net::CookieOptions::MakeAllInclusive();
  options.set_return_excluded_cookies();
  default_storage_partition->GetCookieManagerForBrowserProcess()->GetCookieList(
      url, options, net::CookiePartitionKeyCollection::Todo(),
      base::BindOnce(&PrefetchService::OnGotCookiesForEligibilityCheck,
                     weak_method_factory_.GetWeakPtr(), url, prefetch_container,
                     std::move(result_callback)));
}

void PrefetchService::OnGotCookiesForEligibilityCheck(
    const GURL& url,
    base::WeakPtr<PrefetchContainer> prefetch_container,
    OnEligibilityResultCallback result_callback,
    const net::CookieAccessResultList& cookie_list,
    const net::CookieAccessResultList& excluded_cookies) const {
  if (!prefetch_container) {
    std::move(result_callback).Run(prefetch_container, false, absl::nullopt);
    return;
  }

  if (!cookie_list.empty()) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchNotEligibleUserHasCookies);
    return;
  }

  // Cookies are tricky because cookies for different paths or a higher level
  // domain (e.g.: m.foo.com and foo.com) may not show up in |cookie_list|, but
  // they will show up in |excluded_cookies|. To check for any cookies for a
  // domain, compare the domains of the prefetched |url| and the domains of all
  // the returned cookies.
  bool excluded_cookie_has_tld = false;
  for (const auto& cookie_result : excluded_cookies) {
    if (cookie_result.cookie.IsExpired(base::Time::Now())) {
      // Expired cookies don't count.
      continue;
    }

    if (url.DomainIs(cookie_result.cookie.DomainWithoutDot())) {
      excluded_cookie_has_tld = true;
      break;
    }
  }

  if (excluded_cookie_has_tld) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchNotEligibleUserHasCookies);
    return;
  }

  StartProxyLookupCheck(url, prefetch_container, std::move(result_callback));
}

void PrefetchService::StartProxyLookupCheck(
    const GURL& url,
    base::WeakPtr<PrefetchContainer> prefetch_container,
    OnEligibilityResultCallback result_callback) const {
  // Same origin prefetches (which use the default network context and cannot
  // use the prefetch proxy) can use the existing proxy settings.
  // TODO(https://crbug.com/1343903): Copy proxy settings over to the isolated
  // network context for the prefetch in order to allow non-private cross origin
  // prefetches to be made using the existing proxy settings.
  if (!prefetch_container
           ->IsIsolatedNetworkContextRequiredForCurrentPrefetch()) {
    std::move(result_callback).Run(prefetch_container, true, absl::nullopt);
    return;
  }

  // Start proxy check for this prefetch, and give ownership of the
  // |ProxyLookupClientImpl| to |prefetch_container|.
  auto network_anonymization_key =
      net::NetworkAnonymizationKey::CreateSameSite(net::SchemefulSite(url));
  prefetch_container->TakeProxyLookupClient(
      std::make_unique<ProxyLookupClientImpl>(
          url, network_anonymization_key,
          base::BindOnce(&PrefetchService::OnGotProxyLookupResult,
                         weak_method_factory_.GetWeakPtr(), prefetch_container,
                         std::move(result_callback)),
          g_network_context_for_proxy_lookup_for_testing
              ? g_network_context_for_proxy_lookup_for_testing
              : browser_context_->GetDefaultStoragePartition()
                    ->GetNetworkContext()));
}

void PrefetchService::OnGotProxyLookupResult(
    base::WeakPtr<PrefetchContainer> prefetch_container,
    OnEligibilityResultCallback result_callback,
    bool has_proxy) const {
  if (!prefetch_container) {
    std::move(result_callback).Run(prefetch_container, false, absl::nullopt);
    return;
  }

  prefetch_container->ReleaseProxyLookupClient();
  if (has_proxy) {
    std::move(result_callback)
        .Run(prefetch_container, false,
             PrefetchStatus::kPrefetchNotEligibleExistingProxy);
    return;
  }
  std::move(result_callback).Run(prefetch_container, true, absl::nullopt);
}

void PrefetchService::OnGotEligibilityResult(
    base::WeakPtr<PrefetchContainer> prefetch_container,
    bool eligible,
    absl::optional<PrefetchStatus> status) {
  if (!prefetch_container) {
    return;
  }

  bool is_decoy = false;
  if (!eligible) {
    // Expect a status if the container is alive but prefetch not eligible.
    DCHECK(status.has_value());
    is_decoy =
        prefetch_container->IsProxyRequiredForURL(
            prefetch_container->GetURL()) &&
        ShouldConsiderDecoyRequestForStatus(status.value()) &&
        PrefetchServiceSendDecoyRequestForIneligblePrefetch(
            delegate_ ? delegate_->DisableDecoysBasedOnUserSettings() : false);
  }
  // The prefetch decoy is pushed onto the queue and the network request will be
  // dispatched, but the response will not be used. Thus it is eligible but a
  // failure.
  prefetch_container->SetIsDecoy(is_decoy);
  if (is_decoy) {
    prefetch_container->OnEligibilityCheckComplete(true, absl::nullopt);
  } else {
    prefetch_container->OnEligibilityCheckComplete(eligible, status);
  }

  if (!eligible && !is_decoy) {
    DVLOG(1) << *prefetch_container
             << ": not prefetched (not eligible nor decoy. PrefetchStatus="
             << static_cast<int>(*status) << ")";
    return;
  }

  if (!is_decoy) {
    prefetch_container->SetPrefetchStatus(PrefetchStatus::kPrefetchNotStarted);

    // Registers a cookie listener for this prefetch if it is using an isolated
    // network context. If the cookies in the default partition associated with
    // this URL change after this point, then the prefetched resources should
    // not be served.
    if (prefetch_container
            ->IsIsolatedNetworkContextRequiredForCurrentPrefetch()) {
      prefetch_container->RegisterCookieListener(
          browser_context_->GetDefaultStoragePartition()
              ->GetCookieManagerForBrowserProcess());
    }
  }
  prefetch_queue_.push_back(prefetch_container);

  // Calling |Prefetch| could result in a prefetch being deleted, so
  // |prefetch_container| should not be used after this call.
  Prefetch();
}

void PrefetchService::OnGotEligibilityResultForRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr redirect_head,
    base::WeakPtr<PrefetchContainer> prefetch_container,
    bool eligible,
    absl::optional<PrefetchStatus> status) {
  if (!prefetch_container) {
    return;
  }

  RecordRedirectResult(eligible
                           ? PrefetchRedirectResult::kSuccessRedirectFollowed
                           : PrefetchRedirectResult::kFailedIneligible);

  // If the redirect is ineligible, the prefetch may change into a decoy.
  bool is_decoy = false;
  if (!eligible) {
    // Expect a status if the container is alive but prefetch not eligible.
    DCHECK(status.has_value());
    is_decoy =
        prefetch_container->IsProxyRequiredForURL(redirect_info.new_url) &&
        ShouldConsiderDecoyRequestForStatus(status.value()) &&
        PrefetchServiceSendDecoyRequestForIneligblePrefetch(
            delegate_ ? delegate_->DisableDecoysBasedOnUserSettings() : false);
  }
  prefetch_container->SetIsDecoy(prefetch_container->IsDecoy() || is_decoy);

  // Inform the prefetch container of the result of the eligibility check
  if (prefetch_container->IsDecoy()) {
    prefetch_container->OnEligibilityCheckComplete(true, absl::nullopt);
  } else {
    prefetch_container->OnEligibilityCheckComplete(eligible, status);
    if (eligible &&
        prefetch_container
            ->IsIsolatedNetworkContextRequiredForCurrentPrefetch()) {
      prefetch_container->RegisterCookieListener(
          browser_context_->GetDefaultStoragePartition()
              ->GetCookieManagerForBrowserProcess());
    }
  }

  // If the redirect is not eligible and the prefetch is not a decoy, then stop
  // the prefetch.
  if (!eligible && !prefetch_container->IsDecoy()) {
    active_prefetches_.erase(prefetch_container->GetPrefetchContainerKey());
    prefetch_container->GetLastStreamingURLLoader()->HandleRedirect(
        PrefetchRedirectStatus::kFail, redirect_info, std::move(redirect_head));
    prefetch_container->ResetAllStreamingURLLoaders();

    Prefetch();

    return;
  }

  const auto& devtools_observer = prefetch_container->GetDevToolsObserver();
  if (devtools_observer && !prefetch_container->IsDecoy()) {
    GURL previous_url = prefetch_container->GetPreviousURL();
    auto redirect_head_info = network::ExtractDevToolsInfo(*redirect_head);
    std::pair<const GURL&, const network::mojom::URLResponseHeadDevToolsInfo&>
        redirect_info_for_devtools{previous_url, *redirect_head_info};
    devtools_observer->OnStartSinglePrefetch(
        prefetch_container->RequestId(),
        *prefetch_container->GetResourceRequest(),
        std::move(redirect_info_for_devtools));
  }

  // If the redirect requires a change in network contexts, then stop the
  // current streaming URL loader and start a new streaming URL loader for the
  // redirect URL.
  if (prefetch_container
          ->IsIsolatedNetworkContextRequiredForCurrentPrefetch() !=
      prefetch_container
          ->IsIsolatedNetworkContextRequiredForPreviousRedirectHop()) {
    prefetch_container->GetLastStreamingURLLoader()->HandleRedirect(
        PrefetchRedirectStatus::kSwitchNetworkContext, redirect_info,
        std::move(redirect_head));
    // The new ResponseReader is associated with the new streaming URL loader at
    // the PrefetchStreamingURLLoader constructor.
    SendPrefetchRequest(prefetch_container);

    return;
  }

  // Otherwise, follow the redirect in the same streaming URL loader.
  prefetch_container->GetLastStreamingURLLoader()->HandleRedirect(
      PrefetchRedirectStatus::kFollow, redirect_info, std::move(redirect_head));
  // Associate the new ResponseReader with the current streaming URL loader.
  prefetch_container->GetLastStreamingURLLoader()->SetResponseReader(
      prefetch_container->GetResponseReaderForCurrentPrefetch());
}

void PrefetchService::Prefetch() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

// Asserts that re-entrancy doesn't happen.
#if DCHECK_IS_ON()
  DCHECK(!prefetch_reentrancy_guard_);
  base::AutoReset reset_guard(&prefetch_reentrancy_guard_, true);
#endif

  if (PrefetchCloseIdleSockets()) {
    for (const auto& iter : all_prefetches_) {
      if (iter.second) {
        iter.second->CloseIdleConnections();
      }
    }
  }

  base::WeakPtr<PrefetchContainer> next_prefetch = nullptr;
  base::WeakPtr<PrefetchContainer> prefetch_to_evict = nullptr;
  while ((std::tie(next_prefetch, prefetch_to_evict) =
              PopNextPrefetchContainer()) !=
         std::make_tuple(nullptr, nullptr)) {
    StartSinglePrefetch(next_prefetch, prefetch_to_evict);
  }
}

std::tuple<base::WeakPtr<PrefetchContainer>, base::WeakPtr<PrefetchContainer>>
PrefetchService::PopNextPrefetchContainer() {
  auto new_end = std::remove_if(
      prefetch_queue_.begin(), prefetch_queue_.end(),
      [&](const base::WeakPtr<PrefetchContainer>& prefetch_container) {
        // Remove all prefetches from queue that no longer exist.
        return !prefetch_container;
      });
  prefetch_queue_.erase(new_end, prefetch_queue_.end());

  // Don't start any new prefetches if we are currently at or beyond the limit
  // for the number of concurrent prefetches.
  DCHECK(PrefetchServiceMaximumNumberOfConcurrentPrefetches() >= 0);
  if (active_prefetches_.size() >=
      PrefetchServiceMaximumNumberOfConcurrentPrefetches()) {
    return std::make_tuple(nullptr, nullptr);
  }

  base::WeakPtr<PrefetchContainer> prefetch_to_evict;
  // Get the first prefetch can be prefetched currently. This depends on the
  // state of the initiating document, and the number of completed prefetches
  // (this can also result in previously completed prefetches being evicted).
  auto prefetch_iter = base::ranges::find_if(
      prefetch_queue_,
      [&](const base::WeakPtr<PrefetchContainer>& prefetch_container) {
        bool can_prefetch_now = false;
        std::tie(can_prefetch_now, prefetch_to_evict) =
            prefetch_container->GetPrefetchDocumentManager()->CanPrefetchNow(
                prefetch_container.get());
        // |prefetch_to_evict| should only be set if |can_prefetch_now| is true.
        DCHECK(!prefetch_to_evict || can_prefetch_now);
        return can_prefetch_now;
      });
  if (prefetch_iter == prefetch_queue_.end()) {
    return std::make_tuple(nullptr, nullptr);
  }

  base::WeakPtr<PrefetchContainer> next_prefetch_container = *prefetch_iter;
  prefetch_queue_.erase(prefetch_iter);
  return std::make_tuple(next_prefetch_container, prefetch_to_evict);
}

void PrefetchService::TakeOwnershipOfPrefetch(
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  DCHECK(prefetch_container);

  // Take ownership of the |PrefetchContainer| from the
  // |PrefetchDocumentManager|.
  PrefetchDocumentManager* prefetch_document_manager =
      prefetch_container->GetPrefetchDocumentManager();
  DCHECK(prefetch_document_manager);
  std::unique_ptr<PrefetchContainer> owned_prefetch_container =
      prefetch_document_manager->ReleasePrefetchContainer(
          prefetch_container->GetURL());
  DCHECK(owned_prefetch_container.get() == prefetch_container.get());

  // Create callback to delete the prefetch container after
  // |PrefetchContainerLifetimeInPrefetchService|.
  base::TimeDelta reset_delta = PrefetchContainerLifetimeInPrefetchService();
  std::unique_ptr<base::OneShotTimer> reset_callback = nullptr;
  if (reset_delta.is_positive()) {
    reset_callback = std::make_unique<base::OneShotTimer>();
    reset_callback->Start(
        FROM_HERE, PrefetchContainerLifetimeInPrefetchService(),
        base::BindOnce(&PrefetchService::OnPrefetchTimeout,
                       base::Unretained(this), prefetch_container));
  }

  // Store prefetch and callback to delete prefetch.
  owned_prefetches_[prefetch_container->GetPrefetchContainerKey()] =
      std::make_pair(std::move(owned_prefetch_container),
                     std::move(reset_callback));
}

void PrefetchService::OnPrefetchTimeout(
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  ResetPrefetch(prefetch_container);

  if (PrefetchNewLimitsEnabled() &&
      active_prefetches_.size() <
          PrefetchServiceMaximumNumberOfConcurrentPrefetches()) {
    Prefetch();
  }
}

void PrefetchService::ResetPrefetch(
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  DCHECK(prefetch_container);
  DCHECK(
      owned_prefetches_.find(prefetch_container->GetPrefetchContainerKey()) !=
      owned_prefetches_.end());

  RemovePrefetch(prefetch_container->GetPrefetchContainerKey());

  auto active_prefetch_iter =
      active_prefetches_.find(prefetch_container->GetPrefetchContainerKey());
  if (active_prefetch_iter != active_prefetches_.end()) {
    active_prefetches_.erase(active_prefetch_iter);
  }

  auto prefetches_ready_to_serve_iter = prefetches_ready_to_serve_.find(
      prefetch_container->GetPrefetchContainerKey());
  if (prefetches_ready_to_serve_iter != prefetches_ready_to_serve_.end() &&
      prefetches_ready_to_serve_iter->second->GetPrefetchContainerKey() ==
          prefetch_container->GetPrefetchContainerKey()) {
    prefetches_ready_to_serve_.erase(prefetches_ready_to_serve_iter);
  }

  owned_prefetches_.erase(
      owned_prefetches_.find(prefetch_container->GetPrefetchContainerKey()));
}

void PrefetchService::RemovePrefetch(
    const PrefetchContainer::Key& prefetch_container_key) {
  const auto prefetch_iter = all_prefetches_.find(prefetch_container_key);
  if (prefetch_iter != all_prefetches_.end()) {
    all_prefetches_.erase(prefetch_iter);
  }
}

void PrefetchService::EvictPrefetch(
    const PrefetchContainer::Key& prefetch_container_key) {
  DCHECK(PrefetchNewLimitsEnabled());
  DCHECK(base::Contains(owned_prefetches_, prefetch_container_key));
  base::WeakPtr<PrefetchContainer> prefetch_container =
      owned_prefetches_[prefetch_container_key].first->GetWeakPtr();
  DCHECK(prefetch_container);
  prefetch_container->SetPrefetchStatus(PrefetchStatus::kPrefetchEvicted);
  ResetPrefetch(prefetch_container);
}

void PrefetchService::OnCandidatesUpdated() {
  if (active_prefetches_.size() <
      PrefetchServiceMaximumNumberOfConcurrentPrefetches()) {
    Prefetch();
  }
}

void PrefetchService::StartSinglePrefetch(
    base::WeakPtr<PrefetchContainer> prefetch_container,
    base::WeakPtr<PrefetchContainer> prefetch_to_evict) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(prefetch_container);

  // Do not prefetch for a Holdback control group. Called after the checks in
  // `PopNextPrefetchContainer` because we want to compare against the
  // prefetches that would have been dispatched.
  if (CheckAndSetPrefetchHoldbackStatus(prefetch_container)) {
    DVLOG(1) << *prefetch_container
             << ": not prefetched (holdback control group)";
    return;
  }

  TakeOwnershipOfPrefetch(prefetch_container);

  const bool is_above_limit =
      !PrefetchNewLimitsEnabled() &&
      prefetch_container->GetPrefetchDocumentManager()
              ->GetNumberOfPrefetchRequestAttempted() >=
          PrefetchServiceMaximumNumberOfPrefetchesPerPage().value_or(
              std::numeric_limits<int>::max());
  if (is_above_limit) {
    prefetch_container->SetPrefetchStatus(
        PrefetchStatus::kPrefetchFailedPerPageLimitExceeded);
    ResetPrefetch(prefetch_container);
    return;
  }

  if (prefetch_to_evict) {
    EvictPrefetch(prefetch_to_evict->GetPrefetchContainerKey());
  }

  active_prefetches_.insert(prefetch_container->GetPrefetchContainerKey());

  prefetch_container->GetPrefetchDocumentManager()
      ->OnPrefetchRequestAttempted();

  if (!prefetch_container->IsDecoy()) {
    // The status is updated to be successful or failed when it finishes.
    prefetch_container->SetPrefetchStatus(
        PrefetchStatus::kPrefetchNotFinishedInTime);
  }

  net::HttpRequestHeaders additional_headers;
  additional_headers.SetHeader(
      net::HttpRequestHeaders::kAccept,
      FrameAcceptHeaderValue(/*allow_sxg_responses=*/true, browser_context_));
  prefetch_container->MakeResourceRequest(additional_headers);

  const auto& devtools_observer = prefetch_container->GetDevToolsObserver();
  if (devtools_observer && !prefetch_container->IsDecoy()) {
    devtools_observer->OnStartSinglePrefetch(
        prefetch_container->RequestId(),
        *prefetch_container->GetResourceRequest(), absl::nullopt);
  }

  SendPrefetchRequest(prefetch_container);

  PrefetchDocumentManager* prefetch_document_manager =
      prefetch_container->GetPrefetchDocumentManager();
  if (prefetch_container->GetPrefetchType().IsProxyRequiredWhenCrossOrigin() &&
      !prefetch_container->IsDecoy() &&
      (!prefetch_document_manager ||
       !prefetch_document_manager->HaveCanaryChecksStarted())) {
    // Make sure canary checks have run so we know the result by the time we
    // want to use the prefetch. Checking the canary cache can be a slow and
    // blocking operation (see crbug.com/1266018), so we only do this for the
    // first non-decoy prefetch we make on the page.
    // TODO(crbug.com/1266018): once this bug is fixed, fire off canary check
    // regardless of whether the request is a decoy or not.
    origin_prober_->RunCanaryChecksIfNeeded();

    if (prefetch_document_manager) {
      prefetch_document_manager->OnCanaryChecksStarted();
    }
  }

  // Start a spare renderer now so that it will be ready by the time it is
  // useful to have.
  if (ShouldStartSpareRenderer()) {
    RenderProcessHost::WarmupSpareRenderProcessHost(browser_context_);
  }
}

void PrefetchService::SendPrefetchRequest(
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("speculation_rules_prefetch",
                                          R"(
          semantics {
            sender: "Speculation Rules Prefetch Loader"
            description:
              "Prefetches the mainframe HTML of a page specified via "
              "speculation rules. This is done out-of-band of normal "
              "prefetches to allow total isolation of this request from the "
              "rest of browser traffic and user state like cookies and cache."
            trigger:
              "Used only when this feature and speculation rules feature are "
              "enabled."
            data: "None."
            destination: WEBSITE
          }
          policy {
            cookies_allowed: NO
            setting:
              "Users can control this via a setting specific to each content "
              "embedder."
            policy_exception_justification: "Not implemented."
        })");

  auto streaming_loader = PrefetchStreamingURLLoader::Create(
      GetURLLoaderFactoryForCurrentPrefetch(prefetch_container),
      *prefetch_container->GetResourceRequest(), traffic_annotation,
      PrefetchTimeoutDuration(),
      base::BindOnce(&PrefetchService::OnPrefetchResponseStarted,
                     base::Unretained(this), prefetch_container),
      base::BindOnce(&PrefetchService::OnPrefetchResponseCompleted,
                     base::Unretained(this), prefetch_container),
      base::BindRepeating(&PrefetchService::OnPrefetchRedirect,
                          base::Unretained(this), prefetch_container),
      base::BindOnce(&PrefetchContainer::OnReceivedHead, prefetch_container),
      prefetch_container->GetResponseReaderForCurrentPrefetch());
  prefetch_container->TakeStreamingURLLoader(std::move(streaming_loader));

  DVLOG(1) << *prefetch_container << ": PrefetchStreamingURLLoader is created.";
}

network::mojom::URLLoaderFactory*
PrefetchService::GetURLLoaderFactoryForCurrentPrefetch(
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  DCHECK(prefetch_container);
  if (g_url_loader_factory_for_testing) {
    return g_url_loader_factory_for_testing;
  }
  return prefetch_container->GetOrCreateNetworkContextForCurrentPrefetch(this)
      ->GetURLLoaderFactory();
}

void PrefetchService::OnPrefetchRedirect(
    base::WeakPtr<PrefetchContainer> prefetch_container,
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr redirect_head) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!prefetch_container) {
    RecordRedirectResult(PrefetchRedirectResult::kFailedNullPrefetch);
    return;
  }

  DCHECK(
      active_prefetches_.find(prefetch_container->GetPrefetchContainerKey()) !=
      active_prefetches_.end());

  prefetch_container->AddRedirectHop(redirect_info);

  // Update the prefetch's referrer in case a redirect requires a change in
  // network context and a new request needs to be started.
  prefetch_container->UpdateReferrer(
      GURL(redirect_info.new_referrer),
      blink::ReferrerUtils::NetToMojoReferrerPolicy(
          redirect_info.new_referrer_policy));

  // Check that the prefetch's referrer policy is sufficiently strict to allow
  // for the redirect to be followed.
  net::SchemefulSite previous_site =
      prefetch_container->GetSiteForPreviousRedirectHop(redirect_info.new_url);
  net::SchemefulSite redirect_site(redirect_info.new_url);
  bool is_referrer_policy_sufficiently_strict =
      IsReferrerPolicySufficientlyStrict(
          prefetch_container->GetReferrer().policy);

  absl::optional<PrefetchRedirectResult> failure;

  if (!base::FeatureList::IsEnabled(features::kPrefetchRedirects)) {
    failure = PrefetchRedirectResult::kFailedRedirectsDisabled;
  } else if (redirect_info.new_method != "GET") {
    failure = PrefetchRedirectResult::kFailedInvalidMethod;
  } else if (!redirect_head->headers ||
             redirect_head->headers->response_code() < 300 ||
             redirect_head->headers->response_code() >= 400) {
    failure = PrefetchRedirectResult::kFailedInvalidResponseCode;
  } else if (previous_site != redirect_site &&
             !is_referrer_policy_sufficiently_strict) {
    failure = PrefetchRedirectResult::kFailedInsufficientReferrerPolicy;
  }

  if (failure) {
    active_prefetches_.erase(prefetch_container->GetPrefetchContainerKey());
    prefetch_container->SetPrefetchStatus(
        PrefetchStatus::kPrefetchFailedInvalidRedirect);
    prefetch_container->GetLastStreamingURLLoader()->HandleRedirect(
        PrefetchRedirectStatus::kFail, redirect_info, std::move(redirect_head));
    prefetch_container->ResetAllStreamingURLLoaders();

    Prefetch();
    RecordRedirectResult(*failure);
    return;
  }

  RecordRedirectNetworkContextTransition(
      prefetch_container
          ->IsIsolatedNetworkContextRequiredForPreviousRedirectHop(),
      prefetch_container->IsIsolatedNetworkContextRequiredForCurrentPrefetch());

  CheckEligibilityOfPrefetch(
      redirect_info.new_url, prefetch_container,
      base::BindOnce(&PrefetchService::OnGotEligibilityResultForRedirect,
                     base::Unretained(this), redirect_info,
                     std::move(redirect_head)));
}

PrefetchStreamingURLLoaderStatus PrefetchService::OnPrefetchResponseStarted(
    base::WeakPtr<PrefetchContainer> prefetch_container,
    network::mojom::URLResponseHead* head) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!prefetch_container || prefetch_container->IsDecoy()) {
    return PrefetchStreamingURLLoaderStatus::kPrefetchWasDecoy;
  }

  if (!head) {
    return PrefetchStreamingURLLoaderStatus::kFailedInvalidHead;
  }

  const auto& devtools_observer = prefetch_container->GetDevToolsObserver();
  if (devtools_observer) {
    devtools_observer->OnPrefetchResponseReceived(
        prefetch_container->GetCurrentURL(), prefetch_container->RequestId(),
        *head);
  }

  if (!head->headers) {
    return PrefetchStreamingURLLoaderStatus::kFailedInvalidHeaders;
  }

  RecordPrefetchProxyPrefetchMainframeTotalTime(head);
  RecordPrefetchProxyPrefetchMainframeConnectTime(head);

  int response_code = head->headers->response_code();
  RecordPrefetchProxyPrefetchMainframeRespCode(response_code);
  if (response_code < 200 || response_code >= 300) {
    prefetch_container->SetPrefetchStatus(
        PrefetchStatus::kPrefetchFailedNon2XX);

    if (response_code == net::HTTP_SERVICE_UNAVAILABLE) {
      base::TimeDelta retry_after;
      std::string retry_after_string;
      if (head->headers->EnumerateHeader(nullptr, "Retry-After",
                                         &retry_after_string) &&
          net::HttpUtil::ParseRetryAfterHeader(
              retry_after_string, base::Time::Now(), &retry_after) &&
          delegate_) {
        // Cap the retry after value to a maximum.
        if (retry_after > PrefetchMaximumRetryAfterDelta()) {
          retry_after = PrefetchMaximumRetryAfterDelta();
        }

        delegate_->ReportOriginRetryAfter(prefetch_container->GetURL(),
                                          retry_after);
      }
    }
    return PrefetchStreamingURLLoaderStatus::kFailedNon2XX;
  }

  if (PrefetchServiceHTMLOnly() && head->mime_type != "text/html") {
    prefetch_container->SetPrefetchStatus(
        PrefetchStatus::kPrefetchFailedMIMENotSupported);
    return PrefetchStreamingURLLoaderStatus::kFailedMIMENotSupported;
  }

  return PrefetchStreamingURLLoaderStatus::kHeadReceivedWaitingOnBody;
}

void PrefetchService::OnPrefetchResponseCompleted(
    base::WeakPtr<PrefetchContainer> prefetch_container,
    const network::URLLoaderCompletionStatus& completion_status) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!prefetch_container) {
    return;
  }

  DCHECK(
      active_prefetches_.find(prefetch_container->GetPrefetchContainerKey()) !=
      active_prefetches_.end());
  active_prefetches_.erase(prefetch_container->GetPrefetchContainerKey());

  prefetch_container->OnPrefetchComplete();

  if (prefetch_container->IsDecoy()) {
    prefetch_container->SetPrefetchStatus(
        PrefetchStatus::kPrefetchIsPrivacyDecoy);
    prefetch_container->ResetAllStreamingURLLoaders();
    Prefetch();
    return;
  }

  // TODO(https://crbug.com/1399956): Call
  // SpeculationHostDevToolsObserver::OnPrefetchBodyDataReceived with body of
  // the response.
  const auto& devtools_observer = prefetch_container->GetDevToolsObserver();
  if (devtools_observer) {
    devtools_observer->OnPrefetchRequestComplete(
        prefetch_container->RequestId(), completion_status);
  }

  int net_error = completion_status.error_code;
  int64_t body_length = completion_status.decoded_body_length;

  RecordPrefetchProxyPrefetchMainframeNetError(net_error);

  // Updates the prefetch's status if it hasn't been updated since the request
  // first started. For the prefetch to reach the network stack, it must have
  // `PrefetchStatus::kPrefetchAllowed` or beyond.
  DCHECK(prefetch_container->HasPrefetchStatus());
  if (prefetch_container->GetPrefetchStatus() ==
      PrefetchStatus::kPrefetchNotFinishedInTime) {
    prefetch_container->SetPrefetchStatus(
        net_error == net::OK ? PrefetchStatus::kPrefetchSuccessful
                             : PrefetchStatus::kPrefetchFailedNetError);
    prefetch_container->UpdateServingPageMetrics();
  }

  if (net_error == net::OK) {
    RecordPrefetchProxyPrefetchMainframeBodyLength(body_length);
  }

  if (!prefetch_container->IsPrefetchServable(PrefetchCacheableDuration())) {
    // If the prefetch from the streaming URL loader cannot be served at this
    // point, then it can be discarded.
    prefetch_container->ResetAllStreamingURLLoaders();
  } else {
    PrefetchDocumentManager* prefetch_document_manager =
        prefetch_container->GetPrefetchDocumentManager();
    if (prefetch_document_manager) {
      prefetch_document_manager->OnPrefetchSuccessful(prefetch_container.get());
    }
  }

  Prefetch();
}

void PrefetchService::PrepareToServe(
    const GURL& url,
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  // Ensure |this| has this prefetch.
  if (all_prefetches_.find(prefetch_container->GetPrefetchContainerKey()) ==
      all_prefetches_.end()) {
    DVLOG(1) << *prefetch_container
             << ": didn't promote to ready (not in all_prefetches_)";
    return;
  }

  bool is_servable =
      prefetch_container->IsPrefetchServable(PrefetchCacheableDuration());

  // `url` might be different from
  // `prefetch_container->GetPrefetchContainerKey().second` due to
  // No-Vary-Search.
  PrefetchContainer::Key ready_key(
      prefetch_container->GetPrefetchContainerKey().first, url);

  // If there is already a prefetch with the same URL as |prefetch_container| in
  // |prefetches_ready_to_serve_|, then don't do anything.
  if (prefetches_ready_to_serve_.find(ready_key) !=
      prefetches_ready_to_serve_.end()) {
    DVLOG(1) << *prefetch_container
             << ": didn't promote to ready (another ready prefetch)";
    return;
  }

  // Move prefetch into |prefetches_ready_to_serve_|.
  DVLOG(1) << *prefetch_container << ": promoted to ready";
  prefetches_ready_to_serve_[ready_key] = prefetch_container;

  if (is_servable) {
    // For prefetches that are already servable, start the process of copying
    // cookies from the isolated network context used to make the prefetch to
    // the default network context.
    CopyIsolatedCookies(prefetch_container->CreateReader());
  }
}

void PrefetchService::CopyIsolatedCookies(
    const PrefetchContainer::Reader& reader) {
  DCHECK(reader);

  if (!reader.GetCurrentNetworkContextToServe()) {
    // Not set in unit tests.
    return;
  }

  // We only need to copy cookies if the prefetch used an isolated network
  // context.
  if (!reader.IsIsolatedNetworkContextRequiredToServe()) {
    return;
  }

  reader.OnIsolatedCookieCopyStart();
  net::CookieOptions options = net::CookieOptions::MakeAllInclusive();
  reader.GetCurrentNetworkContextToServe()->GetCookieManager()->GetCookieList(
      reader.GetCurrentURLToServe(), options,
      net::CookiePartitionKeyCollection::Todo(),
      base::BindOnce(&PrefetchService::OnGotIsolatedCookiesForCopy,
                     weak_method_factory_.GetWeakPtr(), reader.Clone()));
}

void PrefetchService::OnGotIsolatedCookiesForCopy(
    PrefetchContainer::Reader reader,
    const net::CookieAccessResultList& cookie_list,
    const net::CookieAccessResultList& excluded_cookies) {
  reader.OnIsolatedCookiesReadCompleteAndWriteStart();
  RecordPrefetchProxyPrefetchMainframeCookiesToCopy(cookie_list.size());

  if (cookie_list.empty()) {
    reader.OnIsolatedCookieCopyComplete();
    return;
  }

  const auto current_url = reader.GetCurrentURLToServe();

  base::RepeatingClosure barrier = base::BarrierClosure(
      cookie_list.size(),
      base::BindOnce(&OnIsolatedCookieCopyComplete, std::move(reader)));

  net::CookieOptions options = net::CookieOptions::MakeAllInclusive();
  for (const net::CookieWithAccessResult& cookie : cookie_list) {
    browser_context_->GetDefaultStoragePartition()
        ->GetCookieManagerForBrowserProcess()
        ->SetCanonicalCookie(cookie.cookie, current_url, options,
                             base::BindOnce(&CookieSetHelper, barrier));
  }
}

void PrefetchService::DumpPrefetchesForDebug() const {
#if DCHECK_IS_ON()
  std::ostringstream ss;
  ss << "PrefetchService[" << this << "]:" << std::endl;

  ss << "Owned:" << std::endl;
  for (const auto& entry : owned_prefetches_) {
    ss << *entry.second.first << std::endl;
  }

  ss << "Ready to serve:" << std::endl;
  for (const auto& entry : prefetches_ready_to_serve_) {
    if (PrefetchContainer* prefetch_container = entry.second.get()) {
      ss << *prefetch_container << std::endl;
    }
  }
  DVLOG(1) << ss.str();
#endif  // DCHECK_IS_ON()
}

void PrefetchService::FindPrefetchContainerToServe(
    const PrefetchContainer::Key& key,
    PrefetchMatchResolver& prefetch_match_resolver) {
  // Search for an exact match first. If one is found and not deleted, produce
  // it.
  auto it = prefetches_ready_to_serve_.find(key);
  if (it != prefetches_ready_to_serve_.end()) {
    PrefetchContainer* prefetch = it->second.get();
    prefetches_ready_to_serve_.erase(it);
    if (prefetch && !prefetch->HasPrefetchBeenConsideredToServe()) {
      prefetch_match_resolver.SetExactPrefetchMatch(*prefetch);
      return;
    }
  }

  // Search for an inexact match using the No-Vary-Search hint.
  // It must either be servable now or potentially servable soon.
  const auto frame_host_id = key.first;
  const GURL& nav_url = key.second;
  for (const auto& active_prefetch : active_prefetches_) {
    if (active_prefetch.first != frame_host_id) {
      continue;
    }
    PrefetchContainer* prefetch = all_prefetches_[active_prefetch].get();
    if (!prefetch || prefetch->HasPrefetchBeenConsideredToServe()) {
      continue;
    }
    const auto& nvs_expected = prefetch->GetNoVarySearchHint();
    if (!nvs_expected ||
        !nvs_expected->AreEquivalent(nav_url, prefetch->GetURL())) {
      continue;
    }
    if (prefetch->IsPrefetchServable(PrefetchCacheableDuration()) ||
        prefetch->ShouldBlockUntilHeadReceived()) {
      prefetch_match_resolver.AddInexactPrefetchMatch(*prefetch);
      // TODO(crbug.com/1462206): We'd like to continue matching here in a
      // follow-up CL. At this time we only wait for one prefetch.
      return;
    }
  }
  return;
}

void PrefetchService::HandlePrefetchContainerToServe(
    const PrefetchContainer::Key& key,
    PrefetchContainer* prefetch_container,
    PrefetchMatchResolver& prefetch_match_resolver) {
  const GURL& url = key.second;
  if (!prefetch_container) {
    DVLOG(1)
        << "PrefetchService::HandlePrefetchContainerToServe(" << url
        << "): PrefetchContainer is null or no matching prefetch was found";
    prefetch_match_resolver.ReleaseOnPrefetchToServeReadyCallback().Run({});
    return;
  }

  // TODO(crbug.com/1462206): Identify if any of the PrefetchContainers can be
  // used immediatedly to serve navigation.
  // If no PrefetchContainers can be used immediately, then make a list of
  // in progress PrefetchContainers that could serve the navigation.
  // If the list is empty then call
  // `TakeOnPrefetchToServeReadyCallback().Run({})`.
  // If the list is not empty then PrefetchService should keep track of
  // `potential_prefetch_matches_container` navigation user data and inform it
  // if there are any more prefetches starting for this navigation. If the
  // navigation matches by No-Vary-Search then it should try to wait for that
  // prefetch as well. The navigation user data needs to keep track of all of
  // the prefetches in progress. The navigation user data will keep track of
  // on_prefetch_to_serve_ready and run the callback when appropriate.
  // GlobalRenderFrameHostId can be used to match the navigation with the
  // new starting prefetches.

  if (prefetch_container->GetRedirectChainSize() > 1 &&
      !base::FeatureList::IsEnabled(features::kPrefetchRedirects)) {
    prefetch_match_resolver.ReleaseOnPrefetchToServeReadyCallback().Run({});
    return;
  }

  if (prefetch_container->IsPrefetchServable(PrefetchCacheableDuration())) {
    DVLOG(1) << "PrefetchService::HandlePrefetchContainerToServe(" << url
             << "): PrefetchContainer is servable";
    prefetch_container->OnGetPrefetchToServe(/*blocked_until_head=*/false);
    ReturnPrefetchToServe(
        prefetch_container->CreateReader(),
        prefetch_match_resolver.ReleaseOnPrefetchToServeReadyCallback());
    return;
  }

  if (prefetch_container->ShouldBlockUntilHeadReceived()) {
    DVLOG(1) << "PrefetchService::HandlePrefetchContainerToServe(" << url
             << "): PrefetchContainer is blocked until head";
    prefetch_container->OnGetPrefetchToServe(/*blocked_until_head=*/true);
    prefetch_container->SetOnReceivedHeadCallback(
        base::BindOnce(&PrefetchService::WaitOnPrefetchToServeHead,
                       weak_method_factory_.GetWeakPtr(), key,
                       prefetch_match_resolver.GetWeakPtr(),
                       prefetch_container->GetWeakPtr()));

    base::TimeDelta block_until_head_timeout = PrefetchBlockUntilHeadTimeout(
        prefetch_container->GetPrefetchType().GetEagerness());
    if (block_until_head_timeout.is_positive()) {
      std::unique_ptr<base::OneShotTimer> block_until_head_timer =
          std::make_unique<base::OneShotTimer>();
      block_until_head_timer->Start(
          FROM_HERE, block_until_head_timeout,
          base::BindOnce(&BlockUntilHeadTimeoutHelper,
                         prefetch_container->GetWeakPtr()));
      prefetch_container->TakeBlockUntilHeadTimer(
          std::move(block_until_head_timer));
    }
    return;
  }

  DVLOG(1) << "PrefetchService::HandlePrefetchContainerToServe(" << key.second
           << "): PrefetchContainer is not servable";
  prefetch_container->OnReturnPrefetchToServe(/*served=*/false);
  prefetch_match_resolver.ReleaseOnPrefetchToServeReadyCallback().Run({});
}

void PrefetchService::GetPrefetchToServe(
    const PrefetchContainer::Key& key,
    PrefetchMatchResolver& prefetch_match_resolver) {
  DumpPrefetchesForDebug();
  FindPrefetchContainerToServe(key, prefetch_match_resolver);
  if (prefetch_match_resolver.HasExactPrefetchMatch()) {
    PrefetchContainer* prefetch_container =
        prefetch_match_resolver.GetExactPrefetchMatch();
    HandlePrefetchContainerToServe(key, prefetch_container,
                                   prefetch_match_resolver);
    return;
  }
  if (prefetch_match_resolver.HasInexactPrefetchMatch()) {
    // TODO(crbug.com/1462206): We'd like to continue matching here in a
    // follow-up CL. At this time we only wait for one prefetch.
    HandlePrefetchContainerToServe(
        key, prefetch_match_resolver.GetInexactPrefetchMatches()[0],
        prefetch_match_resolver);
    return;
  }
  DVLOG(1) << "PrefetchService::GetPrefetchToServe(" << key.second
           << "): No PrefetchContainer is servable";
  prefetch_match_resolver.ReleaseOnPrefetchToServeReadyCallback().Run({});
}

void PrefetchService::WaitOnPrefetchToServeHead(
    const PrefetchContainer::Key& key,
    base::WeakPtr<PrefetchMatchResolver> prefetch_match_resolver,
    base::WeakPtr<PrefetchContainer> prefetch_container) {
  if (!prefetch_match_resolver) {
    // Since prefetch_match_resolver is a NavigationHandleUserData,
    // if it is null it means the navigation has finished so there is nothing to
    // do here.
    return;
  }
  const GURL& nav_url = key.second;
  if (!prefetch_container) {
    ReturnPrefetchToServe(
        {}, prefetch_match_resolver->ReleaseOnPrefetchToServeReadyCallback());
    return;
  }

  prefetch_container->ResetBlockUntilHeadTimer();

  if (!prefetch_container->IsPrefetchServable(PrefetchCacheableDuration())) {
    prefetch_container->OnReturnPrefetchToServe(/*served=*/false);
    ReturnPrefetchToServe(
        {}, prefetch_match_resolver->ReleaseOnPrefetchToServeReadyCallback());
    return;
  }

  if (nav_url == prefetch_container->GetURL()) {
    PrepareToServe(nav_url, prefetch_container);
    GetPrefetchToServe(key, *prefetch_match_resolver);
    return;
  }

  if (const auto* head = prefetch_container->GetHead()) {
    if (!head->parsed_headers ||
        !head->parsed_headers->no_vary_search_with_parse_error ||
        head->parsed_headers->no_vary_search_with_parse_error
            ->is_parse_error()) {
      // is_parse_error() == true includes the case where the header is
      // not there (kOk) and the case where the header is equivalent
      // to default behavior (exactly match URL - kDefaultValue)
      prefetch_container->OnReturnPrefetchToServe(/*served=*/false);
      prefetch_container->UpdateServingPageMetrics();
      ReturnPrefetchToServe(
          {}, prefetch_match_resolver->ReleaseOnPrefetchToServeReadyCallback());
      return;
    }
    auto no_vary_search_data =
        no_vary_search::ParseHttpNoVarySearchDataFromMojom(
            head->parsed_headers->no_vary_search_with_parse_error
                ->get_no_vary_search());
    if (!no_vary_search_data.AreEquivalent(nav_url,
                                           prefetch_container->GetURL())) {
      prefetch_container->OnReturnPrefetchToServe(/*served=*/false);
      prefetch_container->UpdateServingPageMetrics();
      ReturnPrefetchToServe(
          {}, prefetch_match_resolver->ReleaseOnPrefetchToServeReadyCallback());
      return;
    }
    DVLOG(1) << "PrefetchService::WaitOnPrefetchToServeHead::"
             << "url = " << nav_url << "::"
             << "matches by NVS header the prefetch "
             << prefetch_container->GetURL();
    if (auto attempt = prefetch_container->preloading_attempt()) {
      // Before No-Vary-Search hint, the decision to use a prefetched response
      // was made in `DidStartNavigation`. `SetIsAccurateTriggering` is called
      // by `PreloadingDataImpl::DidStartNavigation`. With No-Vary-Search
      // hint the decision to use an in-flight prefetched response is
      // delayed until the headers are received from the server. This
      // happens after `DidStartNavigation`. At this point in the code we
      // have already decided we are going to use the prefetch, so we can
      // safely call `SetIsAccurateTriggering`.
      static_cast<PreloadingAttemptImpl*>(attempt.get())
          ->SetIsAccurateTriggering(nav_url);
    }
    PrepareToServe(nav_url, prefetch_container);
    GetPrefetchToServe(key, *prefetch_match_resolver);
  }
}

void PrefetchService::ReturnPrefetchToServe(
    PrefetchContainer::Reader reader,
    OnPrefetchToServeReady on_prefetch_to_serve_ready) {
  PrefetchContainer* prefetch_container = reader.GetPrefetchContainer();
  if (prefetch_container) {
    prefetch_container->UpdateServingPageMetrics();
  }

  if (!prefetch_container ||
      !prefetch_container->IsPrefetchServable(PrefetchCacheableDuration())) {
    if (prefetch_container) {
      prefetch_container->OnReturnPrefetchToServe(/*served=*/false);
    }
    std::move(on_prefetch_to_serve_ready).Run({});
    return;
  }

  if (reader.HaveDefaultContextCookiesChanged()) {
    prefetch_container->SetPrefetchStatus(
        PrefetchStatus::kPrefetchNotUsedCookiesChanged);
    prefetch_container->UpdateServingPageMetrics();
    prefetch_container->OnReturnPrefetchToServe(/*served=*/false);
    prefetch_container->ResetAllStreamingURLLoaders();
    std::move(on_prefetch_to_serve_ready).Run({});
    return;
  }

  if (!reader.HasIsolatedCookieCopyStarted()) {
    CopyIsolatedCookies(reader);
  }

  prefetch_container->OnReturnPrefetchToServe(/*served=*/true);
  std::move(on_prefetch_to_serve_ready).Run(std::move(reader));
  return;
}

// static
void PrefetchService::SetServiceWorkerContextForTesting(
    ServiceWorkerContext* context) {
  g_service_worker_context_for_testing = context;
}

// static
void PrefetchService::SetHostNonUniqueFilterForTesting(
    bool (*filter)(base::StringPiece)) {
  g_host_non_unique_filter = filter;
}

// static
void PrefetchService::SetURLLoaderFactoryForTesting(
    network::mojom::URLLoaderFactory* url_loader_factory) {
  g_url_loader_factory_for_testing = url_loader_factory;
}

// static
void PrefetchService::SetNetworkContextForProxyLookupForTesting(
    network::mojom::NetworkContext* network_context) {
  g_network_context_for_proxy_lookup_for_testing = network_context;
}

void PrefetchService::RecordExistingPrefetchWithMatchingURL(
    base::WeakPtr<PrefetchContainer> prefetch_container) const {
  bool matching_prefetch = false;
  int num_matching_prefetches = 0;

  int num_matching_eligible_prefetch = 0;
  int num_matching_servable_prefetch = 0;
  int num_matching_prefetch_same_referrer = 0;
  int num_matching_prefetch_same_rfh = 0;

  for (const auto& prefetch_iter : all_prefetches_) {
    if (prefetch_iter.second &&
        prefetch_iter.second->GetURL() == prefetch_container->GetURL()) {
      matching_prefetch = true;
      num_matching_prefetches++;

      if (prefetch_iter.second->IsInitialPrefetchEligible()) {
        num_matching_eligible_prefetch++;
      }

      if (prefetch_iter.second->IsPrefetchServable(
              PrefetchCacheableDuration()) &&
          !prefetch_iter.second->HasPrefetchBeenConsideredToServe()) {
        num_matching_servable_prefetch++;
      }

      if (prefetch_iter.second->GetReferrer().url ==
          prefetch_container->GetReferrer().url) {
        num_matching_prefetch_same_referrer++;
      }

      if (prefetch_iter.second->GetReferringRenderFrameHostId() ==
          prefetch_container->GetReferringRenderFrameHostId()) {
        num_matching_prefetch_same_rfh++;
      }
    }
  }

  base::UmaHistogramBoolean(
      "PrefetchProxy.Prefetch.ExistingPrefetchWithMatchingURL",
      matching_prefetch);
  base::UmaHistogramCounts100(
      "PrefetchProxy.Prefetch.NumExistingPrefetchWithMatchingURL",
      num_matching_prefetches);

  if (matching_prefetch) {
    base::UmaHistogramCounts100(
        "PrefetchProxy.Prefetch.NumExistingEligiblePrefetchWithMatchingURL",
        num_matching_eligible_prefetch);
    base::UmaHistogramCounts100(
        "PrefetchProxy.Prefetch.NumExistingServablePrefetchWithMatchingURL",
        num_matching_servable_prefetch);
    base::UmaHistogramCounts100(
        "PrefetchProxy.Prefetch.NumExistingPrefetchWithMatchingURLAndReferrer",
        num_matching_prefetch_same_referrer);
    base::UmaHistogramCounts100(
        "PrefetchProxy.Prefetch."
        "NumExistingPrefetchWithMatchingURLAndRenderFrameHost",
        num_matching_prefetch_same_rfh);
  }
}

}  // namespace content
