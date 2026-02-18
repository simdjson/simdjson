cmake_minimum_required(VERSION 3.14)

add_library(mylib mylib.cpp)

target_link_libraries(mylib PUBLIC simdjson::simdjson)

target_include_directories(mylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})