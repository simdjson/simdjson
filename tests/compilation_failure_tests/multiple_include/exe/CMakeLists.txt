cmake_minimum_required(VERSION 3.14)

add_executable(myexe main.cpp)

target_link_libraries(myexe PRIVATE mylib)