
# Add executable targets
add_executable(benchmark_serialization_twitter benchmark_serialization_twitter.cpp)
add_executable(benchmark_parsing_twitter benchmark_parsing_twitter.cpp)

 if(TARGET serde-benchmark)
    message(STATUS "serde-benchmark target was created. Linking benchmarks and serde-benchmark.")
    target_link_libraries(benchmark_serialization_twitter PRIVATE serde-benchmark)
    target_link_libraries(benchmark_parsing_twitter PRIVATE serde-benchmark)
    target_compile_definitions(benchmark_serialization_twitter PRIVATE SIMDJSON_RUST_VERSION="${Rust_VERSION}")
    target_compile_definitions(benchmark_parsing_twitter PRIVATE SIMDJSON_RUST_VERSION="${Rust_VERSION}")
endif()
target_link_libraries(benchmark_serialization_twitter PRIVATE simdjson::simdjson nlohmann_json)
target_link_libraries(benchmark_serialization_twitter PRIVATE reflectcpp)
target_compile_definitions(benchmark_serialization_twitter PRIVATE SIMDJSON_BENCH_CPP_REFLECT=1)

target_link_libraries(benchmark_parsing_twitter PRIVATE simdjson::simdjson nlohmann_json)

if(TARGET rapidjson)
    target_link_libraries(benchmark_parsing_twitter PRIVATE rapidjson)
    target_compile_definitions(benchmark_parsing_twitter PRIVATE SIMDJSON_COMPETITION_RAPIDJSON)
endif()

if(TARGET yyjson)
    target_link_libraries(benchmark_parsing_twitter PRIVATE yyjson)
    target_compile_definitions(benchmark_parsing_twitter PRIVATE SIMDJSON_COMPETITION_YYJSON)
    target_link_libraries(benchmark_serialization_twitter PRIVATE yyjson)
    target_compile_definitions(benchmark_serialization_twitter PRIVATE SIMDJSON_COMPETITION_YYJSON)
endif()

target_compile_definitions(benchmark_serialization_twitter PRIVATE JSON_FILE="${EXAMPLE_JSON}")
target_compile_definitions(benchmark_parsing_twitter PRIVATE JSON_FILE="${EXAMPLE_JSON}")