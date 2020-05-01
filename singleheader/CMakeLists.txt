#
# Amalgamation
#
# We should check whether bash is available here and avoid failures on systems where
# bash is unavailable.
set(SINGLEHEADER_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/simdjson.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/simdjson.h
  ${CMAKE_CURRENT_BINARY_DIR}/amalgamate_demo.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/README.md
)
set(SINGLEHEADER_REPO_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.h
  ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/README.md
)
set_source_files_properties(${SINGLEHEADER_FILES} PROPERTIES GENERATED TRUE)

if (MSVC)
  # MSVC doesn't have bash, so we use existing amalgamated files instead of generating them ...
  # (Do not do this if the source and destination are the same!)
  if (NOT (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR}))
    add_custom_command(
      OUTPUT ${SINGLEHEADER_FILES}
      COMMAND ${CMAKE_COMMAND} -E copy
        ${SINGLEHEADER_REPO_FILES}
        ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${SINGLEHEADER_REPO_FILES}
    )
  endif()

else(MSVC)

  ##
  # Important! The script amalgamate.sh is not generally executable. It
  # assumes that bash is at /bin/bash which may not be true.
  ###
  add_custom_command(
    OUTPUT ${SINGLEHEADER_FILES}
    COMMAND ${CMAKE_COMMAND} -E env
      AMALGAMATE_SOURCE_PATH=${PROJECT_SOURCE_DIR}/src
      AMALGAMATE_INPUT_PATH=${PROJECT_SOURCE_DIR}/include
      AMALGAMATE_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}
      bash ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate.sh
      #
      # This is the best way I could find to make amalgamation trigger whenever source files or
      # header files change: since the "simdjson" library has to get rebuilt when that happens, we
      # take a dependency on the generated library file (even though we're not using it). Depending
      # on simdjson-source doesn't do the trick because DEPENDS here can only depend on an
      # *artifact*--it won't scan source and include files the way a concrete library or executable
      # will.
      #
      # It sucks that we have to build the actual library to make it happen, but it's better than\
      # nothing!
      #
      DEPENDS amalgamate.sh simdjson-internal
  )

  # This is used by "make amalgamate" to update the original source files. We obviously don't do
  # this if source and generated files are in the same place--cmake gets mad!
  if (NOT (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR}))
    add_custom_command(
      # We don't really need want this to trigger other stuff, it's a terminal target.
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.cpp ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.h ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate_demo.cpp ${CMAKE_CURRENT_SOURCE_DIR}/README.md
      COMMAND ${CMAKE_COMMAND} -E copy ${SINGLEHEADER_FILES} ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${SINGLEHEADER_FILES}
    )
  endif()
  #
  # "make amalgamate" to generate the header files directly and update the original source
  #
  add_custom_target(amalgamate DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.cpp ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.h ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate_demo.cpp ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
endif(MSVC)

#
# Build the library
#
if(SIMDJSON_BUILD_STATIC)
  MESSAGE( STATUS "Building a static library." )
  add_library(simdjson STATIC "")
else()
  MESSAGE( STATUS "Building a dynamic library." )
  add_library(simdjson SHARED "")
  target_compile_definitions(simdjson INTERFACE SIMDJSON_USING_LIBRARY=1)
  if(MSVC)
    MESSAGE( STATUS "Building a Windows DLL using Visual Studio, exporting all symbols automatically." )
    set_target_properties(simdjson PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)
  endif()
endif()

add_dependencies(simdjson amalgamate)
target_include_directories(simdjson PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_sources(simdjson PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/simdjson.cpp>)
target_link_libraries(simdjson PRIVATE simdjson-flags PUBLIC simdjson-public-flags)

if(NOT MSVC)
  ## We output the library at the root of the current directory where cmake is invoked
  ## This is handy but Visual Studio will happily ignore us
  set_target_properties(simdjson PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  MESSAGE( STATUS "Library output directory (does not apply to Visual Studio): " ${PROJECT_BINARY_DIR})
endif()

#
# Installation
#
install(FILES simdjson.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS simdjson
  EXPORT simdjson-config
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT simdjson-config
  FILE simdjson-config.cmake
  NAMESPACE simdjson::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdjson
)

#
# Test the generated simdjson.cpp/simdjson.h using the generated amalgamate_demo.cpp
#
add_executable(amalgamate_demo $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/amalgamate_demo.cpp>)
target_link_libraries(amalgamate_demo simdjson)
add_test(amalgamate_demo amalgamate_demo ${EXAMPLE_JSON} ${EXAMPLE_NDJSON})
