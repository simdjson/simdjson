#
# Quickstart compile tests don't require any flags
#

# TODO haven't quite decided the right way to run quickstart on Windows. Needs README update.
#
# Note: on macOS and other platforms, the 'command' described below may not work even if the cmake builds.
# For example, it may be necessary to specify the sysroot, which CMake does, but the 'command' does not
# handle such niceties. On a case-by-case basis it is fixable but it requires work that CMake knows how
# to do but that is not trivial.
#
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  # TODO run amalgamate first!
  function(add_quickstart_test TEST_NAME SOURCE_FILE)
    # Second argument is C++ standard name
    if (MSVC)
      if (ARGV2)
        set(QUICKSTART_FLAGS /std:${ARGV2})
      else()
        set(QUICKSTART_FLAGS /WX)
      endif()
      set(QUICKSTART_INCLUDE /I${PROJECT_SOURCE_DIR}/include /I${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/simdjson.cpp)
    else()
      if (ARGV2)
        set(QUICKSTART_FLAGS -Werror -std=${ARGV2})
      else()
        set(QUICKSTART_FLAGS -Werror)
      endif()
      set(QUICKSTART_INCLUDE -I${PROJECT_SOURCE_DIR}/include -I${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/simdjson.cpp)
    endif()

    # Third argument tells whether to compile with -fno-exceptions
    if (ARGV3)
      if (NOT MSVC)
        set(QUICKSTART_FLAGS ${QUICKSTART_FLAGS} -fno-exceptions)
      endif()
    endif()

    add_test(
      NAME ${TEST_NAME}
      COMMAND ${CMAKE_CXX_COMPILER} ${QUICKSTART_FLAGS} -I${PROJECT_SOURCE_DIR}/include -I${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/simdjson.cpp ${SOURCE_FILE}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/examples/quickstart
    )
    set_property(
      TEST ${TEST_NAME}
      APPEND PROPERTY DEPENDS simdjson-source ${PROJECT_SOURCE_DIR}/examples/quickstart/${SOURCE_FILE}
    )
  endfunction(add_quickstart_test)

  if (SIMDJSON_EXCEPTIONS)
    add_quickstart_test(quickstart quickstart.cpp)
    add_quickstart_test(quickstart11 quickstart.cpp c++11)
    add_quickstart_test(quickstart14 quickstart.cpp c++14)
    set_property( TEST quickstart quickstart11 APPEND PROPERTY LABELS acceptance compiletests )
  endif()

  add_quickstart_test(quickstart_noexceptions quickstart_noexceptions.cpp "" true)
  add_quickstart_test(quickstart_noexceptions11 quickstart_noexceptions.cpp c++11 true)
  set_property( TEST quickstart_noexceptions APPEND PROPERTY LABELS acceptance compile )
endif()
