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
using ROCKSDB_NAMESPACE::NewLRUSecondaryCache;
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::RateLimiter;
using ROCKSDB_NAMESPACE::LRUCacheOptions;
using ROCKSDB_NAMESPACE::Cache;
using ROCKSDB_NAMESPACE::SecondaryCache;
using ROCKSDB_NAMESPACE::CacheTier;
using ROCKSDB_NAMESPACE::FlushOptions;
using ROCKSDB_NAMESPACE::LRUSecondaryCacheOptions;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\secondary_test";
#else
std::string kDBPath = "/tmp/secondary_test";
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

void save_data(int times, double throughout,double readsize_throught,int num_insert,int num_lookup){
    std::ofstream outdata; 
    outdata.open("sc_result.txt",std::ios::out|std::ios::app); // opens the file
    if( !outdata ) { // file couldn't be opened
        std::cerr << "Error: file could not be opened" << std::endl;
        exit(1);
    }
    outdata << times << "," << throughout << "," << readsize_throught << "," << num_insert << "," << num_lookup <<std::endl;
    outdata.close();
}
void Compact(DB* db, const Slice& start, const Slice& limit) {
  db->CompactRange(rocksdb::CompactRangeOptions(), &start, &limit);
}

int main(){
    DB* db;
    Options options;
    //set rate limiter to limit IO rate to 100B/s to simulate the cloud storage use cases
    RateLimiter* _rate_limiter = NewGenericRateLimiter(1000, 100 * 1000, 10, RateLimiter::Mode::kAllIo);

    LRUCacheOptions opts(6100, 0, false, 0.5, nullptr, rocksdb::kDefaultToAdaptiveMutex,rocksdb::kDontChargeCacheMetadata);
    std::shared_ptr<SecondaryCache> secondary_cache = NewLRUSecondaryCache(2048*1024);
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
    options.rate_limiter.reset(_rate_limiter);

    Status s = DB::Open(options, kDBPath, &db);
    std::cout<<"put data in db"<<std::endl;
    std::string key;
    const int N = 1000;
    for(int i =0;i<N;i++){
        key = "key"+std::to_string(i);
        s = db->Put(WriteOptions(),key,rand_str(1007));
    }

    std::cout<<"flush data in db"<<std::endl;
    s = db->Flush(FlushOptions());
    std::cout<<secondary_cache->num_inserts()<<",1u"<<std::endl;
    std::cout<<secondary_cache->num_lookups()<<",2u"<<std::endl;

    std::cout<<"compact data"<<std::endl;
    // s = db->CompactRange(rocksdb::CompactionOptions(),"a","z");
    Compact(db,"a","z");
    std::cout<<secondary_cache->num_inserts()<<",2u"<<std::endl;
    std::cout<<secondary_cache->num_lookups()<<",3u"<<std::endl;

    std::string value;
    std::chrono::steady_clock::time_point begin,end;
    const int times = 1000;
    int i =0;
    int read_times = 0;
    u_int64_t read_size = 0;
    begin = std::chrono::steady_clock::now();
    while(i<times){
        key = "key"+std::to_string(rand()%1000);
        _rate_limiter->Request(100, rocksdb::Env::IO_LOW, nullptr, RateLimiter::OpType::kRead);
        s = db->Get(ReadOptions(),key,&value);
        read_times++;
        read_size += value.size();
        end = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()>=1000){
            save_data(i,(double)read_times/i,(double)read_size/i,secondary_cache->num_inserts(),secondary_cache->num_lookups());
            begin = end;
            i++;
        }
    }
    std::string value;
    std::cout<<"get key0"<<std::endl;
    key = "key"+std::to_string(0);
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;
    std::cout<<secondary_cache->num_inserts()<<",2u"<<std::endl;
    std::cout<<secondary_cache->num_lookups()<<",3u"<<std::endl;

    key = "key"+std::to_string(5);
    std::cout<<"get key5"<<std::endl;
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;
    std::cout<<secondary_cache->num_inserts()<<",2u"<<std::endl;
    std::cout<<secondary_cache->num_lookups()<<",4u"<<std::endl;

    key = "key"+std::to_string(5);
    std::cout<<"get key5"<<std::endl;
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;
    std::cout<<secondary_cache->num_inserts()<<",2u"<<std::endl;
    std::cout<<secondary_cache->num_lookups()<<",4u"<<std::endl;

    key = "key"+std::to_string(0);
    std::cout<<"get key0"<<std::endl;
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;
    std::cout<<secondary_cache->num_inserts()<<",2u"<<std::endl;
    std::cout<<secondary_cache->num_lookups()<<",5u"<<std::endl;

    key = "key"+std::to_string(0);
    std::cout<<"get key0"<<std::endl;
    s = db->Get(ReadOptions(),key,&value);
    std::cout<<value.size()<<",1007"<<std::endl;
    std::cout<<secondary_cache->num_inserts()<<",2u"<<std::endl;
    std::cout<<secondary_cache->num_lookups()<<",5u"<<std::endl;

    delete db;
    return 0;
}