target_include_directories(${SIMDJSON_LIB_NAME}
  PUBLIC
  ${PROJECT_SOURCE_DIR}/benchmark
  ${PROJECT_SOURCE_DIR}/benchmark/linux
)

add_cpp_benchmark(parse)
add_cpp_benchmark(statisticalmodel)
