// Standalone Glaze benchmark for Twitter, built with g++ instead of the
// p2996 clang fork (which crashes on Glaze's heavy template metaprogramming).
// Reuses the same data structs and bench() helper as the in-tree benchmarks
// so the throughput numbers are directly comparable.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../twitter_benchmark/twitter_data.h"
#include "../twitter_benchmark/glaze_twitter_data.h"
#include "../benchmark_utils/benchmark_helper.h"

namespace {

std::string read_file(const std::string &filename) {
    printf("# Reading file %s\n", filename.c_str());
    constexpr size_t read_size = 4096;
    std::ifstream stream(filename);
    if (!stream) {
        std::cerr << "Could not open file: " << filename << std::endl;
        std::exit(EXIT_FAILURE);
    }
    stream.exceptions(std::ios_base::badbit);
    std::string out;
    std::string buf(read_size, '\0');
    while (stream.read(&buf[0], read_size)) {
        out.append(buf, 0, size_t(stream.gcount()));
    }
    out.append(buf, 0, size_t(stream.gcount()));
    return out;
}

void bench_glaze_serialization(TwitterData &data) {
    std::string output = glaze_serialize(data);
    size_t output_volume = output.size();
    printf("# output volume: %zu bytes\n", output_volume);

    volatile size_t measured_volume = 0;
    pretty_print(1, output_volume, "bench_glaze",
                 bench([&data, &measured_volume, &output_volume]() {
                     std::string output = glaze_serialize(data);
                     measured_volume = output.size();
                     if (measured_volume != output_volume) {
                         printf("mismatch\n");
                     }
                 }));
}

void bench_glaze_parsing(const std::string &json_str) {
    size_t input_volume = json_str.size();
    printf("# input volume: %zu bytes\n", input_volume);

    volatile bool result = true;
    pretty_print(1, input_volume, "bench_glaze_parsing",
                 bench([&json_str, &result]() {
                     try {
                         TwitterData data = glaze_deserialize(json_str);
                         result = true;
                     } catch (...) {
                         result = false;
                         printf("parse error\n");
                     }
                 }));
}

}  // namespace

int main(int argc, char *argv[]) {
    const char *json_file = std::getenv("TWITTER_JSON");
    if (!json_file) {
        json_file = "buildreflect/jsonexamples/twitter.json";
    }
    if (argc > 1) {
        json_file = argv[1];
    }

    std::string json_str = read_file(json_file);

    const char *mode = std::getenv("BENCH_MODE");
    if (!mode) mode = "all";
    TwitterData data = glaze_deserialize(json_str);

    if (std::string(mode) == "all" || std::string(mode) == "parse") {
        printf("\n=== Glaze Twitter Parsing ===\n");
        bench_glaze_parsing(json_str);
    }
    if (std::string(mode) == "all" || std::string(mode) == "serialize") {
        printf("\n=== Glaze Twitter Serialization ===\n");
        bench_glaze_serialization(data);
    }

    return EXIT_SUCCESS;
}
