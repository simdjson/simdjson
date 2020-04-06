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
