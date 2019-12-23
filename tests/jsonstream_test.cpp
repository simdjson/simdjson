#include <filesystem>

#include <cassert>
#include <cstring>
#ifndef _MSC_VER
#include <dirent.h>
#include <unistd.h>
#else
// Microsoft can't be bothered to provide standard utils.
// #include <dirent_portable.h>
#endif
#include <cinttypes>

#include <cstdio>
#include <cstdlib>
#include <simdjson/jsonstream.h>

#include "simdjson/jsonparser.h"

namespace detail {
using namespace std;
namespace fs = std::filesystem;

enum class get_corpus_noex_error {
  OK,
  SEEK,  //("cannot seek in the file");
  TELL,  // ("cannot tell where we are in the file");
  ALLOC, // ("could not allocate memory");
  READ,  // ("could not read the data");
  LOAD   // ("could not load corpus");
};

struct get_corpus_noex_status {
  get_corpus_noex_error code;
  char const *message;
};

static get_corpus_noex_status get_corpus_noex_errors[]{
    {get_corpus_noex_error::OK, "OK"},
    {get_corpus_noex_error::SEEK, "cannot seek in the file"},
    {get_corpus_noex_error::TELL, "cannot tell where we are in the file"},
    {get_corpus_noex_error::ALLOC, "could not allocate memory"},
    {get_corpus_noex_error::READ, "could not read the data"},
    {get_corpus_noex_error::LOAD, "could not load corpus"}};

char const *get_corpus_noex_error_message(get_corpus_noex_error code_) {
  return get_corpus_noex_errors[ size_t(code_)].message;
}

get_corpus_noex_error get_corpus_noex(simdjson::padded_string &rezult,
                                      std::string const &filename) noexcept {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp == nullptr) {
    perror("\nERROR could not std::fopen(), ");
    return get_corpus_noex_error::LOAD;
  }

    if (std::fseek(fp, 0, SEEK_END) < 0) {
      std::fclose(fp);
      return get_corpus_noex_error::SEEK;
    }
    long llen = std::ftell(fp);
    if ((llen < 0) || (llen == LONG_MAX)) {
      std::fclose(fp);
      return get_corpus_noex_error::TELL;
    }

    size_t len = size_t(llen);
    simdjson::padded_string temp(len);
    rezult.swap(temp);
    if (rezult.data() == nullptr) {
      std::fclose(fp);
      return get_corpus_noex_error::ALLOC;
    }
    std::rewind(fp);
    size_t readb = std::fread(rezult.data(), 1, len, fp);
    std::fclose(fp);
    if (readb != len) {
      return get_corpus_noex_error::READ;
    }
    return get_corpus_noex_error::OK;
 
}

/*
------------------------------------------------------------------------------
The actual test
*/
void the_actual_test(std::string fullpath) {

  simdjson::padded_string p{};

    get_corpus_noex_error ec_ = get_corpus_noex(p, fullpath);

    if (get_corpus_noex_error::OK != ec_) {
      printf("\nCould not load the contents of the file %s, error: %s", fullpath.c_str(),
             get_corpus_noex_error_message(ec_));
    return;
  }

  printf("\nTesting simdjson::JsonStream with: %s", fullpath.c_str());

  simdjson::ParsedJson pj;
  simdjson::JsonStream js{p.data(), p.size()};

  int parse_res = simdjson::SUCCESS_AND_HAS_MORE;
  while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
    parse_res = js.json_parse(pj);
  }

  printf("\n\nStatus: %s\n", parse_res == 0 ? "ok" : "invalid, ");

  if (parse_res != simdjson::SUCCESS)
    printf(" error message: '%s'\n\n", simdjson::error_message(parse_res));
}
/*
------------------------------------------------------------------------------
*/
std::uintmax_t compute_file_size(const fs::path &pathToCheck) {
  if (fs::exists(pathToCheck) && fs::is_regular_file(pathToCheck)) {
    std::error_code err;
    auto filesize = fs::file_size(pathToCheck, err);
    if (!err) {
      return filesize;
    }
  }
  return static_cast<uintmax_t>(-1);
}

void file_callback(fs::directory_entry entry, fs::path filename_) {

  printf("\n========================================="
         "\nUsing %s",
         filename_.generic_string().c_str());

  if (filename_.extension().string() != ".json")
    if (filename_.extension().string() != ".ndjson") {
      printf("\nExtension is not '.json' or '.ndjson', leaving ...");
      return;
    }

  std::error_code err;
  fs::path ap_ = std::filesystem::absolute(filename_, err);

  if (err) {
    printf("\nError on absolute path: %s", err.message().c_str());
    return;
  }

  the_actual_test(entry.path().string());
}

void traverse_folder_imp(const fs::path &pathToShow, int level,
                         bool drill = false) {

  if (!(fs::exists(pathToShow) && fs::is_directory(pathToShow)))
    return;

  for (const auto &entry : fs::directory_iterator(pathToShow)) {
    auto filename = entry.path().filename();
    if (drill && fs::is_directory(entry.status())) {
      traverse_folder_imp(entry, level + 1);
    } else if (fs::is_regular_file(entry.status()))
      file_callback(entry, filename);
    else
      cout << "\n[ not a file or directory? ]" << filename << "\n";
  }
}

// adapted from Modern C++ Programming Cookbook
void traverse_folder(const fs::path &pathToShow) {
  traverse_folder_imp(pathToShow, 0);
}
} // namespace detail

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "\n\nUsage: " << argv[0] << " <directorywithjsonfiles>"
              << std::endl;
#ifndef SIMDJSON_TEST_DATA_DIR
    std::cout << "\n\nWe are going to assume you mean to use the 'jsonchecker' "
                 "directory."
              << std::endl;
    return validate("jsonchecker/") ? EXIT_SUCCESS : EXIT_FAILURE;
#else
    std::cout << "\n\nWe are going to assume you mean to use the '"
              << SIMDJSON_TEST_DATA_DIR << "' directory." << std::endl;

    detail::traverse_folder(SIMDJSON_TEST_DATA_DIR);

    return EXIT_SUCCESS;
#endif
  }
   detail::traverse_folder(argv[1]) ;
  return EXIT_SUCCESS ;
}
