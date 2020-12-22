function(add_compile_only_test TEST_NAME)
  add_test(
    NAME ${TEST_NAME}
    COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )
  set_target_properties(${TEST_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endfunction()