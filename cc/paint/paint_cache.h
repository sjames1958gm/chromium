// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_CACHE_H_
#define CC_PAINT_PAINT_CACHE_H_

#include <map>
#include <set>

#include "base/containers/mru_cache.h"
#include "cc/paint/paint_export.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkTextBlob.h"

namespace cc {

// PaintCache is used to cache high frequency small paint data types, like
// SkTextBlob and SkPath in the GPU service. The ClientPaintCache budgets and
// controls the cache state in the ServicePaintCache, regularly purging old
// entries returned in ClientPaintCache::Purge from the service side cache. In
// addition to this, the complete cache is cleared during the raster context
// idle cleanup. This effectively means that the cache budget is used as working
// memory that is only kept while we are actively rasterizing.
//
// The entries are serialized by the caller during paint op serialization, and
// the cache assumes the deserialization and purging to be done in order for
// accurately tracking the service side state in ServicePaintCache.
//
// Note that while TransferCache should be used for large data types that would
// benefit from a shared cache budgeted across all clients, using a client
// controlled PaintCache with a tighter budget is better for these data types
// since it avoids the need for cross-process ref-counting required by the
// TransferCache.

using PaintCacheId = uint32_t;
using PaintCacheIds = std::vector<PaintCacheId>;
enum class PaintCacheDataType : uint32_t { kTextBlob, kPath, kLast = kPath };
constexpr size_t PaintCacheDataTypeCount =
    static_cast<uint32_t>(PaintCacheDataType::kLast) + 1u;

class CC_PAINT_EXPORT ClientPaintCache {
 public:
  explicit ClientPaintCache(size_t max_budget_bytes);
  ~ClientPaintCache();

  bool Get(PaintCacheDataType type, PaintCacheId id);
  void Put(PaintCacheDataType type, PaintCacheId id, size_t size);

  // Populates |purged_data| with the list of ids which should be purged from
  // the ServicePaintCache.
  using PurgedData = PaintCacheIds[PaintCacheDataTypeCount];
  void Purge(PurgedData* purged_data);

  // Notifies that all entries should be purged from the ServicePaintCache.
  // Returns true if any entries were evicted from this call.
  bool PurgeAll();

 private:
  using CacheMap =
      base::MRUCache<std::pair<PaintCacheDataType, PaintCacheId>, size_t>;
  CacheMap cache_map_;
  const size_t max_budget_;
  size_t bytes_used_ = 0u;

  DISALLOW_COPY_AND_ASSIGN(ClientPaintCache);
};

class CC_PAINT_EXPORT ServicePaintCache {
 public:
  ServicePaintCache();
  ~ServicePaintCache();

  // Stores the |blob| received from the client in the cache.
  void PutTextBlob(PaintCacheId id, sk_sp<SkTextBlob> blob);

  // Retrieves an entry for |id| stored in the cache. Or nullptr if the entry
  // is not found.
  sk_sp<SkTextBlob> GetTextBlob(PaintCacheId id);

  void PutPath(PaintCacheId, SkPath path);
  SkPath* GetPath(PaintCacheId id);

  void Purge(PaintCacheDataType type,
             size_t n,
             const volatile PaintCacheId* ids);
  void PurgeAll();
  bool empty() const { return cached_blobs_.empty() && cached_paths_.empty(); }

 private:
  using BlobMap = std::map<PaintCacheId, sk_sp<SkTextBlob>>;
  BlobMap cached_blobs_;
  using PathMap = std::map<PaintCacheId, SkPath>;
  PathMap cached_paths_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_CACHE_H_
