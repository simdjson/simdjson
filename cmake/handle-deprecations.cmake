if(DEFINED SIMDJSON_BUILD_STATIC)
  message(DEPRECATION "SIMDJSON_BUILD_STATIC is deprecated, setting \
BUILD_SHARED_LIBS with its value and unsetting it")
  set(shared ON)
  if(SIMDJSON_BUILD_STATIC)
    set(shared OFF)
  endif()

  set(BUILD_SHARED_LIBS "${shared}" CACHE BOOL "" FORCE)
  unset(SIMDJSON_BUILD_STATIC CACHE)
endif()

if(DEFINED SIMDJSON_JUST_LIBRARY)
  message(DEPRECATION "SIMDJSON_JUST_LIBRARY is deprecated, setting \
SIMDJSON_DEVELOPER_MODE with its value and unsetting it")
  set(dev_mode ON)
  if(SIMDJSON_JUST_LIBRARY)
    set(dev_mode OFF)
  endif()

  set(SIMDJSON_DEVELOPER_MODE "${dev_mode}" CACHE BOOL "" FORCE)
  unset(SIMDJSON_JUST_LIBRARY CACHE)
endif()
