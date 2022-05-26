#include <chrono>
#include <iostream>
#if (!(_MSC_VER) && !(__MINGW32__) && !(__MINGW64__))
#include <dirent.h>
#endif
#include <unistd.h>

#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#ifndef __cpp_exceptions
#define CXXOPTS_NO_EXCEPTIONS
#endif
#include "cxxopts.hpp"
SIMDJSON_POP_DISABLE_WARNINGS

cxxopts::Options options("minify", "Runs the parser against the given json files in a loop, measuring speed and other statistics.");

void usage(std::string message) {
  std::cerr << message << std::endl;
  std::cerr << options.help() << std::endl;
}

#if CXXOPTS__VERSION_MAJOR < 3
int main(int argc, char *argv[]) {
#else
int main(int argc, const char *argv[]) {
#endif
#ifdef __cpp_exceptions
  try {
#endif
  std::stringstream ss;
  ss << "Parser implementation (by default, detects the most advanced implementation supported on the host machine)."  << std::endl;
  ss << "Available parser implementations:" << std::endl;
  for (auto impl : simdjson::get_available_implementations()) {
    if(impl->supported_by_runtime_system()) {
      ss << "-a " << std::left << std::setw(9) << impl->name() << " - Use the " << impl->description() << " parser implementation." << std::endl;
    }
  }
  options.add_options()
    ("a,arch", ss.str(), cxxopts::value<std::string>())
    ("t,timing", "Report only timing.")
    ("f,file", "File name.", cxxopts::value<std::string>())
    ("h,help", "Print usage.")
  ;

  options.parse_positional({"file"});
  auto result = options.parse(argc, argv);

  if(result.count("help")) {
    usage("");
    return EXIT_SUCCESS;
  }

  if(!result.count("file")) {
    usage("No filename specified.");
    return EXIT_FAILURE;
  }
  if(result.count("arch")) {
    const simdjson::implementation *impl = simdjson::get_available_implementations()[result["arch"].as<std::string>().c_str()];
    if(!impl) {
      usage("Unsupported implementation.");
      return EXIT_FAILURE;
    }
    if(!impl->supported_by_runtime_system()) {
      usage("The selected implementation does not match your current CPU.");
      return EXIT_FAILURE;
    }
    simdjson::get_active_implementation() = impl;
  }

  std::string filename = result["file"].as<std::string>();

  simdjson::padded_string p;
  auto error = simdjson::padded_string::load(filename).get(p);
  if (error) {
    std::cerr << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  simdjson::padded_string copy(p.length()); // does not need to be padded after all!
  size_t copy_len;
  error = simdjson::get_active_implementation()->minify((const uint8_t*)p.data(), p.length(), (uint8_t*)copy.data(), copy_len);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  /**
   * If a user only wants to time the required time, we do not output
   * the result and we simply do the processing in a tight loop.
   * At this point in time, we can assume that the processing will
   * succeed.
   */
  if(result.count("timing")) {
      uint64_t beforens = std::chrono::duration_cast<::std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
             .count();
      error = simdjson::get_active_implementation()->minify((const uint8_t*)p.data(), p.length(), (uint8_t*)copy.data(), copy_len);
      uint64_t afterns = std::chrono::duration_cast<::std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
             .count();
      size_t times = 1;
      while(afterns - beforens < 1000000000) {
          error = simdjson::get_active_implementation()->minify((const uint8_t*)p.data(), p.length(), (uint8_t*)copy.data(), copy_len);
          afterns = std::chrono::duration_cast<::std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
             .count();
          times += 1;
      }
      if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
      printf("%.3f GB/s\n", double(p.length() * times) / double(afterns - beforens));
  } else {
      // This is the expected path:
      printf("%s", copy.data());
  }
  return EXIT_SUCCESS;
#ifdef __cpp_exceptions
  } catch (const cxxopts::OptionException& e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
#endif
}
