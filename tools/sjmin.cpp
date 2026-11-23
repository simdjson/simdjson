#include <cerrno>
#include <chrono>
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
    "  -a, --arch NAME    use the NAME kernel instead of the best one\n"
    "  -l, --list-arch    list the kernels this processor supports and exit\n"
    "  -t, --timing       report the minification speed instead of the result\n"
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

void list_kernels() {
  for (auto impl : simdjson::get_available_implementations()) {
    if (impl->supported_by_runtime_system()) {
      std::printf("%-10s %s\n", impl->name().c_str(), impl->description().c_str());
    }
  }
}

bool read_stream(std::FILE *in, std::string &content) {
  char buffer[65536];
  size_t count;
  while ((count = std::fread(buffer, 1, sizeof(buffer), in)) > 0) {
    content.append(buffer, count);
  }
  return !std::ferror(in);
}

// Minify repeatedly for at least one second and return the throughput. The
// clock is read once per batch: on small inputs, reading it after every call
// would cost as much as the call.
double timed_gigabytes_per_second(const simdjson::padded_string &input, char *output) {
  using clock = std::chrono::steady_clock;
  size_t output_length;
  size_t iterations = 0;
  size_t batch = 1;
  simdjson::error_code error = simdjson::SUCCESS;
  auto start = clock::now();
  std::chrono::duration<double> elapsed;
  do {
    for (size_t i = 0; i < batch; i++) {
      error = simdjson::minify(input.data(), input.size(), output, output_length);
    }
    iterations += batch;
    if (batch < 1024) {
      batch *= 2;
    }
    elapsed = clock::now() - start;
  } while (elapsed.count() < 1.0 && !error);
  return double(input.size()) * double(iterations) / elapsed.count() / 1e9;
}

} // namespace

int main(int argc, char *argv[]) {
  const char *input_name = nullptr;
  const char *output_name = nullptr;
  const char *arch_name = nullptr;
  bool timing = false;
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
    } else if (std::strcmp(arg, "-l") == 0 || std::strcmp(arg, "--list-arch") == 0) {
      list_kernels();
      return EXIT_SUCCESS;
    } else if (std::strcmp(arg, "-t") == 0 || std::strcmp(arg, "--timing") == 0) {
      timing = true;
    } else if (std::strcmp(arg, "-o") == 0 || std::strcmp(arg, "--output") == 0) {
      if (++i == argc) {
        return usage_error("missing file name after", arg);
      }
      output_name = argv[i];
    } else if (std::strcmp(arg, "-a") == 0 || std::strcmp(arg, "--arch") == 0) {
      if (++i == argc) {
        return usage_error("missing kernel name after", arg);
      }
      arch_name = argv[i];
    } else {
      return usage_error("unknown option", arg);
    }
  }

  if (arch_name) {
    const simdjson::implementation *impl = simdjson::get_available_implementations()[arch_name];
    if (!impl) {
      return usage_error("unknown kernel", arch_name);
    }
    if (!impl->supported_by_runtime_system()) {
      std::fprintf(stderr, "%s: kernel %s is not supported by this processor\n", program_name, arch_name);
      return EXIT_FAILURE;
    }
    simdjson::get_active_implementation() = impl;
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

  if (timing) {
    std::printf("%.3f GB/s\n", timed_gigabytes_per_second(input, output.data()));
    return EXIT_SUCCESS;
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
