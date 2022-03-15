# - Find CacheLib
# Find the CacheLib library and includes
#
# Variables defined by this module:
#
#  CacheLib_FOUND            System has CacheLib, include and lib dirs found
#  CacheLib_INCLUDE_DIRS     The CacheLib includes directories.
#  CacheLib_LIBRARIES          The CacheLib library.


find_path(CacheLib_INCLUDE_DIRS 
  NAMES cachelib
)

find_library(CacheLib_ALLOCATOR NAMES libcachelib_allocator.a)
find_library(CacheLib_DATATYPE NAMES libcachelib_datatype.a)
find_library(CacheLib_SHM NAMES libcachelib_shm.a)
find_library(CacheLib_COMMON NAMES libcachelib_common.a)
find_library(CacheLib_NAVY NAMES libcachelib_navy.a)

if(CacheLib_INCLUDE_DIRS 
    AND CacheLib_ALLOCATOR
    AND CacheLib_DATATYPE
    AND CacheLib_SHM
    AND CacheLib_COMMON
    AND CacheLib_NAVY)
        set(CacheLib_LIBRARIES ${CacheLib_DATATYPE} ${CacheLib_COMMON} ${CacheLib_SHM} ${CacheLib_NAVY} ${CacheLib_ALLOCATOR} ${CacheLib_NAVY})
        mark_as_advanced(
            CacheLib_INCLUDE_DIRS
            CacheLib_LIBRARIES
        )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CacheLib DEFAULT_MSG CacheLib_LIBRARIES CacheLib_INCLUDE_DIRS)

if(CacheLib_FOUND AND NOT (TARGET CacheLib::cachelib))
  add_library (CacheLib::cachelib UNKNOWN IMPORTED)
  set_target_properties(CacheLib::cachelib
    PROPERTIES
    IMPORTED_LOCATION "${CacheLib_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${CacheLib_INCLUDE_DIRS}")
endif()


