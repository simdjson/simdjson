# Executable
add_executable(from_benchmark from_benchmark.cpp)

# Compile for C++20.
if(CMAKE_CXX_STANDARD LESS 20)
  target_compile_features(from_benchmark PRIVATE cxx_std_20)
endif()
# Check if -march=native is supported
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" SIMDJSON_SUPPORTS_MARCH_NATIVE)
if(SIMDJSON_SUPPORTS_MARCH_NATIVE)
  target_compile_options(from_benchmark PRIVATE -march=native)
endif()

target_include_directories(from_benchmark PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)