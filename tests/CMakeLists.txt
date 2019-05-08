if(MSVC)
  target_include_directories(${SIMDJSON_LIB_NAME}
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/windows>
  )
endif()

add_cpp_test(basictests)
add_cpp_test(jsoncheck)

add_executable(singleheader ./singleheadertest.cpp ${PROJECT_SOURCE_DIR}/singleheader/simdjson.cpp)
target_compile_definitions(singleheader PRIVATE JSON_TEST_PATH="${PROJECT_SOURCE_DIR}/jsonexamples/twitter.json")
target_link_libraries(singleheader ${SIMDJSON_LIB_NAME})
add_test(singleheader singleheader)