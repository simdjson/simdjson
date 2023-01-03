#include <iostream>
#include <unistd.h>
#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#ifndef __cpp_exceptions
#define CXXOPTS_NO_EXCEPTIONS
#endif
#include "cxxopts.hpp"
SIMDJSON_POP_DISABLE_WARNINGS

#if CXXOPTS__VERSION_MAJOR < 3
int main(int argc, char *argv[]) {
#else
int main(int argc, const char *argv[]) {
#endif
#ifdef __cpp_exceptions
  try {
#endif
  std::string progName = "json2json";

  std::string progUsage = "json2json version ";
  progUsage += SIMDJSON_VERSION;
  progUsage += " (";
  progUsage += simdjson::get_active_implementation()->name();
  progUsage += ")\n";
  progUsage += "Reads json in, out the result of the parsing.\n";
  progUsage += argv[0];
  progUsage += " <jsonfile>\n";

  cxxopts::Options options(progName, progUsage);

  options.add_options()
    ("z,ondemand", "Use On Demand front-end.", cxxopts::value<bool>()->default_value("false"))
  	("d,rawdump", "Dumps the raw content of the tape.", cxxopts::value<bool>()->default_value("false"))
  	("f,file", "File name.", cxxopts::value<std::string>())
  	("h,help", "Print usage.")
  ;

  // the first argument without an option name will be parsed into file
  options.parse_positional({"file"});
  auto result = options.parse(argc, argv);

  if(result.count("help")) {
  	std::cerr << options.help() << std::endl;
  	return EXIT_SUCCESS;
  }
  bool ondemand = result["ondemand"].as<bool>();
  bool rawdump = result["rawdump"].as<bool>();

  if(!result.count("file")) {
    std::cerr << "No filename specified." << std::endl;
    std::cerr << options.help() << std::endl;
    return EXIT_FAILURE;
  }

  const char *filename = result["file"].as<std::string>().c_str();
  if(ondemand) {
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata;
    auto error = simdjson::padded_string::load(filename).get(docdata);
    if(error != simdjson::SUCCESS) { std::cout << error << std::endl; return EXIT_FAILURE; }
    simdjson::ondemand::document doc;
    error = parser.iterate(docdata).get(doc);
    if(error != simdjson::SUCCESS) { std::cout << error << std::endl; return EXIT_FAILURE; }
    // This locates the document and captures a string_view instance, it does
    // not validate the document:
    std::cout << doc;
    // check if there is more content
    const char * endofstream;
    if(doc.current_location().get(endofstream) == simdjson::SUCCESS) {
      // there is more content !
      // let us find what it is:
      size_t len = docdata.data() + docdata.size() - endofstream;
      std::string_view content{endofstream, len};
      std::cerr << "\nThere is leftover content: '" << content << "'" << std::endl;
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.load(filename).get(doc); // do the parsing, return false on error
  if (error) {
    std::cerr << " Parsing failed. Error is '" << error << "'." << std::endl;
    return EXIT_FAILURE;
  }
  if(rawdump) {
    doc.dump_raw_tape(std::cout);
  } else {
    std::cout << doc;
  }
  return EXIT_SUCCESS;
#ifdef __cpp_exceptions
  } catch (const cxxopts::OptionException& e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
#endif
}
