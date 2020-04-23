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
set_source_files_properties(${SINGLEHEADER_FILES} PROPERTIES GENERATED TRUE)

if (MSVC)
  # MSVC doesn't have bash, so we use existing amalgamated files instead of generating them ...
  add_custom_command(
    OUTPUT ${SINGLEHEADER_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy
      simdjson.cpp simdjson.h amalgamate_demo.cpp README.md
      ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS simdjson.cpp simdjson.h amalgamate_demo.cpp README.md
  )
else()
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
    DEPENDS amalgamate.sh simdjson-source
  )
endif()

add_custom_target(amalgamate DEPENDS ${SIINGLEHEADER_FILES})

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
target_link_libraries(amalgamate_demo simdjson-singleheader-include-source)
add_test(amalgamate_demo amalgamate_demo ${EXAMPLE_JSON} ${EXAMPLE_NDJSON})
