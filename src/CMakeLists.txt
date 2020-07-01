#
# For callers who intend to #include simdjson.cpp.
#
# target_link_libraries(my-program simdjson-include-source) gives you the header and source
# directories. It does not specify any compiler flags.
#

add_library(simdjson-include-source INTERFACE)
target_link_libraries(simdjson-include-source INTERFACE simdjson-headers)
target_include_directories(simdjson-include-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
install(TARGETS simdjson-include-source EXPORT simdjson-config)

#
# For callers who intend to compile simdjson.cpp themselves.
#
# target_link_libraries(my-object simdjson-source) gives you the header and source directories, plus
# the .cpp sources. It does not specify any compiler flags.
#
add_library(simdjson-source INTERFACE)
target_sources(simdjson-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/simdjson.cpp)
target_link_libraries(simdjson-source INTERFACE simdjson-include-source)
install(TARGETS simdjson-source EXPORT simdjson-config)

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

target_link_libraries(simdjson PUBLIC simdjson-headers simdjson-flags) # Only expose the headers, not sources
target_link_libraries(simdjson PRIVATE simdjson-source simdjson-internal-flags)

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
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdjson> |  ${GREP} abort || exit 0  && exit 1"
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

if(NOT MSVC)
  ## We output the library at the root of the current directory where cmake is invoked
  ## This is handy but Visual Studio will happily ignore us
  set_target_properties(simdjson PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  MESSAGE( STATUS "Library output directory: " ${PROJECT_BINARY_DIR})

  ############
  # Please do not delete the following, our users want version numbers. See
  # https://github.com/simdjson/simdjson/issues/1014
  # https://github.com/simdjson/simdjson/issues/52
  ########### 
  # We deliberately omit VERSION ${SIMDJSON_LIB_VERSION} since our version numbers do not follow
  # the sonumber. The important point is that the soversion matches ABI compatibility. In simdjson
  # version 0.2.0, 0.3.0 and 0.4.0 are not binary compatible. This is allowed as per the
  # semantic versioning specification.
  # 
  # The soversion and the library version numbers are different numbers. It is common but
  # not universal that the major version number is also the soversion number. However, there
  # are notable exceptions. For example, glibc version 2.23 has a soversion of 6. It is
  # ok because the linker (e.g., ldd) does not need to know 2.23, it only cares about the
  # soversion number (6). 
  #
  # The important part is that if you built against a library that has soname x, named
  # libsimdson.x, then you can also load dynamically against another file named libsimdson.x.
  # Hence, it is ok if the soversion goes up fast. It is not ok to keep the soversion the
  # same and break ABI compatibility (that is very bad).
  ############
  set_target_properties(simdjson PROPERTIES SOVERSION ${SIMDJSON_LIB_SOVERSION})
  ##########
  # End of the do-not-delete message.
  #########
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
