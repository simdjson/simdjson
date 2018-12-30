if(MSVC)
target_include_directories(${SIMDJSON_LIB_NAME}
  PUBLIC ${PROJECT_SOURCE_DIR}/windows
)
endif()

add_cpp_test(jsoncheck)
add_test(jsoncheck jsoncheck)
