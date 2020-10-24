cmake_minimum_required(VERSION 3.12)

project(simdjson
        VERSION 0.6.0
        DESCRIPTION "Parsing gigabytes of JSON per second"
        HOMEPAGE_URL "https://github.com/simdjson/simdjson"
        LANGUAGES CXX C)

set(SIMDJSON_SEMANTIC_VERSION "${PROJECT_VERSION}" CACHE STRING "simdjson semantic version")
set(SIMDJSON_LIB_VERSION "4.0.0" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "4" CACHE STRING "simdjson library soversion")

include(GNUInstallDirs)
include(cmake/simdjson-flags.cmake)
include(cmake/simdjson-user-cmakecache.cmake)


# Create the top level simdjson library (must be done at this level to use both src/ and include/
# directories) and tools
#
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(windows)

if(MSVC)
  option(SIMDJSON_VISUAL_STUDIO_BUILD_WITH_DEBUG_INFO_FOR_PROFILING "Under Visual Studio, add Zi to the compile flag and DEBUG to the link file to add debugging information to the release build for easier profiling inside tools like VTune" OFF)
  if(SIMDJSON_VISUAL_STUDIO_BUILD_WITH_DEBUG_INFO_FOR_PROFILING)
    target_link_options(simdjson PUBLIC /DEBUG)
    target_compile_options(simdjson PUBLIC /Zi)
  endif()
else()
  set_property(TARGET simdjson PROPERTY POSITION_INDEPENDENT_CODE YES)
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

install(FILES singleheader/simdjson.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#
# Compile tools / tests / benchmarks
#




#
# CPack
#
set(CPACK_PACKAGE_VENDOR "Daniel Lemire")
set(CPACK_PACKAGE_CONTACT "lemire@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_RPM_PACKAGE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")

include(CPack)
