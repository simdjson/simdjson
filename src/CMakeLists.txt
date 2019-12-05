if(${CMAKE_C_COMPILER_ID} MATCHES "Intel") # icc / icpc
  # prevent shared libraries from depending on Intel provided libraries
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-intel")
endif()

include(GNUInstallDirs)

# we default on a shared library.
if(SIMDJSON_BUILD_STATIC)
  set(SIMDJSON_LIB_TYPE STATIC)
  MESSAGE( STATUS "Building a static library." )
else()
  MESSAGE( STATUS "Building a dynamic library (default)." )
  set(SIMDJSON_LIB_TYPE SHARED)
endif()

MESSAGE( STATUS "SIMDJSON_LIB_TYPE: " ${SIMDJSON_LIB_TYPE})

# Bring in include files
include(../include/CMakeLists.txt)

set(SIMDJSON_SRC_DIR $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)

set(SIMDJSON_SRC
  jsonioutil.cpp
  jsonminifier.cpp
  jsonparser.cpp
  jsonstream.cpp
  stage1_find_marks.cpp
  stage2_build_tape.cpp
  parsedjson.cpp
  parsedjsoniterator.cpp
  simdjson.cpp
)

# Load headers and sources
set(SIMDJSON_SRC_HEADERS
  jsoncharutils.h
  numberparsing.h
  simdprune_tables.h
  arm64/bitmask.h
  arm64/simd.h
  arm64/stage1_find_marks.h
  arm64/stage2_build_tape.h
  arm64/stringparsing.h
  generic/stage1_find_marks.h
  generic/stage2_build_tape.h
  generic/stage2_streaming_build_tape.h
  generic/stringparsing.h
  generic/utf8_fastvalidate_algorithm.h
  generic/utf8_lookup_algorithm.h
  generic/utf8_lookup2_algorithm.h
  generic/utf8_range_algorithm.h
  generic/utf8_zwegner_algorithm.h
  haswell/bitmask.h
  haswell/simd.h
  haswell/stage1_find_marks.h
  haswell/stage2_build_tape.h
  haswell/stringparsing.h
  westmere/bitmask.h
  westmere/simd.h
  westmere/stage1_find_marks.h
  westmere/stage2_build_tape.h
  westmere/stringparsing.h
)

add_library(${SIMDJSON_LIB_NAME} ${SIMDJSON_LIB_TYPE} ${SIMDJSON_SRC} ${SIMDJSON_INCLUDE} ${SIMDJSON_SRC_HEADERS})

target_include_directories(${SIMDJSON_LIB_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${SIMDJSON_SRC_DIR}>
  $<BUILD_INTERFACE:${SIMDJSON_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS ${SIMDJSON_LIB_NAME}
  EXPORT ${SIMDJSON_LIB_NAME}-config
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT ${SIMDJSON_LIB_NAME}-config
  FILE ${SIMDJSON_LIB_NAME}-config.cmake
  NAMESPACE ${SIMDJSON_LIB_NAME}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${SIMDJSON_LIB_NAME}
)

if(NOT MSVC)
## We output the library at the root of the current directory where cmake is invoked
## This is handy but Visual Studio will happily ignore us
set_target_properties(${SIMDJSON_LIB_NAME} PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  VERSION ${SIMDJSON_LIB_VERSION}
  SOVERSION ${SIMDJSON_LIB_SOVERSION})
MESSAGE( STATUS "Library output directory (does not apply to Visual Studio): " ${CMAKE_BINARY_DIR})
endif()

if(MSVC AND (SIMDJSON_LIB_TYPE STREQUAL "SHARED"))
  if (CMAKE_VERSION VERSION_LESS 3.4)
    MESSAGE( STATUS "To build  a Windows DLL using Visual Studio, you may need cmake 3.4 or better." )
  endif()
  MESSAGE( STATUS "Building a Windows DLL using Visual Studio, exporting all symbols automatically." )
 set_target_properties(${SIMDJSON_LIB_NAME}
    PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)
endif()

if(SIMDJSON_ENABLE_THREADS)
  find_package(Threads REQUIRED)
  target_link_libraries( ${SIMDJSON_LIB_NAME} Threads::Threads)
endif()

