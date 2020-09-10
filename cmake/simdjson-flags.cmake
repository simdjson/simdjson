
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message (STATUS "The simdjson repository appears to be standalone.")  
  option(SIMDJSON_JUST_LIBRARY "Build just the library, omit tests, tools and benchmarks" OFF)
  message (STATUS "By default, we attempt to build everything.")
else()
  message (STATUS "The simdjson repository appears to be used as a subdirectory.")
  option(SIMDJSON_JUST_LIBRARY "Build just the library, omit tests, tools and benchmarks" ON)
  message (STATUS "By default, we just build the library.")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  set(SIMDJSON_IS_UNDER_GIT ON CACHE BOOL  "Whether cmake is under git control")
  message( STATUS "The simdjson repository appears to be under git." )
else()
  set(SIMDJSON_IS_UNDER_GIT OFF CACHE BOOL  "Whether cmake is under git control")
  message( STATUS "The simdjson repository does not appear to be under git." )
endif()

#
# Flags used by exes and by the simdjson library (project-wide flags)
#
add_library(simdjson-flags INTERFACE)
add_library(simdjson-internal-flags INTERFACE)
target_link_libraries(simdjson-internal-flags INTERFACE simdjson-flags)

option(SIMDJSON_SANITIZE "Sanitize addresses" OFF)
if(SIMDJSON_SANITIZE)
  target_compile_options(simdjson-flags INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(simdjson-flags INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)

  # Ubuntu bug for GCC 5.0+ (safe for all versions)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(simdjson-flags INTERFACE -fuse-ld=gold)
  endif()
endif()


if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  if(SIMDJSON_SANITIZE)
    message(WARNING "No build type selected and you have enabled the sanitizer. Consider setting CMAKE_BUILD_TYPE to Debug to help identify the eventual problems.")
  endif()
endif()

if(MSVC)
  option(SIMDJSON_BUILD_STATIC "Build a static library" ON) # turning it on disables the production of a dynamic library
else()
  option(SIMDJSON_BUILD_STATIC "Build a static library" OFF) # turning it on disables the production of a dynamic library
  option(SIMDJSON_USE_LIBCPP "Use the libc++ library" OFF)
endif()
option(SIMDJSON_COMPETITION "Compile competitive benchmarks" ON)

option(SIMDJSON_GOOGLE_BENCHMARKS "compile the Google Benchmark benchmarks" ON)
if(SIMDJSON_COMPETITION)
  message(STATUS "Using SIMDJSON_GOOGLE_BENCHMARKS")
endif()

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

option(SIMDJSON_VISUAL_STUDIO_BUILD_WITH_DEBUG_INFO_FOR_PROFILING "Under Visual Studio, add Zi to the compile flag and DEBUG to the link file to add debugging information to the release build for easier profiling inside tools like VTune" OFF)
if(MSVC)
if("${MSVC_TOOLSET_VERSION}" STREQUAL "140")
  # Visual Studio 2015 issues warnings and we tolerate it,  cmake -G"Visual Studio 14" ..
  target_compile_options(simdjson-internal-flags INTERFACE /W0 /sdl)
else()
  # Recent version of Visual Studio expected (2017, 2019...). Prior versions are unsupported.
  target_compile_options(simdjson-internal-flags INTERFACE /WX /W3 /sdl /w34714) # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4714?view=vs-2019
endif()
if(SIMDJSON_VISUAL_STUDIO_BUILD_WITH_DEBUG_INFO_FOR_PROFILING)
  target_link_options(simdjson-flags  INTERFACE    /DEBUG ) 
  target_compile_options(simdjson-flags INTERFACE  /Zi)
endif()
else()
  target_compile_options(simdjson-internal-flags INTERFACE -fPIC)
  target_compile_options(simdjson-internal-flags INTERFACE -Werror -Wall -Wextra -Weffc++)
  target_compile_options(simdjson-internal-flags INTERFACE -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self -Wconversion -Wno-sign-conversion)
endif()

#
# Optional flags
#

# Implementation selection
option(SIMDJSON_IMPLEMENTATION_HASWELL "Include the haswell implementation" ON)
if(NOT SIMDJSON_IMPLEMENTATION_HASWELL)
  message(DEPRECATION "SIMDJSON_IMPLEMENTATION_HASWELL is deprecated. Use SIMDJSON_IMPLEMENTATION=haswell or SIMDJSON_IMPLEMENTATION=-haswell instead.")
  target_compile_definitions(simdjson-internal-flags INTERFACE SIMDJSON_IMPLEMENTATION_HASWELL=0)
endif()
option(SIMDJSON_IMPLEMENTATION_WESTMERE "Include the westmere implementation" ON)
if(NOT SIMDJSON_IMPLEMENTATION_WESTMERE)
  message(DEPRECATION "SIMDJSON_IMPLEMENTATION_WESTMERE is deprecated. SIMDJSON_IMPLEMENTATION=-westmere instead.")
  target_compile_definitions(simdjson-internal-flags INTERFACE SIMDJSON_IMPLEMENTATION_WESTMERE=0)
endif()
option(SIMDJSON_IMPLEMENTATION_ARM64 "Include the arm64 implementation" ON)
if(NOT SIMDJSON_IMPLEMENTATION_ARM64)
  message(DEPRECATION "SIMDJSON_IMPLEMENTATION_ARM64 is deprecated. Use SIMDJSON_IMPLEMENTATION=-arm64 instead.")
  target_compile_definitions(simdjson-internal-flags INTERFACE SIMDJSON_IMPLEMENTATION_ARM64=0)
endif()
option(SIMDJSON_IMPLEMENTATION_FALLBACK "Include the fallback implementation" ON)
if(NOT SIMDJSON_IMPLEMENTATION_FALLBACK)
  message(DEPRECATION "SIMDJSON_IMPLEMENTATION_FALLBACK is deprecated. Use SIMDJSON_IMPLEMENTATION=-fallback instead.")
  target_compile_definitions(simdjson-internal-flags INTERFACE SIMDJSON_IMPLEMENTATION_FALLBACK=0)
endif()
# e.g. SIMDJSON_IMPLEMENTATION="haswell;westmere;-fallback"
set(SIMDJSON_IMPLEMENTATION "" CACHE STRING "Implementations to include/exclude: semicolon separated list of architectures (haswell/westmere/arm64/fallback). Prepend with - to force an implementation off. (e.g. -DSIMDJSON_IMPLEMENTATION=\"haswell;-fallback\" .) Defaults to compile-time detection.")
foreach(implementation ${SIMDJSON_IMPLEMENTATION})
  string(TOUPPER ${implementation} implementation_upper)
  string(SUBSTRING ${implementation_upper} 0 1 first_char)
  if(first_char STREQUAL "-")
    string(SUBSTRING ${implementation_upper} 1 -1 neg_implementation_upper)
    target_compile_definitions(simdjson-internal-flags INTERFACE "SIMDJSON_IMPLEMENTATION_${neg_implementation_upper}=0")
  else()
    target_compile_definitions(simdjson-internal-flags INTERFACE "SIMDJSON_IMPLEMENTATION_${implementation_upper}=1")
  endif()
endforeach(implementation)

option(SIMDJSON_ONDEMAND_SAFETY_RAILS "Validate ondemand user code at runtime to ensure it is being used correctly. Defaults to ON for debug builds, OFF for release builds." $<IF:$<CONFIG:DEBUG>,ON,OFF>)
if(SIMDJSON_ONDEMAND_SAFETY_RAILS)
  message(STATUS "Ondemand safety rails enabled. Ondemand user code will be checked at runtime. This will be slower than normal!")
  target_compile_definitions(simdjson-internal-flags INTERFACE SIMDJSON_ONDEMAND_SAFETY_RAILS)
endif(SIMDJSON_ONDEMAND_SAFETY_RAILS)

option(SIMDJSON_BASH "Allow usage of bash within CMake" ON)

option(SIMDJSON_GIT "Allow usage of git within CMake" ON)

option(SIMDJSON_EXCEPTIONS "Enable simdjson's exception-throwing interface" ON)
if(NOT SIMDJSON_EXCEPTIONS)
  message(STATUS "simdjson exception interface turned off. Code that does not check error codes will not compile.")
  target_compile_definitions(simdjson-internal-flags INTERFACE SIMDJSON_EXCEPTIONS=0)
endif()

option(SIMDJSON_ENABLE_THREADS "Link with thread support" ON)
if(SIMDJSON_ENABLE_THREADS)
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  target_link_libraries(simdjson-flags INTERFACE Threads::Threads)
  target_link_libraries(simdjson-flags INTERFACE ${CMAKE_THREAD_LIBS_INIT})
  target_compile_options(simdjson-flags INTERFACE ${CMAKE_THREAD_LIBS_INIT})
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_THREADS_ENABLED=1) # This will be set in the code automatically.
endif()

option(SIMDJSON_VERBOSE_LOGGING, "Enable verbose logging for internal simdjson library development." OFF)
if (SIMDJSON_VERBOSE_LOGGING)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_VERBOSE_LOGGING=1)
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

# prevent shared libraries from depending on Intel provided libraries
if(${CMAKE_C_COMPILER_ID} MATCHES "Intel") # icc / icpc
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-intel")
endif()

install(TARGETS simdjson-flags EXPORT simdjson-config)
install(TARGETS simdjson-internal-flags EXPORT simdjson-config)
