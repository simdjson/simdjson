#
# simdjson headers and flags required to compile them
#

add_library(simdjson-headers INTERFACE)

# Include directory
target_include_directories(simdjson-headers INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Flags absolutely needed to compile simdjson at all
target_compile_features(simdjson-headers INTERFACE cxx_std_17)
if(MSVC) # Windows
  # C++ standard flags
  target_compile_options(simdjson-headers INTERFACE /std:c++17)
  # Base flags
  target_compile_options(simdjson-headers INTERFACE /nologo)
  # Warning flags
  target_compile_options(simdjson-headers INTERFACE /W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /wd4267 /wd4244 /wd4113)
else() # Linux
  # Base flags
  target_compile_options(simdjson-headers INTERFACE -fPIC)
  # C++ standard flags
  target_compile_options(simdjson-headers INTERFACE -std=c++17)
  # Warning flags
  target_compile_options(simdjson-headers INTERFACE -Wall -Wextra -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self)
  # Debug and release specific flags
  target_compile_options(simdjson-headers INTERFACE $<$<CONFIG:DEBUG>:-ggdb>)
  target_compile_options(simdjson-headers INTERFACE $<$<CONFIG:RELEASE>:-O3  -DNDEBUG>)
endif()


# Optional flags
option(SIMDJSON_IMPLEMENTATION_HASWELL "Include the haswell implementation" ON)
option(SIMDJSON_IMPLEMENTATION_WESTMERE "Include the westmere implementation" ON)
option(SIMDJSON_IMPLEMENTATION_ARM64 "Include the arm64 implementation" ON)
option(SIMDJSON_IMPLEMENTATION_FALLBACK "Include the fallback implementation" ON)

option(SIMDJSON_EXCEPTIONS "Enable simdjson's exception-throwing interface" ON)
if(NOT SIMDJSON_EXCEPTIONS)
  message(STATUS "simdjson exception interface turned off. Code that does not check error codes will not compile.")
  target_compile_definitions(simdjson-headers INTERFACE SIMDJSON_EXCEPTIONS=0)
endif()

option(SIMDJSON_ENABLE_THREADS "Enable threaded operation" ON)
if(SIMDJSON_ENABLE_THREADS)
  find_package(Threads REQUIRED)
  target_link_libraries(simdjson-headers INTERFACE Threads::Threads)
endif()

option(SIMDJSON_SANITIZE "Sanitize addresses" OFF)
if(SIMDJSON_SANITIZE)
  # Not sure which 
  target_compile_options(simdjson-headers INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(simdjson-headers INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)

  # Ubuntu bug for GCC 5.0+ (safe for all versions)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(simdjson-headers INTERFACE -fuse-ld=gold)
  endif()
endif()

install(TARGETS simdjson-headers
        EXPORT simdjson-headers-config
        INCLUDES DESTINATION include)
install(EXPORT simdjson-headers-config
        FILE simdjson-headers-config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdjson)
install(DIRECTORY simdjson DESTINATION include FILES_MATCHING PATTERN *.h)
install(FILES simdjson.h DESTINATION include)
