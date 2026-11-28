add_executable(benchmark_parsing_recipe_book benchmark_parsing_recipe_book.cpp)
target_link_libraries(benchmark_parsing_recipe_book PRIVATE simdjson::simdjson)
target_compile_definitions(benchmark_parsing_recipe_book PRIVATE JSON_FILE="${BENCH_RECIPE_BOOK_JSON}")

if(TARGET glaze)
    target_link_libraries(benchmark_parsing_recipe_book PRIVATE glaze)
    target_compile_definitions(benchmark_parsing_recipe_book PRIVATE SIMDJSON_COMPETITION_GLAZE)
endif()

if(TARGET Jsonifier)
    target_link_libraries(benchmark_parsing_recipe_book PRIVATE Jsonifier)
endif()
