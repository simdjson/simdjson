target_include_directories(${SIMDJSON_LIB_NAME}
  INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/benchmark>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/benchmark/linux>
)

add_cpp_benchmark(parse)
add_cpp_benchmark(statisticalmodel)
add_cpp_benchmark(parse_stream)
add_cpp_benchmark(get_corpus_benchmark)
add_executable(perfdiff perfdiff.cpp)

# Google Benchmarks
if (SIMDJSON_GOOGLE_BENCHMARKS)
  add_cpp_benchmark(bench_parse_call bench_parse_call.cpp)
  target_link_libraries(bench_parse_call benchmark::benchmark)
endif()