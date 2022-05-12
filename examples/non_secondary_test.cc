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
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::RateLimiter;
using ROCKSDB_NAMESPACE::LRUCacheOptions;
using ROCKSDB_NAMESPACE::Cache;
//Insert enough data
//random read for 100 times
//measure time and record hit times
#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\non_secondary_test";
#else
std::string kDBPath = "/tmp/non_secondary_test";
#endif

std::string rand_str(const int len)  
{
    std::string str;
    char c;
    int idx;
    for(idx = 0;idx < len;idx ++)
    {
        c = 'a' + rand()%26;
        str.push_back(c);
    }
    return str;
}

void save_data(int num, int64_t times){
    std::ofstream outdata; 
    outdata.open("non_secondary_test_result.txt"); // opens the file
    if( !outdata ) { // file couldn't be opened
        std::cerr << "Error: file could not be opened" << std::endl;
        exit(1);
    }
    outdata << num << "," << times << std::endl;
    outdata.close();
}
int main(){
    DB* db;
    Options options;
    //set rate limiter to limit IO rate to 100B/s to simulate the cloud storage use cases
    RateLimiter* _rate_limiter = NewGenericRateLimiter(1000, 100 * 1000, 10, RateLimiter::Mode::kAllIo);

    BlockBasedTableOptions table_options;
    LRUCacheOptions opts(4 * 1024, 0, false, 0.5, nullptr, rocksdb::kDefaultToAdaptiveMutex, rocksdb::kDontChargeCacheMetadata);
    // std::shared_ptr<SecondaryCache> _secondary_cache = NewNVMSecondaryCache(kDBPath+"/secondary");
    // opts.secondary_cache = _secondary_cache;
    std::shared_ptr<Cache> _cache = NewLRUCache(opts);
    table_options.block_cache = _cache;
    table_options.block_size = 4 * 1024;

    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    options.create_if_missing = true;
    options.rate_limiter.reset(_rate_limiter);

    Status s = DB::Open(options, kDBPath, &db);

    std::string key;
    std::chrono::steady_clock::time_point begin,end;
    for(int i =0;i<100;i++){
        key = "key"+std::to_string(i);
        // std::cout<< key << std::endl;
        begin = std::chrono::steady_clock::now();
        _rate_limiter->Request(100, rocksdb::Env::IO_HIGH, nullptr, RateLimiter::OpType::kWrite);
        s = db->Put(WriteOptions(),key,rand_str(1000));
        end = std::chrono::steady_clock::now();
    //     std::cout << key<< ": Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    }
    std::cout << "GET" <<std::endl;
    std::string value;
    for(int i = 0;i<100;i++){
        key = "key"+std::to_string(rand()%100);
        begin = std::chrono::steady_clock::now();
        _rate_limiter->Request(100, rocksdb::Env::IO_LOW, nullptr, RateLimiter::OpType::kRead);
        s = db->Get(ReadOptions(), key, &value);
        end = std::chrono::steady_clock::now();
        // std::cout << key<< ": Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
        save_data(i,std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count());
    }
    delete db;
    return 0;
}