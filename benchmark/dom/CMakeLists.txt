include_directories( .. )
link_libraries(simdjson-windows-headers test-data)
link_libraries(simdjson)

add_executable(perfdiff perfdiff.cpp)
add_executable(parse_stream parse_stream.cpp)

# parse uses counters, and the checkperf targets need parse.
if(TARGET counters)
  add_executable(parse parse.cpp)
  target_link_libraries(parse PRIVATE counters)
  include(checkperf.cmake)
endif()
