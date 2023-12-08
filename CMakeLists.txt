cmake_minimum_required(VERSION 3.14)

project(
    simdjson
    # The version number is modified by tools/release.py
    VERSION 3.6.3
    DESCRIPTION "Parsing gigabytes of JSON per second"
    HOMEPAGE_URL "https://simdjson.org/"
    LANGUAGES CXX C
)

set(SIMDJSON_GITHUB_REPOSITORY "https://github.com/simdjson/simdjson")

string(
    COMPARE EQUAL
    "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
    is_top_project
)

# ---- Options, variables ----

# These version numbers are modified by tools/release.py
set(SIMDJSON_LIB_VERSION "19.0.0" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "19" CACHE STRING "simdjson library soversion")

option(SIMDJSON_BUILD_STATIC_LIB "Build simdjson_static library along with simdjson" OFF)

option(SIMDJSON_ENABLE_THREADS "Link with thread support" ON)

include(cmake/simdjson-props.cmake)
include(cmake/implementation-flags.cmake)
include(cmake/exception-flags.cmake)

option(SIMDJSON_DISABLE_DEPRECATED_API "Disables deprecated APIs" OFF)
if(SIMDJSON_DISABLE_DEPRECATED_API)
  simdjson_add_props(
      target_compile_definitions PUBLIC
      SIMDJSON_DISABLE_DEPRECATED_API=1
  )
endif()

option(SIMDJSON_DEVELOPMENT_CHECKS "Enable development-time aids, such as \
checks for incorrect API usage. Enabled by default in DEBUG." OFF)
if(SIMDJSON_DEVELOPMENT_CHECKS)
  simdjson_add_props(
      target_compile_definitions PUBLIC
      SIMDJSON_DEVELOPMENT_CHECKS
  )
endif()

if(is_top_project)
  option(SIMDJSON_DEVELOPER_MODE "Enable targets for developing simdjson" OFF)
  option(BUILD_SHARED_LIBS "Build simdjson as a shared library" OFF)
endif()

include(cmake/handle-deprecations.cmake)
include(cmake/developer-options.cmake)

# ---- simdjson library ----

set(SIMDJSON_SOURCES src/simdjson.cpp)

add_library(simdjson ${SIMDJSON_SOURCES})
add_library(simdjson::simdjson ALIAS simdjson)
set(SIMDJSON_LIBRARIES simdjson)

if(SIMDJSON_BUILD_STATIC_LIB)
  add_library(simdjson_static STATIC ${SIMDJSON_SOURCES})
  add_library(simdjson::simdjson_static ALIAS simdjson_static)
  list(APPEND SIMDJSON_LIBRARIES simdjson_static)
endif()

set_target_properties(
    simdjson PROPERTIES
    VERSION "${SIMDJSON_LIB_VERSION}"
    SOVERSION "${SIMDJSON_LIB_SOVERSION}"
    # FIXME: symbols should be hidden by default
    WINDOWS_EXPORT_ALL_SYMBOLS YES
)

# FIXME: Use proper CMake integration for exports
if(MSVC AND BUILD_SHARED_LIBS)
  target_compile_definitions(
      simdjson
      PRIVATE SIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY=1
      INTERFACE SIMDJSON_USING_WINDOWS_DYNAMIC_LIBRARY=1
  )
endif()

simdjson_add_props(
    target_include_directories
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    PRIVATE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

simdjson_add_props(target_compile_features PUBLIC cxx_std_11)

# workaround for GNU GCC poor AVX load/store code generation
if(
    CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86(_64)?)$"
)
  simdjson_add_props(
      target_compile_options PRIVATE
      -mno-avx256-split-unaligned-load -mno-avx256-split-unaligned-store
  )
endif()

# GCC and Clang have horrendous Debug builds when using SIMD.
# A common fix is to use '-Og' instead.
# bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54412
if(
    (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
        CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
)
  message(STATUS "Adding -Og to compile flag")
  simdjson_add_props(
      target_compile_options PRIVATE
      $<$<CONFIG:DEBUG>:-Og>
  )
endif()

if(SIMDJSON_ENABLE_THREADS)
  find_package(Threads REQUIRED)
  simdjson_add_props(target_link_libraries PUBLIC Threads::Threads)
  simdjson_add_props(target_compile_definitions PUBLIC SIMDJSON_THREADS_ENABLED=1)
endif()

simdjson_apply_props(simdjson)
if(SIMDJSON_BUILD_STATIC_LIB)
  simdjson_apply_props(simdjson_static)
endif()

# ---- Install rules ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(
    FILES singleheader/simdjson.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT simdjson_Development
)

install(
    TARGETS simdjson
    EXPORT simdjsonTargets
    RUNTIME COMPONENT simdjson_Runtime
    LIBRARY COMPONENT simdjson_Runtime
    NAMELINK_COMPONENT simdjson_Development
    ARCHIVE COMPONENT simdjson_Development
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
configure_file(cmake/simdjson-config.cmake.in simdjson-config.cmake @ONLY)

write_basic_package_version_file(
    simdjson-config-version.cmake
    COMPATIBILITY SameMinorVersion
)

set(
    SIMDJSON_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/simdjson"
    CACHE STRING "CMake package config location relative to the install prefix"
)
mark_as_advanced(SIMDJSON_INSTALL_CMAKEDIR)

install(
    FILES
    "${PROJECT_BINARY_DIR}/simdjson-config.cmake"
    "${PROJECT_BINARY_DIR}/simdjson-config-version.cmake"
    DESTINATION "${SIMDJSON_INSTALL_CMAKEDIR}"
    COMPONENT simdjson_Development
)

install(
    EXPORT simdjsonTargets
    NAMESPACE simdjson::
    DESTINATION "${SIMDJSON_INSTALL_CMAKEDIR}"
    COMPONENT simdjson_Development
)

if(SIMDJSON_BUILD_STATIC_LIB)
  install(
      TARGETS simdjson_static
      EXPORT simdjson_staticTargets
      ARCHIVE COMPONENT simdjson_Development
  )
  install(
      EXPORT simdjson_staticTargets
      NAMESPACE simdjson::
      DESTINATION "${SIMDJSON_INSTALL_CMAKEDIR}"
      COMPONENT simdjson_Development
  )
endif()

# pkg-config
include(cmake/JoinPaths.cmake)
join_paths(PKGCONFIG_INCLUDEDIR "\${prefix}" "${CMAKE_INSTALL_INCLUDEDIR}")
join_paths(PKGCONFIG_LIBDIR "\${prefix}" "${CMAKE_INSTALL_LIBDIR}")

if(SIMDJSON_ENABLE_THREADS)
  set(PKGCONFIG_CFLAGS "-DSIMDJSON_THREADS_ENABLED=1")
  if(CMAKE_THREAD_LIBS_INIT)
    set(PKGCONFIG_LIBS_PRIVATE "Libs.private: ${CMAKE_THREAD_LIBS_INIT}")
  endif()
endif()

configure_file("simdjson.pc.in" "simdjson.pc" @ONLY)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/simdjson.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

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

if(is_top_project AND NOT SIMDJSON_DEVELOPER_MODE)
  message(STATUS "Building only the library. Advanced users and contributors may want to turn SIMDJSON_DEVELOPER_MODE to ON, e.g., via -D SIMDJSON_DEVELOPER_MODE=ON.")
elseif(SIMDJSON_DEVELOPER_MODE AND NOT is_top_project)
  message(AUTHOR_WARNING "Developer mode in simdjson is intended for the developers of simdjson")
endif()

if(NOT SIMDJSON_DEVELOPER_MODE)
  return()
endif()

simdjson_apply_props(simdjson-internal-flags)

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

# Setup tests
enable_testing()
# So we can build just tests with "make all_tests"
add_custom_target(all_tests)

add_subdirectory(windows)
add_subdirectory(dependencies) ## This needs to be before tools because of cxxopts
add_subdirectory(tools)  ## This needs to be before tests because of cxxopts

# Data: jsonexamples is left with only the bare essential.
# most of the data has been moved to https://github.com/simdjson/simdjson-data
add_subdirectory(jsonexamples)


add_subdirectory(singleheader)



#
# Compile tools / tests / benchmarks
#
add_subdirectory(tests)
add_subdirectory(examples)
if(CMAKE_SIZEOF_VOID_P EQUAL 8) # we only include the benchmarks on 64-bit systems.
  add_subdirectory(benchmark)
endif()
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
      COMMAND sh -c "\
${FIND} include src windows tools singleheader tests examples benchmark \
-path benchmark/checkperf-reference -prune -name '*.h' -o -name '*.cpp' \
-type f -exec ${FILE} '{}' \; | ${GREP} -qv ASCII || exit 0  && exit 1"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
endif()

##
## In systems like R, libraries must not use stderr or abort to be acceptable.
## Thus we make it a hard rule that one is not allowed to call abort or stderr.
## The sanitized builds are allowed to abort.
##
if(NOT SIMDJSON_SANITIZE)
  find_program(GREP grep)
  find_program(NM nm)
  if((NOT GREP) OR (NOT NM))
    message("grep and nm are unavailable on this system.")
  else()
    add_test(
      NAME "avoid_abort"
      # Under FreeBSD, the __cxa_guard_abort symbol may appear but it is fine.
      # So we want to look for <space><possibly _>abort as a test.
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdjson> |  ${GREP} ' _*abort' || exit 0  && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_cout"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdjson> |  ${GREP} ' _*cout' || exit 0  && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_cerr"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdjson> |  ${GREP} ' _*cerr' || exit 0  && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_printf"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdjson> |  ${GREP} ' _*printf' || exit 0  && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_stdout"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdjson> |  ${GREP} stdout || exit 0 && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_stderr"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdjson> |  ${GREP} stderr || exit 0 && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
  endif()
endif()
