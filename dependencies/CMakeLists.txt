include(CMakeDependentOption)

option(SIMDJSON_ALLOW_DOWNLOADS
        "Allow dependencies to be downloaded during configure time"
        ON)

cmake_dependent_option(SIMDJSON_COMPETITION "Compile competitive benchmarks" ON
        SIMDJSON_ALLOW_DOWNLOADS OFF)
cmake_dependent_option(SIMDJSON_GOOGLE_BENCHMARKS "compile the Google Benchmark benchmarks" ON
        "SIMDJSON_ALLOW_DOWNLOADS OR MINGW" OFF)

if(SIMDJSON_GOOGLE_BENCHMARKS)
  CPMAddPackage(
        NAME google_benchmarks
        URL https://github.com/google/benchmark/archive/refs/tags/v1.9.5.zip
        OPTIONS
        "BENCHMARK_ENABLE_TESTING OFF"
        "BENCHMARK_ENABLE_INSTALL OFF"
        "BENCHMARK_ENABLE_WERROR OFF"
  )
endif()

# The counters library (header-only) is used by most benchmarks.
cmake_dependent_option(SIMDJSON_COUNTERS
        "Download the counters library (used by most benchmarks)" ON
        SIMDJSON_ALLOW_DOWNLOADS OFF)

# CPM_counters_SOURCE points CPM at a local copy: no download is needed.
if(SIMDJSON_COUNTERS OR CPM_counters_SOURCE)
  CPMAddPackage(
        NAME counters
        URL https://github.com/lemire/counters/archive/refs/tags/v3.1.0.zip
        OPTIONS
        "COUNTERS_BUILD_TESTS OFF"
        "COUNTERS_INSTALL OFF"
  )
else()
  # Offline builds (e.g., distribution packaging) can use an installed copy.
  # The imported target is only visible in this directory, so wrap it in a
  # regular (global) target that the benchmark directories can see.
  find_package(Counters CONFIG QUIET)
  if(TARGET Counters::counters)
    add_library(counters INTERFACE)
    target_link_libraries(counters INTERFACE Counters::counters)
  endif()
endif()

set(SIMDJSON_DATA_URL
        https://github.com/simdjson/simdjson-data/archive/351949906abde446f0314bf79606fb5d884f5be7.zip)
CPMAddPackage(
        NAME simdjson-data
        URL ${SIMDJSON_DATA_URL}
)
# The tests and benchmarks need the data. When it is missing (e.g., an offline
# build with FETCHCONTENT_FULLY_DISCONNECTED=ON, where FetchContent only warns),
# stop here rather than fail later with obscure '-ltest-data' link errors.
if(NOT TARGET test-data)
  message(FATAL_ERROR
    "The simdjson-data test files are missing, and the developer build "
    "(SIMDJSON_DEVELOPER_MODE) needs them for its tests and benchmarks. "
    "Without network access, extract\n"
    "  ${SIMDJSON_DATA_URL}\n"
    "and pass -DCPM_simdjson-data_SOURCE=<extracted directory>. "
    "See 'Offline builds' in HACKING.md.")
endif()

option(SIMDJSON_USE_BOOST_JSON "Try to include BOOST_JSON, this may break your binaries under some systems." OFF)
# This prevents variables declared with set() from unnecessarily escaping and
# should not be called more than once
function(competition_scope_)
  # boost json in standalone mode requires C++17 string_view
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles([[
#include <string_view>

#if __cpp_lib_string_view < 201606
#  error no string view support
#endif

int main() {}
]] SIMDJSON_FOUND_STRING_VIEW)
  if(SIMDJSON_FOUND_STRING_VIEW AND SIMDJSON_USE_BOOST_JSON)
    CPMAddPackage(
        NAME boostjson
        URL https://github.com/boostorg/json/archive/ee8d72d8502b409b5561200299cad30ccdb91415.zip
    )
    add_library(boostjson STATIC "${boostjson_SOURCE_DIR}/src/src.cpp")
    target_compile_definitions(boostjson PUBLIC BOOST_JSON_STANDALONE)
    target_include_directories(boostjson SYSTEM PUBLIC
            "${boostjson_SOURCE_DIR}/include")
    target_compile_definitions(boostjson INTERFACE SIMDJSON_COMPETITION_BOOSTJSON)
  endif()
  CPMAddPackage(
      NAME nlohmann_json
      URL https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.zip
  )

  set_property(TARGET nlohmann_json APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS SIMDJSON_COMPETITION_NLOHMANN_JSON)


  CPMAddPackage(
      NAME rapidjson
      URL https://github.com/Tencent/rapidjson/archive/805d7ed5dfe97a39b8b0816fd5eeed8731dc4936.zip
      DOWNLOAD_ONLY YES
  )
  add_library(rapidjson INTERFACE)
  target_compile_definitions(rapidjson INTERFACE RAPIDJSON_HAS_STDSTRING)
  include (TestBigEndian)
  TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
  if(IS_BIG_ENDIAN)
  target_compile_definitions(rapidjson INTERFACE RAPIDJSON_ENDIAN=1)
  else()
  target_compile_definitions(rapidjson INTERFACE RAPIDJSON_ENDIAN=0)
  endif()
  target_compile_definitions(rapidjson INTERFACE RAPIDJSON_HAS_STDSTRING)
  target_include_directories(rapidjson SYSTEM INTERFACE
          "${rapidjson_SOURCE_DIR}/include")
  if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
    message(STATUS "Disabling rapidjson")
  else()
    target_compile_definitions(rapidjson INTERFACE SIMDJSON_COMPETITION_RAPIDJSON)
  endif()

  CPMAddPackage(
      NAME sajson
      URL https://github.com/chadaustin/sajson/archive/2dcfd350586375f9910f74821d4f07d67ae455ba.zip
      DOWNLOAD_ONLY YES
  )
  add_library(sajson INTERFACE)
  target_compile_definitions(sajson INTERFACE SAJSON_UNSORTED_OBJECT_KEYS)
  target_include_directories(sajson SYSTEM INTERFACE
          "${sajson_SOURCE_DIR}/include")
  target_compile_definitions(sajson INTERFACE SIMDJSON_COMPETITION_SAJSON)

  CPMAddPackage(
      NAME yyjson
      URL https://github.com/ibireme/yyjson/archive/c3856514de0a67d7b66939bf3ed491a2d6e61277.zip
      DOWNLOAD_ONLY YES
  )
  add_library(yyjson STATIC "${yyjson_SOURCE_DIR}/src/yyjson.c")
  target_include_directories(yyjson SYSTEM PUBLIC "${yyjson_SOURCE_DIR}/src")
  target_compile_definitions(yyjson INTERFACE SIMDJSON_COMPETITION_YYJSON)

  add_library(competition-core INTERFACE)
  target_link_libraries(competition-core INTERFACE nlohmann_json rapidjson sajson yyjson)

  if(TARGET boostjson)
     target_compile_definitions(boostjson INTERFACE HAS_BOOST_JSON)
     target_link_libraries(competition-core INTERFACE boostjson)
  endif()

  add_library(competition-all INTERFACE)
  target_link_libraries(competition-all INTERFACE competition-core)
endfunction()

if(SIMDJSON_COMPETITION)
  competition_scope_()
endif()

cmake_dependent_option(SIMDJSON_CXXOPTS "Download cxxopts (necessary for tools)" ON
        SIMDJSON_ALLOW_DOWNLOADS OFF)

if(SIMDJSON_CXXOPTS)
  CPMAddPackage(
        NAME cxxopts
        URL https://github.com/jarro2783/cxxopts/archive/59656709c0c58fcd0ed18b38e02938dbe05284c5.zip
        OPTIONS
        "CXXOPTS_BUILD_EXAMPLES OFF"
        "CXXOPTS_BUILD_TESTS OFF"
        "CXXOPTS_ENABLE_INSTALL OFF"
  )
endif()
