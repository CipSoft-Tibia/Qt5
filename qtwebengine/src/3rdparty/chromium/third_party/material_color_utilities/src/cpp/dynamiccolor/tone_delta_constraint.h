/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CPP_DYNAMICCOLOR_TONE_DELTA_CONSTRAINT_H_
#define CPP_DYNAMICCOLOR_TONE_DELTA_CONSTRAINT_H_

#include "cpp/dynamiccolor/dynamic_color.h"

namespace material_color_utilities {

enum class TonePolarity { kNoPreference, kDarker, kLighter };

struct ToneDeltaConstraint {
  double delta;
  DynamicColor keep_away;
  TonePolarity keep_away_polarity;

  ToneDeltaConstraint(double delta, DynamicColor keep_away,
                      TonePolarity keep_away_polarity)
      : delta(delta),
        keep_away(keep_away),
        keep_away_polarity(keep_away_polarity) {}
};

}  // namespace material_color_utilities

#endif  // CPP_DYNAMICCOLOR_TONE_DELTA_CONSTRAINT_H_
