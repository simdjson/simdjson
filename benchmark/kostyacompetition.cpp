#include "simdjson/jsonparser.h"
#include <algorithm>
#include <unistd.h>
#include <vector>

#include "benchmark.h"
// #define RAPIDJSON_SSE2 // bad for performance
// #define RAPIDJSON_SSE42 // bad for performance
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


using namespace rapidjson;

bool equals(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }

void print_vec(std::vector<double> & x) {
  for(double i : x) {
    std::cout << i << " ";
  }
  std::cout << std::endl;
}

void simdjson_scan(std::vector<double> &answer,
                   simdjson::ParsedJson::Iterator &pjh) {
  double x = 0, y = 0, z = 0;
  int len = 0;

  if (pjh.is_object()) {
    if (pjh.move_to_key("coordinates")) {
      if (pjh.is_array()) {
        if (pjh.down()) {
          do { // moving through array
            
            if (pjh.is_object()) {
              len++;
              if (pjh.down()) {
                do { // moving through hash {x:, y:, z:}
                  if (pjh.get_string_length() == 1) {
                      char c = pjh.get_string()[0];
                      pjh.next();

                      switch(c) {
                        case 'x':
                          x += pjh.get_double();
                          break;

                        case 'y':
                          y += pjh.get_double();
                          break;

                        case 'z':
                          z += pjh.get_double();
                          break;
                      }
                  } else {
                    pjh.next();
                  }

                } while(pjh.next());  // moving through hash {x:, y:, z:}
                pjh.up();
              }
            }
          } while(pjh.next()); // moving through array
        }
      }
    }
  }
  answer.clear();
  answer.push_back(x / len);
  answer.push_back(y / len);
  answer.push_back(z / len);
}

__attribute__((noinline)) std::vector<double>
simdjson_just_dom(simdjson::ParsedJson &pj) {
  std::vector<double> answer;
  simdjson::ParsedJson::Iterator i(pj);
  simdjson_scan(answer, i);
  return answer;
}

__attribute__((noinline)) std::vector<double>
simdjson_compute_stats(const simdjson::padded_string &p) {
  std::vector<double> answer;
  simdjson::ParsedJson pj = simdjson::build_parsed_json(p);
  if (!pj.is_valid()) {
    return answer;
  }
  simdjson::ParsedJson::Iterator i(pj);
  simdjson_scan(answer, i);
  return answer;
}

__attribute__((noinline)) bool
simdjson_just_parse(const simdjson::padded_string &p) {
  simdjson::ParsedJson pj = simdjson::build_parsed_json(p);
  bool answer = !pj.is_valid();
  return answer;
}


__attribute__((noinline)) bool
simdjson_just_parse_noalloc(const simdjson::padded_string &p, simdjson::ParsedJson & pj) {
  json_parse(p, pj);
  bool answer = !pj.is_valid();
  return answer;
}


void rapid_traverse(std::vector<double> &answer, const rapidjson::Value &v) {
    const Value &coordinates = v["coordinates"];
    SizeType len = coordinates.Size();
    double x = 0, y = 0, z = 0;

    for (SizeType i = 0; i < len; i++) {
      const Value &coord = coordinates[i];
      x += coord["x"].GetDouble();
      y += coord["y"].GetDouble();
      z += coord["z"].GetDouble();
    }
    answer.clear();
    answer.push_back(x / len);
    answer.push_back(y / len);
    answer.push_back(z / len);
}

__attribute__((noinline)) std::vector<double>
rapid_just_dom(rapidjson::Document &d) {
  std::vector<double> answer;
  rapid_traverse(answer, d);
  return answer;
}

__attribute__((noinline)) std::vector<double>
rapid_compute_stats(const simdjson::padded_string &p) {
  std::vector<double> answer;
  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  if (d.HasParseError()) {
    free(buffer);
    return answer;
  }
  rapid_traverse(answer, d);
  free(buffer);
  return answer;
}

__attribute__((noinline)) bool
rapid_just_parse(const simdjson::padded_string &p) {
  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  bool answer = d.HasParseError();
  free(buffer);
  return answer;
}

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool just_data = false;

  int c;
  while ((c = getopt(argc, argv, "vt")) != -1)
    switch (c) {
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
    std::cerr
        << "Using different parsers, we compute the content statistics of "
           "JSON documents."
        << std::endl;
    std::cerr << " Try jsonexamples/kostya.json " << std::endl;
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    std::cerr << "Or " << argv[0] << " -v <jsonfile>" << std::endl;
    exit(1);
  }
  const char *filename = argv[optind];
  if (optind + 1 < argc) {
    std::cerr << "warning: ignoring everything after " << argv[optind + 1]
              << std::endl;
  }
  simdjson::padded_string p;
  try {
    simdjson::get_corpus(filename).swap(p);
  } catch (const std::exception &e) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }

  if (verbose) {
    std::cout << "Input has ";
    if (p.size() > 1024 * 1024)
      std::cout << p.size() / (1024 * 1024) << " MB ";
    else if (p.size() > 1024)
      std::cout << p.size() / 1024 << " KB ";
    else
      std::cout << p.size() << " B ";
    std::cout << std::endl;
  }
  std::vector<double> s1 = simdjson_compute_stats(p);
  if (verbose) {
    printf("simdjson: ");
    print_vec(s1);
  }
  std::vector<double> s2 = rapid_compute_stats(p);
  if (verbose) {
    printf("rapid:    ");
    print_vec(s2);
  }
  assert(abs(s1[0] - s2[0]) + abs(s1[1] - s2[1]) + abs(s1[2] - s2[2]) < 0.0000001);
  size_t size = s1.size();

  int repeat = 5;
  int volume = p.size();
  if (just_data) {
    printf(
        "name cycles_per_byte cycles_per_byte_err  gb_per_s gb_per_s_err \n");
  }
  BEST_TIME("simdjson  ", simdjson_compute_stats(p).size(), size, , repeat,
            volume, !just_data);
  BEST_TIME("rapid  ", rapid_compute_stats(p).size(), size, , repeat, volume,
            !just_data);
  BEST_TIME("simdjson (just parse)  ", simdjson_just_parse(p), false, , repeat,
            volume, !just_data);
  BEST_TIME("rapid  (just parse) ", rapid_just_parse(p), false, , repeat,
            volume, !just_data);
  simdjson::ParsedJson dsimdjson = simdjson::build_parsed_json(p);
  BEST_TIME("simdjson (just dom)  ", simdjson_just_dom(dsimdjson).size(), size,
            , repeat, volume, !just_data);
  char *buffer = (char *)malloc(p.size());
  memcpy(buffer, p.data(), p.size());
  rapidjson::Document drapid;
  drapid.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  BEST_TIME("rapid  (just dom) ", rapid_just_dom(drapid).size(), size, , repeat,
            volume, !just_data);
  simdjson::ParsedJson pj;
  bool alloc = pj.allocate_capacity(volume);
  if(!alloc) std::cerr << "allocation failure" << std::endl;
  BEST_TIME("simdjson (just parse, no alloc)  ", simdjson_just_parse_noalloc(p, pj), false, , repeat,
            volume, !just_data);
 
  memcpy(buffer, p.data(), p.size());
  free(buffer);
}
