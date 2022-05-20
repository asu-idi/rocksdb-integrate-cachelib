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

void save_data(int times, double throughout){
    std::ofstream outdata; 
    outdata.open("non_secondary_test_result.txt",std::ios::out|std::ios::app); // opens the file
    if( !outdata ) { // file couldn't be opened
        std::cerr << "Error: file could not be opened" << std::endl;
        exit(1);
    }
    outdata << times << "," << throughout << std::endl;
    outdata.close();
}

    std::vector<std::string> sst_file_name = {"/tmp/rocksdb_data/000037.sst", "/tmp/rocksdb_data/0000301.sst", "/tmp/rocksdb_data/0000459.sst", "/tmp/rocksdb_data/0000594.sst", "/tmp/rocksdb_data/0000699.sst", "/tmp/rocksdb_data/0000838.sst", "/tmp/rocksdb_data/0000934.sst", "/tmp/rocksdb_data/0001037.sst", "/tmp/rocksdb_data/0001143.sst", "/tmp/rocksdb_data/0001252.sst", "/tmp/rocksdb_data/0001359.sst", "/tmp/rocksdb_data/0001456.sst", "/tmp/rocksdb_data/0001562.sst", "/tmp/rocksdb_data/0001674.sst",
       "/tmp/rocksdb_data/0000053.sst", "/tmp/rocksdb_data/0000310.sst", "/tmp/rocksdb_data/0000476.sst", "/tmp/rocksdb_data/0000597.sst", "/tmp/rocksdb_data/0000709.sst", "/tmp/rocksdb_data/0000839.sst", "/tmp/rocksdb_data/0000942.sst", "/tmp/rocksdb_data/0001038.sst", "/tmp/rocksdb_data/0001148.sst", "/tmp/rocksdb_data/0001253.sst", "/tmp/rocksdb_data/0001360.sst", "/tmp/rocksdb_data/0001457.sst", "/tmp/rocksdb_data/0001568.sst", "/tmp/rocksdb_data/0001675.sst",
       "/tmp/rocksdb_data/0000060.sst", "/tmp/rocksdb_data/0000315.sst", "/tmp/rocksdb_data/0000477.sst", "/tmp/rocksdb_data/0000598.sst", "/tmp/rocksdb_data/0000710.sst", "/tmp/rocksdb_data/0000840.sst", "/tmp/rocksdb_data/0000943.sst", "/tmp/rocksdb_data/0001043.sst", "/tmp/rocksdb_data/0001149.sst", "/tmp/rocksdb_data/0001254.sst", "/tmp/rocksdb_data/0001361.sst", "/tmp/rocksdb_data/0001462.sst", "/tmp/rocksdb_data/0001569.sst", "/tmp/rocksdb_data/0001676.sst",
       "/tmp/rocksdb_data/0000068.sst", "/tmp/rocksdb_data/0000316.sst", "/tmp/rocksdb_data/0000478.sst", "/tmp/rocksdb_data/0000599.sst", "/tmp/rocksdb_data/0000711.sst", "/tmp/rocksdb_data/0000841.sst", "/tmp/rocksdb_data/0000944.sst", "/tmp/rocksdb_data/0001044.sst", "/tmp/rocksdb_data/0001150.sst", "/tmp/rocksdb_data/0001255.sst", "/tmp/rocksdb_data/0001366.sst", "/tmp/rocksdb_data/0001463.sst", "/tmp/rocksdb_data/0001570.sst", "/tmp/rocksdb_data/0001677.sst",
       "/tmp/rocksdb_data/0000069.sst", "/tmp/rocksdb_data/0000317.sst", "/tmp/rocksdb_data/0000479.sst", "/tmp/rocksdb_data/0000600.sst", "/tmp/rocksdb_data/0000712.sst", "/tmp/rocksdb_data/0000842.sst", "/tmp/rocksdb_data/0000945.sst", "/tmp/rocksdb_data/0001045.sst", "/tmp/rocksdb_data/0001151.sst", "/tmp/rocksdb_data/0001260.sst", "/tmp/rocksdb_data/0001367.sst", "/tmp/rocksdb_data/0001464.sst", "/tmp/rocksdb_data/0001571.sst", "/tmp/rocksdb_data/0001683.sst",
       "/tmp/rocksdb_data/0000070.sst", "/tmp/rocksdb_data/0000318.sst", "/tmp/rocksdb_data/0000486.sst", "/tmp/rocksdb_data/0000601.sst", "/tmp/rocksdb_data/0000713.sst", "/tmp/rocksdb_data/0000844.sst", "/tmp/rocksdb_data/0000949.sst", "/tmp/rocksdb_data/0001046.sst", "/tmp/rocksdb_data/0001157.sst", "/tmp/rocksdb_data/0001261.sst", "/tmp/rocksdb_data/0001368.sst", "/tmp/rocksdb_data/0001465.sst", "/tmp/rocksdb_data/0001572.sst", "/tmp/rocksdb_data/0001684.sst",
       "/tmp/rocksdb_data/0000156.sst", "/tmp/rocksdb_data/0000319.sst", "/tmp/rocksdb_data/0000487.sst", "/tmp/rocksdb_data/0000602.sst", "/tmp/rocksdb_data/0000714.sst", "/tmp/rocksdb_data/0000845.sst", "/tmp/rocksdb_data/0000950.sst", "/tmp/rocksdb_data/0001052.sst", "/tmp/rocksdb_data/0001158.sst", "/tmp/rocksdb_data/0001262.sst", "/tmp/rocksdb_data/0001369.sst", "/tmp/rocksdb_data/0001466.sst", "/tmp/rocksdb_data/0001577.sst", "/tmp/rocksdb_data/0001685.sst",
       "/tmp/rocksdb_data/0000167.sst", "/tmp/rocksdb_data/0000322.sst", "/tmp/rocksdb_data/0000488.sst", "/tmp/rocksdb_data/0000616.sst", "/tmp/rocksdb_data/0000716.sst", "/tmp/rocksdb_data/0000846.sst", "/tmp/rocksdb_data/0000951.sst", "/tmp/rocksdb_data/0001053.sst", "/tmp/rocksdb_data/0001159.sst", "/tmp/rocksdb_data/0001263.sst", "/tmp/rocksdb_data/0001375.sst", "/tmp/rocksdb_data/0001471.sst", "/tmp/rocksdb_data/0001578.sst", "/tmp/rocksdb_data/0001686.sst",
       "/tmp/rocksdb_data/0000190.sst", "/tmp/rocksdb_data/0000323.sst", "/tmp/rocksdb_data/0000489.sst", "/tmp/rocksdb_data/0000617.sst", "/tmp/rocksdb_data/0000717.sst", "/tmp/rocksdb_data/0000853.sst", "/tmp/rocksdb_data/0000952.sst", "/tmp/rocksdb_data/0001054.sst", "/tmp/rocksdb_data/0001160.sst", "/tmp/rocksdb_data/0001269.sst", "/tmp/rocksdb_data/0001376.sst", "/tmp/rocksdb_data/0001472.sst", "/tmp/rocksdb_data/0001579.sst", "/tmp/rocksdb_data/0001687.sst",
       "/tmp/rocksdb_data/0000191.sst", "/tmp/rocksdb_data/0000332.sst", "/tmp/rocksdb_data/0000490.sst", "/tmp/rocksdb_data/0000618.sst", "/tmp/rocksdb_data/0000725.sst", "/tmp/rocksdb_data/0000854.sst", "/tmp/rocksdb_data/0000957.sst", "/tmp/rocksdb_data/0001055.sst", "/tmp/rocksdb_data/0001161.sst", "/tmp/rocksdb_data/0001270.sst", "/tmp/rocksdb_data/0001377.sst", "/tmp/rocksdb_data/0001473.sst", "/tmp/rocksdb_data/0001580.sst", "/tmp/rocksdb_data/0001693.sst",
       "/tmp/rocksdb_data/0000192.sst", "/tmp/rocksdb_data/0000334.sst", "/tmp/rocksdb_data/0000492.sst", "/tmp/rocksdb_data/0000619.sst", "/tmp/rocksdb_data/0000726.sst", "/tmp/rocksdb_data/0000855.sst", "/tmp/rocksdb_data/0000958.sst", "/tmp/rocksdb_data/0001056.sst", "/tmp/rocksdb_data/0001166.sst", "/tmp/rocksdb_data/0001271.sst", "/tmp/rocksdb_data/0001378.sst", "/tmp/rocksdb_data/0001474.sst", "/tmp/rocksdb_data/0001585.sst", "/tmp/rocksdb_data/0001694.sst",
       "/tmp/rocksdb_data/0000193.sst", "/tmp/rocksdb_data/0000335.sst", "/tmp/rocksdb_data/0000493.sst", "/tmp/rocksdb_data/0000620.sst", "/tmp/rocksdb_data/0000727.sst", "/tmp/rocksdb_data/0000856.sst", "/tmp/rocksdb_data/0000959.sst", "/tmp/rocksdb_data/0001061.sst", "/tmp/rocksdb_data/0001167.sst", "/tmp/rocksdb_data/0001272.sst", "/tmp/rocksdb_data/0001379.sst", "/tmp/rocksdb_data/0001479.sst", "/tmp/rocksdb_data/0001586.sst", "/tmp/rocksdb_data/0001695.sst",
       "/tmp/rocksdb_data/0000194.sst", "/tmp/rocksdb_data/0000337.sst", "/tmp/rocksdb_data/0000494.sst", "/tmp/rocksdb_data/0000623.sst", "/tmp/rocksdb_data/0000741.sst", "/tmp/rocksdb_data/0000857.sst", "/tmp/rocksdb_data/0000960.sst", "/tmp/rocksdb_data/0001062.sst", "/tmp/rocksdb_data/0001168.sst", "/tmp/rocksdb_data/0001273.sst", "/tmp/rocksdb_data/0001384.sst", "/tmp/rocksdb_data/0001480.sst", "/tmp/rocksdb_data/0001587.sst", "/tmp/rocksdb_data/0001696.sst",
       "/tmp/rocksdb_data/0000197.sst", "/tmp/rocksdb_data/0000338.sst", "/tmp/rocksdb_data/0000495.sst", "/tmp/rocksdb_data/0000624.sst", "/tmp/rocksdb_data/0000742.sst", "/tmp/rocksdb_data/0000859.sst", "/tmp/rocksdb_data/0000966.sst", "/tmp/rocksdb_data/0001063.sst", "/tmp/rocksdb_data/0001169.sst", "/tmp/rocksdb_data/0001278.sst", "/tmp/rocksdb_data/0001385.sst", "/tmp/rocksdb_data/0001481.sst", "/tmp/rocksdb_data/0001588.sst", "/tmp/rocksdb_data/0001701.sst",
       "/tmp/rocksdb_data/0000198.sst", "/tmp/rocksdb_data/0000339.sst", "/tmp/rocksdb_data/0000496.sst", "/tmp/rocksdb_data/0000626.sst", "/tmp/rocksdb_data/0000743.sst", "/tmp/rocksdb_data/0000860.sst", "/tmp/rocksdb_data/0000967.sst", "/tmp/rocksdb_data/0001064.sst", "/tmp/rocksdb_data/0001175.sst", "/tmp/rocksdb_data/0001279.sst", "/tmp/rocksdb_data/0001386.sst", "/tmp/rocksdb_data/0001482.sst", "/tmp/rocksdb_data/0001594.sst", "/tmp/rocksdb_data/0001702.sst",
       "/tmp/rocksdb_data/0000199.sst", "/tmp/rocksdb_data/0000340.sst", "/tmp/rocksdb_data/0000497.sst", "/tmp/rocksdb_data/0000627.sst", "/tmp/rocksdb_data/0000744.sst", "/tmp/rocksdb_data/0000861.sst", "/tmp/rocksdb_data/0000968.sst", "/tmp/rocksdb_data/0001069.sst", "/tmp/rocksdb_data/0001176.sst", "/tmp/rocksdb_data/0001280.sst", "/tmp/rocksdb_data/0001387.sst", "/tmp/rocksdb_data/0001488.sst", "/tmp/rocksdb_data/0001595.sst", "/tmp/rocksdb_data/0001703.sst",
       "/tmp/rocksdb_data/0000205.sst", "/tmp/rocksdb_data/0000341.sst", "/tmp/rocksdb_data/0000505.sst", "/tmp/rocksdb_data/0000634.sst", "/tmp/rocksdb_data/0000745.sst", "/tmp/rocksdb_data/0000862.sst", "/tmp/rocksdb_data/0000969.sst", "/tmp/rocksdb_data/0001070.sst", "/tmp/rocksdb_data/0001177.sst", "/tmp/rocksdb_data/0001281.sst", "/tmp/rocksdb_data/0001393.sst", "/tmp/rocksdb_data/0001489.sst", "/tmp/rocksdb_data/0001596.sst", "/tmp/rocksdb_data/0001704.sst",
       "/tmp/rocksdb_data/0000214.sst", "/tmp/rocksdb_data/0000343.sst", "/tmp/rocksdb_data/0000510.sst", "/tmp/rocksdb_data/0000644.sst", "/tmp/rocksdb_data/0000746.sst", "/tmp/rocksdb_data/0000863.sst", "/tmp/rocksdb_data/0000970.sst", "/tmp/rocksdb_data/0001071.sst", "/tmp/rocksdb_data/0001178.sst", "/tmp/rocksdb_data/0001287.sst", "/tmp/rocksdb_data/0001394.sst", "/tmp/rocksdb_data/0001490.sst", "/tmp/rocksdb_data/0001597.sst", "/tmp/rocksdb_data/0001705.sst",
       "/tmp/rocksdb_data/0000215.sst", "/tmp/rocksdb_data/0000344.sst", "/tmp/rocksdb_data/0000511.sst", "/tmp/rocksdb_data/0000645.sst", "/tmp/rocksdb_data/0000749.sst", "/tmp/rocksdb_data/0000866.sst", "/tmp/rocksdb_data/0000974.sst", "/tmp/rocksdb_data/0001072.sst", "/tmp/rocksdb_data/0001179.sst", "/tmp/rocksdb_data/0001288.sst", "/tmp/rocksdb_data/0001395.sst", "/tmp/rocksdb_data/0001491.sst", "/tmp/rocksdb_data/0001598.sst", "/tmp/rocksdb_data/0001711.sst",
       "/tmp/rocksdb_data/0000216.sst", "/tmp/rocksdb_data/0000345.sst", "/tmp/rocksdb_data/0000512.sst", "/tmp/rocksdb_data/0000646.sst", "/tmp/rocksdb_data/0000750.sst", "/tmp/rocksdb_data/0000869.sst", "/tmp/rocksdb_data/0000975.sst", "/tmp/rocksdb_data/0001073.sst", "/tmp/rocksdb_data/0001184.sst", "/tmp/rocksdb_data/0001289.sst", "/tmp/rocksdb_data/0001396.sst", "/tmp/rocksdb_data/0001492.sst", "/tmp/rocksdb_data/0001603.sst", "/tmp/rocksdb_data/0001712.sst",
       "/tmp/rocksdb_data/0000217.sst", "/tmp/rocksdb_data/0000346.sst", "/tmp/rocksdb_data/0000513.sst", "/tmp/rocksdb_data/0000648.sst", "/tmp/rocksdb_data/0000751.sst", "/tmp/rocksdb_data/0000870.sst", "/tmp/rocksdb_data/0000976.sst", "/tmp/rocksdb_data/0001078.sst", "/tmp/rocksdb_data/0001185.sst", "/tmp/rocksdb_data/0001290.sst", "/tmp/rocksdb_data/0001397.sst", "/tmp/rocksdb_data/0001497.sst", "/tmp/rocksdb_data/0001604.sst", "/tmp/rocksdb_data/0001713.sst",
       "/tmp/rocksdb_data/0000218.sst", "/tmp/rocksdb_data/0000353.sst", "/tmp/rocksdb_data/0000514.sst", "/tmp/rocksdb_data/0000651.sst", "/tmp/rocksdb_data/0000752.sst", "/tmp/rocksdb_data/0000871.sst", "/tmp/rocksdb_data/0000977.sst", "/tmp/rocksdb_data/0001079.sst", "/tmp/rocksdb_data/0001186.sst", "/tmp/rocksdb_data/0001291.sst", "/tmp/rocksdb_data/0001402.sst", "/tmp/rocksdb_data/0001498.sst", "/tmp/rocksdb_data/0001605.sst", "/tmp/rocksdb_data/0001714.sst",
       "/tmp/rocksdb_data/0000221.sst", "/tmp/rocksdb_data/0000354.sst", "/tmp/rocksdb_data/0000517.sst", "/tmp/rocksdb_data/0000652.sst", "/tmp/rocksdb_data/0000753.sst", "/tmp/rocksdb_data/0000872.sst", "/tmp/rocksdb_data/0000978.sst", "/tmp/rocksdb_data/0001080.sst", "/tmp/rocksdb_data/0001187.sst", "/tmp/rocksdb_data/0001296.sst", "/tmp/rocksdb_data/0001403.sst", "/tmp/rocksdb_data/0001499.sst", "/tmp/rocksdb_data/0001606.sst", "/tmp/rocksdb_data/0001719.sst",
       "/tmp/rocksdb_data/0000222.sst", "/tmp/rocksdb_data/0000355.sst", "/tmp/rocksdb_data/0000531.sst", "/tmp/rocksdb_data/0000653.sst", "/tmp/rocksdb_data/0000754.sst", "/tmp/rocksdb_data/0000873.sst", "/tmp/rocksdb_data/0000983.sst", "/tmp/rocksdb_data/0001081.sst", "/tmp/rocksdb_data/0001193.sst", "/tmp/rocksdb_data/0001297.sst", "/tmp/rocksdb_data/0001404.sst", "/tmp/rocksdb_data/0001500.sst", "/tmp/rocksdb_data/0001612.sst", "/tmp/rocksdb_data/0001720.sst",
       "/tmp/rocksdb_data/0000223.sst", "/tmp/rocksdb_data/0000356.sst", "/tmp/rocksdb_data/0000532.sst", "/tmp/rocksdb_data/0000654.sst", "/tmp/rocksdb_data/0000755.sst", "/tmp/rocksdb_data/0000874.sst", "/tmp/rocksdb_data/0000984.sst", "/tmp/rocksdb_data/0001086.sst", "/tmp/rocksdb_data/0001194.sst", "/tmp/rocksdb_data/0001298.sst", "/tmp/rocksdb_data/0001405.sst", "/tmp/rocksdb_data/0001506.sst", "/tmp/rocksdb_data/0001613.sst", "/tmp/rocksdb_data/0001721.sst",
       "/tmp/rocksdb_data/0000230.sst", "/tmp/rocksdb_data/0000357.sst", "/tmp/rocksdb_data/0000533.sst", "/tmp/rocksdb_data/0000655.sst", "/tmp/rocksdb_data/0000756.sst", "/tmp/rocksdb_data/0000875.sst", "/tmp/rocksdb_data/0000985.sst", "/tmp/rocksdb_data/0001087.sst", "/tmp/rocksdb_data/0001195.sst", "/tmp/rocksdb_data/0001299.sst", "/tmp/rocksdb_data/0001407.sst", "/tmp/rocksdb_data/0001507.sst", "/tmp/rocksdb_data/0001614.sst", "/tmp/rocksdb_data/0001722.sst",
       "/tmp/rocksdb_data/0000238.sst", "/tmp/rocksdb_data/0000358.sst", "/tmp/rocksdb_data/0000534.sst", "/tmp/rocksdb_data/0000656.sst", "/tmp/rocksdb_data/0000758.sst", "/tmp/rocksdb_data/0000877.sst", "/tmp/rocksdb_data/0000986.sst", "/tmp/rocksdb_data/0001088.sst", "/tmp/rocksdb_data/0001196.sst", "/tmp/rocksdb_data/0001304.sst", "/tmp/rocksdb_data/0001411.sst", "/tmp/rocksdb_data/0001508.sst", "/tmp/rocksdb_data/0001615.sst", "/tmp/rocksdb_data/0001723.sst",
       "/tmp/rocksdb_data/0000239.sst", "/tmp/rocksdb_data/0000360.sst", "/tmp/rocksdb_data/0000535.sst", "/tmp/rocksdb_data/0000657.sst", "/tmp/rocksdb_data/0000759.sst", "/tmp/rocksdb_data/0000878.sst", "/tmp/rocksdb_data/0000987.sst", "/tmp/rocksdb_data/0001089.sst", "/tmp/rocksdb_data/0001201.sst", "/tmp/rocksdb_data/0001305.sst", "/tmp/rocksdb_data/0001412.sst", "/tmp/rocksdb_data/0001509.sst", "/tmp/rocksdb_data/0001616.sst", "/tmp/rocksdb_data/0001728.sst",
       "/tmp/rocksdb_data/0000240.sst", "/tmp/rocksdb_data/0000361.sst", "/tmp/rocksdb_data/0000538.sst", "/tmp/rocksdb_data/0000659.sst", "/tmp/rocksdb_data/0000760.sst", "/tmp/rocksdb_data/0000879.sst", "/tmp/rocksdb_data/0000988.sst", "/tmp/rocksdb_data/0001095.sst", "/tmp/rocksdb_data/0001202.sst", "/tmp/rocksdb_data/0001306.sst", "/tmp/rocksdb_data/0001413.sst", "/tmp/rocksdb_data/0001510.sst", "/tmp/rocksdb_data/0001621.sst", "/tmp/rocksdb_data/0001729.sst",
       "/tmp/rocksdb_data/0000241.sst", "/tmp/rocksdb_data/0000368.sst", "/tmp/rocksdb_data/0000539.sst", "/tmp/rocksdb_data/0000660.sst", "/tmp/rocksdb_data/0000761.sst", "/tmp/rocksdb_data/0000880.sst", "/tmp/rocksdb_data/0000989.sst", "/tmp/rocksdb_data/0001096.sst", "/tmp/rocksdb_data/0001203.sst", "/tmp/rocksdb_data/0001307.sst", "/tmp/rocksdb_data/0001414.sst", "/tmp/rocksdb_data/0001515.sst", "/tmp/rocksdb_data/0001622.sst", "/tmp/rocksdb_data/0001730.sst",
       "/tmp/rocksdb_data/0000242.sst", "/tmp/rocksdb_data/0000369.sst", "/tmp/rocksdb_data/0000540.sst", "/tmp/rocksdb_data/0000661.sst", "/tmp/rocksdb_data/0000762.sst", "/tmp/rocksdb_data/0000881.sst", "/tmp/rocksdb_data/0000993.sst", "/tmp/rocksdb_data/0001097.sst", "/tmp/rocksdb_data/0001204.sst", "/tmp/rocksdb_data/0001313.sst", "/tmp/rocksdb_data/0001415.sst", "/tmp/rocksdb_data/0001516.sst", "/tmp/rocksdb_data/0001623.sst", "/tmp/rocksdb_data/0001731.sst",
       "/tmp/rocksdb_data/0000245.sst", "/tmp/rocksdb_data/0000370.sst", "/tmp/rocksdb_data/0000541.sst", "/tmp/rocksdb_data/0000662.sst", "/tmp/rocksdb_data/0000763.sst", "/tmp/rocksdb_data/0000882.sst", "/tmp/rocksdb_data/0000994.sst", "/tmp/rocksdb_data/0001098.sst", "/tmp/rocksdb_data/0001205.sst", "/tmp/rocksdb_data/0001314.sst", "/tmp/rocksdb_data/0001418.sst", "/tmp/rocksdb_data/0001517.sst", "/tmp/rocksdb_data/0001624.sst", "/tmp/rocksdb_data/0001736.sst",
       "/tmp/rocksdb_data/0000246.sst", "/tmp/rocksdb_data/0000371.sst", "/tmp/rocksdb_data/0000542.sst", "/tmp/rocksdb_data/0000663.sst", "/tmp/rocksdb_data/0000764.sst", "/tmp/rocksdb_data/0000883.sst", "/tmp/rocksdb_data/0000995.sst", "/tmp/rocksdb_data/0001099.sst", "/tmp/rocksdb_data/0001206.sst", "/tmp/rocksdb_data/0001315.sst", "/tmp/rocksdb_data/0001419.sst", "/tmp/rocksdb_data/0001518.sst", "/tmp/rocksdb_data/0001630.sst", "/tmp/rocksdb_data/0001737.sst",
       "/tmp/rocksdb_data/0000247.sst", "/tmp/rocksdb_data/0000372.sst", "/tmp/rocksdb_data/0000546.sst", "/tmp/rocksdb_data/0000664.sst", "/tmp/rocksdb_data/0000765.sst", "/tmp/rocksdb_data/0000884.sst", "/tmp/rocksdb_data/0000996.sst", "/tmp/rocksdb_data/0001104.sst", "/tmp/rocksdb_data/0001211.sst", "/tmp/rocksdb_data/0001316.sst", "/tmp/rocksdb_data/0001420.sst", "/tmp/rocksdb_data/0001523.sst", "/tmp/rocksdb_data/0001631.sst", "/tmp/rocksdb_data/0001738.sst",
       "/tmp/rocksdb_data/0000251.sst", "/tmp/rocksdb_data/0000373.sst", "/tmp/rocksdb_data/0000547.sst", "/tmp/rocksdb_data/0000665.sst", "/tmp/rocksdb_data/0000771.sst", "/tmp/rocksdb_data/0000885.sst", "/tmp/rocksdb_data/0001001.sst", "/tmp/rocksdb_data/0001105.sst", "/tmp/rocksdb_data/0001212.sst", "/tmp/rocksdb_data/0001317.sst", "/tmp/rocksdb_data/0001421.sst", "/tmp/rocksdb_data/0001524.sst", "/tmp/rocksdb_data/0001632.sst", "/tmp/rocksdb_data/0001739.sst",
       "/tmp/rocksdb_data/0000258.sst", "/tmp/rocksdb_data/0000375.sst", "/tmp/rocksdb_data/0000548.sst", "/tmp/rocksdb_data/0000666.sst", "/tmp/rocksdb_data/0000793.sst", "/tmp/rocksdb_data/0000888.sst", "/tmp/rocksdb_data/0001002.sst", "/tmp/rocksdb_data/0001106.sst", "/tmp/rocksdb_data/0001213.sst", "/tmp/rocksdb_data/0001322.sst", "/tmp/rocksdb_data/0001422.sst", "/tmp/rocksdb_data/0001525.sst", "/tmp/rocksdb_data/0001633.sst", "/tmp/rocksdb_data/0001744.sst",
       "/tmp/rocksdb_data/0000265.sst", "/tmp/rocksdb_data/0000376.sst", "/tmp/rocksdb_data/0000549.sst", "/tmp/rocksdb_data/0000669.sst", "/tmp/rocksdb_data/0000794.sst", "/tmp/rocksdb_data/0000889.sst", "/tmp/rocksdb_data/0001003.sst", "/tmp/rocksdb_data/0001107.sst", "/tmp/rocksdb_data/0001218.sst", "/tmp/rocksdb_data/0001323.sst", "/tmp/rocksdb_data/0001423.sst", "/tmp/rocksdb_data/0001526.sst", "/tmp/rocksdb_data/0001638.sst", "/tmp/rocksdb_data/0001745.sst",
       "/tmp/rocksdb_data/0000266.sst", "/tmp/rocksdb_data/0000383.sst", "/tmp/rocksdb_data/0000550.sst", "/tmp/rocksdb_data/0000670.sst", "/tmp/rocksdb_data/0000795.sst", "/tmp/rocksdb_data/0000901.sst", "/tmp/rocksdb_data/0001004.sst", "/tmp/rocksdb_data/0001113.sst", "/tmp/rocksdb_data/0001219.sst", "/tmp/rocksdb_data/0001324.sst", "/tmp/rocksdb_data/0001424.sst", "/tmp/rocksdb_data/0001532.sst", "/tmp/rocksdb_data/0001639.sst", "/tmp/rocksdb_data/0001746.sst",
       "/tmp/rocksdb_data/0000267.sst", "/tmp/rocksdb_data/0000384.sst", "/tmp/rocksdb_data/0000551.sst", "/tmp/rocksdb_data/0000671.sst", "/tmp/rocksdb_data/0000796.sst", "/tmp/rocksdb_data/0000907.sst", "/tmp/rocksdb_data/0001009.sst", "/tmp/rocksdb_data/0001114.sst", "/tmp/rocksdb_data/0001220.sst", "/tmp/rocksdb_data/0001325.sst", "/tmp/rocksdb_data/0001426.sst", "/tmp/rocksdb_data/0001533.sst", "/tmp/rocksdb_data/0001640.sst", "/tmp/rocksdb_data/0001747.sst",
       "/tmp/rocksdb_data/0000268.sst", "/tmp/rocksdb_data/0000385.sst", "/tmp/rocksdb_data/0000553.sst", "/tmp/rocksdb_data/0000675.sst", "/tmp/rocksdb_data/0000797.sst", "/tmp/rocksdb_data/0000910.sst", "/tmp/rocksdb_data/0001010.sst", "/tmp/rocksdb_data/0001115.sst", "/tmp/rocksdb_data/0001221.sst", "/tmp/rocksdb_data/0001331.sst", "/tmp/rocksdb_data/0001429.sst", "/tmp/rocksdb_data/0001534.sst", "/tmp/rocksdb_data/0001641.sst", "/tmp/rocksdb_data/0001748.sst",
       "/tmp/rocksdb_data/0000269.sst", "/tmp/rocksdb_data/0000386.sst", "/tmp/rocksdb_data/0000554.sst", "/tmp/rocksdb_data/0000676.sst", "/tmp/rocksdb_data/0000800.sst", "/tmp/rocksdb_data/0000911.sst", "/tmp/rocksdb_data/0001011.sst", "/tmp/rocksdb_data/0001116.sst", "/tmp/rocksdb_data/0001226.sst", "/tmp/rocksdb_data/0001332.sst", "/tmp/rocksdb_data/0001430.sst", "/tmp/rocksdb_data/0001535.sst", "/tmp/rocksdb_data/0001642.sst", "/tmp/rocksdb_data/0001753.sst",
       "/tmp/rocksdb_data/0000270.sst", "/tmp/rocksdb_data/0000387.sst", "/tmp/rocksdb_data/0000555.sst", "/tmp/rocksdb_data/0000677.sst", "/tmp/rocksdb_data/0000801.sst", "/tmp/rocksdb_data/0000912.sst", "/tmp/rocksdb_data/0001012.sst", "/tmp/rocksdb_data/0001117.sst", "/tmp/rocksdb_data/0001227.sst", "/tmp/rocksdb_data/0001333.sst", "/tmp/rocksdb_data/0001431.sst", "/tmp/rocksdb_data/0001536.sst", "/tmp/rocksdb_data/0001648.sst", "/tmp/rocksdb_data/0001754.sst",
       "/tmp/rocksdb_data/0000273.sst", "/tmp/rocksdb_data/0000388.sst", "/tmp/rocksdb_data/0000556.sst", "/tmp/rocksdb_data/0000678.sst", "/tmp/rocksdb_data/0000802.sst", "/tmp/rocksdb_data/0000913.sst", "/tmp/rocksdb_data/0001013.sst", "/tmp/rocksdb_data/0001122.sst", "/tmp/rocksdb_data/0001228.sst", "/tmp/rocksdb_data/0001334.sst", "/tmp/rocksdb_data/0001432.sst", "/tmp/rocksdb_data/0001541.sst", "/tmp/rocksdb_data/0001649.sst", "/tmp/rocksdb_data/0001755.sst",
       "/tmp/rocksdb_data/0000274.sst", "/tmp/rocksdb_data/0000405.sst", "/tmp/rocksdb_data/0000557.sst", "/tmp/rocksdb_data/0000679.sst", "/tmp/rocksdb_data/0000805.sst", "/tmp/rocksdb_data/0000914.sst", "/tmp/rocksdb_data/0001018.sst", "/tmp/rocksdb_data/0001123.sst", "/tmp/rocksdb_data/0001229.sst", "/tmp/rocksdb_data/0001335.sst", "/tmp/rocksdb_data/0001437.sst", "/tmp/rocksdb_data/0001542.sst", "/tmp/rocksdb_data/0001650.sst", "/tmp/rocksdb_data/0001756.sst",
       "/tmp/rocksdb_data/0000275.sst", "/tmp/rocksdb_data/0000406.sst", "/tmp/rocksdb_data/0000558.sst", "/tmp/rocksdb_data/0000680.sst", "/tmp/rocksdb_data/0000806.sst", "/tmp/rocksdb_data/0000915.sst", "/tmp/rocksdb_data/0001019.sst", "/tmp/rocksdb_data/0001124.sst", "/tmp/rocksdb_data/0001230.sst", "/tmp/rocksdb_data/0001340.sst", "/tmp/rocksdb_data/0001438.sst", "/tmp/rocksdb_data/0001543.sst", "/tmp/rocksdb_data/0001651.sst", "/tmp/rocksdb_data/0001757.sst",
       "/tmp/rocksdb_data/0000283.sst", "/tmp/rocksdb_data/0000413.sst", "/tmp/rocksdb_data/0000559.sst", "/tmp/rocksdb_data/0000682.sst", "/tmp/rocksdb_data/0000808.sst", "/tmp/rocksdb_data/0000916.sst", "/tmp/rocksdb_data/0001020.sst", "/tmp/rocksdb_data/0001125.sst", "/tmp/rocksdb_data/0001235.sst", "/tmp/rocksdb_data/0001341.sst", "/tmp/rocksdb_data/0001439.sst", "/tmp/rocksdb_data/0001544.sst", "/tmp/rocksdb_data/0001656.sst", "/tmp/rocksdb_data/0000290.sst",  
       "/tmp/rocksdb_data/0000414.sst", "/tmp/rocksdb_data/0000560.sst", "/tmp/rocksdb_data/0000689.sst", "/tmp/rocksdb_data/0000809.sst", "/tmp/rocksdb_data/0000918.sst", "/tmp/rocksdb_data/0001021.sst", "/tmp/rocksdb_data/0001131.sst", "/tmp/rocksdb_data/0001236.sst", "/tmp/rocksdb_data/0001342.sst", "/tmp/rocksdb_data/0001440.sst", "/tmp/rocksdb_data/0001550.sst", "/tmp/rocksdb_data/0001657.sst", "/tmp/rocksdb_data/0001760.sst",
       "/tmp/rocksdb_data/0000291.sst", "/tmp/rocksdb_data/0000415.sst", "/tmp/rocksdb_data/0000563.sst", "/tmp/rocksdb_data/0000692.sst", "/tmp/rocksdb_data/0000810.sst", "/tmp/rocksdb_data/0000924.sst", "/tmp/rocksdb_data/0001026.sst", "/tmp/rocksdb_data/0001132.sst", "/tmp/rocksdb_data/0001237.sst", "/tmp/rocksdb_data/0001343.sst", "/tmp/rocksdb_data/0001445.sst", "/tmp/rocksdb_data/0001551.sst", "/tmp/rocksdb_data/0001658.sst",  
       "/tmp/rocksdb_data/0000292.sst", "/tmp/rocksdb_data/0000416.sst", "/tmp/rocksdb_data/0000566.sst", "/tmp/rocksdb_data/0000693.sst", "/tmp/rocksdb_data/0000811.sst", "/tmp/rocksdb_data/0000925.sst", "/tmp/rocksdb_data/0001027.sst", "/tmp/rocksdb_data/0001133.sst", "/tmp/rocksdb_data/0001238.sst", "/tmp/rocksdb_data/0001349.sst", "/tmp/rocksdb_data/0001446.sst", "/tmp/rocksdb_data/0001552.sst", "/tmp/rocksdb_data/0001659.sst",  
       "/tmp/rocksdb_data/0000293.sst", "/tmp/rocksdb_data/0000417.sst", "/tmp/rocksdb_data/0000588.sst", "/tmp/rocksdb_data/0000694.sst", "/tmp/rocksdb_data/0000812.sst", "/tmp/rocksdb_data/0000926.sst", "/tmp/rocksdb_data/0001028.sst", "/tmp/rocksdb_data/0001134.sst", "/tmp/rocksdb_data/0001243.sst", "/tmp/rocksdb_data/0001350.sst", "/tmp/rocksdb_data/0001447.sst", "/tmp/rocksdb_data/0001553.sst", "/tmp/rocksdb_data/0001660.sst",  
       "/tmp/rocksdb_data/0000294.sst", "/tmp/rocksdb_data/0000418.sst", "/tmp/rocksdb_data/0000590.sst", "/tmp/rocksdb_data/0000695.sst", "/tmp/rocksdb_data/0000823.sst", "/tmp/rocksdb_data/0000927.sst", "/tmp/rocksdb_data/0001029.sst", "/tmp/rocksdb_data/0001135.sst", "/tmp/rocksdb_data/0001244.sst", "/tmp/rocksdb_data/0001351.sst", "/tmp/rocksdb_data/0001448.sst", "/tmp/rocksdb_data/0001554.sst", "/tmp/rocksdb_data/0001665.sst",  
       "/tmp/rocksdb_data/0000297.sst", "/tmp/rocksdb_data/0000420.sst", "/tmp/rocksdb_data/0000591.sst", "/tmp/rocksdb_data/0000696.sst", "/tmp/rocksdb_data/0000833.sst", "/tmp/rocksdb_data/0000931.sst", "/tmp/rocksdb_data/0001030.sst", "/tmp/rocksdb_data/0001140.sst", "/tmp/rocksdb_data/0001245.sst", "/tmp/rocksdb_data/0001352.sst", "/tmp/rocksdb_data/0001449.sst", "/tmp/rocksdb_data/0001559.sst", "/tmp/rocksdb_data/0001666.sst",  
       "/tmp/rocksdb_data/0000298.sst", "/tmp/rocksdb_data/0000421.sst", "/tmp/rocksdb_data/0000592.sst", "/tmp/rocksdb_data/0000697.sst", "/tmp/rocksdb_data/0000836.sst", "/tmp/rocksdb_data/0000932.sst", "/tmp/rocksdb_data/0001035.sst", "/tmp/rocksdb_data/0001141.sst", "/tmp/rocksdb_data/0001246.sst", "/tmp/rocksdb_data/0001353.sst", "/tmp/rocksdb_data/0001454.sst", "/tmp/rocksdb_data/0001560.sst", "/tmp/rocksdb_data/0001667.sst",  
       "/tmp/rocksdb_data/0000299.sst", "/tmp/rocksdb_data/0000443.sst", "/tmp/rocksdb_data/0000593.sst", "/tmp/rocksdb_data/0000698.sst", "/tmp/rocksdb_data/0000837.sst", "/tmp/rocksdb_data/0000933.sst", "/tmp/rocksdb_data/0001036.sst", "/tmp/rocksdb_data/0001142.sst", "/tmp/rocksdb_data/0001247.sst", "/tmp/rocksdb_data/0001358.sst", "/tmp/rocksdb_data/0001455.sst", "/tmp/rocksdb_data/0001561.sst", "/tmp/rocksdb_data/0001668.sst"}

int main(){
    DB* db;
    Options options;
    //set rate limiter to limit IO rate to 100kB/s to simulate the cloud storage use cases
    RateLimiter* _rate_limiter = NewGenericRateLimiter(100*1000, 100 * 1000, 10, RateLimiter::Mode::kReadsOnly);

    BlockBasedTableOptions table_options;
    LRUCacheOptions opts(1024 * 1024, 0, false, 0.5, nullptr, rocksdb::kDefaultToAdaptiveMutex, rocksdb::kDontChargeCacheMetadata);
    std::shared_ptr<Cache> _cache = NewLRUCache(opts);
    table_options.block_cache = _cache;
    table_options.block_size = 10 * 1024;

    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    options.create_if_missing = true;
    options.rate_limiter.reset(_rate_limiter);

    Status s = DB::Open(options, kDBPath, &db);
    rocksdb::IngestExternalFileOptions ifo; 
    s = db->IngestExternalFile(sst_file_name, ifo);
    std::string key;
    std::chrono::steady_clock::time_point begin,end;
    std::cout << "GET" <<std::endl;
    std::string value;
    const int times = 1000;
    int i =0;
    int read_times = 0;
    size_t read_size = 0;
    begin = std::chrono::steady_clock::now();
    while(i<times){
        key = "key"+std::to_string(rand()%10000000);
        _rate_limiter->Request(100, rocksdb::Env::IO_LOW, nullptr, RateLimiter::OpType::kRead);
        s = db->Get(ReadOptions(), key, &value);
        read_times++;
        read_size += value.size();
        end = std::chrono::steady_clock::now();
        if(end - begin >= 60s){
            save_data(i,read_size/i);
            begin = end;
            i++;
        }
    }
    delete db;
    return 0;
}