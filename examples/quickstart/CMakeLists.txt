#
# Quickstart compile tests don't require any flags
#

# TODO haven't quite decided the right way to run quickstart on Windows. Needs README update.
#
# Note: on macOS and other platforms, the 'command' described below may not work even if the cmake builds.
# For example, it may be necessary to specify the sysroot, which CMake does, but the 'command' does not
# handle such niceties. On a case-by-case basis it is fixable but it requires work that CMake knows how
# to do but that is not trivial.
#
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  include(add_quickstart_test.cmake)

  # DOM Quick Start
  if (SIMDJSON_EXCEPTIONS)
    add_quickstart_test(quickstart   quickstart.cpp LABELS acceptance)
    add_quickstart_test(quickstart11 quickstart.cpp CXX_STANDARD c++11 LABELS acceptance)
    add_quickstart_test(quickstart14 quickstart.cpp CXX_STANDARD c++14)
  endif()

  add_quickstart_test(quickstart_noexceptions   quickstart_noexceptions.cpp NO_EXCEPTIONS LABELS acceptance)
  add_quickstart_test(quickstart_noexceptions11 quickstart_noexceptions.cpp NO_EXCEPTIONS CXX_STANDARD c++11)

  add_quickstart_test(quickstart2_noexceptions   quickstart2_noexceptions.cpp NO_EXCEPTIONS LABELS acceptance)
  add_quickstart_test(quickstart2_noexceptions11 quickstart2_noexceptions.cpp NO_EXCEPTIONS CXX_STANDARD c++11)

  # On Demand Quick Start
  if (SIMDJSON_EXCEPTIONS)
    add_quickstart_test(quickstart_ondemand   quickstart_ondemand.cpp LABELS quickstart_ondemand acceptance)
    add_quickstart_test(quickstart_ondemand11 quickstart_ondemand.cpp CXX_STANDARD c++11 LABELS quickstart_ondemand acceptance)
    add_quickstart_test(quickstart_ondemand14 quickstart_ondemand.cpp CXX_STANDARD c++14 LABELS quickstart_ondemand)
  endif()

  add_quickstart_test(quickstart_ondemand_noexceptions   quickstart_ondemand_noexceptions.cpp NO_EXCEPTIONS LABELS quickstart_ondemand acceptance)
  add_quickstart_test(quickstart_ondemand_noexceptions11 quickstart_ondemand_noexceptions.cpp NO_EXCEPTIONS CXX_STANDARD c++11 LABELS quickstart_ondemand)

endif()
