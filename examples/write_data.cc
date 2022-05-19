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
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteBatch;
using ROCKSDB_NAMESPACE::WriteOptions;
using ROCKSDB_NAMESPACE::CompactionFilter;
using ROCKSDB_NAMESPACE::ConfigOptions;
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::FlushOptions;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_data";
#else
std::string kDBPath = "/tmp/rocksdb_data";
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

int main(){
    DB* db;
    Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;
    Status s = DB::Open(options, kDBPath, &db);
    std::string key;
    int size;
    long long int storage =0;
    // const unsigned long int max_size = 100 * 1024 * 1024 * 1024;
    int i = 1;
    while(i<10000000){
        key = "key"+std::to_string(i++);
        size = rand() % 10000 + 1;
        s = db->Put(WriteOptions(),key,rand_str(size));
        storage+=size;
    }
    s = db->Flush(FlushOptions());
    std::cout<< storage % 1000000 <<std::endl;
    delete db;
    return 0;
}