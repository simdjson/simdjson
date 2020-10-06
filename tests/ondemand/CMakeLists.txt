# All remaining tests link with simdjson proper
link_libraries(simdjson)
include_directories(..)
add_cpp_test(ondemand_basictests LABELS acceptance per_implementation)

