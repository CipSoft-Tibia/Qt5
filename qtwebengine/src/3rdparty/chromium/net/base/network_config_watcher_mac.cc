// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/network_config_watcher_mac.h"

#include <algorithm>

#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_pump_type.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"

namespace net {

namespace {

// SCDynamicStore API does not exist on iOS.
#if !BUILDFLAG(IS_IOS)
const base::TimeDelta kRetryInterval = base::Seconds(1);
const int kMaxRetry = 5;

// Called back by OS.  Calls OnNetworkConfigChange().
void DynamicStoreCallback(SCDynamicStoreRef /* store */,
                          CFArrayRef changed_keys,
                          void* config_delegate) {
  NetworkConfigWatcherMac::Delegate* net_config_delegate =
      static_cast<NetworkConfigWatcherMac::Delegate*>(config_delegate);
  net_config_delegate->OnNetworkConfigChange(changed_keys);
}
#endif  // !BUILDFLAG(IS_IOS)

}  // namespace

class NetworkConfigWatcherMacThread : public base::Thread {
 public:
  explicit NetworkConfigWatcherMacThread(
      NetworkConfigWatcherMac::Delegate* delegate);
  NetworkConfigWatcherMacThread(const NetworkConfigWatcherMacThread&) = delete;
  NetworkConfigWatcherMacThread& operator=(
      const NetworkConfigWatcherMacThread&) = delete;
  ~NetworkConfigWatcherMacThread() override;

 protected:
  // base::Thread
  void Init() override;
  void CleanUp() override;

 private:
  // The SystemConfiguration calls in this function can lead to contention early
  // on, so we invoke this function later on in startup to keep it fast.
  void InitNotifications();

  // Returns whether initializing notifications has succeeded.
  bool InitNotificationsHelper();

  base::apple::ScopedCFTypeRef<CFRunLoopSourceRef> run_loop_source_;
  const raw_ptr<NetworkConfigWatcherMac::Delegate> delegate_;
#if !BUILDFLAG(IS_IOS)
  int num_retry_ = 0;
#endif  // !BUILDFLAG(IS_IOS)
  base::WeakPtrFactory<NetworkConfigWatcherMacThread> weak_factory_;
};

NetworkConfigWatcherMacThread::NetworkConfigWatcherMacThread(
    NetworkConfigWatcherMac::Delegate* delegate)
    : base::Thread("NetworkConfigWatcher"),
      delegate_(delegate),
      weak_factory_(this) {}

NetworkConfigWatcherMacThread::~NetworkConfigWatcherMacThread() {
  // This is expected to be invoked during shutdown.
  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_thread_join;
  Stop();
}

void NetworkConfigWatcherMacThread::Init() {
  delegate_->Init();

  // TODO(willchan): Look to see if there's a better signal for when it's ok to
  // initialize this, rather than just delaying it by a fixed time.
  const base::TimeDelta kInitializationDelay = base::Seconds(1);
  task_runner()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&NetworkConfigWatcherMacThread::InitNotifications,
                     weak_factory_.GetWeakPtr()),
      kInitializationDelay);
}

void NetworkConfigWatcherMacThread::CleanUp() {
  if (!run_loop_source_.get())
    return;

  CFRunLoopRemoveSource(CFRunLoopGetCurrent(), run_loop_source_.get(),
                        kCFRunLoopCommonModes);
  run_loop_source_.reset();
}

void NetworkConfigWatcherMacThread::InitNotifications() {
  // If initialization fails, retry after a 1s delay.
  bool success = InitNotificationsHelper();

#if !BUILDFLAG(IS_IOS)
  if (!success && num_retry_ < kMaxRetry) {
    LOG(ERROR) << "Retrying SystemConfiguration registration in 1 second.";
    task_runner()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&NetworkConfigWatcherMacThread::InitNotifications,
                       weak_factory_.GetWeakPtr()),
        kRetryInterval);
    num_retry_++;
    return;
  }

#else
  DCHECK(success);
#endif  // !BUILDFLAG(IS_IOS)
}

bool NetworkConfigWatcherMacThread::InitNotificationsHelper() {
#if !BUILDFLAG(IS_IOS)
  // SCDynamicStore API does not exist on iOS.
  // Add a run loop source for a dynamic store to the current run loop.
  SCDynamicStoreContext context = {
      0,          // Version 0.
      delegate_,  // User data.
      nullptr,    // This is not reference counted.  No retain function.
      nullptr,    // This is not reference counted.  No release function.
      nullptr,    // No description for this.
  };
  base::apple::ScopedCFTypeRef<SCDynamicStoreRef> store(SCDynamicStoreCreate(
      nullptr, CFSTR("org.chromium"), DynamicStoreCallback, &context));
  if (!store) {
    int error = SCError();
    LOG(ERROR) << "SCDynamicStoreCreate failed with Error: " << error << " - "
               << SCErrorString(error);
    return false;
  }
  run_loop_source_.reset(
      SCDynamicStoreCreateRunLoopSource(nullptr, store.get(), 0));
  if (!run_loop_source_) {
    int error = SCError();
    LOG(ERROR) << "SCDynamicStoreCreateRunLoopSource failed with Error: "
               << error << " - " << SCErrorString(error);
    return false;
  }
  CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source_.get(),
                     kCFRunLoopCommonModes);
#endif  // !BUILDFLAG(IS_IOS)

  // Set up notifications for interface and IP address changes.
  delegate_->StartReachabilityNotifications();
#if !BUILDFLAG(IS_IOS)
  delegate_->SetDynamicStoreNotificationKeys(store.get());
#endif  // !BUILDFLAG(IS_IOS)
  return true;
}

NetworkConfigWatcherMac::NetworkConfigWatcherMac(Delegate* delegate)
    : notifier_thread_(
          std::make_unique<NetworkConfigWatcherMacThread>(delegate)) {
  // We create this notifier thread because the notification implementation
  // needs a thread with a CFRunLoop, and there's no guarantee that
  // CurrentThread::Get() meets that criterion.
  base::Thread::Options thread_options(base::MessagePumpType::DEFAULT, 0);
  notifier_thread_->StartWithOptions(std::move(thread_options));
}

NetworkConfigWatcherMac::~NetworkConfigWatcherMac() = default;

}  // namespace net
