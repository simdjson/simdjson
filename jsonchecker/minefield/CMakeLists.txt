set(SIMDJSON_MINEFIELD_TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

add_library(jsonchecker-minefield-data INTERFACE)
target_compile_definitions(jsonchecker-minefield-data INTERFACE SIMDJSON_MINEFIELD_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
