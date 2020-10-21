#include "simdjson.h"

#include <unistd.h>
#ifndef _MSC_VER
#include "linux-perf-events.h"
#ifdef __linux__
#include <libgen.h>
#endif //__linux__
#endif // _MSC_VER

#include <memory>

#include "benchmark.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#include "yyjson.h"

// #define RAPIDJSON_SSE2 // bad for performance
// #define RAPIDJSON_SSE42 // bad for performance
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <boost/json/parser.hpp>
#include <boost/json/monotonic_resource.hpp>

#ifdef ALLPARSER

#include "fastjson.cpp"
#include "fastjson_dom.cpp"
#include "gason.cpp"

#include "json11.cpp"
extern "C" {
#include "cJSON.c"
#include "cJSON.h"
#include "jsmn.c"
#include "jsmn.h"
#include "ujdecode.h"
#include "ultrajsondec.c"
}

#include "jsoncpp.cpp"
#include "json/json.h"

#endif

SIMDJSON_POP_DISABLE_WARNINGS

using namespace rapidjson;

#ifdef ALLPARSER
// fastjson has a tricky interface
void on_json_error(void *, simdjson_unused const fastjson::ErrorContext &ec) {
  // std::cerr<<"ERROR: "<<ec.mesg<<std::endl;
}
bool fastjson_parse(const char *input) {
  fastjson::Token token;
  fastjson::dom::Chunk chunk;
  return fastjson::dom::parse_string(input, &token, &chunk, 0, &on_json_error,
                                     NULL);
}
// end of fastjson stuff
#endif

simdjson_never_inline size_t sum_line_lengths(std::stringstream &is) {
  std::string line;
  size_t sumofalllinelengths{0};
  while (std::getline(is, line)) {
    sumofalllinelengths += line.size();
  }
  return sumofalllinelengths;
}

inline void reset_stream(std::stringstream &is) {
  is.clear();
  is.seekg(0, std::ios::beg);
}

bool bench(const char *filename, bool verbose, bool just_data,
           double repeat_multiplier) {
  simdjson::padded_string p;
  auto error = simdjson::padded_string::load(filename).get(p);
  if (error) {
    std::cerr << "Could not load the file " << filename << ": " << error
              << std::endl;
    return false;
  }

  int repeat = static_cast<int>((50000000 * repeat_multiplier) /
                                static_cast<double>(p.size()));
  if (repeat < 10) {
    repeat = 10;
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  if (verbose) {
    std::cout << "Input " << filename << " has ";
    if (p.size() > 1000 * 1000)
      std::cout << p.size() / (1000 * 1000) << " MB";
    else if (p.size() > 1000)
      std::cout << p.size() / 1000 << " KB";
    else
      std::cout << p.size() << " B";
    std::cout << ": will run " << repeat << " iterations." << std::endl;
  }
  size_t volume = p.size();
  if (just_data) {
    std::printf("%-42s %20s %20s %20s %20s \n", "name", "cycles_per_byte",
                "cycles_per_byte_err", "gb_per_s", "gb_per_s_err");
  }
  if (!just_data) {
    const std::string inputcopy(p.data(), p.data() + p.size());
    std::stringstream is;
    is.str(inputcopy);
    const size_t lc = sum_line_lengths(is);
    BEST_TIME("getline ", sum_line_lengths(is), lc, reset_stream(is), repeat,
              volume, !just_data);
  }

  if (!just_data) {
    auto parse_dynamic = [](auto &str) {
      simdjson::dom::parser parser;
      return parser.parse(str).error();
    };
    BEST_TIME("simdjson (dynamic mem) ", parse_dynamic(p), simdjson::SUCCESS, ,
              repeat, volume, !just_data);
  }
  // (static alloc)
  simdjson::dom::parser parser;
  BEST_TIME("simdjson ", parser.parse(p).error(), simdjson::SUCCESS, , repeat,
            volume, !just_data);

  rapidjson::Document d;

  char *buffer = (char *)std::malloc(p.size() + 1);
  std::memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
#ifndef ALLPARSER
  if (!just_data)
#endif
  {
    std::memcpy(buffer, p.data(), p.size());
    BEST_TIME("RapidJSON  ",
              d.Parse<kParseValidateEncodingFlag>((const char *)buffer)
                  .HasParseError(),
              false, , repeat, volume, !just_data);
  }
#ifndef ALLPARSER
  if (!just_data)
#endif
  {
    std::memcpy(buffer, p.data(), p.size());
    BEST_TIME("RapidJSON (accurate number parsing)  ",
              d.Parse<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(
                   (const char *)buffer)
                  .HasParseError(),
              false, , repeat, volume, !just_data);
  }
  BEST_TIME(
      "RapidJSON (insitu)",
      d.ParseInsitu<kParseValidateEncodingFlag>(buffer).HasParseError(), false,
      std::memcpy(buffer, p.data(), p.size()) && (buffer[p.size()] = '\0'),
      repeat, volume, !just_data);
  BEST_TIME("RapidJSON (insitu, accurate number parsing)",
            d.ParseInsitu<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(
                 buffer)
                .HasParseError(),
            false,
            std::memcpy(buffer, p.data(), p.size()) &&
                (buffer[p.size()] = '\0'),
            repeat, volume, !just_data);

  {
    const boost::json::string_view sv(p.data(), p.size());
    boost::json::parser p;
    auto execute = [&p](auto sv) -> bool {
      boost::json::error_code ec;
      boost::json::monotonic_resource mr;
        p.reset( &mr );
        p.write(sv,ec);
       if(!ec)
         auto jv=p.release();
       return !!ec;
    };

    BEST_TIME("Boost.json", execute(sv), false, , repeat, volume, !just_data);
  }
  {
    
    auto execute = [&p]() -> bool {
      yyjson_doc *doc = yyjson_read(p.data(), p.size(), 0);
      bool is_ok = doc != nullptr;
      yyjson_doc_free(doc);
      return is_ok;
    };
    
    BEST_TIME("yyjson", execute(), true, , repeat, volume, !just_data);
  }
#ifndef ALLPARSER
  if (!just_data)
#endif
    BEST_TIME("sajson (dynamic mem)",
              sajson::parse(sajson::dynamic_allocation(),
                            sajson::mutable_string_view(p.size(), buffer))
                  .is_valid(),
              true, std::memcpy(buffer, p.data(), p.size()), repeat, volume,
              !just_data);

  size_t ast_buffer_size = p.size();
  size_t *ast_buffer = (size_t *)std::malloc(ast_buffer_size * sizeof(size_t));
  //  (static alloc, insitu)
  BEST_TIME(
      "sajson",
      sajson::parse(sajson::bounded_allocation(ast_buffer, ast_buffer_size),
                    sajson::mutable_string_view(p.size(), buffer))
          .is_valid(),
      true, std::memcpy(buffer, p.data(), p.size()), repeat, volume,
      !just_data);

  std::memcpy(buffer, p.data(), p.size());
  size_t expected = json::parse(p.data(), p.data() + p.size()).size();
  BEST_TIME("nlohmann-json", json::parse(buffer, buffer + p.size()).size(),
            expected, , repeat, volume, !just_data);

#ifdef ALLPARSER
  std::string json11err;
  BEST_TIME("dropbox (json11)     ",
            ((json11::Json::parse(buffer, json11err).is_null()) ||
             (!json11err.empty())),
            false, std::memcpy(buffer, p.data(), p.size()), repeat, volume,
            !just_data);

  BEST_TIME("fastjson             ", fastjson_parse(buffer), true,
            std::memcpy(buffer, p.data(), p.size()), repeat, volume,
            !just_data);
  JsonValue value;
  JsonAllocator allocator;
  char *endptr;
  BEST_TIME("gason             ", jsonParse(buffer, &endptr, &value, allocator),
            JSON_OK, std::memcpy(buffer, p.data(), p.size()), repeat, volume,
            !just_data);
  void *state;
  BEST_TIME("ultrajson         ",
            (UJDecode(buffer, p.size(), NULL, &state) == NULL), false,
            std::memcpy(buffer, p.data(), p.size()), repeat, volume,
            !just_data);

  {
    std::unique_ptr<jsmntok_t[]> tokens =
        std::make_unique<jsmntok_t[]>(p.size());
    jsmn_parser jparser;
    jsmn_init(&jparser);
    std::memcpy(buffer, p.data(), p.size());
    buffer[p.size()] = '\0';
    BEST_TIME("jsmn           ",
              (jsmn_parse(&jparser, buffer, p.size(), tokens.get(),
                          static_cast<unsigned int>(p.size())) > 0),
              true, jsmn_init(&jparser), repeat, volume, !just_data);
  }
  std::memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  cJSON *tree = cJSON_Parse(buffer);
  BEST_TIME("cJSON           ", ((tree = cJSON_Parse(buffer)) != NULL), true,
            cJSON_Delete(tree), repeat, volume, !just_data);
  cJSON_Delete(tree);

  Json::CharReaderBuilder b;
  Json::CharReader *json_cpp_reader = b.newCharReader();
  Json::Value root;
  Json::String errs;
  BEST_TIME("jsoncpp           ",
            json_cpp_reader->parse(buffer, buffer + volume, &root, &errs), true,
            , repeat, volume, !just_data);
  delete json_cpp_reader;
#endif
  if (!just_data)
    BEST_TIME("memcpy            ",
              (std::memcpy(buffer, p.data(), p.size()) == buffer), true, ,
              repeat, volume, !just_data);
#ifdef __linux__
  if (!just_data) {
    std::printf(
        "\n \n <doing additional analysis with performance counters (Linux "
        "only)>\n");
    std::vector<int> evts;
    evts.push_back(PERF_COUNT_HW_CPU_CYCLES);
    evts.push_back(PERF_COUNT_HW_INSTRUCTIONS);
    evts.push_back(PERF_COUNT_HW_BRANCH_MISSES);
    evts.push_back(PERF_COUNT_HW_CACHE_REFERENCES);
    evts.push_back(PERF_COUNT_HW_CACHE_MISSES);
    LinuxEvents<PERF_TYPE_HARDWARE> unified(evts);
    std::vector<unsigned long long> results;
    std::vector<unsigned long long> stats;
    results.resize(evts.size());
    stats.resize(evts.size());
    std::fill(stats.begin(), stats.end(), 0); // unnecessary
    for (decltype(repeat) i = 0; i < repeat; i++) {
      unified.start();
      auto parse_error = parser.parse(p).error();
      if (parse_error)
        std::printf("bug\n");
      unified.end(results);
      std::transform(stats.begin(), stats.end(), results.begin(), stats.begin(),
                     std::plus<unsigned long long>());
    }
    std::printf(
        "simdjson : cycles %10.0f instructions %10.0f branchmisses %10.0f "
        "cacheref %10.0f cachemisses %10.0f  bytespercachemiss %10.0f "
        "inspercycle %10.1f insperbyte %10.1f\n",
        static_cast<double>(stats[0]) / static_cast<double>(repeat),
        static_cast<double>(stats[1]) / static_cast<double>(repeat),
        static_cast<double>(stats[2]) / static_cast<double>(repeat),
        static_cast<double>(stats[3]) / static_cast<double>(repeat),
        static_cast<double>(stats[4]) / static_cast<double>(repeat),
        static_cast<double>(volume) * static_cast<double>(repeat) /
            static_cast<double>(stats[2]),
        static_cast<double>(stats[1]) / static_cast<double>(stats[0]),
        static_cast<double>(stats[1]) /
            (static_cast<double>(volume) * static_cast<double>(repeat)));

    std::fill(stats.begin(), stats.end(), 0);
    for (decltype(repeat) i = 0; i < repeat; i++) {
      std::memcpy(buffer, p.data(), p.size());
      buffer[p.size()] = '\0';
      unified.start();
      if (d.ParseInsitu<kParseValidateEncodingFlag>(buffer).HasParseError() !=
          false)
        std::printf("bug\n");
      unified.end(results);
      std::transform(stats.begin(), stats.end(), results.begin(), stats.begin(),
                     std::plus<unsigned long long>());
    }
    std::printf(
        "RapidJSON: cycles %10.0f instructions %10.0f branchmisses %10.0f "
        "cacheref %10.0f cachemisses %10.0f  bytespercachemiss %10.0f "
        "inspercycle %10.1f insperbyte %10.1f\n",
        static_cast<double>(stats[0]) / static_cast<double>(repeat),
        static_cast<double>(stats[1]) / static_cast<double>(repeat),
        static_cast<double>(stats[2]) / static_cast<double>(repeat),
        static_cast<double>(stats[3]) / static_cast<double>(repeat),
        static_cast<double>(stats[4]) / static_cast<double>(repeat),
        static_cast<double>(volume) * static_cast<double>(repeat) /
            static_cast<double>(stats[2]),
        static_cast<double>(stats[1]) / static_cast<double>(stats[0]),
        static_cast<double>(stats[1]) /
            (static_cast<double>(volume) * static_cast<double>(repeat)));

    std::fill(stats.begin(), stats.end(), 0); // unnecessary
    for (decltype(repeat) i = 0; i < repeat; i++) {
      std::memcpy(buffer, p.data(), p.size());
      unified.start();
      if (sajson::parse(sajson::bounded_allocation(ast_buffer, ast_buffer_size),
                        sajson::mutable_string_view(p.size(), buffer))
              .is_valid() != true)
        std::printf("bug\n");
      unified.end(results);
      std::transform(stats.begin(), stats.end(), results.begin(), stats.begin(),
                     std::plus<unsigned long long>());
    }
    std::printf(
        "sajson   : cycles %10.0f instructions %10.0f branchmisses %10.0f "
        "cacheref %10.0f cachemisses %10.0f  bytespercachemiss %10.0f "
        "inspercycle %10.1f insperbyte %10.1f\n",
        static_cast<double>(stats[0]) / static_cast<double>(repeat),
        static_cast<double>(stats[1]) / static_cast<double>(repeat),
        static_cast<double>(stats[2]) / static_cast<double>(repeat),
        static_cast<double>(stats[3]) / static_cast<double>(repeat),
        static_cast<double>(stats[4]) / static_cast<double>(repeat),
        static_cast<double>(volume) * static_cast<double>(repeat) /
            static_cast<double>(stats[2]),
        static_cast<double>(stats[1]) / static_cast<double>(stats[0]),
        static_cast<double>(stats[1]) /
            (static_cast<double>(volume) * static_cast<double>(repeat)));
  }
#endif //  __linux__

  std::free(ast_buffer);
  std::free(buffer);
  return true;
}

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool just_data = false;
  double repeat_multiplier = 1;
  int c;
  while ((c = getopt(argc, argv, "r:vt")) != -1)
    switch (c) {
    case 'r':
      repeat_multiplier = atof(optarg);
      break;
    case 't':
      just_data = true;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      abort();
    }
  if (optind >= argc) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    std::cerr << "Or " << argv[0] << " -v <jsonfile>" << std::endl;
    std::cerr << "The '-t' flag outputs a table." << std::endl;
    std::cerr << "The '-r <N>' flag sets the repeat multiplier: set it above 1 "
                 "to do more iterations, and below 1 to do fewer."
              << std::endl;
    exit(1);
  }
  int result = EXIT_SUCCESS;
  for (int fileind = optind; fileind < argc; fileind++) {
    if (!bench(argv[fileind], verbose, just_data, repeat_multiplier)) {
      result = EXIT_FAILURE;
    }
    std::printf("\n\n");
  }
  return result;
}
