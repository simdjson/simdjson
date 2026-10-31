add_subdirectory(dom)


include_directories( . )
link_libraries(simdjson-windows-headers test-data)
link_libraries(simdjson)
# Most benchmarks include <counters/event_counter.h>. The counters library is
# downloaded, so it is missing when SIMDJSON_ALLOW_DOWNLOADS=OFF (or
# SIMDJSON_COUNTERS=OFF) unless an installed copy is found: skip those
# benchmarks rather than fail the build.
if(TARGET counters)
  link_libraries(counters)
else()
  message(STATUS "The counters library is unavailable: most benchmarks are disabled.")
endif()
if(SIMDJSON_STATIC_REFLECTION_ENABLED)
  add_compile_definitions(SIMDJSON_STATIC_REFLECTION=1)
endif(SIMDJSON_STATIC_REFLECTION_ENABLED)

add_executable(get_corpus_benchmark get_corpus_benchmark.cpp)
if(TARGET counters)
  add_executable(benchfeatures benchfeatures.cpp)
  # Uses counters + from/benchmark_helpers.h, not Google Benchmark, so it
  # does not belong behind the benchmark::benchmark guard below.
  add_executable(bench_object_revert_position bench_object_revert_position.cpp)
endif()

if (TARGET benchmark::benchmark)
  link_libraries(benchmark::benchmark)
  add_executable(bench_parse_call bench_parse_call.cpp)
  add_executable(bench_dom_api bench_dom_api.cpp)
  add_executable(bench_array_iteration bench_array_iteration.cpp)
  add_executable(bench_stream_formats bench_stream_formats.cpp)
  add_executable(bench_number_accessors bench_number_accessors.cpp)
  add_executable(bench_number_accessors_int bench_number_accessors.cpp)
  target_compile_definitions(bench_number_accessors_int PRIVATE BENCH_ONLY_INT)
  add_executable(bench_number_accessors_double bench_number_accessors.cpp)
  target_compile_definitions(bench_number_accessors_double PRIVATE BENCH_ONLY_DOUBLE)
  add_executable(bench_number_accessors_float bench_number_accessors.cpp)
  target_compile_definitions(bench_number_accessors_float PRIVATE BENCH_ONLY_FLOAT)
  add_executable(bench_number_accessors_number bench_number_accessors.cpp)
  target_compile_definitions(bench_number_accessors_number PRIVATE BENCH_ONLY_NUMBER)
  if(SIMDJSON_EXCEPTIONS)
    add_executable(bench_ondemand bench_ondemand.cpp)
    target_compile_options(bench_ondemand PRIVATE $<$<CXX_COMPILER_ID:GNU>:-ffp-contract=off>)
    if(TARGET yyjson)
      target_link_libraries(bench_ondemand PRIVATE yyjson)
    endif()
    if(TARGET rapidjson)
      target_link_libraries(bench_ondemand PRIVATE rapidjson)
    endif()
    if(TARGET sajson)
      target_link_libraries(bench_ondemand PRIVATE sajson)
    endif()
    if(TARGET nlohmann_json)
      target_link_libraries(bench_ondemand PRIVATE nlohmann_json)
    endif()
    if(TARGET boostjson)
      target_link_libraries(bench_ondemand PRIVATE boostjson)
    endif()
    if(SIMDJSON_COMPETITION AND "cxx_std_23" IN_LIST CMAKE_CXX_COMPILE_FEATURES AND NOT CMAKE_VERSION VERSION_LESS 3.28)
      CPMAddPackage(
        NAME jsonifier
        URL https://github.com/nihilai-collective/jsonifier/archive/60aa533.zip
        EXCLUDE_FROM_ALL YES
      )
      target_compile_definitions(Jsonifier INTERFACE SIMDJSON_COMPETITION_JSONIFIER)
      target_link_libraries(bench_ondemand PRIVATE Jsonifier)
      target_compile_features(bench_ondemand PRIVATE cxx_std_23)
    endif()
  endif()
endif()


# Every benchmark in these subdirectories uses counters.
if(TARGET counters)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-std=c++20" SIMDJSON_COMPILER_SUPPORTS_CXX20)
  if(SIMDJSON_STATIC_REFLECTION_ENABLED)
    # static_reflect downloads its competitors: not in an offline build.
    if(COMMAND CPMAddPackage)
      add_subdirectory(static_reflect)
    endif()
  else()
    if(SIMDJSON_EXCEPTIONS AND SIMDJSON_COMPILER_SUPPORTS_CXX20)
      add_subdirectory(from)
      add_subdirectory(car_builder)
    endif()
  endif(SIMDJSON_STATIC_REFLECTION_ENABLED)
endif()


