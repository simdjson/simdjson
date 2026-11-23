add_executable(sjmin sjmin.cpp)
simdjson_apply_props(sjmin)
target_link_libraries(sjmin PRIVATE simdjson simdjson-internal-flags)
if(MSVC)
  target_compile_definitions(sjmin PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(SIMDJSON_INSTALL AND SIMDJSON_BUILD_TOOLS)
  include(GNUInstallDirs)
  install(TARGETS sjmin RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  install(FILES "${PROJECT_SOURCE_DIR}/man/sjmin.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endif()

# sjq, the jq-compatible processor (tools/sjq). It comes with
# SIMDJSON_BUILD_TOOLS; in developer mode it is off unless asked for.
option(SIMDJSON_SJQ "Build the sjq command-line JSON processor" ${SIMDJSON_BUILD_TOOLS})
if(SIMDJSON_SJQ)
  add_subdirectory(sjq)
endif()
