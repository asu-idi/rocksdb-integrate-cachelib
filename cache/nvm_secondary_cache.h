// Copyright (c) 2011-present, Facebook, Inc. All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <memory>

#include "memory/memory_allocator.h"
#include "rocksdb/secondary_cache.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"

#include <cachelib/allocator/CacheAllocator.h>
#include <cachelib/allocator/CacheAllocatorConfig.h>
#include <cachelib/allocator/CacheTraits.h>
#include <cachelib/allocator/CacheItem.h>
#include <cachelib/allocator/Handle.h>
#include <cachelib/allocator/nvmcache/CacheApiWrapper.h>
#include <cachelib/allocator/nvmcache/NvmCache.h>
#include <cachelib/allocator/nvmcache/NavyConfig.h>

namespace ROCKSDB_NAMESPACE {
using namespace facebook::cachelib;
using CacheT = CacheAllocator<LruCacheTrait>;
using CacheConfig = typename CacheT::Config;
using CacheKey = typename CacheT::Key;
using ItemHandle = typename CacheT::ReadHandle;
using NvmCacheT = NvmCache<CacheT>;
using NvmCacheConfig = typename NvmCacheT::Config;
using NavyConfig = navy::NavyConfig;

// NVM SecondaryCacheResultHandle catch the cachelib Itemhandle and convert it 
// to rocksdb's handle
class NVMSecondaryCacheResultHandle : public SecondaryCacheResultHandle {
    public:
    
    NVMSecondaryCacheResultHandle(ItemHandle *hdl, void* value,size_t size)
        : handle(hdl), value_(value), size_(size) {}

    ~NVMSecondaryCacheResultHandle() override {
        //cache release the cachelib handle;
        handle->release();
    }

    bool IsReady() override { return handle->isReady(); }

    void Wait() override { handle->wait(); }

    void* Value() override { 
        return value_; 
    }

    size_t Size() override { return Value() ? size_ : 0; }

    private:
    ItemHandle *handle;
    void* value_;
    size_t size_;
};


class NVMSecondaryCache : public SecondaryCache {
    public:

    NVMSecondaryCache();

    NVMSecondaryCache(const NVMSecondaryCacheOptions& opts);

    virtual ~NVMSecondaryCache() override;

    const char* Name() const override { return "NVMSecondaryCache"; }

    Status Insert(const Slice& key, void* value,
                const Cache::CacheItemHelper* helper) override;

    std::unique_ptr<SecondaryCacheResultHandle> Lookup(
      const Slice& key, const Cache::CreateCallback& create_cb,
      bool /*wait*/) override;

    void Erase(const Slice& key) override;

    void WaitAll(std::vector<SecondaryCacheResultHandle*> /*handles*/) override;

    std::string GetPrintableOptions() const override;

//   uint32_t num_inserts() { return num_inserts_; }

//   uint32_t num_lookups() { return num_lookups_; }

    private:
    std::unique_ptr<CacheT> cache_;
    CacheConfig config_;
    NvmCacheConfig nvmConfig_;
    NavyConfig navyConfig_;
    PoolId defaultPool_;
    // CacheT& cache_;
    // NVMSecondaryCacheOptions nvmSecondaryConfig_{};
    // std::unique_ptr<NvmCacheT> nvmCache_;
    // PoolId defaultPool_;

//   uint32_t num_inserts_;
//   uint32_t num_lookups_;
};

}  // namespace ROCKSDB_NAMESPACE
