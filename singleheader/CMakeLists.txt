#
# Amalgamation
#

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

find_program(BASH bash)

if (NOT BASH)
  # We do not have bash, so we use existing amalgamated files instead of generating them ...
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

else(BASH)

  ##
  # Important! The script amalgamate.sh is not generally executable.
  ###
  add_custom_command(
    OUTPUT ${SINGLEHEADER_FILES}
    COMMAND ${CMAKE_COMMAND} -E env
      AMALGAMATE_SOURCE_PATH=${PROJECT_SOURCE_DIR}/src
      AMALGAMATE_INPUT_PATH=${PROJECT_SOURCE_DIR}/include
      AMALGAMATE_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}
      ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate.sh
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
      DEPENDS amalgamate.sh simdjson
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
# Include this if you intend to #include "simdjson.cpp" in your own .cpp files.
#
add_library(simdjson-singleheader-include-source INTERFACE)
target_include_directories(simdjson-singleheader-include-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

#
# Include this to get "simdjson.cpp" included in your project as one of the sources.
#
add_library(simdjson-singleheader-source INTERFACE)
target_sources(simdjson-singleheader-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/simdjson.cpp>)
target_link_libraries(simdjson-singleheader-source INTERFACE simdjson-singleheader-include-source)
add_dependencies(simdjson-singleheader-source amalgamate)

#
# Test the generated simdjson.cpp/simdjson.h using the generated amalgamate_demo.cpp
#
add_executable(amalgamate_demo $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/amalgamate_demo.cpp>)
target_link_libraries(amalgamate_demo simdjson-singleheader-include-source simdjson-internal-flags)
add_test(amalgamate_demo amalgamate_demo ${EXAMPLE_JSON} ${EXAMPLE_NDJSON})

install(FILES simdjson.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
