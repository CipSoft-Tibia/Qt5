// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEGMENTATION_PLATFORM_INTERNAL_DATABASE_CACHED_RESULT_PROVIDER_H_
#define COMPONENTS_SEGMENTATION_PLATFORM_INTERNAL_DATABASE_CACHED_RESULT_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "components/segmentation_platform/internal/database/client_result_prefs.h"
#include "components/segmentation_platform/public/proto/prediction_result.pb.h"
#include "components/segmentation_platform/public/result.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace segmentation_platform {
struct Config;
class ClientResultPrefs;

// CachedResultProvider layer is to read results for clients from prefs at
// startup and cache it for the session. It does the following:
// 1. Reading the client result from the prefs on startup.
// 2. PostProcessing the client result.
// 3. Caching the post processed results and serving client requests from this
// cache. The cache is only read once on startup and never updated thereafter.
class CachedResultProvider {
 public:
  CachedResultProvider(PrefService* pref_service,
                       const std::vector<std::unique_ptr<Config>>& configs);

  CachedResultProvider(std::unique_ptr<ClientResultPrefs> prefs,
                       const std::vector<std::unique_ptr<Config>>& configs);
  ~CachedResultProvider();

  // Disallow copy/assign.
  CachedResultProvider(CachedResultProvider&) = delete;
  CachedResultProvider& operator=(CachedResultProvider&) = delete;

  // Returns cached post-processed result from last session for the client.
  // TODO(salg): Remove this and replace with GetPredictionResultForClient.
  ClassificationResult GetCachedResultForClient(
      const std::string& segmentation_key);

  // Returns cached un-processed result from last session for the client.
  absl::optional<proto::PredictionResult> GetPredictionResultForClient(
      const std::string& segmentation_key);

 private:
  // Configs for all registered clients.
  const raw_ref<const std::vector<std::unique_ptr<Config>>> configs_;

  // The underlying pref backed store to read the pref values from.
  std::unique_ptr<ClientResultPrefs> result_prefs_;

  // Map to store unprocessed result from last session for all clients.
  std::map<std::string, proto::PredictionResult>
      client_result_from_last_session_map_;

  base::WeakPtrFactory<CachedResultProvider> weak_ptr_factory_{this};
};

}  // namespace segmentation_platform

#endif  // COMPONENTS_SEGMENTATION_PLATFORM_INTERNAL_DATABASE_CACHED_RESULT_PROVIDER_H_
