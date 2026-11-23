# sjq: a jq-compatible command-line JSON processor built on simdjson.
#
# Built with -D SIMDJSON_BUILD_TOOLS=ON (with sjmin), or -D SIMDJSON_SJQ=ON
# in developer mode, where it gets the warning flags of the other tools. It
# needs C++17 and exceptions; the library itself does not.

if(NOT SIMDJSON_EXCEPTIONS)
  message(STATUS "sjq is not built: it requires SIMDJSON_EXCEPTIONS=ON.")
  return()
endif()

find_package(Threads REQUIRED)

# The engine, shared by the executable, the tests and the fuzzer.
add_library(sjq-core STATIC
  builtins.cpp
  input.cpp
  ondemand.cpp
  parser.cpp
)
target_compile_features(sjq-core PUBLIC cxx_std_17)
target_include_directories(sjq-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sjq-core PUBLIC simdjson Threads::Threads)
if(SIMDJSON_DEVELOPER_MODE)
  # The warnings (and -Werror) used for the other tools.
  target_link_libraries(sjq-core PUBLIC simdjson-internal-flags simdjson-windows-headers)
endif()

add_executable(sjq sjq.cpp)
target_link_libraries(sjq PRIVATE sjq-core)

if(WIN32)
  # Map input files with simdjson::padded_memory_map, which needs Windows 10
  # version 1803 or later and the onecore import library. The definitions
  # are private to sjq: the library is built as configured.
  target_compile_definitions(sjq-core PUBLIC
    SIMDJSON_ENABLE_MEMORY_FILE_MAPPING_ON_WINDOWS=1
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
    NTDDI_VERSION=0x0A000006
  )
  target_link_libraries(sjq-core PUBLIC onecore)
  # The default main thread stack on Windows is 1 MiB; sjq recurses up to
  # 10000 levels on deep values (as on Linux and macOS, with 8 MiB).
  if(MSVC)
    target_link_options(sjq PRIVATE "/STACK:8388608")
  elseif(MINGW)
    target_link_options(sjq PRIVATE "-Wl,--stack,8388608")
  endif()
  # The manifest makes UTF-8 the code page (Windows 10 version 1903 and
  # later), so that arguments and file names can hold any character.
  if(MSVC)
    target_sources(sjq PRIVATE sjq.manifest)
  elseif(MINGW)
    enable_language(RC)
    target_sources(sjq PRIVATE sjq.rc)
  endif()
endif()

# Release builds: drop unused code and, when available, link the C++
# standard library statically so that the binary can be copied around.
if(APPLE)
  target_link_options(sjq PRIVATE "-Wl,-dead_strip")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_link_options(sjq PRIVATE "-Wl,--gc-sections")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT APPLE))
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++.a
    OUTPUT_VARIABLE SJQ_LIBSTDCXX_A
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(IS_ABSOLUTE "${SJQ_LIBSTDCXX_A}" AND EXISTS "${SJQ_LIBSTDCXX_A}")
    target_link_options(sjq PRIVATE "-static-libstdc++")
  endif()
endif()
if(MINGW)
  # Without it, sjq.exe needs libgcc_s_seh-1.dll next to it.
  target_link_options(sjq PRIVATE "-static-libgcc" "-static")
endif()

if(SIMDJSON_INSTALL AND SIMDJSON_BUILD_TOOLS)
  include(GNUInstallDirs)
  install(TARGETS sjq RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  if(EXISTS "${PROJECT_SOURCE_DIR}/man/sjq.1")
    install(FILES "${PROJECT_SOURCE_DIR}/man/sjq.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
  endif()
endif()

# Tests: python3 tools/sjq/tests/run_tests.py (see that file). The comparison
# with jq runs only when a jq is found.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  enable_testing()
  add_test(NAME sjq_tests
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py
            --sjq $<TARGET_FILE:sjq> --data ${PROJECT_SOURCE_DIR}/jsonexamples
            --group golden --group targeted --group jqsuite)
  set_tests_properties(sjq_tests PROPERTIES TIMEOUT 900)
  find_program(SJQ_JQ_EXECUTABLE jq)
  if(SJQ_JQ_EXECUTABLE)
    add_test(NAME sjq_diff_jq
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py
              --sjq $<TARGET_FILE:sjq> --data ${PROJECT_SOURCE_DIR}/jsonexamples
              --jq ${SJQ_JQ_EXECUTABLE} --group diff)
    set_tests_properties(sjq_diff_jq PROPERTIES TIMEOUT 900)
  endif()
endif()
