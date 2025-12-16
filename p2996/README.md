# Building the experimental LLVM compiler for static reflection

The simdjson has experimental support for static reflection, when the compiler supports it.
We focus on the [latest p2996 paper](https://isocpp.org/files/papers/P2996R10.html) that is targeting C++26.

## Current status
There are 2 versions of compiler that aim to support the C++26 reflection paper ([p2996](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r10.html)).

1. [Clang-p2996 llvm branch](https://github.com/bloomberg/clang-p2996/tree/p2996) that open-source and available in the [Compiler Explorer](https://godbolt.org/z/eoEej3E6j).
2. EDG reflection branch that is only publicly available in the [Compiler Explorer](https://godbolt.org).

For now, we will resort to #1, since it is open-source.

We are excited to hear that this reflection proposal seems to be on-track for C++26. [As per Herb Sutter](https://herbsutter.com/2024/03/22/trip-report-winter-iso-c-standards-meeting-tokyo-japan/).

## Instructions for building simdjson for static reflection (using docker)

We are assuming that you are running Linux or macOS. We recommend that Windows users rely on WSL.

Follow the following two steps:

1. Clone the project

```bash
git clone https://github.com/simdjson/simdjson.git
cd simdjson
```

As of March 2025, support for static reflection is available only in the `json_builder_init` branch, so you should switch:

```bash
git checkout -B json_builder_init
```

2. Make sure that you have [docker installed and running](https://docs.docker.com/engine/install/) on your system. Most Linux distributions support docker though some (like RedHat) have the equivalent (Podman). Users of Apple systems may want to [consider OrbStack](https://orbstack.dev). You do not need to familiar with docker, you just need to make sure that you are have it running.

3. While inside the `simdjson` repository, run the following bash script:

```bash
bash ./p2996/run_docker.sh bash
```

This will enter a bash shell with access to the repo directory. Note that this will take some time when running it for the first time, since the specific container image has to be built.

This step builds and executes a docker container defined by our Dockerfile which provides the necessary environment.

Importantly, we build the experimental LLVM compiler based on the current state of the
`p2996` branch of the `https://github.com/bloomberg/clang-p2996.git` repository. It is possible that the build could fail. Furthermore, you may need to refresh the build from time to time. We provide a script which allows you to delete the docker image you have built (`bash ./p2996/remove_docker.sh`) so you can start anew.




4. While inside the docker shell, configure the build system with cmake:
```bash
CXX=clang++ cmake -B buildreflect -D SIMDJSON_STATIC_REFLECTION=ON -DSIMDJSON_DEVELOPER_MODE=ON
```
This only needs to be done once.

5. Build the code...
```bash
cmake --build buildreflect --target benchmark_serialization_citm_catalog benchmark_serialization_twitter
```


6. Run the tests...
```bash
cmake --build buildreflect
ctest --test-dir buildreflect --output-on-failure
```

7. Run the benchmarks.
```bash
./buildreflect/benchmark/static_reflect/citm_catalog_benchmark/benchmark_serialization_citm_catalog
./buildreflect/benchmark/static_reflect/twitter_benchmark/benchmark_serialization_twitter
```

You can modify the source code with your favorite editor and run again steps 5 (Build the code) and 6 (Run the tests) and 7 (Run the benchmark). Importantly, you should remain in the docker shell.

You can create a new docker shell at any time by running step 3 (bash script).

## Using static reflection in your own projects

You can import simdjson in your own CMake project. You can configure your project like so:

```
# Fetch simdjson from GitHub
# Replace GIT_TAG by the commit you require.
include(FetchContent)
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG 015daad6a95a4f67c08ed5980d24b57be221c38f
    CMAKE_ARGS -DSIMDJSON_STATIC_REFLECTION=ON
)
FetchContent_MakeAvailable(simdjson)

##########
# You may also use CPM.
# https://github.com/cpm-cmake/CPM.cmake
# CPMAddPackage(
#    NAME simdjson
#    GITHUB_REPOSITORY simdjson/simdjson
#    GIT_TAG 015daad6a95a4f67c08ed5980d24b57be221c38f
#    OPTIONS "SIMDJSON_STATIC_REFLECTION ON"
# )
target_link_libraries(webservice PRIVATE simdjson::simdjson)
```

Replace the `GIT_TAG` by the appropriate value.

Once C++26 support will be officially available in mainstream compilers,
we will simplify these instructions and it will no longer be needed
to specify `SIMDJSON_STATIC_REFLECTION`.