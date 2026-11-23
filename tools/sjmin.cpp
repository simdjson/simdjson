#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "simdjson.h"

namespace {

const char *program_name = "sjmin";

void print_usage(std::FILE *out) {
  std::fprintf(out,
    "Usage: %s [OPTION]... [FILE]\n"
    "Remove the insignificant white space from the JSON document in FILE.\n"
    "With no FILE, or when FILE is -, read standard input.\n"
    "\n"
    "  -o, --output FILE  write to FILE instead of standard output\n"
    "  -h, --help         display this help and exit\n"
    "  -v, --version      output version information and exit\n"
    "\n"
    "The input is not validated. Exit status is 0 on success, 1 on error,\n"
    "2 on a usage error.\n",
    program_name);
}

int usage_error(const char *message, const char *argument) {
  std::fprintf(stderr, "%s: %s %s\n", program_name, message, argument);
  std::fprintf(stderr, "Try '%s --help' for more information.\n", program_name);
  return 2;
}

bool read_stream(std::FILE *in, std::string &content) {
  char buffer[65536];
  size_t count;
  while ((count = std::fread(buffer, 1, sizeof(buffer), in)) > 0) {
    content.append(buffer, count);
  }
  return !std::ferror(in);
}

} // namespace

int main(int argc, char *argv[]) {
  const char *input_name = nullptr;
  const char *output_name = nullptr;
  bool options_done = false;

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];
    if (options_done || arg[0] != '-' || std::strcmp(arg, "-") == 0) {
      if (input_name) {
        return usage_error("extra operand", arg);
      }
      input_name = arg;
    } else if (std::strcmp(arg, "--") == 0) {
      options_done = true;
    } else if (std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0) {
      print_usage(stdout);
      return EXIT_SUCCESS;
    } else if (std::strcmp(arg, "-v") == 0 || std::strcmp(arg, "--version") == 0) {
      std::printf("%s (simdjson) %s\n", program_name, SIMDJSON_VERSION);
      return EXIT_SUCCESS;
    } else if (std::strcmp(arg, "-o") == 0 || std::strcmp(arg, "--output") == 0) {
      if (++i == argc) {
        return usage_error("missing file name after", arg);
      }
      output_name = argv[i];
    } else {
      return usage_error("unknown option", arg);
    }
  }

  simdjson::padded_string input;
  if (input_name == nullptr || std::strcmp(input_name, "-") == 0) {
    input_name = "-";
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    std::string content;
    if (!read_stream(stdin, content)) {
      std::fprintf(stderr, "%s: error reading standard input\n", program_name);
      return EXIT_FAILURE;
    }
    input = simdjson::padded_string(content);
  } else {
    auto error = simdjson::padded_string::load(input_name).get(input);
    if (error) {
      std::fprintf(stderr, "%s: %s: %s\n", program_name, input_name, simdjson::error_message(error));
      return EXIT_FAILURE;
    }
  }

  simdjson::padded_string output(input.size());
  size_t output_length;
  auto error = simdjson::minify(input.data(), input.size(), output.data(), output_length);
  if (error) {
    std::fprintf(stderr, "%s: %s: %s\n", program_name, input_name, simdjson::error_message(error));
    return EXIT_FAILURE;
  }

  std::FILE *out = stdout;
  if (output_name && std::strcmp(output_name, "-") != 0) {
    out = std::fopen(output_name, "wb");
    if (out == nullptr) {
      std::fprintf(stderr, "%s: %s: %s\n", program_name, output_name, std::strerror(errno));
      return EXIT_FAILURE;
    }
  } else {
    output_name = "standard output";
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
  }
  bool write_failed = std::fwrite(output.data(), 1, output_length, out) != output_length;
  if (out == stdout) {
    write_failed |= std::fflush(out) != 0;
  } else {
    write_failed |= std::fclose(out) != 0;
  }
  if (write_failed) {
    std::fprintf(stderr, "%s: error writing %s\n", program_name, output_name);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
