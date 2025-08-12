# Executable
add_executable(from_benchmark from_benchmark.cpp)

# Compile for C++20.
target_compile_features(from_benchmark PRIVATE cxx_std_20)

# Link simdjson
target_link_libraries(from_benchmark PRIVATE simdjson::simdjson)

# Ajouter le répertoire parent comme répertoire d'inclusion
target_include_directories(from_benchmark PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)