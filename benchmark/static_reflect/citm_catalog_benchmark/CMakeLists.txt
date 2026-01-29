add_executable(benchmark_serialization_citm_catalog benchmark_serialization_citm_catalog.cpp)
add_executable(benchmark_parsing_citm benchmark_parsing_citm.cpp)

# Link with Rust benchmarking code if available
if(TARGET serde-benchmark)
    message(STATUS "serde-benchmark target was created. Linking CITM catalog benchmark with serde-benchmark.")
    target_link_libraries(benchmark_serialization_citm_catalog PRIVATE serde-benchmark)
    target_compile_definitions(benchmark_serialization_citm_catalog PRIVATE SIMDJSON_RUST_VERSION="${Rust_VERSION}")
endif()

target_link_libraries(benchmark_serialization_citm_catalog PRIVATE simdjson::simdjson nlohmann_json)
target_link_libraries(benchmark_serialization_citm_catalog PRIVATE reflectcpp)
target_compile_definitions(benchmark_serialization_citm_catalog PRIVATE SIMDJSON_BENCH_CPP_REFLECT=1)

if(TARGET yyjson)
    target_link_libraries(benchmark_serialization_citm_catalog PRIVATE yyjson)
    target_compile_definitions(benchmark_serialization_citm_catalog PRIVATE SIMDJSON_COMPETITION_YYJSON)
endif()

target_compile_definitions(benchmark_serialization_citm_catalog PRIVATE JSON_FILE="${BENCH_CITM_JSON}")

# Configuration for parsing benchmark
if(TARGET serde-benchmark)
    target_link_libraries(benchmark_parsing_citm PRIVATE serde-benchmark)
    target_compile_definitions(benchmark_parsing_citm PRIVATE SIMDJSON_RUST_VERSION="${Rust_VERSION}")
endif()

target_link_libraries(benchmark_parsing_citm PRIVATE simdjson::simdjson nlohmann_json)

if(TARGET rapidjson)
    target_link_libraries(benchmark_parsing_citm PRIVATE rapidjson)
    target_compile_definitions(benchmark_parsing_citm PRIVATE SIMDJSON_COMPETITION_RAPIDJSON)
endif()

if(TARGET yyjson)
    target_link_libraries(benchmark_parsing_citm PRIVATE yyjson)
    target_compile_definitions(benchmark_parsing_citm PRIVATE SIMDJSON_COMPETITION_YYJSON)
endif()

target_compile_definitions(benchmark_parsing_citm PRIVATE JSON_FILE="${BENCH_CITM_JSON}")