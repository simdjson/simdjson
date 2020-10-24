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
