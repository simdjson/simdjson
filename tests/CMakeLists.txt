# Helper so we don't have to repeat ourselves so much
function(add_cpp_test TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  add_test(${TEST_NAME} ${TEST_NAME})
endfunction(add_cpp_test)


link_libraries(simdjson)

add_cpp_test(basictests)
add_cpp_test(errortests)
add_cpp_test(jsoncheck)
add_cpp_test(parse_many_test)
add_cpp_test(pointercheck)
add_cpp_test(integer_tests)

if(MSVC)
  include_directories($<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/windows>)
  add_custom_command(TARGET basictests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:simdjson>"
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE_DIR:basictests>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:basictests>")                 # <--this is out-file path
endif()

## Next bit should not be needed!
#if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
# next line is a workaround for an odr-violation in basictests regarding the globals 0x432a40 and 0x52045c under clang
#set_tests_properties(basictests PROPERTIES
#    ENVIRONMENT ASAN_OPTIONS="detect_odr_violation=0")
#endif()

## This causes problems
# add_executable(singleheader ./singleheadertest.cpp ${PROJECT_SOURCE_DIR}/singleheader/simdjson.cpp)
# target_link_libraries(singleheader simdjson)
# add_test(singleheader singleheader)
