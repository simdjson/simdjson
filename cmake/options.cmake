if(MSVC)
  option(SIMDJSON_VISUAL_STUDIO_BUILD_WITH_DEBUG_INFO_FOR_PROFILING "Under Visual Studio, add Zi to the compile flag and DEBUG to the link file to add debugging information to the release build for easier profiling inside tools like VTune" OFF)
  if(SIMDJSON_VISUAL_STUDIO_BUILD_WITH_DEBUG_INFO_FOR_PROFILING)
    target_link_options(simdjson PUBLIC /DEBUG)
    target_compile_options(simdjson PUBLIC /Zi)
  endif()
else()
  set_property(TARGET simdjson PROPERTY POSITION_INDEPENDENT_CODE
          "$<STREQUAL:$<TARGET_PROPERTY:simdjson,TYPE>,SHARED_LIBRARY>")
endif()

# To have a default when no value is given this must be a STRING type variable
set(SIMDJSON_ONDEMAND_SAFETY_RAILS "" CACHE STRING "\
Validate ondemand user code at runtime to ensure it is being used correctly. \
Defaults to ON for debug builds, OFF for release builds.")

if(SIMDJSON_ONDEMAND_SAFETY_RAILS STREQUAL "")
  target_compile_definitions(simdjson PUBLIC $<$<CONFIG:Debug>:SIMDJSON_ONDEMAND_SAFETY_RAILS>)
elseif(SIMDJSON_ONDEMAND_SAFETY_RAILS)
  target_compile_definitions(simdjson PUBLIC SIMDJSON_ONDEMAND_SAFETY_RAILS)
endif()

option(SIMDJSON_EXCEPTIONS "Enable simdjson's exception-throwing interface" ON)
if(NOT SIMDJSON_EXCEPTIONS)
  target_compile_definitions(simdjson PUBLIC SIMDJSON_EXCEPTIONS=0)
endif()

option(SIMDJSON_ENABLE_THREADS "Link with thread support" ON)
if(SIMDJSON_ENABLE_THREADS)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  target_link_libraries(simdjson PUBLIC Threads::Threads)
  target_compile_definitions(simdjson PUBLIC SIMDJSON_THREADS_ENABLED=1)
endif()

option(SIMDJSON_VERBOSE_LOGGING, "Enable verbose logging for internal simdjson library development." OFF)
if (SIMDJSON_VERBOSE_LOGGING)
  target_compile_definitions(simdjson PUBLIC SIMDJSON_VERBOSE_LOGGING=1)
endif()
