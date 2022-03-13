// //  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
// //  This source code is licensed under both the GPLv2 (found in the
// //  COPYING file in the root directory) and Apache 2.0 License
// //  (found in the LICENSE.Apache file in the root directory).

// #include "cache/secondary_nvm_cache.h"
// // #include "secondary_nvm_cache.h"

// #include <memory>

// #include "memory/memory_allocator.h"
// #include "util/compression.h"
// #include "util/string_util.h"

// namespace ROCKSDB_NAMESPACE {

// namespace {

// // void DeletionCallback(const Slice& /*key*/, void* obj) {
// //   delete reinterpret_cast<CacheAllocationPtr*>(obj);
// //   obj = nullptr;
// // }

// }  // namespace

// SecondaryNVMCache::SecondaryNVMCache(Config config,bool truncate): config_(config.validate()){
//     nvmCache_ = std::make_unique<NvmCacheT>(*this, *config_.nvmConfig, truncate,
//                                           config_.itemDestructor);
// }

// SecondaryNVMCache::~SecondaryNVMCache() { nvmcache_.reset(); }

// std::unique_ptr<SecondaryCacheResultHandle> SecondaryNVMCache::Lookup(
//     const Slice& key, const Cache::CreateCallback& create_cb, bool /*wait*/) {

// }

// Status SecondaryNVMCache::Insert(const Slice& key, void* value,
//                                  const Cache::CacheItemHelper* helper) {

// }

// void SecondaryNVMCache::Erase(const Slice& key) { 

// }

// std::string SecondaryNVMCache::GetPrintableOptions() const {

// }

// std::shared_ptr<SecondaryCache> NewSecondaryNVMCache() {

// }

// std::shared_ptr<SecondaryCache> NewSecondaryNVMCache(){
// }

// }  // namespace ROCKSDB_NAMESPACE
