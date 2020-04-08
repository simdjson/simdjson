include(GNUInstallDirs)

if(SIMDJSON_BUILD_STATIC)
  MESSAGE( STATUS "Building a static library." )
  add_library(simdjson STATIC "")
else()
  MESSAGE( STATUS "Building a dynamic library." )
  add_library(simdjson SHARED "")
endif()

target_sources(simdjson PRIVATE simdjson.cpp)
target_include_directories(simdjson PRIVATE .)
target_include_directories(simdjson PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(SIMDJSON_NO_EXCEPTIONS)
  message(STATUS "simdjson exception interface turned off. Code that does not check error codes will not compile.")
  target_compile_definitions(simdjson PUBLIC SIMDJSON_EXCEPTIONS=0)
endif()

if(${CMAKE_C_COMPILER_ID} MATCHES "Intel") # icc / icpc
  # prevent shared libraries from depending on Intel provided libraries
  target_link_options(simdjson PUBLIC "-static-intel")
endif()

if(SIMDJSON_ENABLE_THREADS)
  find_package(Threads REQUIRED)
  target_link_libraries(simdjson PRIVATE Threads::Threads)
endif()

# For targets that want to include the source directly
add_library(simdjson-source INTERFACE)
target_include_directories(simdjson-source INTERFACE .)
target_include_directories(simdjson-source INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

# Installation

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

if(NOT MSVC)
## We output the library at the root of the current directory where cmake is invoked
## This is handy but Visual Studio will happily ignore us
set_target_properties(simdjson PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
MESSAGE( STATUS "Library output directory (does not apply to Visual Studio): " ${CMAKE_BINARY_DIR})
endif()

if(NOT SIMDJSON_BUILD_STATIC)
  target_compile_definitions(simdjson INTERFACE SIMDJSON_USING_LIBRARY=1)
endif()

if(MSVC AND (NOT SIMDJSON_BUILD_STATIC))
  if (CMAKE_VERSION VERSION_LESS 3.4)
    MESSAGE( STATUS "To build a Windows DLL using Visual Studio, you may need cmake 3.4 or better." )
  endif()
  MESSAGE( STATUS "Building a Windows DLL using Visual Studio, exporting all symbols automatically." )
  set_target_properties(simdjson
    PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)
endif()

