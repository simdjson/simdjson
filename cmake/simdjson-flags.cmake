#
# Flags used by exes and by the simdjson library (project-wide flags)
#
add_library(simdjson-flags INTERFACE)
add_library(simdjson-internal-flags INTERFACE)
target_link_libraries(simdjson-internal-flags INTERFACE simdjson-flags)

target_compile_definitions(simdjson-flags INTERFACE ${impl_definitions})

option(SIMDJSON_SANITIZE_UNDEFINED "Sanitize undefined behavior" OFF)
if(SIMDJSON_SANITIZE_UNDEFINED)
  target_compile_options(simdjson-flags INTERFACE -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(simdjson-flags INTERFACE -fsanitize=undefined -fno-sanitize-recover=all)
endif()

option(SIMDJSON_SANITIZE "Sanitize addresses" OFF)
if(SIMDJSON_SANITIZE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    message(STATUS "The address sanitizer under Apple's clang appears to be incompatible with the undefined-behavior sanitizer.")
    message(STATUS "You may set SIMDJSON_SANITIZE_UNDEFINED to sanitize undefined behavior.")
    target_compile_options(simdjson-flags INTERFACE -fsanitize=address  -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_compile_definitions(simdjson-flags INTERFACE ASAN_OPTIONS=detect_leaks=1)
    target_link_libraries(simdjson-flags INTERFACE -fsanitize=address  -fno-omit-frame-pointer -fno-sanitize-recover=all)
  else()
    message(STATUS "Setting both the address sanitizer and the undefined sanitizer.")
    target_compile_options(simdjson-flags INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
    target_link_libraries(simdjson-flags INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  endif()
  # Ubuntu bug for GCC 5.0+ (safe for all versions)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(simdjson-flags INTERFACE -fuse-ld=gold)
  endif()
endif()


if(SIMDJSON_SANITIZE_THREADS)
  message(STATUS "Setting both the thread sanitizer and the undefined-behavior sanitizer.")
  target_compile_options(simdjson-flags INTERFACE -fsanitize=thread -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(simdjson-flags INTERFACE -fsanitize=thread -fsanitize=undefined -fno-sanitize-recover=all)

  # Ubuntu bug for GCC 5.0+ (safe for all versions)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(simdjson-flags INTERFACE -fuse-ld=gold)
  endif()
endif()

get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT CMAKE_BUILD_TYPE)
  # Deliberately not including SIMDJSON_SANITIZE_THREADS since thread behavior depends on the build type.
  if(SIMDJSON_SANITIZE OR SIMDJSON_SANITIZE_UNDEFINED)
    message(STATUS "No build type selected and you have enabled the sanitizer, default to Debug. Consider setting CMAKE_BUILD_TYPE.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  else()
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  endif()
endif()

if(NOT MSVC)
  option(SIMDJSON_USE_LIBCPP "Use the libc++ library" OFF)
endif()

if(MSVC AND BUILD_SHARED_LIBS)
  # This will require special handling.
  set(SIMDJSON_WINDOWS_DLL TRUE)
endif()

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
  if("${MSVC_TOOLSET_VERSION}" STRLESS "142")
    set(SIMDJSON_LEGACY_VISUAL_STUDIO TRUE)
    message (STATUS "A legacy Visual Studio version was detected. We recommend Visual Studio 2019 or better on a 64-bit system.")
  endif()
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
  endif(SIMDJSON_VISUAL_STUDIO_BUILD_WITH_DEBUG_INFO_FOR_PROFILING)
else(MSVC)
  if(NOT WIN32)
    target_compile_options(simdjson-internal-flags INTERFACE -fPIC)
  endif()
  target_compile_options(simdjson-internal-flags INTERFACE -Werror -Wall -Wextra -Weffc++)
  target_compile_options(simdjson-internal-flags INTERFACE -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self -Wconversion -Wno-sign-conversion)
endif(MSVC)


# workaround for GNU GCC poor AVX load/store code generation
if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86(_64)?)$"))
  target_compile_options(simdjson-flags INTERFACE -mno-avx256-split-unaligned-load -mno-avx256-split-unaligned-store)
endif()

#
# Other optional flags
#
option(SIMDJSON_DEVELOPMENT_CHECKS "Enable development-time aids, such as checks for incorrect API usage. Enabled by default in DEBUG." OFF)
if(SIMDJSON_DEVELOPMENT_CHECKS)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_DEVELOPMENT_CHECKS)
endif()

option(SIMDJSON_BASH "Allow usage of bash within CMake" ON)

endif()

option(SIMDJSON_VERBOSE_LOGGING, "Enable verbose logging for internal simdjson library development." OFF)
if (SIMDJSON_VERBOSE_LOGGING)
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_VERBOSE_LOGGING=1)
endif()

option(SIMDJSON_DISABLE_DEPRECATED_API "Disables deprecated APIs" Off)
if (SIMDJSON_DISABLE_DEPRECATED_API)
    target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_DISABLE_DEPRECATED_API=1)
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

include (CheckSymbolExists)
CHECK_SYMBOL_EXISTS(fork unistd.h HAVE_POSIX_FORK)
CHECK_SYMBOL_EXISTS(wait sys/wait.h HAVE_POSIX_WAIT)

install(TARGETS simdjson-flags EXPORT simdjson-config)

# I do not think we want to export our internal flags!
# install(TARGETS simdjson-internal-flags EXPORT simdjson-config)
