
#include "simdjson/common_defs.h"
#include "simdjson/jsonioutil.h"
#include "simdjson/jsonparser.h"
#include <chrono>
#include <cstring>
#include <iostream>

using CLOCK = std::chrono::steady_clock;

static double bench(std::string_view filename,
                    simdjson::padded_string &p_string_) {

  std::chrono::time_point<CLOCK> start_clock = CLOCK::now();

  // invalidates the previous p_string_
  p_string_ = simdjson::get_corpus(filename.data());

  std::chrono::duration<double> elapsed = CLOCK::now() - start_clock;

  // GB per nano seconds
  return (p_string_.size() / (1024. * 1024 * 1024.)) / elapsed.count();
}

int main(int argc, char *argv[]) {
  int optind = 1;
  if (optind >= argc) {
    std::cerr << "Reads document as far as possible. " << std::endl;
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    exit(1);
  }
  const char *filename = argv[optind];
  if (optind + 1 < argc) {
    std::cerr << "warning: ignoring everything after " << argv[optind + 1]
              << std::endl;
  }
  simdjson::padded_string p_string_{};
  p_string_ = simdjson::get_corpus(filename);
  double meanval = 0;
  double maxval = 0;
  double minval = 10000;
  std::cout << "file size: " << (p_string_.size() / (1024. * 1024 * 1024.))
            << " GB" << std::endl;
  size_t times = p_string_.size() > 1024 * 1024 * 1024 ? 5 : 50;
  try {
    for (size_t i = 0; i < times; i++) {
      double tval = bench(filename, p_string_);
      if (maxval < tval)
        maxval = tval;
      if (minval > tval)
        minval = tval;
      meanval += tval;
    }
  } catch (const std::exception &) { // caught by reference to base
    std::cerr << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "average speed: " << meanval / times << " GB/s" << std::endl;
  std::cout << "min speed    : " << minval << " GB/s" << std::endl;
  std::cout << "max speed    : " << maxval << " GB/s" << std::endl;
  return EXIT_SUCCESS;
}