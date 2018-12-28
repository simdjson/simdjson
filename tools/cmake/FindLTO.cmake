# gcc and clang require a special 'ar' and 'ranlib' to create a
# static libX.a that allows for further inlining, but icc does not.

macro(append var string)
  set(${var} "${${var}} ${string}")
endmacro(append)

if(BUILD_LTO) 
  if ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel") # icc 
    append(CMAKE_CXX_FLAGS "-ipo")
  elseif("${CMAKE_C_COMPILER_ID}" MATCHES "MSVC") # Microsoft
    # TODO
  elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang") # Clang or AppleClang
    append(CMAKE_CXX_FLAGS "-flto")
    append(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold -flto")
    append(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=gold -flto")
    set(_LLVM_VERSION_STRING "3.8") # FIXME: determine version automatically
    set(CMAKE_AR "llvm-ar-${_LLVM_VERSION_STRING}")
    set(CMAKE_RANLIB "llvm-ranlib-${_LLVM_VERSION_STRING}")
  else() # assume GCC compatible syntax if not matched
    append(CMAKE_CXX_FLAGS "-flto")
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
  endif()
endif(BUILD_LTO)
