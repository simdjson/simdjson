include(CMakeDependentOption)
include(import.cmake)

option(SIMDJSON_ALLOW_DOWNLOADS
        "Allow dependencies to be downloaded during configure time"
        ON)

cmake_dependent_option(SIMDJSON_COMPETITION "Compile competitive benchmarks" ON
        SIMDJSON_ALLOW_DOWNLOADS OFF)
cmake_dependent_option(SIMDJSON_GOOGLE_BENCHMARKS "compile the Google Benchmark benchmarks" ON
        SIMDJSON_ALLOW_DOWNLOADS OFF)

if(SIMDJSON_GOOGLE_BENCHMARKS)
  set_off(BENCHMARK_ENABLE_TESTING)
  set_off(BENCHMARK_ENABLE_INSTALL)
  set_off(BENCHMARK_ENABLE_WERROR)

  import_dependency(google_benchmarks google/benchmark v1.7.1)
  add_dependency(google_benchmarks)
endif()

# The bulk of our benchmarking and testing data has been moved simdjson/simdjson-data
import_dependency(simdjson-data simdjson/simdjson-data a5b13babe65c1bba7186b41b43d4cbdc20a5c470)
add_dependency(simdjson-data)

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
]] USE_BOOST_JSON)
  if(USE_BOOST_JSON)
    import_dependency(boostjson boostorg/json ee8d72d)
    add_library(boostjson STATIC "${boostjson_SOURCE_DIR}/src/src.cpp")
    target_compile_definitions(boostjson PUBLIC BOOST_JSON_STANDALONE)
    target_include_directories(boostjson SYSTEM PUBLIC
            "${boostjson_SOURCE_DIR}/include")
    target_compile_definitions(boostjson INTERFACE SIMDJSON_COMPETITION_BOOSTJSON)
  endif()

  import_dependency(cjson DaveGamble/cJSON c69134d)
  add_library(cjson STATIC "${cjson_SOURCE_DIR}/cJSON.c")
  target_include_directories(cjson SYSTEM PUBLIC "${cjson_SOURCE_DIR}")
  target_compile_definitions(cjson INTERFACE SIMDJSON_COMPETITION_CJSON)

  import_dependency(fastjson mikeando/fastjson 485f994)
  add_library(fastjson STATIC
          "${fastjson_SOURCE_DIR}/src/fastjson.cpp"
          "${fastjson_SOURCE_DIR}/src/fastjson2.cpp"
          "${fastjson_SOURCE_DIR}/src/fastjson_dom.cpp")
  target_include_directories(fastjson SYSTEM PUBLIC
          "${fastjson_SOURCE_DIR}/include")
  target_compile_definitions(fastjson INTERFACE SIMDJSON_COMPETITION_FASTJSON)

  import_dependency(gason vivkin/gason 7aee524)
  add_library(gason STATIC "${gason_SOURCE_DIR}/src/gason.cpp")
  target_include_directories(gason SYSTEM PUBLIC "${gason_SOURCE_DIR}/src")
  target_compile_definitions(gason INTERFACE SIMDJSON_COMPETITION_GASON)

  import_dependency(jsmn zserge/jsmn 18e9fe4)
  add_library(jsmn STATIC "${jsmn_SOURCE_DIR}/jsmn.c")
  target_include_directories(jsmn SYSTEM PUBLIC "${jsmn_SOURCE_DIR}")
  target_compile_definitions(jsmn INTERFACE SIMDJSON_COMPETITION_JSMN)

  message(STATUS "Importing json (nlohmann/json@v3.10.5)")
  set(nlohmann_json_SOURCE_DIR "${dep_root}/json")
  if(NOT EXISTS "${nlohmann_json_SOURCE_DIR}")
    file(DOWNLOAD
            "https://github.com/nlohmann/json/releases/download/v3.10.5/json.hpp"
            "${nlohmann_json_SOURCE_DIR}/nlohmann/json.hpp")
  endif()
  add_library(nlohmann_json INTERFACE)
  target_include_directories(nlohmann_json SYSTEM INTERFACE "${nlohmann_json_SOURCE_DIR}")
  target_compile_definitions(nlohmann_json INTERFACE SIMDJSON_COMPETITION_NLOHMANN_JSON)

  import_dependency(json11 dropbox/json11 ec4e452)
  add_library(json11 STATIC "${json11_SOURCE_DIR}/json11.cpp")
  target_include_directories(json11 SYSTEM PUBLIC "${json11_SOURCE_DIR}")
  target_compile_definitions(json11 INTERFACE SIMDJSON_COMPETITION_JSON11)

  set(jsoncpp_SOURCE_DIR "${simdjson_SOURCE_DIR}/dependencies/jsoncppdist")
  add_library(jsoncpp STATIC "${jsoncpp_SOURCE_DIR}/jsoncpp.cpp")
  target_include_directories(jsoncpp SYSTEM PUBLIC "${jsoncpp_SOURCE_DIR}")
  target_compile_definitions(jsoncpp INTERFACE SIMDJSON_COMPETITION_JSONCPP)

  import_dependency(rapidjson Tencent/rapidjson f54b0e4)
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
  target_compile_definitions(rapidjson INTERFACE SIMDJSON_COMPETITION_RAPIDJSON)

  import_dependency(sajson chadaustin/sajson 2dcfd35)
  add_library(sajson INTERFACE)
  target_compile_definitions(sajson INTERFACE SAJSON_UNSORTED_OBJECT_KEYS)
  target_include_directories(sajson SYSTEM INTERFACE
          "${sajson_SOURCE_DIR}/include")
  target_compile_definitions(sajson INTERFACE SIMDJSON_COMPETITION_SAJSON)

  import_dependency(ujson4c esnme/ujson4c e14f3fd)
  add_library(ujson4c STATIC
          "${ujson4c_SOURCE_DIR}/src/ujdecode.c"
          "${ujson4c_SOURCE_DIR}/3rdparty/ultrajsondec.c")
  target_include_directories(ujson4c SYSTEM PUBLIC
          "${ujson4c_SOURCE_DIR}/src"
          "${ujson4c_SOURCE_DIR}/3rdparty")
  target_compile_definitions(ujson4c INTERFACE SIMDJSON_COMPETITION_UJSON4C)

  import_dependency(yyjson ibireme/yyjson c385651)
  add_library(yyjson STATIC "${yyjson_SOURCE_DIR}/src/yyjson.c")
  target_include_directories(yyjson SYSTEM PUBLIC "${yyjson_SOURCE_DIR}/src")
  target_compile_definitions(yyjson INTERFACE SIMDJSON_COMPETITION_YYJSON)

  add_library(competition-core INTERFACE)
  target_link_libraries(competition-core INTERFACE nlohmann_json rapidjson sajson cjson jsmn yyjson)

  if(USE_BOOST_JSON)
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
  set_off(CXXOPTS_BUILD_EXAMPLES)
  set_off(CXXOPTS_BUILD_TESTS)
  set_off(CXXOPTS_ENABLE_INSTALL)

  import_dependency(cxxopts jarro2783/cxxopts 794c975)
  add_dependency(cxxopts)
endif()
