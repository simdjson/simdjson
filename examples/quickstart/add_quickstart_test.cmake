# TODO run amalgamate first!
function(add_quickstart_test TEST_NAME SOURCE_FILE)
  cmake_parse_arguments(PARSE_ARGV 2 ARGS "NO_EXCEPTIONS;NATIVE_ARCH" "" "CXX_STANDARD;LABELS")

  # Standard compiler flags
  if (MSVC)
    list(APPEND QUICKSTART_FLAGS /WX)
  else()
    list(APPEND QUICKSTART_FLAGS -Werror)
  endif()

  # Native architecture compiler flag
  if (ARGS_NATIVE_ARCH)
    if (MSVC)
      message(ERROR "MSVC quickstart native compile not supported because we don't know the right flag to pass to MSVC yet")
    else()
      list(APPEND QUICKSTART_FLAGS -march=native)
    endif()
  endif()

  # C++ standard compiler flag
  if (MSVC)
    if (ARGS_CXX_STANDARD)
      list(APPEND QUICKSTART_FLAGS /std:${ARGS_CXX_STANDARD})
    endif()
  else()
    if (ARGS_CXX_STANDARD)
      list(APPEND QUICKSTART_FLAGS -std=${ARGS_CXX_STANDARD})
    endif()
  endif()

  # No Exceptions compiler flag
  if (ARGS_NO_EXCEPTIONS)
    if (NOT MSVC)
      list(APPEND QUICKSTART_FLAGS -fno-exceptions)
    endif()
  endif()

  add_test(
    NAME ${TEST_NAME}
    COMMAND ${CMAKE_CXX_COMPILER} ${QUICKSTART_FLAGS} -I${PROJECT_SOURCE_DIR}/include -I${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/simdjson.cpp ${SOURCE_FILE}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/examples/quickstart
  )
  list(APPEND ARGS_LABELS quickstart compile no_mingw)
  set_property(TEST ${TEST_NAME} APPEND PROPERTY LABELS ${ARGS_LABELS})
  set_property(
    TEST ${TEST_NAME}
    APPEND PROPERTY DEPENDS simdjson-source ${PROJECT_SOURCE_DIR}/examples/quickstart/${SOURCE_FILE}
  )
endfunction(add_quickstart_test)
