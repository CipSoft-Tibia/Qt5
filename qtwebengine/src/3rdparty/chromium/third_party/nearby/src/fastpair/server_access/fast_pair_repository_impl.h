// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_NEARBY_FASTPAIR_SERVER_ACCESS_FAST_PAIR_REPOSITORY_IMPL_H_
#define THIRD_PARTY_NEARBY_FASTPAIR_SERVER_ACCESS_FAST_PAIR_REPOSITORY_IMPL_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "fastpair/repository/fast_pair_metadata_repository.h"
#include "fastpair/server_access/fast_pair_metadata_downloader.h"
#include "fastpair/server_access/fast_pair_repository.h"
#include "internal/network/http_client_factory.h"
#include "absl/strings/string_view.h"

namespace nearby {
namespace fastpair {
class FastPairRepositoryImpl : public FastPairRepository {
 public:
  FastPairRepositoryImpl();
  explicit FastPairRepositoryImpl(
      std::unique_ptr<FastPairMetadataRepositoryFactory> repository);
  FastPairRepositoryImpl(const FastPairRepositoryImpl&) = delete;
  FastPairRepositoryImpl& operator=(const FastPairRepositoryImpl&) = delete;
  ~FastPairRepositoryImpl() override = default;

  void GetDeviceMetadata(absl::string_view hex_model_id,
                         DeviceMetadataCallback callback) override;

 private:
  std::unique_ptr<FastPairMetadataDownloader> downloader_;
  std::unique_ptr<network::HttpClientFactory> http_factory_;
  std::unique_ptr<FastPairMetadataRepositoryFactory> repository_factory_;
};
}  // namespace fastpair
}  // namespace nearby

#endif  // THIRD_PARTY_NEARBY_FASTPAIR_SERVER_ACCESS_FAST_PAIR_REPOSITORY_IMPL_H_
