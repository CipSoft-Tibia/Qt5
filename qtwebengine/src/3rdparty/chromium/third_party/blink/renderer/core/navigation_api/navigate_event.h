// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_NAVIGATION_API_NAVIGATE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_NAVIGATION_API_NAVIGATE_EVENT_H_

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/web/web_frame_load_type.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_navigation_focus_reset.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_navigation_scroll_behavior.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/focused_element_change_observer.h"
#include "third_party/blink/renderer/core/execution_context/execution_context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/navigation_api/navigation_api.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

class AbortSignal;
class NavigationDestination;
class NavigateEventInit;
class NavigationInterceptOptions;
class ExceptionState;
class FormData;
class ScriptPromise;
class V8NavigationInterceptHandler;

class NavigateEvent final : public Event,
                            public ExecutionContextClient,
                            public FocusedElementChangeObserver {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static NavigateEvent* Create(ExecutionContext* context,
                               const AtomicString& type,
                               NavigateEventInit* init) {
    return MakeGarbageCollected<NavigateEvent>(context, type, init);
  }

  NavigateEvent(ExecutionContext* context,
                const AtomicString& type,
                NavigateEventInit* init);

  void SetUrl(const KURL& url) { url_ = url; }

  String navigationType() { return navigation_type_; }
  NavigationDestination* destination() { return destination_; }
  bool canIntercept() const { return can_intercept_; }
  bool userInitiated() const { return user_initiated_; }
  bool hashChange() const { return hash_change_; }
  AbortSignal* signal() { return signal_; }
  FormData* formData() const { return form_data_; }
  String downloadRequest() const { return download_request_; }
  ScriptValue info() const { return info_; }

  void intercept(NavigationInterceptOptions*, ExceptionState&);

  void scroll(ExceptionState&);
  void PotentiallyProcessScrollBehavior();

  const HeapVector<ScriptPromise>& GetNavigationActionPromisesList() {
    return navigation_action_promises_list_;
  }
  bool HasNavigationActions() const { return has_navigation_actions_; }
  void FinalizeNavigationActionPromisesList();

  void ResetFocusIfNeeded();
  bool ShouldSendAxEvents() const;

  void SaveStateFromDestinationItem(HistoryItem*);

  // FocusedElementChangeObserver implementation:
  void DidChangeFocus() final;

  const AtomicString& InterfaceName() const final;
  void Trace(Visitor*) const final;

 private:
  void DefinitelyProcessScrollBehavior();

  String navigation_type_;
  Member<NavigationDestination> destination_;
  bool can_intercept_;
  bool user_initiated_;
  bool hash_change_;
  Member<AbortSignal> signal_;
  Member<FormData> form_data_;
  String download_request_;
  ScriptValue info_;
  absl::optional<V8NavigationFocusReset> focus_reset_behavior_ = absl::nullopt;
  absl::optional<V8NavigationScrollBehavior> scroll_behavior_ = absl::nullopt;
  absl::optional<HistoryItem::ViewState> history_item_view_state_;

  KURL url_;
  bool has_navigation_actions_ = false;
  HeapVector<ScriptPromise> navigation_action_promises_list_;
  HeapVector<Member<V8NavigationInterceptHandler>>
      navigation_action_handlers_list_;

  bool did_process_scroll_behavior_ = false;
  bool did_finish_ = false;
  bool did_change_focus_during_intercept_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_NAVIGATION_API_NAVIGATE_EVENT_H_
