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

NVMSecondaryCache::NVMSecondaryCache(const NVMSecondaryCacheOptions& opts) {
    config_.setCacheSize(1 * 1024 * 1024 * 1024) // 1 GB
        .setCacheName("My cache") 
        .setAccessConfig({25, 10})
        .validate();

    navyConfig_.setSimpleFile(opts.fileName, opts.fileSize);
    navyConfig_.setDeviceMetadataSize(opts.deviceMetadataSize);
    navyConfig_.setBlockSize(opts.blockSize);
    navyConfig_.setNavyReqOrderingShards(opts.navyReqOrderingShards);

    navyConfig_.setReaderAndWriterThreads(opts.readerThreads, opts.writerThreads);

    navyConfig_.enableRandomAdmPolicy()
        .setAdmProbability(opts.admProbability);

    navyConfig_.blockCache().setRegionSize(opts.regionSize);

    navyConfig_.bigHash()
        .setSizePctAndMaxItemSize(opts.sizePct, opts.smallItemMaxSize)
        .setBucketSize(opts.bigHashBucketSize)
        .setBucketBfSize(opts.bigHashBucketBfSize);

    // navyConfig_.setBlockSize(4096);
    // navyConfig_.setSimpleFile(opts.fileName, opts.fileSize);
    // navyConfig_.blockCache().setRegionSize(16 * 1024 * 1024);


    nvmConfig_.navyConfig = navyConfig_;
    config_.enableNvmCache(nvmConfig_);
    cache_.reset();
    cache_ = std::make_unique<CacheT>(config_);
    defaultPool_ = cache_->addPool("default", cache_->getCacheMemoryStats().cacheSize);
}


NVMSecondaryCache::~NVMSecondaryCache() { }


std::unique_ptr<SecondaryCacheResultHandle> NVMSecondaryCache::Lookup(
      const Slice& key, const Cache::CreateCallback& create_cb, bool wait,
      bool& is_in_sec_cache){
    std::unique_ptr<SecondaryCacheResultHandle> handle;
    if(key.empty()){
        return handle;
    }
    // find in cachelib
    // convert Slice to folly::StringPiece(std::string)
    std::string key_;
    key_.append(key.data(),key.size());
    // return cachelib handle
    ItemHandle nvm_handle = cache_->find(key_);
    if(nvm_handle) {
        void* value = nullptr;
        size_t charge = 0;
        Status s;
        CacheAllocationPtr* ptr = const_cast<CacheAllocationPtr*>(reinterpret_cast<const CacheAllocationPtr*>(nvm_handle->getMemory()));
        size_t size = nvm_handle->getSize();
/*
        void* str1 = nullptr;
        memcpy(str1,nvm_handle->getMemory(), nvm_handle->getSize());
        std::cout<<"nvm_handle value: "<< str1 <<std::endl;
*/
        s = create_cb(ptr, size, &value, &charge);
        if(s.ok()) {
            handle.reset(new NVMSecondaryCacheResultHandle(&nvm_handle,value,charge));
     	}
    }
    return handle;
}

Status NVMSecondaryCache::Insert(const Slice& key, void* value,
                                 const Cache::CacheItemHelper* helper) {
    if(key.empty()){
        return Status::Corruption("Error with empty key.");
    }
    std::string key_;
    size_t size = (*helper->size_cb)(value);
    CacheAllocationPtr ptr = AllocateBlock(size, nullptr);
    Status s = (*helper->saveto_cb)(value, 0, size, ptr.get());
    if (!s.ok()) {
      return s;
    }
    // convert Slice to folly::StringPiece(std::string)
    key_.append(key.data(),key.size());
    // auto handle = cache_->allocate(defaultPool_, key_, size);
    auto handle = cache_->allocate(defaultPool_, key_, size);
    if(handle) {
        std::memcpy(handle->getMemory(), ptr.get(), size);
	cache_->insertOrReplace(handle);
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
    auto res = cache_->remove(key_);
    return;
}

void NVMSecondaryCache::WaitAll(std::vector<SecondaryCacheResultHandle*> handles) {
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
    return std::make_shared<NVMSecondaryCache>(opts);
}

}  // namespace ROCKSDB_NAMESPACE
