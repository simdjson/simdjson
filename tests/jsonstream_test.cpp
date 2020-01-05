
#include "simdjson/jsonparser.h"
#include <simdjson/jsonstream.h>

/*
Originally in the C standard library as <limits.h>.
Part of the type support library, in particular it's part of the
C numeric limits interface.
*/
#include <climits>

// this is MSVC STD LIB code
// it actually does not depend on C++20 __cplusplus 
// which is yet undefined as of 2020 Q1

#if !defined(_SIMDJSON_HAS_CXX17) && !defined(_SIMDJSON_HAS_CXX20)
#if defined(_MSVC_LANG)
#define _SIMDJSON__STL_LANG _MSVC_LANG
#else // ^^^ use _MSVC_LANG / use __cplusplus vvv
#define _SIMDJSON__STL_LANG __cplusplus
#endif // ^^^ use __cplusplus ^^^

#if _SIMDJSON__STL_LANG > 201703L
#define _SIMDJSON_HAS_CXX17 1
#define _SIMDJSON_HAS_CXX20 1
#elif _SIMDJSON__STL_LANG > 201402L
#define _SIMDJSON_HAS_CXX17 1
#define _SIMDJSON_HAS_CXX20 0
#else // _SIMDJSON__STL_LANG <= 201402L
#define _SIMDJSON_HAS_CXX17 0
#define _SIMDJSON_HAS_CXX20 0
#endif // Use the value of _SIMDJSON__STL_LANG to define _SIMDJSON_HAS_CXX17 and
       // _SIMDJSON_HAS_CXX20

#undef _SIMDJSON__STL_LANG
#endif // !defined(_SIMDJSON_HAS_CXX17) && !defined(_SIMDJSON_HAS_CXX20)

#if !defined(_SIMDJSON_HAS_CXX17)
#error This code requires C++17 or better
#endif

#if defined __has_include

#if __has_include(<filesystem>)
#include <filesystem>
#else
// NOTE: no C++17 compiler should 
// depend on this
#include <experimental/filesystem>
#endif // __has_include(<filesystem>)

#else
#error __has_include is undefined?
#endif // defined __has_include

/*
------------------------------------------------------------------------------
*/
namespace detail {
using namespace std;

// no dirent.h necessary
namespace fs = std::filesystem;

/*
------------------------------------------------------------------------------
no exceptions thrown
*/
enum class get_corpus_noex_error : unsigned short {
  OK,
  SEEK,
  TELL,
  ALLOC,
  READ,
  LOAD
};

struct get_corpus_noex_status final {
  get_corpus_noex_error code;
  char const *message;
};

inline get_corpus_noex_status get_corpus_noex_errors[]{
    {get_corpus_noex_error::OK, "OK"},
    {get_corpus_noex_error::SEEK, "cannot seek in the file"},
    {get_corpus_noex_error::TELL, "cannot tell where we are in the file"},
    {get_corpus_noex_error::ALLOC, "could not allocate memory"},
    {get_corpus_noex_error::READ, "could not read the data"},
    {get_corpus_noex_error::LOAD, "could not load corpus"}};

// no std::string necessary
char const *error_message(get_corpus_noex_error code_) {
  return get_corpus_noex_errors[size_t(code_)].message;
}
/*
------------------------------------------------------------------------------
no exceptions thrown
*/
static get_corpus_noex_error get_corpus_noex(
    simdjson::padded_string &rezult, std::string_view filename) noexcept
{
  std::FILE *fp = std::fopen(filename.data(), "rb");
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
    perror("\nERROR could not std::ftell(), ");
    return get_corpus_noex_error::TELL;
  }

  size_t len = size_t(llen);
  simdjson::padded_string temp(len);
  rezult.swap(temp);
  if (rezult.data() == nullptr) {
    std::fclose(fp);
    perror("\nERROR could not allocate padded string, ");
    return get_corpus_noex_error::ALLOC;
  }
  std::rewind(fp);
  size_t readb = std::fread(rezult.data(), 1, len, fp);
  std::fclose(fp);
  if (readb != len) {
    perror("\nERROR could not std::fread(), ");
    return get_corpus_noex_error::READ;
  }
  return get_corpus_noex_error::OK;
}

/*
------------------------------------------------------------------------------
The actual test
*/
static void the_actual_test(std::string_view fullpath) {

  simdjson::padded_string pstring_{};

  /* populate pstring with file contents */
  get_corpus_noex_error ec_ = get_corpus_noex(pstring_, fullpath);

  if (get_corpus_noex_error::OK != ec_) {
    printf("\nCould not load the contents of the file %s, error: %s",
           fullpath.data(), error_message(ec_));
    return;
  }

  printf("\nTesting simdjson::JsonStream with: %s", fullpath.data());

  simdjson::ParsedJson pj;
  simdjson::JsonStream js{pstring_.data(), pstring_.size()};

  int parse_res = simdjson::SUCCESS_AND_HAS_MORE;
  while (parse_res == simdjson::SUCCESS_AND_HAS_MORE) {
    parse_res = js.json_parse(pj);
  }

  printf("\n\nStatus: %s\n", parse_res == 0 ? "ok" : "invalid, ");

  if (parse_res != simdjson::SUCCESS)
    printf(" error message: '%s'\n\n", simdjson::error_message(parse_res).c_str());
}
/*
------------------------------------------------------------------------------
*/
static std::uintmax_t compute_file_size(const fs::path &pathToCheck) {
  if (fs::exists(pathToCheck) && fs::is_regular_file(pathToCheck)) {
    std::error_code err;
    auto filesize = fs::file_size(pathToCheck, err);
    if (!err) {
      return filesize;
    }
  }
  return static_cast<uintmax_t>(-1);
}

/*
------------------------------------------------------------------------------
*/
static void file_callback(fs::directory_entry entry) {

  fs::path filename_{entry.path().filename()};
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

/*
------------------------------------------------------------------------------
*/
static void traverse_folder_imp(const fs::path &path_to_use, int level,
                         bool drill = false) {

  if (!(fs::exists(path_to_use) && fs::is_directory(path_to_use)))
    /* silent error? not good */
    return;

  for (const auto &entry : fs::directory_iterator(path_to_use)) {
    if (fs::is_directory(entry.status())) {
      if (drill)
        traverse_folder_imp(entry, level + 1);
    } else if (fs::is_regular_file(entry.status())) {
      file_callback(entry);
    } else {
      /* could be symlink? ignore ... */
    }
  }
}

/*
------------------------------------------------------------------------------
*/
static void traverse_folder(const fs::path &path_to_use, bool drill = false) {
  traverse_folder_imp(path_to_use, 0 /* level */, drill);
}
} // namespace detail

#ifdef SIMDJSON_TEST_DATA_DIR
constexpr const char *simdjson_test_data_dir{SIMDJSON_TEST_DATA_DIR};
#else
constexpr const char *simdjson_test_data_dir{nullptr}; // empty
#endif
/*
------------------------------------------------------------------------------
*/
int main(int argc, char *argv[]) {
  using namespace std;
  string folder_to_use{};
  if (argc == 2) {
    folder_to_use = argv[1];
  } else {
    printf("\n\nUsage:\n %s 'directory_with_json_files'\n", argv[0]);
    if (simdjson_test_data_dir) {
      printf("\nDefault to SIMDJSON_TEST_DATA_DIR");
      folder_to_use = simdjson_test_data_dir;
    } else {
      printf("\n\nNo argument given and SIMDJSON_TEST_DATA_DIR not found?\n\n");
      return (EXIT_FAILURE);
    }
  }
  printf("\n\nGoing to use '%s' as a folder with json file to sample.\n",
         folder_to_use.c_str());
  detail::traverse_folder(folder_to_use);
  /*
  Do not depend on the json files tested
  */
  return EXIT_SUCCESS;
}
