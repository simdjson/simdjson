# From the ROOT, run:
# docker build -t simdjsonbench -f benchmark/Dockerfile . && docker run --privileged -t simdjsonbench
FROM gcc:8.3

# # Build latest
# ENV latest_release=v0.2.1
# WORKDIR /usr/src/$latest_release/
# RUN git clone --depth 1 https://github.com/lemire/simdjson/ -b $latest_release .
# RUN make parse

# # Build master
# WORKDIR /usr/src/master/
# RUN git clone --depth 1 https://github.com/lemire/simdjson/ .
# RUN make parse

# Build the current source
COPY . /usr/src/current/
WORKDIR /usr/src/current/
RUN make checkperf