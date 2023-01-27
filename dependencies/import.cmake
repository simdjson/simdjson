set(dep_root "${simdjson_SOURCE_DIR}/dependencies/.cache")
if(DEFINED ENV{simdjson_DEPENDENCY_CACHE_DIR})
  set(dep_root "$ENV{simdjson_DEPENDENCY_CACHE_DIR}")
endif()

function(import_dependency NAME GITHUB_REPO COMMIT)
  message(STATUS "Importing ${NAME} (${GITHUB_REPO}@${COMMIT})")
  set(target "${dep_root}/${NAME}")

  # If the folder exists in the cache, then we assume that everything is as
  # should be and do nothing
  if(EXISTS "${target}")
    set("${NAME}_SOURCE_DIR" "${target}" PARENT_SCOPE)
    return()
  endif()

  set(zip_url "https://github.com/${GITHUB_REPO}/archive/${COMMIT}.zip")
  set(archive "${dep_root}/archive.zip")
  set(dest "${dep_root}/_extract")

  file(DOWNLOAD "${zip_url}" "${archive}")
  file(MAKE_DIRECTORY "${dest}")
  execute_process(
          WORKING_DIRECTORY "${dest}"
          COMMAND "${CMAKE_COMMAND}" -E tar xf "${archive}")
  file(REMOVE "${archive}")

  # GitHub archives only ever have one folder component at the root, so this
  # will always match that single folder
  file(GLOB dir LIST_DIRECTORIES YES "${dest}/*")

  file(RENAME "${dir}" "${target}")

  set("${NAME}_SOURCE_DIR" "${target}" PARENT_SCOPE)
endfunction()

# Delegates to the dependency
macro(add_dependency NAME)
  if(NOT DEFINED "${NAME}_SOURCE_DIR")
    message(FATAL_ERROR "Missing ${NAME}_SOURCE_DIR variable")
  endif()

  add_subdirectory("${${NAME}_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/_deps/${NAME}" EXCLUDE_FROM_ALL)
endmacro()

function(set_off NAME)
  set("${NAME}" OFF CACHE INTERNAL "")
endfunction()
