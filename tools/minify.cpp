#include <iostream>
#ifndef _MSC_VER
#include <dirent.h>
#include <unistd.h>
#endif

#include "simdjson.h"

// Stash the exe_name in main() for functions to use
char* exe_name;

void print_usage(std::ostream& out) {
  out << "Usage: " << exe_name << "  [-a ARCH] <jsonfile>" << std::endl;
  out << std::endl;
  out << "Runs the parser against the given json files in a loop, measuring speed and other statistics." << std::endl;
  out << std::endl;
  out << "Options:" << std::endl;
  out << std::endl;
  out << "-a IMPL      - Use the given parser implementation. By default, detects the most advanced" << std::endl;
  out << "               implementation supported on the host machine." << std::endl;
  for (auto impl : simdjson::available_implementations) {
    out << "-a " << std::left << std::setw(9) << impl->name() << " - Use the " << impl->description() << " parser implementation." << std::endl;
  }
}

void exit_usage(std::string message) {
  std::cerr << message << std::endl;
  std::cerr << std::endl;
  print_usage(std::cerr);
  exit(EXIT_FAILURE);
}


struct option_struct {
  char* filename;
 
  option_struct(int argc, char **argv) {
    #ifndef _MSC_VER
      int c;

      while ((c = getopt(argc, argv, "a:")) != -1) {
        switch (c) {
        case 'a': {
          const simdjson::implementation *impl = simdjson::available_implementations[optarg];
          if (!impl) {
            std::string exit_message = std::string("Unsupported option value -a ") + optarg + ": expected -a  with one of ";
            for (auto imple : simdjson::available_implementations) {
              exit_message += imple->name();
              exit_message += " ";
            }
            exit_usage(exit_message);
          }
          simdjson::active_implementation = impl;
          break;
        }
        default:
          // reaching here means an argument was given to getopt() which did not have a case label
          exit_usage("Unexpected argument - missing case for option "+
                     std::string(1,static_cast<char>(c))+
                     " (programming error)");
        }
      }
    #else
      int optind = 1;
    #endif

    // All remaining arguments are considered to be files
    if(optind + 1 == argc) {
      filename = argv[optind];
    } else {
      exit_usage("Please specify exactly one input file.");
    }
  }
};

int main(int argc, char *argv[]) {
  exe_name = argv[0];
  option_struct options(argc, argv);
  std::string filename = options.filename;
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
