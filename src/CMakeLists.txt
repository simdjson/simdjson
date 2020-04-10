#
# For callers who intend to #include simdjson.cpp.
#
# target_link_libraries(my-program simdjson-include-source) gives you the header and source
# directories. It does not specify any compiler flags.
#
add_library(simdjson-include-source INTERFACE)
target_link_libraries(simdjson-include-source INTERFACE simdjson-headers)
target_include_directories(simdjson-include-source INTERFACE .)

#
# For callers who intend to compile simdjson.cpp themselves.
#
# target_link_libraries(my-object simdjson-source) gives you the header and source directories, plus
# the .cpp sources. It does not specify any compiler flags.
#
add_library(simdjson-source INTERFACE)
target_sources(simdjson-source INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/simdjson.cpp)
target_link_libraries(simdjson-source INTERFACE simdjson-include-source)

#
# simdjson is the distributed library compiled with flags.
#
# target_link_libraries(my-object simdjson) gives you the .so or .a to link against, plus the header
# directory. It does not specify any compiler flags, even though simdjson.so/a was compiled with
# target_link_libraries(simdjson PRIVATE simdjson-flags).
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

target_link_libraries(simdjson PRIVATE simdjson-source simdjson-flags)
target_link_libraries(simdjson INTERFACE simdjson-headers) # Only expose the headers, not sources

if(NOT MSVC)
  ## We output the library at the root of the current directory where cmake is invoked
  ## This is handy but Visual Studio will happily ignore us
  set_target_properties(simdjson PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  MESSAGE( STATUS "Library output directory (does not apply to Visual Studio): " ${CMAKE_BINARY_DIR})
endif()

#
# Installation
#
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
