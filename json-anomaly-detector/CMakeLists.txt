cmake_minimum_required(VERSION 3.14)
project(json-anomaly-detector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point to the root of the repo
set(SIMDJSON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

include_directories(${SIMDJSON_ROOT}/include)

# Check for ncurses
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# Check for curl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

add_executable(json-anomaly-detector
    src/main.cpp
    ${SIMDJSON_ROOT}/src/simdjson.cpp
)

target_link_libraries(json-anomaly-detector
    ${CURSES_LIBRARIES}
    ${CURL_LIBRARIES}
    pthread
    dl
)
