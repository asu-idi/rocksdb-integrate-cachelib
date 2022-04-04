// Copyright (c) 2011-present, Facebook, Inc. All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <memory>

#include "cache/lru_cache.h"
#include "memory/memory_allocator.h"
#include "rocksdb/secondary_cache.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "util/compression.h"

#include <cachelib/allocator/CacheAllocator.h>
#include <cachelib/allocator/nvmcache/NvmCache.h>

namespace ROCKSDB_NAMESPACE {
using namespace facebook::cachelib;

class NVMSecondaryCacheResultHandle : public SecondaryCacheResultHandle {
 public:
  NVMSecondaryCacheResultHandle(void* value, size_t size)
      : value_(value), size_(size) {}
  virtual ~NVMSecondaryCacheResultHandle() override = default;

  NVMSecondaryCacheResultHandle(const NVMSecondaryCacheResultHandle&) = delete;
  NVMSecondaryCacheResultHandle& operator=(
      const NVMSecondaryCacheResultHandle&) = delete;

  bool IsReady() override { return true; }

  void Wait() override {}

  void* Value() override { return value_; }

  size_t Size() override { return size_; }

 private:
  void* value_;
  size_t size_;
};

// The NVMSecondaryCache is a concrete implementation of
// rocksdb::SecondaryCache.

class NVMSecondaryCache : public SecondaryCache {
 public:
  NVMSecondaryCache(
      Config config
      //basic params about nvmcache&navy
    /*nvmcache_config to initiate*/);

  virtual ~NVMSecondaryCache() override;

  const char* Name() const override { return "NVMSecondaryCache"; }

  Status Insert(const Slice& key, void* value,
                const Cache::CacheItemHelper* helper) override;

  std::unique_ptr<SecondaryCacheResultHandle> Lookup(
      const Slice& key, const Cache::CreateCallback& create_cb,
      bool /*wait*/) override;

  void Erase(const Slice& key) override;

  void WaitAll(std::vector<SecondaryCacheResultHandle*> /*handles*/) override {}

  std::string GetPrintableOptions() const override;

 private:
//   std::shared_ptr<Cache> cache_;
  std::unique_ptr<CacheT> nvmCache_;
  NVMSecondaryCacheOptions cache_options_;
  const Config config_{};
};

}  // namespace ROCKSDB_NAMESPACE
