include_directories( . linux )
link_libraries(simdjson simdjson-flags simdjson-windows-headers)
# add_executable(benchfeatures benchfeatures.cpp) # doesn't presently compile at all
add_executable(get_corpus_benchmark get_corpus_benchmark.cpp)
add_executable(perfdiff perfdiff.cpp)
add_executable(parse parse.cpp)
add_executable(parse_stream parse_stream.cpp)
add_executable(statisticalmodel statisticalmodel.cpp)

add_executable(parse_noutf8validation parse.cpp)
target_compile_definitions(parse_noutf8validation PRIVATE SIMDJSON_SKIPUTF8VALIDATION)
add_executable(parse_nonumberparsing parse.cpp)
target_compile_definitions(parse_nonumberparsing PRIVATE SIMDJSON_SKIPNUMBERPARSING)
add_executable(parse_nostringparsing parse.cpp)
target_compile_definitions(parse_nostringparsing PRIVATE SIMDJSON_SKIPSTRINGPARSING)

if (SIMDJSON_GOOGLE_BENCHMARKS)
  link_libraries(benchmark::benchmark)
  add_executable(bench_parse_call bench_parse_call.cpp)
  add_executable(bench_dom_api bench_dom_api.cpp)
  target_link_libraries(bench_dom_api test-data)
endif()

if (SIMDJSON_COMPETITION)
  add_executable(distinctuseridcompetition distinctuseridcompetition.cpp)
  target_link_libraries(distinctuseridcompetition competition-core)
  add_executable(minifiercompetition minifiercompetition.cpp)
  target_link_libraries(minifiercompetition competition-core)
  add_executable(parseandstatcompetition parseandstatcompetition.cpp)
  target_link_libraries(parseandstatcompetition competition-core)
  add_executable(parsingcompetition parsingcompetition.cpp)
  target_link_libraries(parsingcompetition competition-core)
  add_executable(allparsingcompetition parsingcompetition.cpp)
  target_link_libraries(allparsingcompetition competition-all)
  target_compile_definitions(allparsingcompetition PRIVATE ALLPARSER)
endif()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_test(NAME checkperf
    COMMAND ${CMAKE_COMMAND} -E env
      CHECKPERF_REPOSITORY=https://github.com/simdjson/simdjson
      CHECKPERF_BRANCH=master
      CHECKPERF_DIR=${CMAKE_CURRENT_BINARY_DIR}/simdjson-master
      CHECKPERF_CMAKECACHE=${SIMDJSON_USER_CMAKECACHE}
      bash ${CMAKE_CURRENT_SOURCE_DIR}/checkperf.sh ${PROJECT_SOURCE_DIR}/jsonexamples/twitter.json)
  set_property(TEST checkperf APPEND PROPERTY LABELS per_implementation)
  set_property(TEST checkperf APPEND PROPERTY DEPENDS parse perfdiff ${SIMDJSON_USER_CMAKECACHE})
  set_property(TEST checkperf PROPERTY RUN_SERIAL TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
