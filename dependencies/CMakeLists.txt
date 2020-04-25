# Initializes a git submodule if it hasn't been initialized before
# Does NOT attempt to update or otherwise modify git submodules that are already initialized.
function(initialize_submodule dir)
  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/.git)
    find_package(Git QUIET REQUIRED)
    message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/.git does not exist. Initializing ${dir} submodule ...")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init ${dir}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_EXIT_CODE)
    if(NOT GIT_EXIT_CODE EQUAL "0")
      message(FATAL_ERROR "${GIT_EXECUTABLE} submodule update --init dependencies/${dir} failed with exit code ${GIT_EXIT_CODE}, please checkout submodules")
    endif()
  endif()
endfunction(initialize_submodule)

function(git_submodule_target dir)
  find_package(Git QUIET REQUIRED)
  # This conditionally creates the git repository
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/.git
    COMMAND ${GIT_EXECUTABLE} ${SIMDJSON_GIT_ARGS} submodule update --init --depth 1 ${dir}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_custom_target(${dir}-submodule DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/.git)
endfunction(git_submodule_target)

function(add_competition_submodule_library dir)
  git_submodule_target(${dir})
  add_library(competition-${dir} ${ARGN})
  add_dependencies(competition-${dir} ${dir}-submodule)
endfunction(add_competition_submodule_library)

if (SIMDJSON_GOOGLE_BENCHMARKS)
  option(BENCHMARK_ENABLE_TESTING OFF)
  set(BENCHMARK_ENABLE_TESTING OFF)
  option(BENCHMARK_ENABLE_INSTALL OFF)
  set(BENCHMARK_ENABLE_INSTALL OFF)
  initialize_submodule(benchmark)
  add_subdirectory(benchmark)
endif()

if (SIMDJSON_COMPETITION)
  add_competition_submodule_library(cJSON INTERFACE)
  target_include_directories(competition-cJSON INTERFACE cJSON)

  add_competition_submodule_library(fastjson INTERFACE)
  target_include_directories(competition-fastjson INTERFACE fastjson/src fastjson/include)

  add_competition_submodule_library(gason INTERFACE)
  target_include_directories(competition-gason INTERFACE gason/src)

  add_competition_submodule_library(jsmn INTERFACE)
  target_include_directories(competition-jsmn INTERFACE jsmn)

  add_competition_submodule_library(json INTERFACE)
  target_include_directories(competition-json INTERFACE json/single_include)

  add_competition_submodule_library(json11 INTERFACE)
  target_include_directories(competition-json11 INTERFACE json11)

  add_library(competition-jsoncppdist INTERFACE)
  target_include_directories(competition-jsoncppdist INTERFACE jsoncppdist)

  add_competition_submodule_library(rapidjson INTERFACE)
  target_include_directories(competition-rapidjson INTERFACE rapidjson/include)

  add_competition_submodule_library(sajson INTERFACE)
  target_include_directories(competition-sajson INTERFACE sajson/include)

  initialize_submodule(ujson4c) # ujdecode.c must exist at compile time to create the library in cmake
  add_competition_submodule_library(ujson4c ujson4c/src/ujdecode.c)
  target_include_directories(competition-ujson4c PUBLIC ujson4c/3rdparty ujson4c/src)

  add_library(competition-core INTERFACE)
  target_link_libraries(competition-core INTERFACE competition-json competition-rapidjson competition-sajson competition-cJSON competition-jsmn)

  add_library(competition-all INTERFACE)
  target_link_libraries(competition-all INTERFACE competition-core competition-jsoncppdist competition-json11 competition-fastjson competition-gason competition-ujson4c)
endif()