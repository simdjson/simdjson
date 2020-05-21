set(generated_files
    utf-8.json escape.json
    0-structurals.json 7-structurals.json 15-structurals.json 23-structurals.json
)
find_package(Ruby QUIET)
if (RUBY_EXECUTABLE)
  file(GLOB template_files ${CMAKE_CURRENT_SOURCE_DIR}/miss-templates/*.txt)
  add_custom_command(
    OUTPUT ${generated_files}
    DEPENDS ${template_files}
    COMMAND ${RUBY_EXECUTABLE} genfeaturejson.rb ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_custom_target(generated-data DEPENDS ${generated_files})
else (RUBY_EXECUTABLE)
  # TODO make this work without Ruby and make it part of the normal build
  add_custom_target(generated-data)
endif (RUBY_EXECUTABLE)
