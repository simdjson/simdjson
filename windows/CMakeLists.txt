add_library(simdjson-windows-headers INTERFACE)
if(MSVC)
  target_include_directories(simdjson-windows-headers INTERFACE .)
endif()