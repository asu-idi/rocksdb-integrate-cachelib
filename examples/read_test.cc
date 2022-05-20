#include <chrono>
#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/cache.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/table.h"
#include "rocksdb/rate_limiter.h"
#include "rocksdb/secondary_cache.h"
#include "rocksdb/utilities/options_util.h"
#include "rocksdb/advanced_options.h"

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
using ROCKSDB_NAMESPACE::Cache;
using ROCKSDB_NAMESPACE::SecondaryCache;
using ROCKSDB_NAMESPACE::CacheTier;
using ROCKSDB_NAMESPACE::FlushOptions;
using ROCKSDB_NAMESPACE::LRUSecondaryCacheOptions;
using ROCKSDB_NAMESPACE::kDontChargeCacheMetadata;
//Insert enough data
//random read for 100 times
//measure time and record hit times

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_data";
#else
std::string kDBPath = "/tmp/rocksdb_data";
#endif

void save_data(int times, double throughout, double hit_rate, int insert){
    std::ofstream outdata; 
    outdata.open("read_test_result.txt",std::ios::out|std::ios::app); // opens the file
    if( !outdata ) { // file couldn't be opened
        std::cerr << "Error: file could not be opened" << std::endl;
        exit(1);
    }
    outdata << times << "," << throughout << "," << hit_rate << ","<< insert << std::endl;
    outdata.close();
}

int main(){
    DB* db;
    Options options;
    //set rate limiter to limit IO rate to 100KB/s to simulate the cloud storage use cases
    RateLimiter* _rate_limiter = NewGenericRateLimiter(100*1024, 100 * 1000, 10, RateLimiter::Mode::kAllIo);

    // BlockBasedTableOptions table_options;
    // LRUCacheOptions opts(1024 * 1024, 0, false, 0.5, nullptr, rocksdb::kDefaultToAdaptiveMutex, rocksdb::kDontChargeCacheMetadata);
    // std::shared_ptr<SecondaryCache> _secondary_cache = NewNVMSecondaryCache("/tmp/cachelib/secondary");
    // opts.secondary_cache = _secondary_cache;
    // std::shared_ptr<Cache> _cache = NewLRUCache(opts);
    // table_options.block_cache = _cache;
    // table_options.block_size = 10 * 1024;
    // options.lowest_used_cache_tier = CacheTier::kNonVolatileBlockTier;
    // options.IncreaseParallelism();
    // options.OptimizeLevelStyleCompaction();
    // options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    LRUCacheOptions opts(1024 * 1024, 0, false, 0.5, nullptr, rocksdb::kDefaultToAdaptiveMutex,rocksdb::kDontChargeCacheMetadata);
    std::shared_ptr<SecondaryCache> secondary_cache = rocksdb::NewLRUSecondaryCache(10*1024*1024);
    opts.secondary_cache = secondary_cache;
    std::shared_ptr<Cache> cache = NewLRUCache(opts);
    BlockBasedTableOptions table_options;
    table_options.block_cache = cache;
    table_options.block_size = 10 * 1024;
    options.write_buffer_size = 4090 * 4096;
    options.target_file_size_base = 2 * 1024 * 1024;
    options.max_bytes_for_level_base = 10 * 1024 * 1024;
    options.max_open_files = 5000;
    options.wal_recovery_mode = rocksdb::WALRecoveryMode::kTolerateCorruptedTailRecords;
    options.compaction_pri = rocksdb::CompactionPri::kByCompensatedSize;

    options.create_if_missing = true;
    options.rate_limiter.reset(_rate_limiter);

    Status s = DB::Open(options, kDBPath, &db);
    std::string key;
    std::chrono::steady_clock::time_point begin,end;
    std::string value;
    const int times = 1000;
    int i =0;
    int read_times = 0;
    u_int64_t read_size = 0;
    int hit_times =0;
    begin = std::chrono::steady_clock::now();
    while(i<times){
        key = "key"+std::to_string(rand()%10000000);
        _rate_limiter->Request(100, rocksdb::Env::IO_LOW, nullptr, RateLimiter::OpType::kRead);
        s = db->Get(ReadOptions(), key, &value);
        read_times++;
        read_size += value.size();
        end = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()>=1000){
            save_data(i,(double)read_size/i*1024*1024,(double)secondary_cache->num_lookups()/read_times,secondary_cache->num_inserts());
            begin = end;
            i++;
        }
    }
    delete db;
    return 0;
}