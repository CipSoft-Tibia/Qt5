// Copyright 2023 The Centipede Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_CENTIPEDE_FEATURE_SET_H_
#define THIRD_PARTY_CENTIPEDE_FEATURE_SET_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "./centipede/coverage.h"
#include "./centipede/defs.h"
#include "./centipede/feature.h"
#include "./centipede/util.h"

namespace centipede {

// Set of features with their frequencies.
// Features that have a frequency >= frequency_threshold
// are considered too frequent and thus less interesting for further fuzzing.
// All features must be in [0, feature_domains::kLastDomain.begin()).
class FeatureSet {
 public:
  explicit FeatureSet(uint8_t frequency_threshold)
      : frequency_threshold_(frequency_threshold) {}

  // Returns the number of features in `features` not present in `this`.
  // Removes all features from `features` that are too frequent.
  size_t CountUnseenAndPruneFrequentFeatures(FeatureVec &features) const;

  // For every feature in `features` increment its frequency.
  // If a feature wasn't seen before, it is added to `this`.
  void IncrementFrequencies(const FeatureVec &features);

  // How many different features are in the set.
  size_t size() const { return num_features_; }

  // Returns features that originate from CFG counters, converted to PCIndexVec.
  PCIndexVec ToCoveragePCs() const;

  // Returns the number of features in `this` from the given feature domain.
  size_t CountFeatures(feature_domains::Domain domain);

  // Returns the frequency associated with `feature`.
  size_t Frequency(feature_t feature) const { return frequencies_[feature]; }

  // Computes combined weight of `features`.
  // The less frequent the feature is, the bigger its weight.
  // The weight of a FeatureVec is a sum of individual feature weights.
  uint64_t ComputeWeight(const FeatureVec &features) const;

  // Returns a debug string representing the state of *this.
  std::string DebugString() const;

 private:
  // Computes the frequency threshold based on the domain of `feature`.
  // For now, just uses 1 for kPCPair and frequency_threshold_ for all others.
  // Rationale: the kPCPair features might be too numerous, we don't want to
  // store more than one of each such feature in the corpus.
  uint8_t FrequencyThreshold(feature_t feature) const {
    if (feature_domains::kPCPair.Contains(feature)) return 1;
    return frequency_threshold_;
  }

  const uint8_t frequency_threshold_;

  static constexpr size_t kSize = feature_domains::kLastDomain.begin();

  // Maps features to their frequencies.
  // This array is huge but sparse, and depending on the enabled features
  // some parts of it will never be written to or read from.
  // Unused parts of MmapNoReserveArray don't actually reserve memory.
  MmapNoReserveArray<kSize> frequencies_;

  // Counts all unique features added to this.
  size_t num_features_ = 0;

  // Counts features in each domain.
  size_t features_per_domain_[feature_domains::kLastDomain.domain_id()] = {};

  // Maintains the set of PC indices that correspond to added features.
  absl::flat_hash_set<PCIndex> pc_index_set_;
};

}  // namespace centipede

#endif  // THIRD_PARTY_CENTIPEDE_FEATURE_SET_H_
