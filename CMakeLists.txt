cmake_minimum_required(VERSION 3.9) # CMP0069 NEW
include(GNUInstallDirs)

message(STATUS "cmake version ${CMAKE_VERSION}")
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

project(simdjson
  DESCRIPTION "Parsing gigabytes of JSON per second"
  LANGUAGES CXX C
)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 3)
set(PROJECT_VERSION_PATCH 1)
set(SIMDJSON_LIB_VERSION "0.3.1" CACHE STRING "simdjson library version")
set(SIMDJSON_LIB_SOVERSION "1" CACHE STRING "simdjson library soversion")
set(SIMDJSON_GITHUB_REPOSITORY https://github.com/simdjson/simdjson)

if(MSVC)
  option(SIMDJSON_BUILD_STATIC "Build a static library" ON) # turning it on disables the production of a dynamic library
  option(SIMDJSON_COMPETITION "Compile competitive benchmarks" OFF)
else()
  option(SIMDJSON_BUILD_STATIC "Build a static library" OFF) # turning it on disables the production of a dynamic library
  option(SIMDJSON_COMPETITION "Compile competitive benchmarks" ON)
  option(SIMDJSON_USE_LIBCPP "Use the libc++ library" OFF)
endif()
option(SIMDJSON_GOOGLE_BENCHMARKS "compile the Google Benchmark benchmarks" OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake")


# We compile tools, tests, etc. with C++ 17. Override yourself if you need on a target.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MACOSX_RPATH OFF)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)


# LTO seems to create all sorts of fun problems. Let us
# disable temporarily.
#include(CheckIPOSupported)
#check_ipo_supported(RESULT ltoresult)
#if(ltoresult)
#  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#endif()

#
# Flags used by exes and by the simdjson library (project-wide flags)
#
add_library(simdjson-flags INTERFACE)
if(MSVC)
  target_compile_options(simdjson-flags INTERFACE /nologo /D_CRT_SECURE_NO_WARNINGS)
  target_compile_options(simdjson-flags INTERFACE /WX /W3)
else()
  target_compile_options(simdjson-flags INTERFACE -fPIC)
  target_compile_options(simdjson-flags INTERFACE -Werror -Wall -Wextra -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self -Wconversion -Wno-sign-conversion)
endif()


if(SIMDJSON_USE_LIBCPP)
  target_link_libraries(simdjson-flags INTERFACE -stdlib=libc++ -lc++abi)
  # instead of the above line, we could have used
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++  -lc++abi")
  # The next line is needed empirically.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  # we update CMAKE_SHARED_LINKER_FLAGS, this gets updated later as well
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++abi")
endif(SIMDJSON_USE_LIBCPP)

# Optional flags
option(SIMDJSON_IMPLEMENTATION_HASWELL "Include the haswell implementation" ON)
if(NOT SIMDJSON_IMPLEMENTATION_HASWELL)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_IMPLEMENTATION_HASWELL=0)
endif()
option(SIMDJSON_IMPLEMENTATION_WESTMERE "Include the westmere implementation" ON)
if(NOT SIMDJSON_IMPLEMENTATION_WESTMERE)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_IMPLEMENTATION_WESTMERE=0)
endif()
option(SIMDJSON_IMPLEMENTATION_ARM64 "Include the arm64 implementation" ON)
if(NOT SIMDJSON_IMPLEMENTATION_ARM64)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_IMPLEMENTATION_ARM64=0)
endif()
option(SIMDJSON_IMPLEMENTATION_FALLBACK "Include the fallback implementation" ON)
if(NOT SIMDJSON_IMPLEMENTATION_FALLBACK)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_IMPLEMENTATION_FALLBACK=0)
endif()

option(SIMDJSON_EXCEPTIONS "Enable simdjson's exception-throwing interface" ON)
if(NOT SIMDJSON_EXCEPTIONS)
  message(STATUS "simdjson exception interface turned off. Code that does not check error codes will not compile.")
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_EXCEPTIONS=0)
endif()

option(SIMDJSON_ENABLE_THREADS "Enable threaded operation" ON)
if(SIMDJSON_ENABLE_THREADS)
  find_package(Threads REQUIRED)
  target_link_libraries(simdjson-flags INTERFACE Threads::Threads)
endif()

option(SIMDJSON_SANITIZE "Sanitize addresses" OFF)
if(SIMDJSON_SANITIZE)
  # Not sure which 
  target_compile_options(simdjson-flags INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(simdjson-flags INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)

  # Ubuntu bug for GCC 5.0+ (safe for all versions)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(simdjson-flags INTERFACE -fuse-ld=gold)
  endif()
endif()

# prevent shared libraries from depending on Intel provided libraries
if(${CMAKE_C_COMPILER_ID} MATCHES "Intel") # icc / icpc
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-intel")
endif()

# Workaround for https://gitlab.kitware.com/cmake/cmake/issues/15415#note_633938:
function(export_private_library NAME)
  install(TARGETS ${NAME}
    EXPORT ${NAME}-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  install(EXPORT ${NAME}-config
    FILE ${NAME}-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdjson-private
  )
endfunction()

export_private_library(simdjson-flags)

#
# ${SIMDJSON_USER_CMAKECACHE} contains the *user-specified* simdjson options so you can call cmake on
# another branch or repository with the same options.
#
# Not supported on Windows at present, because the only thing that uses it is checkperf, which we
# don't run on Windows.
#
if (NOT MSVC)
  set(SIMDJSON_USER_CMAKECACHE ${CMAKE_CURRENT_BINARY_DIR}/.simdjson-user-CMakeCache.txt)
  add_custom_command(
    OUTPUT ${SIMDJSON_USER_CMAKECACHE}
    COMMAND bash -c "grep SIMDJSON_ ${PROJECT_BINARY_DIR}/CMakeCache.txt | grep -v SIMDJSON_LIB_ > ${SIMDJSON_USER_CMAKECACHE}"
    VERBATIM # Makes it not do weird escaping with the command
  )
  add_custom_target(simdjson-user-cmakecache ALL DEPENDS ${SIMDJSON_USER_CMAKECACHE})
endif()

#
# Set up test data
#
enable_testing()
add_subdirectory(jsonchecker)
add_subdirectory(jsonexamples)
add_library(test-data INTERFACE)
target_link_libraries(test-data INTERFACE jsonchecker-data jsonexamples-data)

#
# Create the top level simdjson library (must be done at this level to use both src/ and include/
# directories) and tools
#
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(windows)
add_subdirectory(tools)
add_subdirectory(singleheader)

#
# Compile tools / tests / benchmarks
#

add_subdirectory(dependencies)
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmark)
add_subdirectory(fuzz)

#
# CPack
#
set(CPACK_PACKAGE_VENDOR "Daniel Lemire")
set(CPACK_PACKAGE_CONTACT "lemire@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Parsing gigabytes of JSON per second")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_RPM_PACKAGE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")

include(CPack)
