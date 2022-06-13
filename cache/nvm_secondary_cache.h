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
#include <cachelib/allocator/CacheAllocatorConfig.h>
#include <cachelib/allocator/CacheTraits.h>
#include <cachelib/allocator/nvmcache/NvmCache.h>
#include <cachelib/allocator/nvmcache/CacheApiWrapper.h>

namespace ROCKSDB_NAMESPACE {
using namespace facebook::cachelib;
using CacheT = CacheAllocator<LruCacheTrait>;
using NvmCacheT = NvmCache<CacheT>;
using ItemHandle = typename NvmCacheT::ItemHandle;

// NVM SecondaryCacheResultHandle catch the cachelib Itemhandle and convert it 
// to rocksdb's handle
class NVMSecondaryCacheResultHandle : public SecondaryCacheResultHandle {
    public:
    NVMSecondaryCacheResultHandle(ItemHandle *hdl)
        : handle(hdl) {
        if(handle){
            value_ = reinterpret_cast<const char*>(handle->getMemory());
            size_ = handle->getSize();
        }else{
            value_ = nullptr;
            size_ = 0;
        }
    }

    NVMSecondaryCacheResultHandle(ItemHandle *hdl, void* value,size_t size)
        : handle(hdl), value_(value), size_(size) {}

    ~NVMSecondaryCacheResultHandle() override {
        //cache release the cachelib handle;
        handle->release();
    }

    bool IsReady() override { return handle->isReady(); }

    void Wait() override { handle->wait(); }

    void* Value() override { 
        assert(is_ready_);
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

    NVMSecondaryCache(CacheT& cache, const NVMSecondaryCacheOptions& options, NvmCacheConfig nvmconfig);

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

//   uint32_t num_inserts() { return num_inserts_; }

//   uint32_t num_lookups() { return num_lookups_; }

    private:

    CacheT& cache_;
    NVMSecondaryCacheOptions nvmSecondaryConfig_{};
    std::unique_ptr<NvmCacheT> nvmCache_;
    PoolId defaultPool_;
//   std::shared_ptr<Cache> cache_;
//   LRUSecondaryCacheOptions cache_options_;
//   uint32_t num_inserts_;
//   uint32_t num_lookups_;
};

}  // namespace ROCKSDB_NAMESPACE
