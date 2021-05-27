# Relevant targets:
# checkperf-parse: builds the reference checkperf-parse, syncing reference repository if needed
# checkperf: builds the targets needed for checkperf (parse, perfdiff, checkperf-parse)
# update-checkperf-repo: updates the reference repository we're checking performance against
# checkperf-repo: initialize and sync reference repository (first time only)
# TEST checkperf: runs the actual checkperf test

option(SIMDJSON_ENABLE_DOM_CHECKPERF "Enable DOM performance comparison with main branch" OFF)


# Clone the repository if it's not there
find_package(Git QUIET)
if (SIMDJSON_ENABLE_DOM_CHECKPERF AND Git_FOUND AND (GIT_VERSION_STRING VERSION_GREATER  "2.1.4") AND (NOT CMAKE_GENERATOR MATCHES Ninja) AND (NOT MSVC) ) # We use "-C" which requires a recent git
  message(STATUS "Git is available and it is recent. We are enabling checkperf targets.")
  # sync_git_repository(myrepo ...) creates two targets:
  # myrepo - if the repo does not exist, creates and syncs it against the origin branch
  # update_myrepo - will update the repo against the origin branch (and create if needed)
  function(sync_git_repository name dir remote branch url)
    # This conditionally creates the git repository
    add_custom_command(
      OUTPUT ${dir}/.git/config
      COMMAND ${GIT_EXECUTABLE} init ${dir}
      COMMAND ${GIT_EXECUTABLE} -C ${dir} remote add ${remote} ${url}
    )
    add_custom_target(init-${name} DEPENDS ${dir}/.git/config)
    # This conditionally syncs the git repository, first time only
    add_custom_command(
      OUTPUT ${dir}/.git/FETCH_HEAD
      COMMAND ${GIT_EXECUTABLE} remote set-url ${remote} ${url}
      COMMAND ${GIT_EXECUTABLE} fetch --depth=1 ${remote} ${branch}
      COMMAND ${GIT_EXECUTABLE} reset --hard ${remote}/${branch}
      WORKING_DIRECTORY ${dir}
      DEPENDS init-${name}
    )
    # This is the ${name} target, which will create and sync the repo first time only
    add_custom_target(${name} DEPENDS ${dir}/.git/FETCH_HEAD)
    # This is the update-${name} target, which will sync the repo (creating it if needed)
    add_custom_target(
      update-${name}
      COMMAND ${GIT_EXECUTABLE} remote set-url ${remote} ${url}
      COMMAND ${GIT_EXECUTABLE} fetch --depth=1 ${remote} ${branch}
      COMMAND ${GIT_EXECUTABLE} reset --hard ${remote}/${branch}
      WORKING_DIRECTORY ${dir}
      DEPENDS init-${name}
    )
  endfunction(sync_git_repository)

  set(SIMDJSON_CHECKPERF_REMOTE origin CACHE STRING "Remote repository to compare performance against")
  set(SIMDJSON_CHECKPERF_BRANCH master CACHE STRING "Branch to compare performance against")
  set(SIMDJSON_CHECKPERF_DIR ${CMAKE_CURRENT_BINARY_DIR}/checkperf-reference/${SIMDJSON_CHECKPERF_BRANCH} CACHE STRING "Location to put checkperf performance comparison repository")
  set(SIMDJSON_CHECKPERF_ARGS ${EXAMPLE_JSON} CACHE STRING "Arguments to pass to parse during checkperf")
  sync_git_repository(checkperf-repo ${SIMDJSON_CHECKPERF_DIR} ${SIMDJSON_CHECKPERF_REMOTE} ${SIMDJSON_CHECKPERF_BRANCH} ${SIMDJSON_GITHUB_REPOSITORY})

  # Commands to cause cmake on benchmark/checkperf-master/build/
  # - first, copy CMakeCache.txt
  add_custom_command(
    OUTPUT ${SIMDJSON_CHECKPERF_DIR}/build/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMDJSON_CHECKPERF_DIR}/build
    COMMAND ${CMAKE_COMMAND} -E copy ${SIMDJSON_USER_CMAKECACHE} ${SIMDJSON_CHECKPERF_DIR}/build/CMakeCache.txt
    DEPENDS checkperf-repo simdjson-user-cmakecache
  )
  # - second, cmake ..
  add_custom_command(
    OUTPUT ${SIMDJSON_CHECKPERF_DIR}/build/cmake_install.cmake # We make many things but this seems the most cross-platform one we can depend on
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

  # - third, build parse.
  if (CMAKE_CONFIGURATION_TYPES)
    set(CHECKPERF_PARSE ${SIMDJSON_CHECKPERF_DIR}/build/benchmark/$<CONFIGURATION>/parse)
  else()
    set(CHECKPERF_PARSE ${SIMDJSON_CHECKPERF_DIR}/build/benchmark/dom/parse)
  endif()
  add_custom_target(
    checkperf-parse ALL # TODO is ALL necessary?
    # Build parse
    COMMAND ${CMAKE_COMMAND} --build . --target parse --config $<CONFIGURATION>
    WORKING_DIRECTORY ${SIMDJSON_CHECKPERF_DIR}/build
    DEPENDS ${SIMDJSON_CHECKPERF_DIR}/build/cmake_install.cmake # We make many things but this seems the most cross-platform one we can depend on
  )

  # Target to build everything needed for the checkperf test
  add_custom_target(checkperf DEPENDS parse perfdiff checkperf-parse)

  # Add the actual checkperf test
  add_test(
    NAME checkperf
    # COMMAND ECHO $<TARGET_FILE:perfdiff> \"$<TARGET_FILE:parse> -t ${SIMDJSON_CHECKPERF_ARGS}\" \"${CHECKPERF_PARSE} -t ${SIMDJSON_CHECKPERF_ARGS}\" }
    COMMAND $<TARGET_FILE:perfdiff> $<TARGET_FILE:parse> ${CHECKPERF_PARSE} -H -t ${SIMDJSON_CHECKPERF_ARGS}
  )
  set_property(TEST checkperf APPEND PROPERTY LABELS per_implementation explicitonly)
  set_property(TEST checkperf APPEND PROPERTY DEPENDS parse perfdiff ${SIMDJSON_USER_CMAKECACHE})
  set_property(TEST checkperf PROPERTY RUN_SERIAL TRUE)
  add_dependencies(per_implementation_tests checkperf)
  add_dependencies(explicitonly_tests checkperf)
else()
  if (CMAKE_GENERATOR MATCHES Ninja)
   message(STATUS "We disable the checkperf targets under Ninja.")
  else()
   message(STATUS "Either git is unavailable or else it is too old. We are disabling checkperf targets.")
  endif()
endif ()
