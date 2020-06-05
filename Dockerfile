###
#
# Though simdjson requires only commonly available compilers and tools, it can
# be convenient to build it and test it inside a docker container: it makes it
# possible to test and benchmark simdjson under even relatively out-of-date
# Linux servers. It should also work under macOS and Windows, though not
# at native speeds, maybe.
#
# Assuming that you have a working docker server, this file
# allows you to build, test and benchmark simdjson.
#
#  We build the library and associated files in the dockerbuild subdirectory.
# It may be necessary to delete it before creating the image:
#
# rm -r -f dockerbuild
#
# The need to delete the directory has nothing to do with docker per se: it is
# simply cleaner in CMake to start from a fresh directory. This is important: if you
# reuse the same directory with different configurations, you may get broken builds.
#
#
# Then you can build the image as follows:
#
# docker build -t simdjson --build-arg USER_ID=$(id -u)  --build-arg GROUP_ID=$(id -g) .
#
# Please note that the image does not contain a copy of the code. However, the image will contain the
# the compiler and the build system. This means that if you change the source code, after you have built
# the image, you won't need to rebuild the image. In fact, unless you want to try a different compiler, you
# do not need to ever rebuild the image, even if you do a lot of work on the source code.
#
# We specify the users to avoid having files owned by a privileged user (root) in our directory. Some
# people like to run their machine as the "root" user. We do not think it is cool.
#
# Then you need to build the project:
#
# docker run  -v $(pwd):/project:Z  simdjson
#
# Should you change a source file, you may need to call this command again. Because the output
# files are persistent between calls to this command (they reside in the dockerbuild directory),
# this command can be fast.
#
# Next you can test it as follows:
#
# docker run -it -v $(pwd):/project:Z   simdjson sh -c "cd dockerbuild && ctest . --output-on-failure -E checkperf"
#
# The run the complete tests requires you to have built all of simdjson.
#
# Building all of simdjson takes a long time. Instead, you can build just one target:
#
# docker run -it -v $(pwd):/project:Z  simdjson sh -c "[ -d dockerbuild ] || mkdir dockerbuild && cd dockerbuild && cmake ..  && cmake --build . --target parse"
#
# Note that it is safe to remove dockerbuild before call the previous command, as the repository gets rebuild. It is also possible, by changing the command, to use a different directory name.
#
# You can run performance tests:
#
# docker run -it --privileged -v $(pwd):/project:Z  simdjson sh -c "cd dockerbuild && for i in ../jsonexamples/*.json; do echo \$i; ./benchmark/parse \$i;  done"
#
# The "--privileged" is recommended so you can get performance counters under Linux.
#
# You can also grab a fresh copy of simdjson and rebuild it, to make comparisons:
#
# docker run -it -v $(pwd):/project:Z  simdjson sh -c "git clone https://github.com/simdjson/simdjson.git && cd simdjson && mkdir build && cd build && cmake .. && cmake --build . --target parse "
#
# Then you can run comparisons:
#
# docker run -it --privileged -v $(pwd):/project:Z  simdjson sh -c "for i in jsonexamples/*.json; do echo \$i; dockerbuild/benchmark/parse \$i| grep GB| head -n 1; simdjson/build/benchmark/parse \$i | grep GB |head -n 1;  done"
#
####
FROM ubuntu:20.10
################
# We would prefer to use the conan io images but they do not support 64-bit ARM? The small gcc images appear to
# be broken on ARM.
# Furthermore, we would not expect users to frequently rebuild the container, so using ubuntu is probably fine.
###############
ARG USER_ID
ARG GROUP_ID
RUN apt-get update -qq
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata
RUN apt-get install -y cmake g++ git
RUN mkdir project

RUN addgroup --gid $GROUP_ID user; exit 0
RUN adduser --disabled-password --gecos '' --uid $USER_ID --gid $GROUP_ID user; exit 0
USER user
RUN gcc --version
WORKDIR /project

CMD ["sh","-c","[ -d dockerbuild ] || mkdir dockerbuild && cd dockerbuild && cmake .. && cmake --build . "]
