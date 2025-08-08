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
        URL https://github.com/google/benchmark/archive/refs/tags/v1.7.1.zip
        OPTIONS
        "BENCHMARK_ENABLE_TESTING OFF"
        "BENCHMARK_ENABLE_INSTALL OFF"
        "BENCHMARK_ENABLE_WERROR OFF"
  )
endif()

CPMAddPackage(
        NAME simdjson-data
        URL https://github.com/simdjson/simdjson-data/archive/a5b13babe65c1bba7186b41b43d4cbdc20a5c470.zip
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
        URL https://github.com/boostorg/json/archive/ee8d72d8502b409b5561200299cad30ccdb91415.zip
    )
    add_library(boostjson STATIC "${boostjson_SOURCE_DIR}/src/src.cpp")
    target_compile_definitions(boostjson PUBLIC BOOST_JSON_STANDALONE)
    target_include_directories(boostjson SYSTEM PUBLIC
            "${boostjson_SOURCE_DIR}/include")
    target_compile_definitions(boostjson INTERFACE SIMDJSON_COMPETITION_BOOSTJSON)
  endif()
  CPMAddPackage(
      NAME cjson
      URL https://github.com/DaveGamble/cJSON/archive/c69134d01746dcf551dd7724b4edb12f922eb0d1.zip
      DOWNLOAD_ONLY YES
  )
  add_library(cjson STATIC "${cjson_SOURCE_DIR}/cJSON.c")
  target_include_directories(cjson SYSTEM PUBLIC "${cjson_SOURCE_DIR}")
  target_compile_definitions(cjson INTERFACE SIMDJSON_COMPETITION_CJSON)

  CPMAddPackage(
      NAME fastjson
      URL https://github.com/mikeando/fastjson/archive/485f994a61a64ac73fa6a40d4d639b99b463563b.zip
      DOWNLOAD_ONLY YES
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
      URL https://github.com/vivkin/gason/archive/7aee524189da1c1ecd19f67981e3d903dae25470.zip
      DOWNLOAD_ONLY YES
  )
  add_library(gason STATIC "${gason_SOURCE_DIR}/src/gason.cpp")
  target_include_directories(gason SYSTEM PUBLIC "${gason_SOURCE_DIR}/src")
  target_compile_definitions(gason INTERFACE SIMDJSON_COMPETITION_GASON)

  CPMAddPackage(
      NAME jsmn
      URL https://github.com/zserge/jsmn/archive/18e9fe42cbfe21d65076f5c77ae2be379ad1270f.zip
      DOWNLOAD_ONLY YES
  )
  add_library(jsmn STATIC "${jsmn_SOURCE_DIR}/jsmn.c")
  target_include_directories(jsmn SYSTEM PUBLIC "${jsmn_SOURCE_DIR}")
  target_compile_definitions(jsmn INTERFACE SIMDJSON_COMPETITION_JSMN)

  CPMAddPackage(
      NAME nlohmann_json
      URL https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.zip
  )

  set_property(TARGET nlohmann_json APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS SIMDJSON_COMPETITION_NLOHMANN_JSON)

  CPMAddPackage(
      NAME json11
      URL https://github.com/dropbox/json11/archive/ec4e45219af1d7cde3d58b49ed762376fccf1ace.zip
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
      URL https://github.com/Tencent/rapidjson/archive/f54b0e47a08782a6131cc3d60f94d038fa6e0a51.zip
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
      NAME ujson4c
      URL https://github.com/esnme/ujson4c/archive/e14f3fd5207fe30d1bdea723f260609e69d1abfa.zip
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
      URL https://github.com/ibireme/yyjson/archive/c3856514de0a67d7b66939bf3ed491a2d6e61277.zip
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
        URL https://github.com/jarro2783/cxxopts/archive/59656709c0c58fcd0ed18b38e02938dbe05284c5.zip
        OPTIONS
        "CXXOPTS_BUILD_EXAMPLES OFF"
        "CXXOPTS_BUILD_TESTS OFF"
        "CXXOPTS_ENABLE_INSTALL OFF"
  )
endif()
