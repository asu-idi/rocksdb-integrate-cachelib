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

NVMSecondaryCache::NVMSecondaryCache(const ItemDestructor& itemDestructor, 
        NvmCacheConfig config, CacheT& cache, bool truncate)
        :nvmConfig_(config), cache_(cache),itemDestructor_(itemDestructor) {
    nvmCache_ = std::make_unique<NvmCacheT>(cache_, nvmConfig_, truncate, itemDestructor_);
}


NVMSecondaryCache::~NVMSecondaryCache() { nvmCache_.reset(); }


std::unique_ptr<SecondaryCacheResultHandle> NVMSecondaryCache::Lookup(
    const Slice& key, const Cache::CreateCallback& create_cb, bool /*wait*/) {
    std::unique_ptr<SecondaryCacheResultHandle> handle;
    if(key.empty()){
        return handle;
    }
    // convert Slice to folly::StringPiece(std::string)
    std::string key_;
    key_.append(key.data(),key.size());

    // return cachelib handle
    auto nvm_handle = nvmCache_->find(key_);
    void* value = nvm_handle->getMemory();
    size_t charge = nvm_handle->getSize();
    // convert cachelib handle to secondarycachehandle
    handle.reset(new NVMSecondaryCacheResultHandle(value, charge));
    return handle;
}

Status NVMSecondaryCache::Insert(const Slice& key, void* value,
                                 const Cache::CacheItemHelper* helper) {
    if(key.empty()){
        return Status::Corruption("Error with empty key.");
    }
    // convert Slice to folly::StringPiece(std::string)
    std::string key_;
    key_.append(key.data(),key.size());
    
    auto handle = cache_.peek(key_);
    nvmCache_->put(handle, nvmCache_->createPutToken(key_));
    return Status::OK();
}

void NVMSecondaryCache::Erase(const Slice& key) { 
    if(key.empty()){
        return;
    }
    // convert Slice to folly::StringPiece(std::string)
    std::string key_;
    key_.append(key.data(),key.size());
    nvmCache_->remove(key_,nvmCache_->createDeleteTombStone(key_));
    return;
}

std::string NVMSecondaryCache::GetPrintableOptions() const {
    return "";
}

// std::shared_ptr<SecondaryCache> NewNVMSecondaryCache() {

// }

// std::shared_ptr<SecondaryCache> NewNVMSecondaryCache(
//     std::string _fileName , uint64_t _fileSize, bool _truncateFile,
//     uint64_t _deviceMetadataSize, uint64_t _blockSize, uint64_t _navyReqOrderingShards,
//     unsigned int _readerThreads, unsigned int _writerThreads, 
//     double _admProbability, uint32_t _regionSize, unsigned int _sizePct, uint64_t _smallItemMaxSize, 
//     uint32_t _bigHashBucketSize, uint64_t _bigHashBucketBfSize,
//     CompressionType _compression_type, uint32_t _compress_format_version){

//     }

// std::shared_ptr<SecondaryCache> NewNVMSecondaryCache(const NVMSecondaryCacheOptions& opts){
//     return NewNVMSecondaryCache(
//       opts.capacity, opts.num_shard_bits, opts.strict_capacity_limit,
//       opts.high_pri_pool_ratio, opts.memory_allocator, opts.use_adaptive_mutex,
//       opts.metadata_charge_policy, opts.compression_type,
//       opts.compress_format_version);
// }

}  // namespace ROCKSDB_NAMESPACE
