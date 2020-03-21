if(MSVC)
  target_include_directories(${SIMDJSON_LIB_NAME}
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/windows>
  )
endif()

add_cpp_test(basictests)
add_cpp_test(errortests)

## Next bit should not be needed!
#if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
# next line is a workaround for an odr-violation in basictests regarding the globals 0x432a40 and 0x52045c under clang
#set_tests_properties(basictests PROPERTIES
#    ENVIRONMENT ASAN_OPTIONS="detect_odr_violation=0")
#endif()

add_cpp_test(jsoncheck)
add_cpp_test(parse_many_test)
add_cpp_test(pointercheck)
add_cpp_test(integer_tests)

target_compile_definitions(basictests PRIVATE JSON_TEST_PATH="${PROJECT_SOURCE_DIR}/jsonexamples/twitter.json" NDJSON_TEST_PATH="${PROJECT_SOURCE_DIR}/jsonexamples/amazon_cellphones.ndjson")
target_compile_definitions(errortests PRIVATE JSON_TEST_PATH="${PROJECT_SOURCE_DIR}/jsonexamples/twitter.json")

## This causes problems
# add_executable(singleheader ./singleheadertest.cpp ${PROJECT_SOURCE_DIR}/singleheader/simdjson.cpp)
# target_link_libraries(singleheader ${SIMDJSON_LIB_NAME})
# add_test(singleheader singleheader)

if(MSVC)
  add_custom_command(TARGET basictests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:simdjson>"
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE_DIR:basictests>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:basictests>")                 # <--this is out-file path
endif()

