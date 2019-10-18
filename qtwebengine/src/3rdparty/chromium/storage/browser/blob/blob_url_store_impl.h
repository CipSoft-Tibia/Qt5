// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_
#define STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_

#include <memory>

#include "base/component_export.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "storage/browser/blob/blob_url_registry.h"
#include "third_party/blink/public/mojom/blob/blob_url_store.mojom.h"
#include "url/origin.h"

namespace storage {

class BlobUrlRegistry;

class COMPONENT_EXPORT(STORAGE_BROWSER) BlobURLStoreImpl
    : public blink::mojom::BlobURLStore {
 public:
  BlobURLStoreImpl(const url::Origin& origin,
                   base::WeakPtr<BlobUrlRegistry> registry);
  ~BlobURLStoreImpl() override;

  void Register(mojo::PendingRemote<blink::mojom::Blob> blob,
                const GURL& url,
                RegisterCallback callback) override;
  void Revoke(const GURL& url) override;
  void Resolve(const GURL& url, ResolveCallback callback) override;
  void ResolveAsURLLoaderFactory(
      const GURL& url,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver)
      override;
  void ResolveForNavigation(
      const GURL& url,
      mojo::PendingReceiver<blink::mojom::BlobURLToken> token) override;

 private:
  // Checks if the passed in url is a valid blob url for this blob url store.
  // Returns false and reports a bad mojo message if not.
  bool BlobUrlIsValid(const GURL& url, const char* method) const;

  const url::Origin origin_;
  base::WeakPtr<BlobUrlRegistry> registry_;

  std::set<GURL> urls_;

  base::WeakPtrFactory<BlobURLStoreImpl> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BlobURLStoreImpl);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_
