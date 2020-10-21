# Initializes a git submodule if it hasn't been initialized before


find_package(Git QUIET) # We want the library to build even if git is missing
if ((Git_FOUND) AND SIMDJSON_GIT AND (SIMDJSON_IS_UNDER_GIT))
  message(STATUS "Git is available.")
  # Does NOT attempt to update or otherwise modify git submodules that are already initialized.
  function(initialize_submodule DIRECTORY)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/.git)
      message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/.git does not exist. Initializing ${DIRECTORY} submodule ...")
      execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                      RESULT_VARIABLE GIT_EXIT_CODE)
      if(NOT GIT_EXIT_CODE EQUAL "0")
        message(FATAL_ERROR "${GIT_EXECUTABLE} submodule update --init dependencies/${DIRECTORY} failed with exit code ${GIT_EXIT_CODE}, please checkout submodules")
      endif()
    endif()
  endfunction(initialize_submodule)

  if (SIMDJSON_GOOGLE_BENCHMARKS)
    message (STATUS "'SIMDJSON_GOOGLE_BENCHMARKS' is requested, configuring..." )
    option(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    option(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    initialize_submodule(benchmark)
    add_subdirectory(benchmark)
  endif()

  if (SIMDJSON_COMPETITION)
    initialize_submodule(cJSON)
    add_library(competition-cJSON INTERFACE)
    target_include_directories(competition-cJSON INTERFACE cJSON)

    initialize_submodule(fastjson)
    add_library(competition-fastjson INTERFACE)
    target_include_directories(competition-fastjson INTERFACE fastjson/src fastjson/include)

    initialize_submodule(gason)
    add_library(competition-gason INTERFACE)
    target_include_directories(competition-gason INTERFACE gason/src)

    initialize_submodule(jsmn)
    add_library(competition-jsmn INTERFACE)
    target_include_directories(competition-jsmn INTERFACE jsmn)

    initialize_submodule(json)
    add_library(competition-json INTERFACE)
    target_include_directories(competition-json INTERFACE json/single_include)

    initialize_submodule(json11)
    add_library(competition-json11 INTERFACE)
    target_include_directories(competition-json11 INTERFACE json11)

    add_library(competition-jsoncppdist INTERFACE)
    target_include_directories(competition-jsoncppdist INTERFACE jsoncppdist)

    initialize_submodule(rapidjson)
    add_library(competition-rapidjson INTERFACE)
    target_include_directories(competition-rapidjson INTERFACE rapidjson/include)

    initialize_submodule(sajson)
    add_library(competition-sajson INTERFACE)
    target_include_directories(competition-sajson INTERFACE sajson/include)

    initialize_submodule(ujson4c)
    add_library(competition-ujson4c ujson4c/src/ujdecode.c)
    target_include_directories(competition-ujson4c PUBLIC ujson4c/3rdparty ujson4c/src)

    initialize_submodule(boost.json)
    add_library(boostjson boost.json/src/src.cpp)
    target_compile_definitions(boostjson PUBLIC BOOST_JSON_STANDALONE)
    target_include_directories(boostjson PUBLIC boost.json/include)

    initialize_submodule(yyjson)
    add_library(yyjson yyjson/src/yyjson.c)
    target_include_directories(yyjson PUBLIC yyjson/src)

    add_library(competition-core INTERFACE)
    target_link_libraries(competition-core INTERFACE competition-json competition-rapidjson competition-sajson competition-cJSON competition-jsmn boostjson yyjson)

    add_library(competition-all INTERFACE)
    target_link_libraries(competition-all INTERFACE competition-core competition-jsoncppdist competition-json11 competition-fastjson competition-gason competition-ujson4c)
  endif()

  initialize_submodule(cxxopts)
  message(STATUS "We acquired cxxopts and we are adding it as a library and target.")
  add_library(cxxopts INTERFACE)
  target_include_directories(cxxopts INTERFACE cxxopts/include)
else()
  message(STATUS "Git is unavailable.")
  if(SIMDJSON_COMPETITION)
    message (STATUS "'SIMDJSON_COMPETITION' is requested, but we cannot download the remote repositories." )
  endif()
  if(SIMDJSON_GOOGLE_BENCHMARKS)
    message (STATUS "'SIMDJSON_GOOGLE_BENCHMARKS' is requested, but we cannot download the remote repositories." )
  endif()
endif()
