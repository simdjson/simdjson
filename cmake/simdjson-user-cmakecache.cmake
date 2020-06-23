#
# ${SIMDJSON_USER_CMAKECACHE} contains the *user-specified* simdjson options so you can call cmake on
# another branch or repository with the same options.
#
# Not supported on Windows at present, because the only thing that uses it is checkperf, which we
# don't run on Windows.
#
set(SIMDJSON_USER_CMAKECACHE ${CMAKE_CURRENT_BINARY_DIR}/.simdjson-user-CMakeCache.txt)
if (MSVC)
  add_custom_command(
    OUTPUT ${SIMDJSON_USER_CMAKECACHE}
    COMMAND findstr SIMDJSON_ ${PROJECT_BINARY_DIR}/CMakeCache.txt > ${SIMDJSON_USER_CMAKECACHE}.tmp
    COMMAND findstr /v SIMDJSON_LIB_ ${SIMDJSON_USER_CMAKECACHE}.tmp > ${SIMDJSON_USER_CMAKECACHE}
    VERBATIM # Makes it not do weird escaping with the command
  )
else()
  add_custom_command(
    OUTPUT ${SIMDJSON_USER_CMAKECACHE}
    COMMAND grep SIMDJSON_ ${PROJECT_BINARY_DIR}/CMakeCache.txt > ${SIMDJSON_USER_CMAKECACHE}.tmp
    COMMAND grep -v SIMDJSON_LIB_ ${SIMDJSON_USER_CMAKECACHE}.tmp > ${SIMDJSON_USER_CMAKECACHE}
    VERBATIM # Makes it not do weird escaping with the command
  )
endif()
add_custom_target(simdjson-user-cmakecache DEPENDS ${SIMDJSON_USER_CMAKECACHE})
