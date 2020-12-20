if (TARGET benchmark::benchmark)
  link_libraries(benchmark::benchmark)
  add_executable(bench_ondemand_largerandom bench_ondemand_largerandom.cpp)
endif()
