#
# Flags used by exes and by the simdjson library (project-wide flags)
#
add_library(simdjson-flags INTERFACE)
add_library(simdjson-internal-flags INTERFACE)
target_link_libraries(simdjson-internal-flags INTERFACE simdjson-flags)





# LTO seems to create all sorts of fun problems. Let us
# disable temporarily.
#include(CheckIPOSupported)
#check_ipo_supported(RESULT ltoresult)
#if(ltoresult)
#  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#endif()



#
# Optional flags
#

#
# Implementation selection
#

#
# Other optional flags
#
option(SIMDJSON_ONDEMAND_SAFETY_RAILS "Validate ondemand user code at runtime to ensure it is being used correctly. Defaults to ON for debug builds, OFF for release builds." $<IF:$<CONFIG:DEBUG>,ON,OFF>)
if(SIMDJSON_ONDEMAND_SAFETY_RAILS)
  message(STATUS "Ondemand safety rails enabled. Ondemand user code will be checked at runtime. This will be slower than normal!")
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_ONDEMAND_SAFETY_RAILS)
endif(SIMDJSON_ONDEMAND_SAFETY_RAILS)

option(SIMDJSON_BASH "Allow usage of bash within CMake" ON)

option(SIMDJSON_GIT "Allow usage of git within CMake" ON)

option(SIMDJSON_EXCEPTIONS "Enable simdjson's exception-throwing interface" ON)
if(NOT SIMDJSON_EXCEPTIONS)
  message(STATUS "simdjson exception interface turned off. Code that does not check error codes will not compile.")
  target_compile_definitions(simdjson-flags INTERFACE SIMDJSON_EXCEPTIONS=0)
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
