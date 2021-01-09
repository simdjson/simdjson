#
# Provides the simdjson headers.
#
# target_link_libraries(my-project simdjson-headers) grants the headers. It does not provide the
# source, libraries or any compiler flags.
#
add_library(simdjson-headers INTERFACE)
target_compile_features(simdjson-headers INTERFACE cxx_std_11) # headers require at least C++11
target_include_directories(simdjson-headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCDIR}>)

install(TARGETS simdjson-headers EXPORT simdjson-config INCLUDES DESTINATION include)
