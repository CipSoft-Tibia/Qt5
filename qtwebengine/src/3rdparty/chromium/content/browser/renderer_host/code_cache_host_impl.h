// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_CODE_CACHE_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_CODE_CACHE_HOST_IMPL_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "components/services/storage/public/mojom/cache_storage_control.mojom.h"
#include "content/browser/code_cache/generated_code_cache.h"
#include "content/browser/code_cache/generated_code_cache_context.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "third_party/blink/public/mojom/cache_storage/cache_storage.mojom.h"
#include "third_party/blink/public/mojom/loader/code_cache.mojom.h"

class GURL;

namespace content {

// class GeneratedCodeCache;
// class GeneratedCodeCacheContext;

// The implementation of a CodeCacheHost, which stores and retrieves resource
// metadata, either bytecode or native code, generated by a renderer process.
//
// This implements two independent caches:
//
//  GeneratedCodeCache:
//    Entries are keyed by URL, cache type, and network isolation key.
//    See: - `DidGenerateCacheableMetadata`
//         - `FetchCachedCode`
//         - `ClearCodeCacheEntry`
//
//  CacheStorage:
//    Entries are keyed by URL, cache name, and storage key.
//    This class only supports writing such data. Data is stored as
//    "side data" in the cache storage cache.
//    See: - `DidGenerateCacheableMetadataInCacheStorage`
//         - `CacheStorageCache::WriteSideData`
//
// This class is sequence-friendly and is not necessarily bound to a physical
// thread.
class CONTENT_EXPORT CodeCacheHostImpl : public blink::mojom::CodeCacheHost {
 public:
  // Holds a receiver set which will automatically add and clear receivers on
  // the code cache thread.
  class ReceiverSet {
   public:
    explicit ReceiverSet(
        scoped_refptr<GeneratedCodeCacheContext> generated_code_cache_context);
    ~ReceiverSet();

    using CodeCacheHostReceiverHandler = base::OnceCallback<void(
        CodeCacheHostImpl*,
        mojo::ReceiverId,
        mojo::UniqueReceiverSet<blink::mojom::CodeCacheHost>&)>;
    void Add(int render_process_id,
             const net::NetworkIsolationKey& nik,
             const blink::StorageKey& storage_key,
             mojo::PendingReceiver<blink::mojom::CodeCacheHost> receiver,
             CodeCacheHostReceiverHandler handler);
    void Add(int render_process_id,
             const net::NetworkIsolationKey& nik,
             const blink::StorageKey& storage_key,
             mojo::PendingReceiver<blink::mojom::CodeCacheHost> receiver);
    void Clear();

   private:
    scoped_refptr<GeneratedCodeCacheContext> generated_code_cache_context_;
    std::unique_ptr<mojo::UniqueReceiverSet<blink::mojom::CodeCacheHost>,
                    base::OnTaskRunnerDeleter>
        receiver_set_;
  };

  // |render_process_host| is used to get the storge partition that should be
  // used by the fetch requests. This could be null in tests that use
  // SetCacheStorageControlForTesting.
  CodeCacheHostImpl(
      int render_process_id,
      scoped_refptr<GeneratedCodeCacheContext> generated_code_cache_context,
      const net::NetworkIsolationKey& nik,
      const blink::StorageKey& storage_key);

  CodeCacheHostImpl(const CodeCacheHostImpl&) = delete;
  CodeCacheHostImpl& operator=(const CodeCacheHostImpl&) = delete;

  ~CodeCacheHostImpl() override;

  void SetCacheStorageControlForTesting(
      storage::mojom::CacheStorageControl* cache_storage_control);

 private:
  // blink::mojom::CodeCacheHost implementation.
  void DidGenerateCacheableMetadata(blink::mojom::CodeCacheType cache_type,
                                    const GURL& url,
                                    base::Time expected_response_time,
                                    mojo_base::BigBuffer data) override;
  void FetchCachedCode(blink::mojom::CodeCacheType cache_type,
                       const GURL& url,
                       FetchCachedCodeCallback) override;
  void ClearCodeCacheEntry(blink::mojom::CodeCacheType cache_type,
                           const GURL& url) override;
  void DidGenerateCacheableMetadataInCacheStorage(
      const GURL& url,
      base::Time expected_response_time,
      mojo_base::BigBuffer data,
      const std::string& cache_storage_cache_name) override;

  // Helpers.
  GeneratedCodeCache* GetCodeCache(blink::mojom::CodeCacheType cache_type);
  void OnReceiveCachedCode(blink::mojom::CodeCacheType cache_type,
                           base::TimeTicks start_time,
                           FetchCachedCodeCallback callback,
                           const base::Time& response_time,
                           mojo_base::BigBuffer data);

  // Our render process host ID, used to bind to the correct render process.
  const int render_process_id_;

  // Used to override the CacheStorageControl from the RHPI as needed.
  raw_ptr<storage::mojom::CacheStorageControl>
      cache_storage_control_for_testing_ = nullptr;

  scoped_refptr<GeneratedCodeCacheContext> generated_code_cache_context_;

  // The key used to partition code cached in the `GeneratedCodeCache`.
  const net::NetworkIsolationKey network_isolation_key_;

  // The key used to partition code cached in the cache API.
  const blink::StorageKey storage_key_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<CodeCacheHostImpl> weak_ptr_factory_{this};
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_CODE_CACHE_HOST_IMPL_H_
