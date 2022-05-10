//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "cache/nvm_secondary_cache.h"

#include <memory>

#include "memory/memory_allocator.h"
#include "util/compression.h"
#include "util/string_util.h"
#include <cachelib/allocator/CacheAllocator.h>
#include <cachelib/allocator/nvmcache/NavyConfig.h>
#include <cachelib/allocator/nvmcache/NvmItem.h>

namespace ROCKSDB_NAMESPACE {

// NVMSecondaryCache::NVMSecondaryCache(const ItemDestructor& itemDestructor, 
//         NvmCacheConfig config, CacheT& cache, bool truncate)
//         :nvmConfig_(config), cache_(cache),itemDestructor_(itemDestructor) {
//     nvmCache_ = std::make_unique<NvmCacheT>(cache_, nvmConfig_, truncate, itemDestructor_);
// }

NVMSecondaryCache::NVMSecondaryCache(CacheT& cache, const NVMSecondaryCacheOptions& options, NvmCacheConfig nvmconfig)
        :cache_(cache),nvmSecondaryConfig_(options),nvmConfig_(nvmconfig) {
    nvmCache_ = std::make_unique<NvmCacheT>(cache_, nvmConfig_, false, nullptr);
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


std::shared_ptr<SecondaryCache> NewNVMSecondaryCache(
    std::string _fileName , uint64_t _fileSize, bool _truncateFile,
    uint64_t _deviceMetadataSize, uint64_t _blockSize, uint64_t _navyReqOrderingShards,
    unsigned int _readerThreads, unsigned int _writerThreads, 
    double _admProbability, uint32_t _regionSize, unsigned int _sizePct, uint64_t _smallItemMaxSize, 
    uint32_t _bigHashBucketSize, uint64_t _bigHashBucketBfSize,
    CompressionType _compression_type, uint32_t _compress_format_version){
        NVMSecondaryCacheOptions opts = NVMSecondaryCacheOptions(
            _fileName, _fileSize, _truncateFile, _deviceMetadataSize, _blockSize, _navyReqOrderingShards, 
            _readerThreads, _writerThreads, _admProbability, _regionSize, _sizePct , _smallItemMaxSize, 
            _bigHashBucketSize, _bigHashBucketBfSize, _compression_type, _compress_format_version);
        return NewNVMSecondaryCache(opts);
    }

std::shared_ptr<SecondaryCache> NewNVMSecondaryCache(const NVMSecondaryCacheOptions& opts){
    facebook::cachelib::LruAllocator::Config _config;
    // std::atomic<size_t> nEvictions_{0};
    std::set<uint32_t> poolAllocsizes_{20 * 1024};

    _config.enableCachePersistence("/tmp");
    // _config.setRemoveCallback(
    //     [this](const facebook::cachelib::LruAllocator::RemoveCbData&) { nEvictions_++; });
    _config.setCacheSize(20 * 1024 * 1024);
    _config.enablePoolRebalancing(nullptr, std::chrono::seconds{0});
    facebook::cachelib::LruAllocator::NvmCacheConfig nvmConfig;

    nvmConfig.navyConfig.setSimpleFile(opts.fileName, opts.fileSize, opts.truncateFile /*optional*/);
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
    cache_->addPool("default", 8 * 1024 * 1024, poolAllocsizes_);
    return std::make_shared<NVMSecondaryCache>(*cache_,opts,nvmConfig);
}

}  // namespace ROCKSDB_NAMESPACE
