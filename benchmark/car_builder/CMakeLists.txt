# Executable
add_executable(benchmark_car_builder benchmark_car_builder.cpp)

# Compile for C++20.
target_compile_features(benchmark_car_builder PRIVATE cxx_std_20)

# Check if -march=native is supported
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" SIMDJSON_SUPPORTS_MARCH_NATIVE)
if(SIMDJSON_SUPPORTS_MARCH_NATIVE)
  target_compile_options(benchmark_car_builder PRIVATE -march=native)
endif()

target_include_directories(benchmark_car_builder PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)