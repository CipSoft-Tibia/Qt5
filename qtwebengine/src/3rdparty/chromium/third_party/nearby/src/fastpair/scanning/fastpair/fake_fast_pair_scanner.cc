// Copyright 2022 Google LLC
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

#include "fastpair/scanning/fastpair/fake_fast_pair_scanner.h"

namespace nearby {
namespace fastpair {

void FakeFastPairScanner::AddObserver(Observer* observer) {
  observer_.AddObserver(observer);
}

void FakeFastPairScanner::RemoveObserver(Observer* observer) {
  observer_.RemoveObserver(observer);
}

void FakeFastPairScanner::NotifyDeviceFound(const BlePeripheral& peripheral) {
  for (auto& obs : observer_) obs->OnDeviceFound(peripheral);
}

void FakeFastPairScanner::NotifyDeviceLost(const BlePeripheral& peripheral) {
  for (auto& obs : observer_) obs->OnDeviceLost(peripheral);
}

}  // namespace fastpair
}  // namespace nearby
