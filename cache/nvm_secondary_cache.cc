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

// // NVMSecondaryCache::NVMSecondaryCache(const ItemDestructor& itemDestructor, 
// //         NvmCacheConfig config, CacheT& cache, bool truncate)
// //         :nvmConfig_(config), cache_(cache),itemDestructor_(itemDestructor) {
// //     nvmCache_ = std::make_unique<NvmCacheT>(cache_, nvmConfig_, truncate, itemDestructor_);
// // }

NVMSecondaryCache::NVMSecondaryCache(CacheT& cache, const NVMSecondaryCacheOptions& options, NvmCacheConfig nvmconfig)
        :cache_(cache),nvmSecondaryConfig_(options){
    nvmCache_ = std::make_unique<NvmCacheT>(cache_, nvmConfig_, false, nullptr);
    defaultPool = cache_->addPool("default", cache_->getCacheMemoryStats().cacheSize);
}


NVMSecondaryCache::~NVMSecondaryCache() { nvmCache_.reset(); }


std::unique_ptr<SecondaryCacheResultHandle> NVMSecondaryCache::Lookup(
    const Slice& key, const Cache::CreateCallback& create_cb, bool /*wait*/) {
    std::unique_ptr<SecondaryCacheResultHandle> handle;
    if(key.empty()){
        return handle;
    }
    // find in cachelib
    // convert Slice to folly::StringPiece(std::string)
    std::string key_;
    key_.append(key.data(),key.size());
    // return cachelib handle
    auto nvm_handle = nvmCache_->find(key_);
    if(nvm_handle) {
        void* value = nullptr;
        size_t charge = 0;
        Status s;
        char* ptr = reinterpret_cast<const char*>(nvm_handle->getMemory());
        size_t size = DecodeFixed64(ptr);
        ptr += sizeof(uint64_t);
        s = create_cb(ptr, size, &value, &charge);
        if(s.ok()) {
            handle.reset(new NVMSecondaryCacheResultHandle(nvm_handle,value,charge));
        } else {
            nvm_handle->release();
        }
    }
    return handle;
}

Status NVMSecondaryCache::Insert(const Slice& key, void* value,
                                 const Cache::CacheItemHelper* helper) {
    if(key.empty()){
        return Status::Corruption("Error with empty key.");
    }
    size_t size;
    char* buf;
    Status s;
    std::string key_;
    size = (*helper->size_cb)(value);
    buf = new char[size + sizeof(uint64_t)];
    EncodeFixed64(buf, size);
    s = (*helper->saveto_cb)(value, 0, size, buf + sizeof(uint64_t));
    if (!s.ok()) {
      delete[] buf;
      return s;
    }
    // convert Slice to folly::StringPiece(std::string)
    key_.append(key.data(),key.size());
    // auto handle = cache_->allocate(defaultPool_, key_, size);
    auto handle = CacheAPIWrapperForNvm<CacheT>::findInternal(cache_, key_);
    if(handle) {
        nvmCache_->put(handle, nvmCache_->createPutToken(key_));
        return Status::OK();
    }
    return Status::NotFound();
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

void NVMSecondaryCache::WaitAll(std::vector<SecondaryCacheResultHandle*> handles) override {
    for (SecondaryCacheResultHandle* handle : handles) {
        if(!handle) {
            continue;
        }
        NVMSecondaryCacheResultHandle* sec_handle = static_cast<NVMSecondaryCacheResultHandle*>(handle);
        sec_handle->Wait();
    }
}

std::string NVMSecondaryCache::GetPrintableOptions() const {
    return "";
} 


std::shared_ptr<SecondaryCache> NewNVMSecondaryCache(
    std::string _fileName , uint64_t _fileSize,
    uint64_t _deviceMetadataSize, uint64_t _blockSize, uint64_t _navyReqOrderingShards,
    unsigned int _readerThreads, unsigned int _writerThreads, 
    double _admProbability, uint32_t _regionSize, unsigned int _sizePct, uint64_t _smallItemMaxSize, 
    uint32_t _bigHashBucketSize, uint64_t _bigHashBucketBfSize) {
    NVMSecondaryCacheOptions opts = NVMSecondaryCacheOptions(
        _fileName, _fileSize, _deviceMetadataSize, _blockSize, _navyReqOrderingShards, 
        _readerThreads, _writerThreads, _admProbability, _regionSize, _sizePct , _smallItemMaxSize, 
        _bigHashBucketSize, _bigHashBucketBfSize);
    return NewNVMSecondaryCache(opts);
}

std::shared_ptr<SecondaryCache> NewNVMSecondaryCache(const NVMSecondaryCacheOptions& opts) {
    facebook::cachelib::LruAllocator::Config _config;

    _config.setCacheSize(1 * 1024 * 1024 ) // 1 MB
        .setCacheName("My cache") // unique identifier for the cache
        .setAccessConfig({25, 10});

    facebook::cachelib::LruAllocator::NvmCacheConfig nvmConfig;

    nvmConfig.navyConfig.setSimpleFile(opts.fileName, opts.fileSize);
    nvmConfig.navyConfig.setDeviceMetadataSize(opts.deviceMetadataSize);
    nvmConfig.navyConfig.setBlockSize(opts.blockSize);
    nvmConfig.navyConfig.setNavyReqOrderingShards(opts.navyReqOrderingShards);

    nvmConfig.navyConfig.setReaderAndWriterThreads(opts.readerThreads, opts.writerThreads);

    nvmConfig.navyConfig.enableRandomAdmPolicy()
        .setAdmProbability(opts.admProbability);

    nvmConfig.navyConfig.blockCache().setRegionSize(opts.regionSize);

    nvmConfig.navyConfig.bigHash()
        .setSizePctAndMaxItemSize(opts.sizePct, opts.smallItemMaxSize)
        .setBucketSize(opts.bigHashBucketSize)
        .setBucketBfSize(opts.bigHashBucketBfSize);

    _config.enableNvmCache(nvmConfig);
    std::unique_ptr<facebook::cachelib::LruAllocator>cache_;
    cache_.reset();
    cache_ = std::make_unique<facebook::cachelib::LruAllocator>(_config);
    return std::make_shared<NVMSecondaryCache>(*cache_,opts,nvmConfig);
}

}  // namespace ROCKSDB_NAMESPACE
