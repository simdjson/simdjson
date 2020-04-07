add_test(
  NAME issue150
  COMMAND ${PROJECT_SOURCE_DIR}/scripts/issue150.sh
  WORKING_DIRECTORY $<TARGET_FILE_DIR:allparserscheckfile>
)