# The bulk of our data files is found in https://github.com/simdjson/simdjson-data
set(EXAMPLE_JSON ${CMAKE_CURRENT_BINARY_DIR}/twitter.json PARENT_SCOPE)
set(EXAMPLE_NDJSON ${CMAKE_CURRENT_BINARY_DIR}/amazon_cellphones.ndjson PARENT_SCOPE)

# Copy static files to the build dir so they live alongside the generated ones
file(GLOB_RECURSE example_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.json *.ndjson)
foreach(example_file ${example_files})
  configure_file(${example_file} ${example_file} COPYONLY)
endforeach(example_file)
