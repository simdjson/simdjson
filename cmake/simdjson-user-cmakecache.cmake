#
# ${USER_CMAKECACHE} contains the *user-specified* simdjson options so you can
# call cmake on another branch or repository with the same options.
#

file(READ "${BINARY_DIR}/CMakeCache.txt" cache)
# Escape semicolons, so the lines can be safely iterated in CMake
string(REPLACE ";" "\\;" cache "${cache}")
# Turn the contents into a list
string(REPLACE "\n" ";" cache "${cache}")

message(STATUS "${USER_CMAKECACHE}")

file(REMOVE "${USER_CMAKECACHE}")
foreach(line IN LISTS cache)
  if(line MATCHES "^SIMDJSON_" AND NOT line MATCHES "^SIMDJSON_LIB_")
    file(APPEND "${USER_CMAKECACHE}" "${line}\n")
  endif()
endforeach()
