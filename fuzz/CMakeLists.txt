# First attempt at a fuzzer, using libFuzzer.
#
# compile like this:
# mkdir build-fuzzer
# cd build-fuzzer
# export LDFLAGS="-fsanitize=address,undefined"
# export CXXFLAGS="-fsanitize=fuzzer-no-link,address,undefined"
# export CFLAGS="-fsanitize=fuzzer-no-link,address,undefined"
# export CXX=clang++
# export CC=clang++
# cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DENABLE_FUZZING=On
# ninja

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
 add_executable(fuzz_parser fuzz_parser.cpp)
 target_link_libraries(fuzz_parser PRIVATE  ${SIMDJSON_LIB_NAME} )
 target_link_libraries(fuzz_parser PRIVATE "-fsanitize=fuzzer")

 add_executable(fuzz_minify fuzz_minify.cpp)
 target_link_libraries(fuzz_minify PRIVATE  ${SIMDJSON_LIB_NAME} )
 target_link_libraries(fuzz_minify PRIVATE "-fsanitize=fuzzer")

 add_executable(fuzz_dump fuzz_dump.cpp)
 target_link_libraries(fuzz_dump PRIVATE  ${SIMDJSON_LIB_NAME} )
 target_link_libraries(fuzz_dump PRIVATE "-fsanitize=fuzzer")
endif()

