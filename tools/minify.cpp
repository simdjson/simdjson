#include <iostream>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include <unistd.h>

#include "simdjson.h"
#include "cxxopts.hpp"

cxxopts::Options options("minify", "Runs the parser against the given json files in a loop, measuring speed and other statistics.");

void usage(std::string message) {
  std::cerr << message << std::endl;
  std::cerr << options.help() << std::endl;
  std::cerr << "Available parser implementations:" << std::endl;
  for (auto impl : simdjson::available_implementations) {
    std::cerr << "-a " << std::left << std::setw(9) << impl->name() << " - Use the " << impl->description() << " parser implementation." << std::endl;
  }
}

int main(int argc, char *argv[]) {
  options.add_options()
    ("a,arch", "Parser implementation (by default, detects the most advanced implementation supported on the host machine).", cxxopts::value<std::string>())
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
    const simdjson::implementation *impl = simdjson::available_implementations[result["arch"].as<std::string>().c_str()];
    if(!impl) {
      usage("Unsupported implementation.");
      return EXIT_FAILURE;
    }
    simdjson::active_implementation = impl;
  }

  std::string filename = result["file"].as<std::string>();

  auto [p, error] = simdjson::padded_string::load(filename);
  if (error) {
    std::cerr << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  simdjson::padded_string copy(p.length());
  size_t copy_len;
  error = simdjson::active_implementation->minify((const uint8_t*)p.data(), p.length(), (uint8_t*)copy.data(), copy_len);
  if (error) { std::cerr << error << std::endl; return 1; }
  printf("%s", copy.data());
}
