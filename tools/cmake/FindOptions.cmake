macro(append var string)
  set(${var} "${${var}} ${string}")
endmacro(append)

set(SANITIZE_FLAGS "")
if(SIMDJSON_SANITIZE)
  set(SIMDJSON_SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all")
  if (CMAKE_COMPILER_IS_GNUCC)
    # Ubuntu bug for GCC 5.0+ (safe for all versions)
    append(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold")
    append(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=gold")
  endif()
endif()



# some compilers like clang do not automagically define __AVX2__ and __BMI2__ even when the hardware supports it
if(NOT MSVC)
   set (OPT_FLAGS "${OPT_FLAGS} -mavx2 -mbmi -mbmi2 -mpclmul")
else()
   set (OPT_FLAGS "${OPT_FLAGS} /arch:AVX2 /std:c++latest")
endif()


if(NOT MSVC)
set(CXXSTD_FLAGS "-std=c++17 -fPIC")
endif()

set(WARNING_FLAGS "-Wall")
if(NOT MSVC)
set(WARNING_FLAGS "${WARNING_FLAGS} -Wextra -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self")
set(CMAKE_C_FLAGS_DEBUG "-ggdb")
set(CMAKE_C_FLAGS_RELEASE "-O3")
endif()

set(CMAKE_C_FLAGS "${STD_FLAGS} ${OPT_FLAGS} ${INCLUDE_FLAGS} ${WARNING_FLAGS} ${SIMDJSON_SANITIZE_FLAGS} ")

if(NOT MSVC)
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()
set(CMAKE_CXX_FLAGS "${CXXSTD_FLAGS} ${OPT_FLAGS} ${INCLUDE_FLAGS} ${WARNING_FLAGS} ${SIMDJSON_SANITIZE_FLAGS} ")

if(MSVC)
add_definitions( "${OPT_FLAGS} /W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /wd4267 /wd4244  /wd4113 /nologo")
endif()
