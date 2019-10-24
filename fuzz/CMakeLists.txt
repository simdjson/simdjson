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
 add_executable(fuzzer fuzz.cpp)
 target_link_libraries(fuzzer PRIVATE  ${SIMDJSON_LIB_NAME} )
 target_link_libraries(fuzzer PRIVATE "-fsanitize=fuzzer")

 add_executable(fuzz_like_json2json fuzz_like_json2json.cpp)
 target_link_libraries(fuzz_like_json2json PRIVATE  ${SIMDJSON_LIB_NAME} )
 target_link_libraries(fuzz_like_json2json PRIVATE "-fsanitize=fuzzer")

 add_executable(fuzz_minify fuzz_minify.cpp)
 target_link_libraries(fuzz_minify PRIVATE  ${SIMDJSON_LIB_NAME} )
 target_link_libraries(fuzz_minify PRIVATE "-fsanitize=fuzzer")

endif()

