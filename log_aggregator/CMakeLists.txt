cmake_minimum_required(VERSION 3.10)
project(log_aggregator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags for performance
if(MSVC)
    add_compile_options(/O2)
else()
    add_compile_options(-O3 -march=native)
endif()

add_executable(log_aggregator main.cpp ../singleheader/simdjson.cpp)
target_include_directories(log_aggregator PRIVATE ../singleheader)
