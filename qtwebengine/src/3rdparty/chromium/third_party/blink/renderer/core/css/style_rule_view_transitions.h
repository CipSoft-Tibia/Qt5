// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_RULE_VIEW_TRANSITIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_RULE_VIEW_TRANSITIONS_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/parser/at_rule_descriptors.h"
#include "third_party/blink/renderer/core/css/style_rule.h"

namespace blink {

class CORE_EXPORT StyleRuleViewTransitions : public StyleRuleBase {
 public:
  explicit StyleRuleViewTransitions(CSSPropertyValueSet&);
  StyleRuleViewTransitions(const StyleRuleViewTransitions&);
  ~StyleRuleViewTransitions();

  const CSSValue* GetNavigationTrigger() const;

  StyleRuleViewTransitions* Copy() const {
    return MakeGarbageCollected<StyleRuleViewTransitions>(*this);
  }

  void TraceAfterDispatch(blink::Visitor*) const;

 private:
  Member<const CSSValue> navigation_trigger_;
};

template <>
struct DowncastTraits<StyleRuleViewTransitions> {
  static bool AllowFrom(const StyleRuleBase& rule) {
    return rule.IsViewTransitionsRule();
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_RULE_VIEW_TRANSITIONS_H_
