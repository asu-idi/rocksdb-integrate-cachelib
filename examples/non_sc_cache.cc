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
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::RateLimiter;
using ROCKSDB_NAMESPACE::LRUCacheOptions;
using ROCKSDB_NAMESPACE::Cache;
using ROCKSDB_NAMESPACE::FlushOptions;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\non_sc";
#else
std::string kDBPath = "/tmp/non_sc";
#endif

void save_dataint times, double throughout,double readsize_throught){
    std::ofstream outdata; 
    outdata.open("non_sc_result.txt",std::ios::out|std::ios::app); // opens the file
    if( !outdata ) { // file couldn't be opened
        std::cerr << "Error: file could not be opened" << std::endl;
        exit(1);
    }
    outdata << times << "," << throughout << "," << readsize_throught <<std::endl;
    outdata.close();
}

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
    //set rate limiter to limit IO rate to 1kB/s to simulate the cloud storage use cases
    RateLimiter* _rate_limiter = NewGenericRateLimiter(1000, 100 * 1000, 10, RateLimiter::Mode::kReadsOnly);

    BlockBasedTableOptions table_options;
    LRUCacheOptions opts(4*1024, 0, false, 0.5, nullptr, rocksdb::kDefaultToAdaptiveMutex, rocksdb::kDontChargeCacheMetadata);
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
    std::cout << "GET" <<std::endl;
    std::string value;
    int i=0;
    while(i<1000){
        key = "key"+std::to_string(i++);
        s = db->Put(WriteOptions(),key,rand_str(1000));
    }

    s = db->Flush(FlushOptions());
    Compact(db,"a","z");
    const int times = 1000;
    int i =0;
    int read_times = 0;
    u_int64_t read_size = 0;
    begin = std::chrono::steady_clock::now();
    while(i<times){
        key = "key"+std::to_string(rand()%1000);
        _rate_limiter->Request(100, rocksdb::Env::IO_LOW, nullptr, RateLimiter::OpType::kRead);
        s = db->Get(ReadOptions(), key, &value);
        read_times++;
        read_size += value.size();
        end = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()>=1000){
            save_data(i,(double)read_times/i,(double)read_size/i);
            begin = end;
            i++;
        }
    }
    delete db;
    return 0;
}