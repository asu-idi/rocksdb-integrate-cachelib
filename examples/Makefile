include ../make_config.mk

ifndef DISABLE_JEMALLOC
	ifdef JEMALLOC
		PLATFORM_CXXFLAGS += -DROCKSDB_JEMALLOC -DJEMALLOC_NO_DEMANGLE
	endif
	EXEC_LDFLAGS := $(JEMALLOC_LIB) $(EXEC_LDFLAGS) -lpthread
	PLATFORM_CXXFLAGS += $(JEMALLOC_INCLUDE)
endif

ifneq ($(USE_RTTI), 1)
	CXXFLAGS += -fno-rtti
endif

CFLAGS += -Wstrict-prototypes

.PHONY: clean 

all: simple_example column_families_example compact_files_example c_simple_example optimistic_transaction_example transaction_example compaction_filter_example options_file_example


non_sc: non_sc_cache.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

sc_cache: sc_cache.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

big_sc_test: big_sc_test.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

small_sc_test: small_sc_test.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

write_data: write_data.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

secondary_test: secondary_test.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

non_secondary_test: non_secondary_test.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

read_test: read_test.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

simple_example:  simple_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

column_families_example:  column_families_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

compaction_filter_example:  compaction_filter_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

compact_files_example:  compact_files_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@ -I../include

c_simple_example:  c_simple_example.o
	$(CXX) $@.o -o$@ ../build/librocksdb.a $(PLATFORM_LDFLAGS) $(EXEC_LDFLAGS)

optimistic_transaction_example:  optimistic_transaction_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

transaction_example:  transaction_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

options_file_example:  options_file_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

multi_processes_example:  multi_processes_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../build/librocksdb.a -I../include -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)

clean:
	rm -rf ./simple_example ./column_families_example ./compact_files_example ./compaction_filter_example ./c_simple_example c_simple_example.o ./optimistic_transaction_example ./transaction_example ./options_file_example ./multi_processes_example ./read_test

librocksdb:
	cd .. && $(MAKE) static_lib
