add_subdirectory(quickstart)

# Add simdjson one-liner demo examples
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/github_legacy.cpp AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/github_modern.cpp)
    # FetchContent to download cpr
    include(FetchContent)
    set(CPR_ENABLE_SSL OFF CACHE BOOL "")
    FetchContent_Declare(
      cpr
      GIT_REPOSITORY https://github.com/libcpr/cpr.git
      GIT_TAG 1.10.5
    )
    FetchContent_MakeAvailable(cpr)

    # Build the examples
    add_executable(github_legacy github_legacy.cpp)
    target_link_libraries(github_legacy simdjson cpr::cpr)

    add_executable(github_modern github_modern.cpp)
    target_link_libraries(github_modern simdjson cpr::cpr)
    target_compile_options(github_modern PRIVATE -std=c++26 -freflection)
    target_compile_definitions(github_modern PRIVATE SIMDJSON_STATIC_REFLECTION=1)
endif()
