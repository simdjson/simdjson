#
# Quickstart compile tests don't require any flags
#

# TODO run amalgamate first!

function(add_quickstart_test TEST_NAME SOURCE_FILE)
  # Second argument is C++ standard name
  if (${ARGV2})
    if (MSVC)
      set(QUICKSTART_FLAGS /std:${ARGV2})
    else()
      set(QUICKSTART_FLAGS -Werror -std=${ARGV2})
    endif()
  else()
    if(MSVC)
      set(QUICKSTART_FLAGS "")
    else()
      set(QUICKSTART_FLAGS -Werror)
    endif()
  endif()

  # Third argument tells whether to compile with -fno-exceptions
  if (${ARGV3})
    if (NOT MSVC)
      set(QUICKSTART_FLAGS "${QUICKSTART_FLAGS} -fno-exceptions")
    endif()
  endif()

  add_test(
    NAME ${TEST_NAME}
    COMMAND ${CMAKE_CXX_COMPILER} ${QUICKSTART_FLAGS} -I${PROJECT_SOURCE_DIR}/include -I${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/simdjson.cpp ${SOURCE_FILE}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/examples/quickstart
  )
  set_property(
    TEST ${TEST_NAME}
    APPEND PROPERTY DEPENDS simdjson-source ${PROJECT_SOURCE_DIR}/examples/quickstart/quickstart.cpp
  )
endfunction(add_quickstart_test)

if (SIMDJSON_EXCEPTIONS)
  add_quickstart_test(quickstart quickstart.cpp)
  add_quickstart_test(quickstart11 quickstart.cpp c++11)
  add_quickstart_test(quickstart14 quickstart.cpp c++14)
  set_property( TEST quickstart quickstart11 APPEND PROPERTY LABELS quicktests )
  set_property( TEST quickstart14            APPEND PROPERTY LABELS slowtests  )
endif()

add_quickstart_test(quickstart_noexceptions quickstart_noexceptions.cpp "" true)
add_quickstart_test(quickstart_noexceptions11 quickstart_noexceptions.cpp c++11 true)
set_property( TEST quickstart_noxceptions APPEND PROPERTY LABELS quicktests )
set_property( TEST quickstart_noexceptions11 APPEND PROPERTY LABELS slowtests )
