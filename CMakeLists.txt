cmake_minimum_required(VERSION 3.13)

project(
    simdjson
    # The version number is modified by tools/release.py
    VERSION 0.9.1
    DESCRIPTION "Parsing gigabytes of JSON per second"
    HOMEPAGE_URL "https://github.com/simdjson/simdjson"
    LANGUAGES CXX C
)

string(
    COMPARE EQUAL
    "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
    is_top_project
)

# ---- Options, variables ----

set(
    SIMDJSON_SEMANTIC_VERSION "${PROJECT_VERSION}"
    CACHE STRING "simdjson semantic version"
)
set(SIMDJSON_GITHUB_REPOSITORY "${PROJECT_HOMEPAGE_URL}")
# These version numbers are modified by tools/release.py
set(SIMDJSON_LIB_VERSION "8.0.0" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "8" CACHE STRING "simdjson library soversion")

if(is_top_project)
  option(SIMDJSON_DEVELOPER_MODE "Enable targets for developing simdjson" OFF)
endif()

# ---- simdjson library ----

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(windows)

# ---- Install rules ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(FILES singleheader/simdjson.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/simdjson-config.cmake.in"
      "${PROJECT_BINARY_DIR}/simdjson-config.cmake"
      INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/simdjson"
      NO_SET_AND_CHECK_MACRO
      NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(
      "${PROJECT_BINARY_DIR}/simdjson-config-version.cmake"
      VERSION ${SIMDJSON_SEMANTIC_VERSION}
      COMPATIBILITY SameMinorVersion)
install(FILES "${PROJECT_BINARY_DIR}/simdjson-config.cmake"
              "${PROJECT_BINARY_DIR}/simdjson-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/simdjson")

#
# CPack
#
if(is_top_project)
  set(CPACK_PACKAGE_VENDOR "Daniel Lemire")
  set(CPACK_PACKAGE_CONTACT "lemire@gmail.com")
  set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
  set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

  set(CPACK_RPM_PACKAGE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

  set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")

  include(CPack)
endif()

# ---- Developer mode extras ----

if(NOT SIMDJSON_DEVELOPER_MODE)
  return()
endif()

set(
    SIMDJSON_USER_CMAKECACHE
    "${CMAKE_BINARY_DIR}/.simdjson-user-CMakeCache.txt"
)
add_custom_target(
    simdjson-user-cmakecache
    COMMAND "${CMAKE_COMMAND}"
    -D "BINARY_DIR=${CMAKE_BINARY_DIR}"
    -D "USER_CMAKECACHE=${SIMDJSON_USER_CMAKECACHE}"
    -P "${PROJECT_SOURCE_DIR}/cmake/simdjson-user-cmakecache.cmake"
    VERBATIM
)

include(cmake/simdjson-flags.cmake)
# Setup tests
enable_testing()
# So we can build just tests with "make all_tests"
add_custom_target(all_tests)
add_subdirectory(jsonchecker)
add_subdirectory(jsonexamples)
add_library(test-data INTERFACE)
target_link_libraries(test-data INTERFACE jsonchecker-data jsonchecker-minefield-data jsonexamples-data)

add_subdirectory(dependencies) ## This needs to be before tools because of cxxopts
add_subdirectory(tools)  ## This needs to be before tests because of cxxopts
add_subdirectory(singleheader)

#
# Compile tools / tests / benchmarks
#
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmark)
add_subdirectory(fuzz)

#
# Source files should be just ASCII
#
find_program(FIND find)
find_program(FILE file)
find_program(GREP grep)
if(FIND AND FILE AND GREP)
  add_test(
      NAME just_ascii
      COMMAND sh -c "${FIND}  include src windows tools singleheader tests examples benchmark -path benchmark/checkperf-reference -prune -name '*.h'  -o -name '*.cpp' -type f  -exec ${FILE} '{}' \; |${GREP} -v ASCII || exit 0  && exit 1"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
endif()
