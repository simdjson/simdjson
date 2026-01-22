# Include reflect-cpp
CPMAddPackage(
  NAME reflect-cpp
  GITHUB_REPOSITORY getml/reflect-cpp
  GIT_TAG v0.17.0
  EXCLUDE_FROM_ALL YES
)

option(SIMDJSON_USE_RUST "Build the static_reflect benchmark" OFF)

if(SIMDJSON_USER_RUST)
  if(NOT WIN32)
  # We want the check whether Rust is available before trying to build a crate.
  CPMAddPackage(
    NAME corrosion
    GITHUB_REPOSITORY corrosion-rs/corrosion
    VERSION 0.4.4
    DOWNLOAD_ONLY ON
    OPTIONS "Rust_FIND_QUIETLY OFF"
  )
  include("${corrosion_SOURCE_DIR}/cmake/FindRust.cmake")
  endif()

  if(RUST_FOUND)
    message(STATUS "Rust found: " ${Rust_VERSION} )
    add_subdirectory("${corrosion_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/_deps/corrosion" EXCLUDE_FROM_ALL)
    # Important: we want to build in release mode!
    corrosion_import_crate(MANIFEST_PATH "serde-benchmark/Cargo.toml" NO_LINKER_OVERRIDE PROFILE release)
  else()
    message(STATUS "Rust/Cargo is unavailable." )
    message(STATUS "We will not benchmark serde-benchmark." )
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      message(STATUS "Under macOS, you may be able to install rust with")
      message(STATUS "curl https://sh.rustup.rs -sSf | sh")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      message(STATUS "Under Linux, you may be able to install rust with a command such as")
      message(STATUS "apt-get install cargo" )
      message(STATUS "or" )
      message(STATUS "curl https://sh.rustup.rs -sSf | sh")
    endif()
  endif()
else(SIMDJSON_USER_RUST)
  message(STATUS "We will not benchmark serde-benchmark." )
endif(SIMDJSON_USER_RUST)

# Add the benchmark executable targets
add_subdirectory(twitter_benchmark)
add_subdirectory(citm_catalog_benchmark)