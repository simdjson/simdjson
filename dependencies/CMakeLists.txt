include(CMakeDependentOption)

option(SIMDJSON_ALLOW_DOWNLOADS
        "Allow dependencies to be downloaded during configure time"
        ON)

cmake_dependent_option(SIMDJSON_COMPETITION "Compile competitive benchmarks" ON
        SIMDJSON_ALLOW_DOWNLOADS OFF)
cmake_dependent_option(SIMDJSON_GOOGLE_BENCHMARKS "compile the Google Benchmark benchmarks" ON
        SIMDJSON_ALLOW_DOWNLOADS OFF)

if(SIMDJSON_GOOGLE_BENCHMARKS)
  CPMAddPackage(
        NAME google_benchmarks
        GITHUB_REPOSITORY google/benchmark
        GIT_TAG v1.7.1
        OPTIONS
        "BENCHMARK_ENABLE_TESTING OFF"
        "BENCHMARK_ENABLE_INSTALL OFF"
        "BENCHMARK_ENABLE_WERROR OFF"
  )
endif()

CPMAddPackage(
        NAME simdjson-data
        GITHUB_REPOSITORY simdjson/simdjson-data
        GIT_TAG a5b13babe65c1bba7186b41b43d4cbdc20a5c470
)

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
        GITHUB_REPOSITORY boostorg/json
        GIT_TAG ee8d72d
    )
    add_library(boostjson STATIC "${boostjson_SOURCE_DIR}/src/src.cpp")
    target_compile_definitions(boostjson PUBLIC BOOST_JSON_STANDALONE)
    target_include_directories(boostjson SYSTEM PUBLIC
            "${boostjson_SOURCE_DIR}/include")
    target_compile_definitions(boostjson INTERFACE SIMDJSON_COMPETITION_BOOSTJSON)
  endif()

  CPMAddPackage(
        NAME cjson
        GITHUB_REPOSITORY DaveGamble/cJSON
        GIT_TAG c69134d
        DOWNLOAD_ONLY YES
  )
  add_library(cjson STATIC "${cjson_SOURCE_DIR}/cJSON.c")
  target_include_directories(cjson SYSTEM PUBLIC "${cjson_SOURCE_DIR}")
  target_compile_definitions(cjson INTERFACE SIMDJSON_COMPETITION_CJSON)

  CPMAddPackage(
        NAME fastjson
        GITHUB_REPOSITORY mikeando/fastjson
        GIT_TAG 485f994
  )
  add_library(fastjson STATIC
          "${fastjson_SOURCE_DIR}/src/fastjson.cpp"
          "${fastjson_SOURCE_DIR}/src/fastjson2.cpp"
          "${fastjson_SOURCE_DIR}/src/fastjson_dom.cpp")
  target_include_directories(fastjson SYSTEM PUBLIC
          "${fastjson_SOURCE_DIR}/include")
  target_compile_definitions(fastjson INTERFACE SIMDJSON_COMPETITION_FASTJSON)

  CPMAddPackage(
        NAME gason
        GITHUB_REPOSITORY vivkin/gason
        GIT_TAG 7aee524
        DOWNLOAD_ONLY YES
  )
  add_library(gason STATIC "${gason_SOURCE_DIR}/src/gason.cpp")
  target_include_directories(gason SYSTEM PUBLIC "${gason_SOURCE_DIR}/src")
  target_compile_definitions(gason INTERFACE SIMDJSON_COMPETITION_GASON)

  CPMAddPackage(
        NAME jsmn
        GITHUB_REPOSITORY zserge/jsmn
        GIT_TAG 18e9fe4
  )
  add_library(jsmn STATIC "${jsmn_SOURCE_DIR}/jsmn.c")
  target_include_directories(jsmn SYSTEM PUBLIC "${jsmn_SOURCE_DIR}")
  target_compile_definitions(jsmn INTERFACE SIMDJSON_COMPETITION_JSMN)

  CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        GIT_TAG v3.10.5
  )
  #nlohmann_json::nlohmann_json
#  target_compile_definitions(nlohmann_json PUBLIC SIMDJSON_COMPETITION_NLOHMANN_JSON)
  set_property(TARGET nlohmann_json APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS SIMDJSON_COMPETITION_NLOHMANN_JSON)

  CPMAddPackage(
        NAME json11
        GITHUB_REPOSITORY dropbox/json11
        GIT_TAG ec4e452
        DOWNLOAD_ONLY YES
  )
  add_library(json11 STATIC "${json11_SOURCE_DIR}/json11.cpp")
  target_include_directories(json11 SYSTEM PUBLIC "${json11_SOURCE_DIR}")
  target_compile_definitions(json11 INTERFACE SIMDJSON_COMPETITION_JSON11)

  set(jsoncpp_SOURCE_DIR "${simdjson_SOURCE_DIR}/dependencies/jsoncppdist")
  add_library(jsoncpp STATIC "${jsoncpp_SOURCE_DIR}/jsoncpp.cpp")
  target_include_directories(jsoncpp SYSTEM PUBLIC "${jsoncpp_SOURCE_DIR}")
  target_compile_definitions(jsoncpp INTERFACE SIMDJSON_COMPETITION_JSONCPP)

  CPMAddPackage(
        NAME rapidjson
        GITHUB_REPOSITORY Tencent/rapidjson
        GIT_TAG f54b0e4
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
        GITHUB_REPOSITORY chadaustin/sajson
        GIT_TAG 2dcfd35
        DOWNLOAD_ONLY YES
  )
  add_library(sajson INTERFACE)
  target_compile_definitions(sajson INTERFACE SAJSON_UNSORTED_OBJECT_KEYS)
  target_include_directories(sajson SYSTEM INTERFACE
          "${sajson_SOURCE_DIR}/include")
  target_compile_definitions(sajson INTERFACE SIMDJSON_COMPETITION_SAJSON)

  CPMAddPackage(
        NAME ujson4c
        GITHUB_REPOSITORY esnme/ujson4c
        GIT_TAG e14f3fd
        DOWNLOAD_ONLY YES
  )
  add_library(ujson4c STATIC
          "${ujson4c_SOURCE_DIR}/src/ujdecode.c"
          "${ujson4c_SOURCE_DIR}/3rdparty/ultrajsondec.c")
  target_include_directories(ujson4c SYSTEM PUBLIC
          "${ujson4c_SOURCE_DIR}/src"
          "${ujson4c_SOURCE_DIR}/3rdparty")
  target_compile_definitions(ujson4c INTERFACE SIMDJSON_COMPETITION_UJSON4C)
  CPMAddPackage(
        NAME yyjson
        GITHUB_REPOSITORY ibireme/yyjson
        GIT_TAG c385651
        DOWNLOAD_ONLY YES
  )
  add_library(yyjson STATIC "${yyjson_SOURCE_DIR}/src/yyjson.c")
  target_include_directories(yyjson SYSTEM PUBLIC "${yyjson_SOURCE_DIR}/src")
  target_compile_definitions(yyjson INTERFACE SIMDJSON_COMPETITION_YYJSON)

  add_library(competition-core INTERFACE)
  target_link_libraries(competition-core INTERFACE nlohmann_json rapidjson sajson cjson jsmn yyjson)

  if(TARGET boostjson)
     target_compile_definitions(boostjson INTERFACE HAS_BOOST_JSON)
     target_link_libraries(competition-core INTERFACE boostjson)
  endif()

  add_library(competition-all INTERFACE)
  target_link_libraries(competition-all INTERFACE competition-core jsoncpp json11 fastjson gason ujson4c)
endfunction()

if(SIMDJSON_COMPETITION)
  competition_scope_()
endif()

cmake_dependent_option(SIMDJSON_CXXOPTS "Download cxxopts (necessary for tools)" ON
        SIMDJSON_ALLOW_DOWNLOADS OFF)

if(SIMDJSON_CXXOPTS)
  CPMAddPackage(
        NAME cxxopts
        GITHUB_REPOSITORY jarro2783/cxxopts
        GIT_TAG 5965670
        OPTIONS
        "CXXOPTS_BUILD_EXAMPLES OFF"
        "CXXOPTS_BUILD_TESTS OFF"
        "CXXOPTS_ENABLE_INSTALL OFF"
  )
endif()
