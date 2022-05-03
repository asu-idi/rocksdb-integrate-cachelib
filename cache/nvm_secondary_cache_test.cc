//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "cache/nvm_secondary_cache.h"

#include <algorithm>
#include <cstdint>

// #include "memory/jemalloc_nodump_allocator.h"
// #include "memory/memory_allocator.h"
// #include "test_util/testharness.h"
// #include "test_util/testutil.h"
// #include "util/compression.h"
#include "util/random.h"

namespace ROCKSDB_NAMESPACE {

class NVMSecondaryCacheTest {
 public:
  NVMSecondaryCacheTest() : fail_create_(false) {}
  ~NVMSecondaryCacheTest() {}

  class TestItem {
   public:
    TestItem(const char* buf, size_t size) : buf_(new char[size]), size_(size) {
      memcpy(buf_.get(), buf, size);
    }
    ~TestItem() {}

    char* Buf() { return buf_.get(); }
    size_t Size() { return size_; }

   private:
    std::unique_ptr<char[]> buf_;
    size_t size_;
  };

  static size_t SizeCallback(void* obj) {
    return reinterpret_cast<TestItem*>(obj)->Size();
  }

  static Status SaveToCallback(void* from_obj, size_t from_offset,
                               size_t length, void* out) {
    TestItem* item = reinterpret_cast<TestItem*>(from_obj);
    const char* buf = item->Buf();
    EXPECT_EQ(length, item->Size());
    EXPECT_EQ(from_offset, 0);
    memcpy(out, buf, length);
    return Status::OK();
  }

  static void DeletionCallback(const Slice& /*key*/, void* obj) {
    delete reinterpret_cast<TestItem*>(obj);
    obj = nullptr;
  }

  static Cache::CacheItemHelper helper_;

  static Status SaveToCallbackFail(void* /*obj*/, size_t /*offset*/,
                                   size_t /*size*/, void* /*out*/) {
    return Status::NotSupported();
  }

  static Cache::CacheItemHelper helper_fail_;

  Cache::CreateCallback test_item_creator = [&](const void* buf, size_t size,
                                                void** out_obj,
                                                size_t* charge) -> Status {
    if (fail_create_) {
      return Status::NotSupported();
    }
    *out_obj = reinterpret_cast<void*>(new TestItem((char*)buf, size));
    *charge = size;
    return Status::OK();
  };

  void SetFailCreate(bool fail) { fail_create_ = fail; }

  void BasicTest(bool sec_cache_is_compressed, bool use_jemalloc) {
    NVMSecondaryCacheOptions opts;
    opts.fileName = "/tmp/nvm";

    std::shared_ptr<SecondaryCache> cache = NewNVMSecondaryCache(opts);
    // Lookup an non-existent key.
    std::unique_ptr<SecondaryCacheResultHandle> handle0 =
        cache->Lookup("k0", test_item_creator, true);
    // // ASSERT_EQ(handle0, nullptr);

    Random rnd(301);
    // Insert and Lookup the first item.
    std::string str1 ="test1";
    TestItem item1(str1.data(), str1.length());
    cache->Insert("k1", &item1, &NVMSecondaryCacheTest::helper_);
    std::unique_ptr<SecondaryCacheResultHandle> handle1 =
        cache->Lookup("k1", test_item_creator, true);
    // ASSERT_NE(handle1, nullptr);
    // delete reinterpret_cast<TestItem*>(handle1->Value());
    std::unique_ptr<TestItem> val1 =
        std::unique_ptr<TestItem>(static_cast<TestItem*>(handle1->Value()));
    // ASSERT_NE(val1, nullptr);
    // ASSERT_EQ(memcmp(val1->Buf(), item1.Buf(), item1.Size()), 0);

    // Insert and Lookup the second item.
    std::string str2="test2";

    TestItem item2(str2.data(), str2.length());
    cache->Insert("k2", &item2, &NVMSecondaryCacheTest::helper_);
    std::unique_ptr<SecondaryCacheResultHandle> handle2 =
        cache->Lookup("k2", test_item_creator, true);
    // ASSERT_NE(handle2, nullptr);
    std::unique_ptr<TestItem> val2 =
        std::unique_ptr<TestItem>(static_cast<TestItem*>(handle2->Value()));
    // ASSERT_NE(val2, nullptr);
    // ASSERT_EQ(memcmp(val2->Buf(), item2.Buf(), item2.Size()), 0);

    // Lookup the first item again to make sure it is still in the cache.
    std::unique_ptr<SecondaryCacheResultHandle> handle1_1 =
        cache->Lookup("k1", test_item_creator, true);
    // ASSERT_NE(handle1_1, nullptr);
    std::unique_ptr<TestItem> val1_1 =
        std::unique_ptr<TestItem>(static_cast<TestItem*>(handle1_1->Value()));
    // ASSERT_NE(val1_1, nullptr);
    // ASSERT_EQ(memcmp(val1_1->Buf(), item1.Buf(), item1.Size()), 0);

    std::vector<SecondaryCacheResultHandle*> handles = {handle1.get(),
                                                        handle2.get()};
    cache->WaitAll(handles);

    cache->Erase("k1");
    handle1 = cache->Lookup("k1", test_item_creator, true);
    // ASSERT_EQ(handle1, nullptr);

    cache.reset();
  }

 private:
  bool fail_create_;
};

Cache::CacheItemHelper NVMSecondaryCacheTest::helper_(
    NVMSecondaryCacheTest::SizeCallback, NVMSecondaryCacheTest::SaveToCallback,
    NVMSecondaryCacheTest::DeletionCallback);

Cache::CacheItemHelper NVMSecondaryCacheTest::helper_fail_(
    NVMSecondaryCacheTest::SizeCallback,
    NVMSecondaryCacheTest::SaveToCallbackFail,
    NVMSecondaryCacheTest::DeletionCallback);


}  // namespace ROCKSDB_NAMESPACE

int main(int argc, char** argv) {
  ROCKSDB_NAMESPACE::NVMSecondaryCacheTest test;
  test.BasicTest(false,false);
  return 0;
}
