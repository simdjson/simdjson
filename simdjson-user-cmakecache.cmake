#
# ${SIMDJSON_USER_CMAKECACHE} contains the *user-specified* simdjson options so you can call cmake on
# another branch or repository with the same options.
#
# Not supported on Windows at present, because the only thing that uses it is checkperf, which we
# don't run on Windows.
#
if (NOT MSVC)
  set(SIMDJSON_USER_CMAKECACHE ${CMAKE_CURRENT_BINARY_DIR}/.simdjson-user-CMakeCache.txt)
  add_custom_command(
    OUTPUT ${SIMDJSON_USER_CMAKECACHE}
    COMMAND bash -c "grep SIMDJSON_ ${PROJECT_BINARY_DIR}/CMakeCache.txt | grep -v SIMDJSON_LIB_ > ${SIMDJSON_USER_CMAKECACHE}"
    VERBATIM # Makes it not do weird escaping with the command
  )
  add_custom_target(simdjson-user-cmakecache ALL DEPENDS ${SIMDJSON_USER_CMAKECACHE})
endif()
