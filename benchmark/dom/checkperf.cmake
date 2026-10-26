option(SIMDJSON_ENABLE_DOM_CHECKPERF "Enable DOM performance comparison with main branch" OFF)

if (SIMDJSON_ENABLE_DOM_CHECKPERF)
  find_package(Git QUIET)
  if (MSVC)
    message(STATUS "checkperf targets are disabled under MSVC.")
  elseif (NOT Git_FOUND OR (GIT_VERSION_STRING VERSION_LESS_EQUAL "2.1.4"))
    message(STATUS "Either git is unavailable or else it is too old (requires > 2.1.4). We are disabling checkperf targets.")
  else()
    message(STATUS "Git is available and it is recent. We are enabling checkperf targets.")
    function(sync_git_repository name dir remote branch url)
      add_custom_command(
        OUTPUT ${dir}/.git/config
        COMMAND ${GIT_EXECUTABLE} init ${dir}
        COMMAND ${GIT_EXECUTABLE} -C ${dir} remote add ${remote} ${url}
      )
      add_custom_target(init-${name} DEPENDS ${dir}/.git/config)
      # Depend on the generated file. Ninja does not order a custom command on a target.
      add_custom_command(
        OUTPUT ${dir}/.git/FETCH_HEAD
        COMMAND ${GIT_EXECUTABLE} remote set-url ${remote} ${url}
        COMMAND ${GIT_EXECUTABLE} fetch --depth=1 ${remote} ${branch}
        COMMAND ${GIT_EXECUTABLE} reset --hard ${remote}/${branch}
        WORKING_DIRECTORY ${dir}
        DEPENDS ${dir}/.git/config
      )
      add_custom_target(${name} DEPENDS ${dir}/.git/FETCH_HEAD)
      add_custom_target(
        update-${name}
        COMMAND ${GIT_EXECUTABLE} remote set-url ${remote} ${url}
        COMMAND ${GIT_EXECUTABLE} fetch --depth=1 ${remote} ${branch}
        COMMAND ${GIT_EXECUTABLE} reset --hard ${remote}/${branch}
        WORKING_DIRECTORY ${dir}
        DEPENDS ${dir}/.git/config
      )
    endfunction(sync_git_repository)

    set(SIMDJSON_CHECKPERF_REMOTE origin CACHE STRING "Remote repository to compare performance against")
    set(SIMDJSON_CHECKPERF_BRANCH master CACHE STRING "Branch to compare performance against")
    set(SIMDJSON_CHECKPERF_DIR ${CMAKE_CURRENT_BINARY_DIR}/checkperf-reference/${SIMDJSON_CHECKPERF_BRANCH} CACHE STRING "Location to put checkperf performance comparison repository")
    set(SIMDJSON_CHECKPERF_ARGS ${EXAMPLE_JSON} CACHE STRING "Arguments to pass to parse during checkperf")
    sync_git_repository(checkperf-repo ${SIMDJSON_CHECKPERF_DIR} ${SIMDJSON_CHECKPERF_REMOTE} ${SIMDJSON_CHECKPERF_BRANCH} ${SIMDJSON_GITHUB_REPOSITORY})

    add_custom_command(
      OUTPUT ${SIMDJSON_CHECKPERF_DIR}/build/CMakeCache.txt
      COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMDJSON_CHECKPERF_DIR}/build
      COMMAND ${CMAKE_COMMAND} -E copy ${SIMDJSON_USER_CMAKECACHE} ${SIMDJSON_CHECKPERF_DIR}/build/CMakeCache.txt
      DEPENDS ${SIMDJSON_CHECKPERF_DIR}/.git/FETCH_HEAD ${SIMDJSON_USER_CMAKECACHE}
    )
    add_custom_command(
      OUTPUT ${SIMDJSON_CHECKPERF_DIR}/build/cmake_install.cmake
      COMMAND
      ${CMAKE_COMMAND} -E env CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER}
      ${CMAKE_COMMAND}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DSIMDJSON_GOOGLE_BENCHMARKS=OFF
      -DSIMDJSON_COMPETITION=OFF
      -DSIMDJSON_DEVELOPER_MODE=YES
      -G ${CMAKE_GENERATOR}
      ..
      WORKING_DIRECTORY ${SIMDJSON_CHECKPERF_DIR}/build
      DEPENDS ${SIMDJSON_CHECKPERF_DIR}/build/CMakeCache.txt
    )

    if (CMAKE_CONFIGURATION_TYPES)
      set(CHECKPERF_PARSE ${SIMDJSON_CHECKPERF_DIR}/build/benchmark/$<CONFIGURATION>/parse)
    else()
      set(CHECKPERF_PARSE ${SIMDJSON_CHECKPERF_DIR}/build/benchmark/dom/parse)
    endif()
    add_custom_target(
      checkperf-parse
      COMMAND ${CMAKE_COMMAND} --build . --target parse --config $<CONFIGURATION>
      WORKING_DIRECTORY ${SIMDJSON_CHECKPERF_DIR}/build
      DEPENDS ${SIMDJSON_CHECKPERF_DIR}/build/cmake_install.cmake
    )

    add_custom_target(checkperf DEPENDS parse perfdiff checkperf-parse)

    add_test(
      NAME checkperf
      COMMAND $<TARGET_FILE:perfdiff> $<TARGET_FILE:parse> ${CHECKPERF_PARSE} -H -t ${SIMDJSON_CHECKPERF_ARGS}
    )
    set_property(TEST checkperf APPEND PROPERTY LABELS per_implementation explicitonly)
    set_property(TEST checkperf APPEND PROPERTY DEPENDS parse perfdiff ${SIMDJSON_USER_CMAKECACHE})
    set_property(TEST checkperf PROPERTY RUN_SERIAL TRUE)
    add_dependencies(per_implementation_tests checkperf)
    add_dependencies(explicitonly_tests checkperf)
  endif()
endif()
