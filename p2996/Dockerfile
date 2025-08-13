# Stage 1. Check out LLVM source code and run the build.
FROM debian:12 AS builder
# First, Update the apt's source list and include the sources of the packages.
RUN grep deb /etc/apt/sources.list | \
    sed 's/^deb/deb-src /g' >> /etc/apt/sources.list
# Install compiler, python and subversion.
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates gnupg \
           build-essential cmake make python3 zlib1g wget subversion unzip ninja-build git linux-perf && \
    rm -rf /var/lib/apt/lists/*
ARG CLANG_COMMIT=d77eff1cbd78fd065668acf93b1f5f400d39134d
RUN git clone --depth=1 https://github.com/bloomberg/clang-p2996.git /tmp/clang-source && \
    cd /tmp/clang-source && \
    git fetch origin $CLANG_COMMIT --depth=1 && \
    git checkout $CLANG_COMMIT
RUN cmake -S /tmp/clang-source/llvm -B /tmp/clang-source/build-llvm -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_UNREACHABLE_OPTIMIZE=ON \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
    -DCLANG_DEFAULT_CXX_STDLIB=libc++ \
    -DLLVM_ENABLE_PROJECTS=clang -G Ninja
RUN cmake --build /tmp/clang-source/build-llvm -j
RUN cmake --install /tmp/clang-source/build-llvm --prefix /tmp/clang-install


# Stage 2. Produce a minimal release image with build results.
FROM debian:12
ARG USER_NAME
ARG USER_ID
ARG GROUP_ID
LABEL maintainer "LLVM Developers"
# Install packages for minimal useful image.
RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential  ca-certificates rust-all libcurl4-openssl-dev cmake make wget python3 python3-dev sudo curl ninja-build vim git binutils linux-perf && \
    rm -rf /var/lib/apt/lists/*
# Copy build results of stage 1 to /usr/local.
COPY --from=builder /tmp/clang-install/ /usr/local/
RUN P=`/usr/local/bin/clang++ -v 2>&1 | grep Target | cut -d' ' -f2-`; echo /usr/local/lib/$P > /etc/ld.so.conf.d/$P.conf
RUN ldconfig
RUN addgroup --gid $GROUP_ID user; exit 0
RUN adduser --disabled-password --gecos '' --uid $USER_ID --gid $GROUP_ID $USER_NAME; exit 0
RUN echo "$USER_NAME:$USER_NAME" | chpasswd && adduser $USER_NAME sudo
RUN echo '----->'
RUN echo 'root:Docker!' | chpasswd
ENV TERM xterm-256color
USER $USER_NAME
