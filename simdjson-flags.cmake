#
# User options
#
if(MSVC)
  option(SIMDJSON_BUILD_STATIC "Build a static library" ON) # turning it on disables the production of a dynamic library
  option(SIMDJSON_COMPETITION "Compile competitive benchmarks" OFF)
else()
  option(SIMDJSON_BUILD_STATIC "Build a static library" OFF) # turning it on disables the production of a dynamic library
  option(SIMDJSON_COMPETITION "Compile competitive benchmarks" ON)
endif()
option(SIMDJSON_ENABLE_THREADS "Enable threaded operation" ON)
option(SIMDJSON_EXCEPTIONS "Enable simdjson's exception-throwing interface" ON)
option(SIMDJSON_GOOGLE_BENCHMARKS "compile the Google Benchmark benchmarks" OFF)
option(SIMDJSON_IMPLEMENTATION_HASWELL "Include the haswell implementation" ON)
option(SIMDJSON_IMPLEMENTATION_WESTMERE "Include the westmere implementation" ON)
option(SIMDJSON_IMPLEMENTATION_ARM64 "Include the arm64 implementation" ON)
option(SIMDJSON_IMPLEMENTATION_FALLBACK "Include the fallback implementation" ON)
option(SIMDJSON_SANITIZE "Sanitize addresses" OFF)

#
# Global CMAKE options
#
message(STATUS "cmake version ${CMAKE_VERSION}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MACOSX_RPATH OFF)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake")
include(GNUInstallDirs)

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

#
# Flags used by exes and by the simdjson library (project-wide flags)
#
# We compile tools, tests, etc. with C++ 17. Override yourself if you need on a target.

add_library(simdjson-flags INTERFACE)
if(MSVC)
  target_compile_options(simdjson-flags INTERFACE /nologo /D_CRT_SECURE_NO_WARNINGS)
  target_compile_options(simdjson-flags INTERFACE /WX /W3 /wd4005 /wd4996 /wd4113)
else()
  target_compile_options(simdjson-flags INTERFACE -fPIC)
  target_compile_options(simdjson-flags INTERFACE -Werror -Wall -Wextra -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self)
endif()

# Optional flags
if(NOT SIMDJSON_IMPLEMENTATION_HASWELL)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_IMPLEMENTATION_HASWELL=0)
endif()
if(NOT SIMDJSON_IMPLEMENTATION_WESTMERE)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_IMPLEMENTATION_WESTMERE=0)
endif()
if(NOT SIMDJSON_IMPLEMENTATION_ARM64)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_IMPLEMENTATION_ARM64=0)
endif()
if(NOT SIMDJSON_IMPLEMENTATION_FALLBACK)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_IMPLEMENTATION_FALLBACK=0)
endif()

if(NOT SIMDJSON_EXCEPTIONS)
  message(STATUS "simdjson exception interface turned off. Code that does not check error codes will not compile.")
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_EXCEPTIONS=0)
endif()

if(SIMDJSON_ENABLE_THREADS)
  find_package(Threads REQUIRED)
  target_link_libraries(simdjson-flags INTERFACE Threads::Threads)
endif()

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

# LTO seems to create all sorts of fun problems. Let us
# disable temporarily.
#include(CheckIPOSupported)
#check_ipo_supported(RESULT ltoresult)
#if(ltoresult)
#  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#endif()

export_private_library(simdjson-flags)
