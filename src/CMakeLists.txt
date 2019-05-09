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
set(SIMDJSON_SRC
    jsonioutil.cpp
    jsonminifier.cpp
    jsonparser.cpp
    stage1_find_marks.cpp
    stage2_build_tape.cpp
    parsedjson.cpp
    parsedjsoniterator.cpp
    simdjson.cpp
    )

add_library(${SIMDJSON_LIB_NAME} ${SIMDJSON_LIB_TYPE} ${SIMDJSON_SRC})

target_include_directories(${SIMDJSON_LIB_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
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
