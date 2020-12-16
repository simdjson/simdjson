# All remaining tests link with simdjson proper
link_libraries(simdjson)
include_directories(..)
add_cpp_test(ondemand_basictests LABELS acceptance per_implementation)
if(HAVE_POSIX_FORK AND HAVE_POSIX_WAIT) # assert tests use fork and wait, which aren't on MSVC
  add_cpp_test(ondemand_assert_out_of_order_values LABELS per_implementation explicitonly assert)
endif()
