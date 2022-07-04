//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "cache/nvm_secondary_cache.h"

#include <algorithm>
#include <cstdint>

#include "memory/jemalloc_nodump_allocator.h"
#include "memory/memory_allocator.h"
#include "rocksdb/convenience.h"
#include "rocksdb/secondary_cache.h"
#include "test_util/testharness.h"
#include "test_util/testutil.h"
#include "util/compression.h"
#include "util/random.h"

namespace ROCKSDB_NAMESPACE {

class NVMSecondaryCacheTest : public testing::Test {
 public:
  NVMSecondaryCacheTest() : fail_create_(false) {}
  ~NVMSecondaryCacheTest() {}

 protected:
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

  void BasicTestHelper2(std::shared_ptr<SecondaryCache> sec_cache) {
    bool is_in_sec_cache{true};
    std::unique_ptr<SecondaryCacheResultHandle> handle0 =
        sec_cache->Lookup("k0", test_item_creator, true, is_in_sec_cache);
    ASSERT_EQ(handle0, nullptr);
  }

  void BasicTestHelper(std::shared_ptr<SecondaryCache> sec_cache) {
    bool is_in_sec_cache{true};
    // Lookup an non-existent key.
    std::unique_ptr<SecondaryCacheResultHandle> handle0 =
        sec_cache->Lookup("k0", test_item_creator, true, is_in_sec_cache);
    ASSERT_EQ(handle0, nullptr);

    Random rnd(301);
    // Insert and Lookup the first item.
    std::string str1;
    str1 = rnd.RandomString(1020);
    TestItem item1(str1.data(), str1.length());
    ASSERT_OK(sec_cache->Insert("k1", &item1,
                                &NVMSecondaryCacheTest::helper_));

    std::unique_ptr<SecondaryCacheResultHandle> handle1 =
        sec_cache->Lookup("k1", test_item_creator, true, is_in_sec_cache);
    ASSERT_NE(handle1, nullptr);
    ASSERT_FALSE(is_in_sec_cache);

    std::unique_ptr<TestItem> val1 =
        std::unique_ptr<TestItem>(static_cast<TestItem*>(handle1->Value()));
    ASSERT_NE(val1, nullptr);
    ASSERT_EQ(memcmp(val1->Buf(), item1.Buf(), item1.Size()), 0);

    // Lookup the first item again.
    std::unique_ptr<SecondaryCacheResultHandle> handle1_1 =
        sec_cache->Lookup("k1", test_item_creator, true, is_in_sec_cache);
    ASSERT_EQ(handle1_1, nullptr);

    // Insert and Lookup the second item.
    std::string str2;
    str2 = rnd.RandomString(1020);
    TestItem item2(str2.data(), str2.length());
    ASSERT_OK(sec_cache->Insert("k2", &item2,
                                &NVMSecondaryCacheTest::helper_));
    std::unique_ptr<SecondaryCacheResultHandle> handle2 =
        sec_cache->Lookup("k2", test_item_creator, true, is_in_sec_cache);
    ASSERT_NE(handle2, nullptr);
    std::unique_ptr<TestItem> val2 =
        std::unique_ptr<TestItem>(static_cast<TestItem*>(handle2->Value()));
    ASSERT_NE(val2, nullptr);
    ASSERT_EQ(memcmp(val2->Buf(), item2.Buf(), item2.Size()), 0);

    std::vector<SecondaryCacheResultHandle*> handles = {handle1.get(),
                                                        handle2.get()};
    sec_cache->WaitAll(handles);

    sec_cache.reset();
  }

  void BasicTest() {
    NVMSecondaryCacheOptions opts;
    opts.fileName="/cachelib/sc";

    std::shared_ptr<SecondaryCache> sec_cache =
        NewNVMSecondaryCache(opts);
  }

  void FailsTest() {
    NVMSecondaryCacheOptions opts;
    opts.fileName="/cachelib/sc";
    std::shared_ptr<SecondaryCache> sec_cache =
        NewNVMSecondaryCache(opts);

    // Insert and Lookup the first item.
    Random rnd(301);
    std::string str1(rnd.RandomString(1000));
    TestItem item1(str1.data(), str1.length());
    ASSERT_OK(sec_cache->Insert("k1", &item1,
                                &NVMSecondaryCacheTest::helper_));

    // Insert and Lookup the second item.
    std::string str2(rnd.RandomString(200));
    TestItem item2(str2.data(), str2.length());
    // k1 is evicted.
    ASSERT_OK(sec_cache->Insert("k2", &item2,
                                &NVMSecondaryCacheTest::helper_));
    bool is_in_sec_cache{false};
    std::unique_ptr<SecondaryCacheResultHandle> handle1_1 =
        sec_cache->Lookup("k1", test_item_creator, true, is_in_sec_cache);
    ASSERT_EQ(handle1_1, nullptr);
    std::unique_ptr<SecondaryCacheResultHandle> handle2 =
        sec_cache->Lookup("k2", test_item_creator, true, is_in_sec_cache);
    ASSERT_NE(handle2, nullptr);
    std::unique_ptr<TestItem> val2 =
        std::unique_ptr<TestItem>(static_cast<TestItem*>(handle2->Value()));
    ASSERT_NE(val2, nullptr);
    ASSERT_EQ(memcmp(val2->Buf(), item2.Buf(), item2.Size()), 0);

    // Create Fails.
    SetFailCreate(true);
    std::unique_ptr<SecondaryCacheResultHandle> handle2_1 =
        sec_cache->Lookup("k2", test_item_creator, true, is_in_sec_cache);
    ASSERT_EQ(handle2_1, nullptr);

    // Save Fails.
    std::string str3 = rnd.RandomString(10);
    TestItem item3(str3.data(), str3.length());
    ASSERT_NOK(sec_cache->Insert("k3", &item3,
                                 &NVMSecondaryCacheTest::helper_fail_));

    sec_cache.reset();
  }

 private:
  bool fail_create_;
};

Cache::CacheItemHelper NVMSecondaryCacheTest::helper_(
    NVMSecondaryCacheTest::SizeCallback,
    NVMSecondaryCacheTest::SaveToCallback,
    NVMSecondaryCacheTest::DeletionCallback);

Cache::CacheItemHelper NVMSecondaryCacheTest::helper_fail_(
    NVMSecondaryCacheTest::SizeCallback,
    NVMSecondaryCacheTest::SaveToCallbackFail,
    NVMSecondaryCacheTest::DeletionCallback);

TEST_F(NVMSecondaryCacheTest, BasicTest) {
  BasicTest();
}

#ifndef ROCKSDB_LITE

TEST_F(NVMSecondaryCacheTest, BasicTestFromString) {
  std::string sec_cache_uri =
      "nvm_secondary_cache://"
      "filename=/cachelib/sc;";
  std::cout<<sec_cache_uri<<std::endl;
  std::shared_ptr<SecondaryCache> sec_cache;
  Status s = SecondaryCache::CreateFromString(ConfigOptions(), sec_cache_uri,
                                              &sec_cache);
  EXPECT_OK(s);
  BasicTestHelper2(sec_cache);
}

// TEST_F(NVMSecondaryCacheTest, BasicTestFromStringWithCompression) {
//   std::string sec_cache_uri;
//   if (LZ4_Supported()) {
//     sec_cache_uri =
//         "NVM_secondary_cache://"
//         "capacity=2048;num_shard_bits=0;compression_type=kLZ4Compression;"
//         "compress_format_version=2";
//   } else {
//     ROCKSDB_GTEST_SKIP("This test requires LZ4 support.");
//     sec_cache_uri =
//         "NVM_secondary_cache://"
//         "capacity=2048;num_shard_bits=0;compression_type=kNoCompression";
//   }

//   std::shared_ptr<SecondaryCache> sec_cache;
//   Status s = SecondaryCache::CreateFromString(ConfigOptions(), sec_cache_uri,
//                                               &sec_cache);
//   EXPECT_OK(s);
//   BasicTestHelper(sec_cache);
// }

#endif  // ROCKSDB_LITE

// TEST_F(NVMSecondaryCacheTest, FailsTest) {
//   FailsTest();
// }

}  // namespace ROCKSDB_NAMESPACE

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
