#
# For callers who intend to #include simdjson.cpp.
#
# target_link_libraries(my-program simdjson-include-source) gives you the header and source
# directories. It does not specify any compiler flags.
#

add_library(simdjson-include-source INTERFACE)
target_link_libraries(simdjson-include-source INTERFACE simdjson-headers)
target_include_directories(simdjson-include-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

#
# simdjson is the distributed library compiled with flags.
#
# target_link_libraries(my-object simdjson) gives you the .so or .a to link against, plus the header
# directory. It does not specify any compiler flags, even though simdjson.so/a was compiled with
# target_link_libraries(simdjson PRIVATE simdjson-flags).
#
# TODO: For some reason simdjson-internal cannot pass checkperf if built as a static library
add_library(simdjson-internal SHARED simdjson.cpp)
target_link_libraries(simdjson-internal PUBLIC simdjson-headers simdjson-flags)
target_link_libraries(simdjson-internal PRIVATE simdjson-include-source)
