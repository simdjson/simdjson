if(MSVC)
  target_include_directories(${SIMDJSON_LIB_NAME}
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/windows>
  )
endif()

add_cpp_test(basictests)
add_cpp_test(jsoncheck)
add_cpp_test(pointercheck)

## This causes problems
# add_executable(singleheader ./singleheadertest.cpp ${PROJECT_SOURCE_DIR}/singleheader/simdjson.cpp)
# target_compile_definitions(singleheader PRIVATE JSON_TEST_PATH="${PROJECT_SOURCE_DIR}/jsonexamples/twitter.json")
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

