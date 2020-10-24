cmake_minimum_required(VERSION 3.12)

project(simdjson
        VERSION 0.6.0
        DESCRIPTION "Parsing gigabytes of JSON per second"
        HOMEPAGE_URL "https://github.com/simdjson/simdjson"
        LANGUAGES CXX C)

set(SIMDJSON_SEMANTIC_VERSION "${PROJECT_VERSION}" CACHE STRING "simdjson semantic version")
set(SIMDJSON_LIB_VERSION "4.0.0" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "4" CACHE STRING "simdjson library soversion")

add_library(simdjson-headers INTERFACE)
target_compile_features(simdjson-headers INTERFACE cxx_std_11)
target_include_directories(simdjson-headers
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

include(cmake/sources.cmake)

add_library(simdjson ${sources})
add_library(simdjson::simdjson ALIAS simdjson)

set_target_properties(simdjson PROPERTIES
        VERSION ${SIMDJSON_LIB_VERSION}
        SOVERSION ${SIMDJSON_LIB_SOVERSION}
        WINDOWS_EXPORT_ALL_SYMBOLS YES)

add_subdirectory(windows)

include(cmake/implementations.cmake)
include(cmake/options.cmake)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(simdjson_directory "simdjson-${PROJECT_VERSION}")
set(simdjson_include_directory
        "${CMAKE_INSTALL_INCLUDEDIR}/${simdjson_directory}")

install(TARGETS simdjson
        EXPORT simdjsonTargets
        RUNTIME #
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT simdjson_Runtime
        LIBRARY #
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT simdjson_Runtime
        NAMELINK_COMPONENT simdjson_Development
        ARCHIVE #
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT simdjson_Development
        INCLUDES #
        DESTINATION "${simdjson_include_directory}")

set(simdjson_install_cmakedir
        "${CMAKE_INSTALL_LIBDIR}/cmake/${simdjson_directory}")

write_basic_package_version_file(
        simdjsonConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

install(EXPORT simdjsonTargets
        FILE simdjson-config.cmake
        NAMESPACE simdjson::
        DESTINATION "${simdjson_install_cmakedir}")

install(FILES
        "${PROJECT_SOURCE_DIR}/cmake/simdjsonConfig.cmake"
        "${PROJECT_BINARY_DIR}/simdjsonConfigVersion.cmake"
        DESTINATION "${simdjson_install_cmakedir}")

include(cmake/cpack.cmake)
