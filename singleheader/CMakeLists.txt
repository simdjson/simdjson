include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)

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

# Under Windows, exectuting a bash script works, except that you cannot generally
# do bash C:/path to my script. You need a  "mounted" path: /mnt/c/path

if (BASH AND (NOT WIN32) AND SIMDJSON_BASH)
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

  ##
  # This is used by "make amalgamate" to update the original source files.
  # You can invoke it as cmake --build . --target amalgamate 
  # We obviously don't do
  # this if source and generated files are in the same place--cmake gets mad!
  if (NOT (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR}))
    add_custom_target(amalgamate)
    add_custom_command(TARGET amalgamate
      # We don't want CMake to know that it is writing to the source directory. No magic  
      # file regeneration in the source directory without the user's knowledge.
      # OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.cpp ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.h ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate_demo.cpp ${CMAKE_CURRENT_SOURCE_DIR}/README.md
      COMMAND ${CMAKE_COMMAND} -E copy ${SINGLEHEADER_FILES} ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${SINGLEHEADER_FILES}
    )
  endif()


  ##
  # Adding the ability for CMake to modify the source is problematic. In particular, it will
  # happily regenerate the source files that are missing, silently. We do not want to do this.
  # If they are missing source files, the build should fail. You should not get silent patching
  # by CMake. The user can easily regenerate the files, deliberately.
  #
  # DO NOT DO THIS:
  # add_custom_target(amalgamate DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.cpp ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.h ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate_demo.cpp ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
  ##

else()

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

endif()

#
# Do not depend on singleheader files directly: depend on this target instead.
# Otherwise the custom command may get triggered multiple times and race with itself!
#
add_custom_target(singleheader-files DEPENDS ${SINGLEHEADER_FILES})

#
# Include this if you intend to #include "simdjson.cpp" in your own .cpp files.
#
add_library(simdjson-singleheader-include-source INTERFACE)
target_include_directories(simdjson-singleheader-include-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
add_dependencies(simdjson-singleheader-include-source singleheader-files)

#
# Include this to get "simdjson.cpp" included in your project as one of the sources.
#
add_library(simdjson-singleheader-source INTERFACE)
target_sources(simdjson-singleheader-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/simdjson.cpp>)
target_link_libraries(simdjson-singleheader-source INTERFACE simdjson-singleheader-include-source)

#
# We do not want CMake to update the original source files automatically "in passing" as
# part of a test that generates the files anew. This kind of side-effect is  bad.
#
# add_dependencies(simdjson-singleheader-source amalgamate) <=== NO NO NO
#
#

#
# Test the generated simdjson.cpp/simdjson.h using the generated amalgamate_demo.cpp
#
add_executable(amalgamate_demo $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/amalgamate_demo.cpp>)
target_link_libraries(amalgamate_demo simdjson-singleheader-include-source simdjson-internal-flags)
add_test(amalgamate_demo amalgamate_demo ${EXAMPLE_JSON} ${EXAMPLE_NDJSON})

install(FILES simdjson.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_library(simdjson-singleheader STATIC "")
target_link_libraries(simdjson-singleheader simdjson-singleheader-source simdjson-internal-flags)
add_compile_only_test(simdjson-singleheader)

#
# Test the existing simdjson.cpp/simdjson.h using the existing amalgamate_demo.cpp, using
# the files from the repository.
#
# By design, this will fail if the original files are missing (it should).
#
if (NOT (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR}))

  add_library(simdjson-singleheader-include-source-direct-from-repository INTERFACE)
  target_include_directories(simdjson-singleheader-include-source-direct-from-repository INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

  add_library(simdjson-singleheader-source-direct-from-repository INTERFACE)
  target_sources(simdjson-singleheader-source-direct-from-repository INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(simdjson-singleheader-source-direct-from-repository INTERFACE simdjson-singleheader-include-source-direct-from-repository)

  add_executable(amalgamate_demo_direct_from_repository ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate_demo.cpp)
  target_link_libraries(amalgamate_demo_direct_from_repository simdjson-singleheader-include-source-direct-from-repository simdjson-internal-flags)
  add_test(amalgamate_demo_direct_from_repository amalgamate_demo_direct_from_repository ${EXAMPLE_JSON} ${EXAMPLE_NDJSON})

endif()