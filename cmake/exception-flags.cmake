option(SIMDJSON_EXCEPTIONS "Enable simdjson's exception-throwing interface" ON)
if(NOT SIMDJSON_EXCEPTIONS)
  message(STATUS "simdjson exception interface turned off. \
Code that does not check error codes will not compile.")
  simdjson_add_props(target_compile_definitions PUBLIC SIMDJSON_EXCEPTIONS=0)
  if(MSVC)
    if(NOT is_top_project)
      message(AUTHOR_WARNING "Turning SIMDJSON_EXCEPTIONS off requires \
editing CMAKE_CXX_FLAGS")
    endif()

    # CMake currently /EHsc as a default flag in CMAKE_CXX_FLAGS on MSVC.
    # Replacing this with a more general abstraction is a WIP
    # (see https://gitlab.kitware.com/cmake/cmake/-/issues/20610)
    # /EHs enables standard C++ stack unwinding when catching exceptions
    # (non-structured exception handling)
    # /EHc used in conjection with /EHs indicates that extern "C" functions
    # never throw (terminate-on-throw)
    # Here, we disable both with the - argument negation operator
    string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

    # Because we cannot change the flag above on an individual target (yet), the
    # definition below must similarly be added globally
    add_definitions(-D_HAS_EXCEPTIONS=0)
  elseif(CMAKE_COMPILER_IS_GNUCC)
    simdjson_add_props(target_link_libraries PRIVATE -fno-exceptions)
  endif()
endif()
