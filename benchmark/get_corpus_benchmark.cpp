
#include "simdjson.h"
#include <chrono>
#include <cstring>
#include <iostream>

never_inline
double bench(std::string filename, simdjson::padded_string& p) {
  std::chrono::time_point<std::chrono::steady_clock> start_clock =
      std::chrono::steady_clock::now();
  simdjson::padded_string::load(filename).first.swap(p);
  std::chrono::time_point<std::chrono::steady_clock> end_clock =
      std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed = end_clock - start_clock;
  return (p.size() / (1024. * 1024 * 1024.)) / elapsed.count();
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
  simdjson::padded_string p;
  bench(filename, p); 
  double meanval = 0;
  double maxval = 0;
  double minval = 10000;
std::cout << "file size: "<<  (p.size() / (1024. * 1024 * 1024.)) << " GB" <<std::endl;
  size_t times = p.size() > 1024*1024*1024 ? 5 : 50;
#if __cpp_exceptions
  try {
#endif
    for(size_t i = 0; i < times; i++) {
      double tval = bench(filename, p);
      if(maxval < tval) maxval = tval;
      if(minval > tval) minval = tval;
      meanval += tval;
    }
#if __cpp_exceptions
   } catch (const std::exception &) { // caught by reference to base
    std::cerr << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
   }
#endif
   std::cout << "average speed: " << meanval / times << " GB/s"<< std::endl;
   std::cout << "min speed    : " << minval << " GB/s" << std::endl;
   std::cout << "max speed    : " << maxval << " GB/s" << std::endl;
   return EXIT_SUCCESS;
}