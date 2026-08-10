include_directories( .. )
link_libraries(simdjson-windows-headers test-data)
link_libraries(simdjson)
link_libraries(counters)

add_executable(perfdiff perfdiff.cpp)
add_executable(parse parse.cpp)
add_executable(parse_stream parse_stream.cpp)

include(checkperf.cmake)
