# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: Copyright (c) 2019-2023 Lars Melchior and contributors

set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_HASH_SUM "c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d")

if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

# Expand relative path. This is important if the provided path contains a tilde (~)
get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION} ABSOLUTE)

set(CPM_DOWNLOAD_URL
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake)

# Download only when the file is not already there with the expected hash
# (e.g., in a CPM_SOURCE_CACHE prepared beforehand for an offline build).
set(CPM_EXISTING_HASH "")
if(EXISTS ${CPM_DOWNLOAD_LOCATION})
  file(SHA256 ${CPM_DOWNLOAD_LOCATION} CPM_EXISTING_HASH)
endif()
if(NOT CPM_EXISTING_HASH STREQUAL CPM_HASH_SUM)
  file(DOWNLOAD ${CPM_DOWNLOAD_URL} ${CPM_DOWNLOAD_LOCATION}
       STATUS CPM_DOWNLOAD_STATUS)
  list(GET CPM_DOWNLOAD_STATUS 0 CPM_DOWNLOAD_STATUS_CODE)
  list(GET CPM_DOWNLOAD_STATUS 1 CPM_DOWNLOAD_STATUS_MESSAGE)
  if(CPM_DOWNLOAD_STATUS_CODE EQUAL 0)
    file(SHA256 ${CPM_DOWNLOAD_LOCATION} CPM_EXISTING_HASH)
    if(NOT CPM_EXISTING_HASH STREQUAL CPM_HASH_SUM)
      set(CPM_DOWNLOAD_STATUS_MESSAGE
          "SHA256 mismatch, expected ${CPM_HASH_SUM}, got ${CPM_EXISTING_HASH}")
      set(CPM_DOWNLOAD_STATUS_CODE 1)
    endif()
  endif()
  if(NOT CPM_DOWNLOAD_STATUS_CODE EQUAL 0)
    file(REMOVE ${CPM_DOWNLOAD_LOCATION})
    message(FATAL_ERROR
      "Could not download CPM.cmake ${CPM_DOWNLOAD_VERSION}, which the developer "
      "build (SIMDJSON_DEVELOPER_MODE) uses to manage its dependencies: "
      "${CPM_DOWNLOAD_STATUS_MESSAGE}.\n"
      "Without network access, download it beforehand from\n"
      "  ${CPM_DOWNLOAD_URL}\n"
      "to <dir>/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake and pass -DCPM_SOURCE_CACHE=<dir>. "
      "See 'Offline builds' in HACKING.md.")
  endif()
endif()

include(${CPM_DOWNLOAD_LOCATION})
