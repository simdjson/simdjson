#
# Accumulate flags
#
set(simdjson_props_script "${PROJECT_BINARY_DIR}/simdjson-props.cmake")
set(simdjson_props_content "")
set(simdjson_props_flushed NO)

function(simdjson_add_props command)
  set(args "")
  math(EXPR limit "${ARGC} - 1")
  foreach(i RANGE 1 "${limit}")
    set(value "${ARGV${i}}")
    if(value MATCHES "^(PRIVATE|PUBLIC)$")
      string(TOLOWER "${value}" value)
      set(value "\${${value}}")
    else()
      set(value "[==[${value}]==]")
    endif()
    string(APPEND args " ${value}")
  endforeach()

  set(simdjson_props_flushed NO PARENT_SCOPE)
  set(
      simdjson_props_content
      "${simdjson_props_content}${command}(\"\${target}\"${args})\n"
      PARENT_SCOPE
  )
endfunction()

macro(simdjson_flush_props)
  if(NOT simdjson_props_flushed)
    set(simdjson_props_flushed YES PARENT_SCOPE)
    file(WRITE "${simdjson_props_script}" "${simdjson_props_content}")
  endif()
endmacro()

function(simdjson_apply_props target)
  set(private PRIVATE)
  set(public PUBLIC)
  get_target_property(TYPE "${target}" TYPE)
  if(TYPE STREQUAL "INTERFACE_LIBRARY")
    set(private INTERFACE)
    set(public INTERFACE)
  endif()

  simdjson_flush_props()
  include("${simdjson_props_script}")
endfunction()
