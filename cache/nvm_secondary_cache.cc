//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "cache/nvm_secondary_cache.h"

#include <memory>

#include "memory/memory_allocator.h"
#include "util/compression.h"
#include "util/string_util.h"

namespace ROCKSDB_NAMESPACE {

namespace {

// void DeletionCallback(const Slice& /*key*/, void* obj) {
//   delete reinterpret_cast<CacheAllocationPtr*>(obj);
//   obj = nullptr;
// }

}  // namespace

NVMSecondaryCache::NVMSecondaryCache(const ItemDestructor& itemDestructor, NvmCacheConfig config, CacheT& cache, bool truncate)
:nvmConfig_(config.validateAndSetDefaults()), itemDestructor_(itemDestructor){
    nvmCache_ = std::make_unique<NvmCacheT>(cache, nvmConfig_, truncate,itemDestructor_);
}


NVMSecondaryCache::~NVMSecondaryCache() { nvmCache_.reset(); }


std::unique_ptr<SecondaryCacheResultHandle> NVMSecondaryCache::Lookup(
    const Slice& key, const Cache::CreateCallback& create_cb, bool /*wait*/) {

}

Status NVMSecondaryCache::Insert(const Slice& key, void* value,
                                 const Cache::CacheItemHelper* helper) {

}

void NVMSecondaryCache::Erase(const Slice& key) { 

}

std::string NVMSecondaryCache::GetPrintableOptions() const {

}

std::shared_ptr<SecondaryCache> NewNVMSecondaryCache() {

}

std::shared_ptr<SecondaryCache> NewNVMSecondaryCache(){
}

}  // namespace ROCKSDB_NAMESPACE
