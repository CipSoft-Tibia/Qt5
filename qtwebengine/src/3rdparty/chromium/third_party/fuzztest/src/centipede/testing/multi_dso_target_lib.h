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

#ifndef THIRD_PARTY_CENTIPEDE_TESTING_MULTI_DSO_TARGET_LIB_H_
#define THIRD_PARTY_CENTIPEDE_TESTING_MULTI_DSO_TARGET_LIB_H_

#include <cstddef>
#include <cstdint>

void DSO(const uint8_t *data, size_t size);

#endif  // THIRD_PARTY_CENTIPEDE_TESTING_MULTI_DSO_TARGET_LIB_H_
