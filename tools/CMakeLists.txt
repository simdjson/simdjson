if(TARGET cxxopts) # we only build the tools if cxxopts is available
  message(STATUS "We have cxxopts as a dependency and we are building the tools (e.g., json2json).")
  foreach(tool IN ITEMS json2json jsonstats jsonpointer minify)
    add_executable("${tool}" "${tool}.cpp")
    simdjson_apply_props("${tool}")
    target_link_libraries(
        "${tool}" PRIVATE
        simdjson simdjson-internal-flags simdjson-windows-headers cxxopts
    )
  endforeach()
else()
  message(STATUS "We are missing cxxopts as a dependency so the tools (e.g., json2json) are omitted.")
endif()
