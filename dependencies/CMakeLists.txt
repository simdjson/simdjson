# Init submodules so they can be used
find_package(Git QUIET)
if(GIT_FOUND)
  option(SIMDJSON_GIT_SUBMODULE "Check submodules during build" ON)
  if(SIMDJSON_GIT_SUBMODULE)
    message(STATUS "Updating submodules")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_EXIT_CODE)
    if(NOT GIT_EXIT_CODE EQUAL "0")
      message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
  endif()
endif()

if (SIMDJSON_GOOGLE_BENCHMARKS)
  option(BENCHMARK_ENABLE_TESTING OFF)
  set(BENCHMARK_ENABLE_TESTING OFF)
  option(BENCHMARK_ENABLE_INSTALL OFF)
  set(BENCHMARK_ENABLE_INSTALL OFF)
  add_subdirectory(benchmark)
endif()

add_library(competition-cJSON INTERFACE)
target_include_directories(competition-cJSON INTERFACE cJSON)

add_library(competition-fastjson INTERFACE)
target_include_directories(competition-fastjson INTERFACE fastjson/src fastjson/include)

add_library(competition-gason INTERFACE)
target_include_directories(competition-gason INTERFACE gason/src)

add_library(competition-jsmn INTERFACE)
target_include_directories(competition-jsmn INTERFACE jsmn)

add_library(competition-json INTERFACE)
target_include_directories(competition-json INTERFACE json/single_include)

add_library(competition-json11 INTERFACE)
target_include_directories(competition-json11 INTERFACE json11)

add_library(competition-jsoncppdist INTERFACE)
target_include_directories(competition-jsoncppdist INTERFACE jsoncppdist)

add_library(competition-rapidjson INTERFACE)
target_include_directories(competition-rapidjson INTERFACE rapidjson/include)

add_library(competition-sajson INTERFACE)
target_include_directories(competition-sajson INTERFACE sajson/include)

add_library(competition-ujson4c ujson4c/src/ujdecode.c)
target_include_directories(competition-ujson4c PUBLIC ujson4c/3rdparty ujson4c/src)

add_library(competition-core INTERFACE)
target_link_libraries(competition-core INTERFACE competition-json competition-rapidjson competition-sajson competition-cJSON competition-jsmn)

add_library(competition-all INTERFACE)
target_link_libraries(competition-all INTERFACE competition-core competition-jsoncppdist competition-json11 competition-fastjson competition-gason competition-ujson4c)
