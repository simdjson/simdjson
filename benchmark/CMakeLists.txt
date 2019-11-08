target_include_directories(${SIMDJSON_LIB_NAME}
  INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/benchmark>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/benchmark/linux>
)

add_cpp_benchmark(parse)
add_cpp_benchmark(statisticalmodel)
add_cpp_benchmark(parse_stream)
add_executable(perfdiff perfdiff.cpp)
