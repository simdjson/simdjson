# First attempt at a fuzzer, using libFuzzer.
#
# compile like this:
# mkdir build-fuzzer
# cd build-fuzzer
# export LDFLAGS="-fsanitize=address,undefined"
# export CXXFLAGS="-fsanitize=fuzzer-no-link,address,undefined"
# export CXX=clang++
# cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DENABLE_FUZZING=On
# ninja

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
 add_executable(fuzzer fuzz.cpp)
 target_link_libraries(fuzzer PRIVATE  ${SIMDJSON_LIB_NAME} )
 target_link_libraries(fuzzer PRIVATE "-fsanitize=fuzzer")
endif()

