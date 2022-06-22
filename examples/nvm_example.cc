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
using ROCKSDB_NAMESPACE::NVMSecondaryCacheOptions;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\secondary_test";
#else
std::string kDBPath = "/tmp/nvm_example";
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

void Compact(DB* db, const Slice& start, const Slice& limit) {
  db->CompactRange(rocksdb::CompactRangeOptions(), &start, &limit);
}

int main(){
    DB* db;
    Options options;
    // //set rate limiter to limit IO rate to 100B/s to simulate the cloud storage use cases
    // RateLimiter* _rate_limiter = NewGenericRateLimiter(1000, 100 * 1000, 10, RateLimiter::Mode::kReadsOnly);

    LRUCacheOptions opts(4*1024, 0, false, 0.5, nullptr, rocksdb::kDefaultToAdaptiveMutex,rocksdb::kDontChargeCacheMetadata);
    std::shared_ptr<SecondaryCache> secondary_cache = NewNVMSecondaryCache("/tmp/nvm_example/nvmsc");
    opts.secondary_cache = secondary_cache;
    std::shared_ptr<Cache> cache = NewLRUCache(opts);
    BlockBasedTableOptions table_options;
    table_options.block_cache = cache;
    table_options.block_size = 4 * 1024;
    options.write_buffer_size = 4090 * 4096;
    options.target_file_size_base = 2 * 1024 * 1024;
    options.max_bytes_for_level_base = 10 * 1024 * 1024;
    options.max_open_files = 5000;
    options.wal_recovery_mode = rocksdb::WALRecoveryMode::kTolerateCorruptedTailRecords;
    options.compaction_pri = rocksdb::CompactionPri::kByCompensatedSize;

    options.create_if_missing = true;
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    options.paranoid_file_checks = true;
    // options.env = fault_env_.get();
    // options.rate_limiter.reset(_rate_limiter);

    Status s = DB::Open(options, kDBPath, &db);
    std::cout<<"put data in db"<<std::endl;
    std::string key;
    const int N = 100;
    for(int i =0;i<N;i++){
        key = "key"+std::to_string(i);
        s = db->Put(WriteOptions(),key,rand_str(1000));
    }

    std::cout<<"flush data in db"<<std::endl;
    s = db->Flush(FlushOptions());

    std::cout<<"compact data"<<std::endl;
    // s = db->CompactRange(rocksdb::CompactionOptions(),"a","z");
    Compact(db,"a","z");


    std::string value;

    std::cout<<"get key0"<<std::endl;
    key = "key"+std::to_string(0);
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;

    key = "key"+std::to_string(5);
    std::cout<<"get key5"<<std::endl;
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;


    key = "key"+std::to_string(5);
    std::cout<<"get key5"<<std::endl;
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;


    key = "key"+std::to_string(0);
    std::cout<<"get key0"<<std::endl;
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;


    key = "key"+std::to_string(0);
    std::cout<<"get key0"<<std::endl;
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;

    delete db;
    return 0;
}