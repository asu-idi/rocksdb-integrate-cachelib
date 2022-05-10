#include <chrono>
#include <cstdio>
#include <string>


#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/cache.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/table.h"
#include "rocksdb/rate_limiter.h"
#include "rocksdb/utilities/options_util.h"

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::DBOptions;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::PinnableSlice;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteBatch;
using ROCKSDB_NAMESPACE::WriteOptions;
using ROCKSDB_NAMESPACE::BlockBasedTableOptions;
using ROCKSDB_NAMESPACE::CompactionFilter;
using ROCKSDB_NAMESPACE::ConfigOptions;
using ROCKSDB_NAMESPACE::NewLRUCache;
using ROCKSDB_NAMESPACE::NewNVMSecondaryCache;
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::RateLimiter;
using ROCKSDB_NAMESPACE::LRUCacheOptions;

//Insert enough data
//random read for 100 times
//measure time and record hit times

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\read_test";
#else
std::string kDBPath = "/tmp/read_test";
#endif

int main(){
    DB* db;
    Options options;
    //set rate limiter to limit IO rate to 100B/s to simulate the cloud storage use cases
    RateLimiter* _rate_limiter = NewGenericRateLimiter(100, 100 * 1000, 10, RateLimiter::Mode::kAllIo);

    BlockBasedTableOptions table_options;
    LRUCacheOptions opts(4 * 1024, 0, false, 0.5, nullptr, kDefaultToAdaptiveMutex, kDontChargeCacheMetadata);
    std::shared_ptr<secondary_cache> _secondary_cache = NewNVMSecondaryCache(kDBPath+"/secondary");
    opts.secondary_cache = _secondary_cache;
    std::shared_ptr<Cache> _cache = NewLRUCache(opts);
    table_options.block_cache = _cache;
    table_options.block_size = 4 * 1024;

    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));

    options.create_if_missing = true;
    options.rate_limiter = _rate_limiter;

    Status s = DB::Open(options, kDBPath, &db);
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    s = db->Put(WriteOptions(), "key1", "value");
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    assert(s.ok());
    std::string value;
    // get value
    s = db->Get(ReadOptions(), "key1", &value);
    assert(s.ok());
    assert(value == "value");

    delete db;
    return 0;
}