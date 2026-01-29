#include <cassert>
#include <cstdlib>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>
#include "citm_catalog_data.h"
#include "nlohmann_citm_catalog_data.h"
#include "../benchmark_utils/benchmark_helper.h"

#ifdef SIMDJSON_COMPETITION_RAPIDJSON
#include "rapidjson_citm_catalog_data.h"
#endif

#ifdef SIMDJSON_COMPETITION_YYJSON
#include "yyjson_citm_catalog_data.h"
#endif

#ifdef SIMDJSON_RUST_VERSION
#include "../serde-benchmark/serde_benchmark.h"

void bench_rust_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_rust_parsing",
               bench([&json_str, &result]() {
                 serde_benchmark::CitmCatalog *catalog = serde_benchmark::citm_from_str(json_str.c_str(), json_str.size());
                 result = (catalog != nullptr);
                 if (catalog) {
                   serde_benchmark::free_citm(catalog);
                 }
                 if (!result) {
                   printf("parse error\n");
                 }
               }));
}
#endif

template <class T> void bench_simdjson_static_reflection_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  // Pre-allocate padded buffer outside the benchmark loop
  std::string mutable_json = json_str;
  simdjson::pad(mutable_json);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_simdjson_static_reflection_parsing",
               bench([&mutable_json, &result]() {
                 simdjson::ondemand::parser parser;
                 simdjson::ondemand::document doc;
                 if(parser.iterate(mutable_json).get(doc)) {
                   result = false;
                   return;
                 }
                 T my_struct;
                 if(doc.get<T>().get(my_struct)) {
                   result = false;
                 }
                 if (!result) {
                   printf("parse error\n");
                 }
               }));
}

#if SIMDJSON_STATIC_REFLECTION
template <class T> void bench_simdjson_from_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  // Pre-allocate padded buffer outside the benchmark loop
  simdjson::padded_string padded = simdjson::padded_string(json_str);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_simdjson_from_parsing",
               bench([&padded, &result]() {
                 T my_struct;
                 auto err = simdjson::from(padded).get(my_struct);
                  if (err) {
                    result = false;
                    printf("parse error: %s\n", simdjson::error_message(err));
                    return;
                  }
               }));
}
#endif

// nlohmann::json deserialization functions
void from_json(const nlohmann::json &j, CITMPrice &p) {
    j.at("amount").get_to(p.amount);
    j.at("audienceSubCategoryId").get_to(p.audienceSubCategoryId);
    j.at("seatCategoryId").get_to(p.seatCategoryId);
}

void from_json(const nlohmann::json &j, CITMArea &a) {
    j.at("areaId").get_to(a.areaId);
    j.at("blockIds").get_to(a.blockIds);
}

void from_json(const nlohmann::json &j, CITMSeatCategory &s) {
    j.at("areas").get_to(s.areas);
    j.at("seatCategoryId").get_to(s.seatCategoryId);
}

void from_json(const nlohmann::json &j, CITMPerformance &p) {
    j.at("id").get_to(p.id);
    j.at("eventId").get_to(p.eventId);
    if (j.contains("logo") && !j["logo"].is_null()) {
        p.logo = j["logo"].get<std::string>();
    }
    if (j.contains("name") && !j["name"].is_null()) {
        p.name = j["name"].get<std::string>();
    }
    j.at("prices").get_to(p.prices);
    j.at("seatCategories").get_to(p.seatCategories);
    if (j.contains("seatMapImage") && !j["seatMapImage"].is_null()) {
        p.seatMapImage = j["seatMapImage"].get<std::string>();
    }
    j.at("start").get_to(p.start);
    j.at("venueCode").get_to(p.venueCode);
}

void from_json(const nlohmann::json &j, CITMEvent &e) {
    j.at("id").get_to(e.id);
    j.at("name").get_to(e.name);
    if (j.contains("description") && !j["description"].is_null()) {
        e.description = j["description"].get<std::string>();
    }
    if (j.contains("logo") && !j["logo"].is_null()) {
        e.logo = j["logo"].get<std::string>();
    }
    j.at("subTopicIds").get_to(e.subTopicIds);
    if (j.contains("subjectCode") && !j["subjectCode"].is_null()) {
        e.subjectCode = j["subjectCode"].get<std::string>();
    }
    if (j.contains("subtitle") && !j["subtitle"].is_null()) {
        e.subtitle = j["subtitle"].get<std::string>();
    }
    j.at("topicIds").get_to(e.topicIds);
}

void from_json(const nlohmann::json &j, CitmCatalog &c) {
    j.at("events").get_to(c.events);
    j.at("performances").get_to(c.performances);
}

CitmCatalog nlohmann_deserialize(const std::string &json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    return j.get<CitmCatalog>();
}

void bench_nlohmann_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_nlohmann_parsing",
               bench([&json_str, &result]() {
                 try {
                   CitmCatalog data = nlohmann_deserialize(json_str);
                   result = true;
                 } catch (...) {
                   result = false;
                   printf("parse error\n");
                 }
               }));
}

#ifdef SIMDJSON_COMPETITION_RAPIDJSON
CitmCatalog rapidjson_deserialize(const std::string &json_str) {
    rapidjson::Document doc;
    doc.Parse(json_str.c_str());

    if (doc.HasParseError()) {
        throw std::runtime_error("RapidJSON parse error");
    }

    CitmCatalog catalog;

    // Parse events
    if (doc.HasMember("events") && doc["events"].IsObject()) {
        for (auto& m : doc["events"].GetObject()) {
            CITMEvent event;
            const auto& e = m.value;

            event.id = e["id"].GetUint64();
            event.name = e["name"].GetString();
            if (e.HasMember("description") && !e["description"].IsNull()) {
                event.description = e["description"].GetString();
            }
            if (e.HasMember("logo") && !e["logo"].IsNull()) {
                event.logo = e["logo"].GetString();
            }

            event.subTopicIds.clear();
            for (auto& id : e["subTopicIds"].GetArray()) {
                event.subTopicIds.push_back(id.GetUint64());
            }

            if (e.HasMember("subjectCode") && !e["subjectCode"].IsNull()) {
                event.subjectCode = e["subjectCode"].GetString();
            }
            if (e.HasMember("subtitle") && !e["subtitle"].IsNull()) {
                event.subtitle = e["subtitle"].GetString();
            }

            event.topicIds.clear();
            for (auto& id : e["topicIds"].GetArray()) {
                event.topicIds.push_back(id.GetUint64());
            }

            catalog.events[m.name.GetString()] = event;
        }
    }

    // Parse performances
    if (doc.HasMember("performances") && doc["performances"].IsArray()) {
        for (auto& p : doc["performances"].GetArray()) {
            CITMPerformance perf;

            perf.id = p["id"].GetUint64();
            perf.eventId = p["eventId"].GetUint64();
            if (p.HasMember("logo") && !p["logo"].IsNull()) {
                perf.logo = p["logo"].GetString();
            }
            if (p.HasMember("name") && !p["name"].IsNull()) {
                perf.name = p["name"].GetString();
            }

            // Parse prices
            for (auto& price : p["prices"].GetArray()) {
                CITMPrice pr;
                pr.amount = price["amount"].GetUint64();
                pr.audienceSubCategoryId = price["audienceSubCategoryId"].GetUint64();
                pr.seatCategoryId = price["seatCategoryId"].GetUint64();
                perf.prices.push_back(pr);
            }

            // Parse seat categories
            for (auto& sc : p["seatCategories"].GetArray()) {
                CITMSeatCategory seatCat;
                seatCat.seatCategoryId = sc["seatCategoryId"].GetUint64();

                for (auto& area : sc["areas"].GetArray()) {
                    CITMArea ar;
                    ar.areaId = area["areaId"].GetUint64();
                    for (auto& block : area["blockIds"].GetArray()) {
                        ar.blockIds.push_back(block.GetUint64());
                    }
                    seatCat.areas.push_back(ar);
                }
                perf.seatCategories.push_back(seatCat);
            }

            if (p.HasMember("seatMapImage") && !p["seatMapImage"].IsNull()) {
                perf.seatMapImage = p["seatMapImage"].GetString();
            }
            perf.start = p["start"].GetUint64();
            perf.venueCode = p["venueCode"].GetString();

            catalog.performances.push_back(perf);
        }
    }

    return catalog;
}

void bench_rapidjson_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_rapidjson_parsing",
               bench([&json_str, &result]() {
                 try {
                   CitmCatalog data = rapidjson_deserialize(json_str);
                   result = true;
                 } catch (...) {
                   result = false;
                   printf("parse error\n");
                 }
               }));
}
#endif

#ifdef SIMDJSON_COMPETITION_YYJSON
CitmCatalog yyjson_deserialize(const std::string &json_str) {
    yyjson_doc *doc = yyjson_read(json_str.c_str(), json_str.size(), 0);
    if (!doc) {
        throw std::runtime_error("YYJson parse error");
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    CitmCatalog catalog;

    // Parse events
    yyjson_val *events = yyjson_obj_get(root, "events");
    if (events) {
        size_t idx, max;
        yyjson_val *key, *val;
        yyjson_obj_foreach(events, idx, max, key, val) {
            CITMEvent event;

            event.id = yyjson_get_uint(yyjson_obj_get(val, "id"));
            const char* name = yyjson_get_str(yyjson_obj_get(val, "name"));
            if (name) event.name = name;

            yyjson_val *desc = yyjson_obj_get(val, "description");
            if (desc && !yyjson_is_null(desc)) {
                const char* str = yyjson_get_str(desc);
                if (str) event.description = str;
            }

            yyjson_val *logo = yyjson_obj_get(val, "logo");
            if (logo && !yyjson_is_null(logo)) {
                const char* str = yyjson_get_str(logo);
                if (str) event.logo = str;
            }

            yyjson_val *subTopics = yyjson_obj_get(val, "subTopicIds");
            if (subTopics) {
                size_t sidx, smax;
                yyjson_val *sval;
                yyjson_arr_foreach(subTopics, sidx, smax, sval) {
                    event.subTopicIds.push_back(yyjson_get_uint(sval));
                }
            }

            yyjson_val *subjectCode = yyjson_obj_get(val, "subjectCode");
            if (subjectCode && !yyjson_is_null(subjectCode)) {
                const char* str = yyjson_get_str(subjectCode);
                if (str) event.subjectCode = str;
            }

            yyjson_val *subtitle = yyjson_obj_get(val, "subtitle");
            if (subtitle && !yyjson_is_null(subtitle)) {
                const char* str = yyjson_get_str(subtitle);
                if (str) event.subtitle = str;
            }

            yyjson_val *topics = yyjson_obj_get(val, "topicIds");
            if (topics) {
                size_t tidx, tmax;
                yyjson_val *tval;
                yyjson_arr_foreach(topics, tidx, tmax, tval) {
                    event.topicIds.push_back(yyjson_get_uint(tval));
                }
            }

            const char* keyStr = yyjson_get_str(key);
            if (keyStr) {
                catalog.events[keyStr] = event;
            }
        }
    }

    // Parse performances
    yyjson_val *performances = yyjson_obj_get(root, "performances");
    if (performances) {
        size_t idx, max;
        yyjson_val *val;
        yyjson_arr_foreach(performances, idx, max, val) {
            CITMPerformance perf;

            perf.id = yyjson_get_uint(yyjson_obj_get(val, "id"));
            perf.eventId = yyjson_get_uint(yyjson_obj_get(val, "eventId"));

            yyjson_val *logo = yyjson_obj_get(val, "logo");
            if (logo && !yyjson_is_null(logo)) {
                const char* str = yyjson_get_str(logo);
                if (str) perf.logo = str;
            }

            yyjson_val *name = yyjson_obj_get(val, "name");
            if (name && !yyjson_is_null(name)) {
                const char* str = yyjson_get_str(name);
                if (str) perf.name = str;
            }

            // Parse prices
            yyjson_val *prices = yyjson_obj_get(val, "prices");
            if (prices) {
                size_t pidx, pmax;
                yyjson_val *pval;
                yyjson_arr_foreach(prices, pidx, pmax, pval) {
                    CITMPrice price;
                    price.amount = yyjson_get_uint(yyjson_obj_get(pval, "amount"));
                    price.audienceSubCategoryId = yyjson_get_uint(yyjson_obj_get(pval, "audienceSubCategoryId"));
                    price.seatCategoryId = yyjson_get_uint(yyjson_obj_get(pval, "seatCategoryId"));
                    perf.prices.push_back(price);
                }
            }

            // Parse seat categories
            yyjson_val *seatCats = yyjson_obj_get(val, "seatCategories");
            if (seatCats) {
                size_t scidx, scmax;
                yyjson_val *scval;
                yyjson_arr_foreach(seatCats, scidx, scmax, scval) {
                    CITMSeatCategory seatCat;
                    seatCat.seatCategoryId = yyjson_get_uint(yyjson_obj_get(scval, "seatCategoryId"));

                    yyjson_val *areas = yyjson_obj_get(scval, "areas");
                    if (areas) {
                        size_t aidx, amax;
                        yyjson_val *aval;
                        yyjson_arr_foreach(areas, aidx, amax, aval) {
                            CITMArea area;
                            area.areaId = yyjson_get_uint(yyjson_obj_get(aval, "areaId"));

                            yyjson_val *blocks = yyjson_obj_get(aval, "blockIds");
                            if (blocks) {
                                size_t bidx, bmax;
                                yyjson_val *bval;
                                yyjson_arr_foreach(blocks, bidx, bmax, bval) {
                                    area.blockIds.push_back(yyjson_get_uint(bval));
                                }
                            }
                            seatCat.areas.push_back(area);
                        }
                    }
                    perf.seatCategories.push_back(seatCat);
                }
            }

            yyjson_val *seatMapImage = yyjson_obj_get(val, "seatMapImage");
            if (seatMapImage && !yyjson_is_null(seatMapImage)) {
                const char* str = yyjson_get_str(seatMapImage);
                if (str) perf.seatMapImage = str;
            }

            perf.start = yyjson_get_uint(yyjson_obj_get(val, "start"));
            const char* venueCode = yyjson_get_str(yyjson_obj_get(val, "venueCode"));
            if (venueCode) perf.venueCode = venueCode;

            catalog.performances.push_back(perf);
        }
    }

    yyjson_doc_free(doc);
    return catalog;
}

void bench_yyjson_parsing(const std::string &json_str) {
  size_t input_volume = json_str.size();
  printf("# input volume: %zu bytes\n", input_volume);

  volatile bool result = true;
  pretty_print(1, input_volume, "bench_yyjson_parsing",
               bench([&json_str, &result]() {
                 try {
                   CitmCatalog data = yyjson_deserialize(json_str);
                   result = true;
                 } catch (...) {
                   result = false;
                   printf("parse error\n");
                 }
               }));
}
#endif

std::string read_file(std::string filename) {
  printf("# Reading file %s\n", filename.c_str());
  constexpr size_t read_size = 4096;
  auto stream = std::ifstream(filename);
  stream.exceptions(std::ios_base::badbit);

  if (!stream) {
    std::cerr << "Error: Failed to open file " << filename << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string out;
  auto buf = std::string(read_size, '\0');
  while (stream.read(&buf[0], read_size)) {
    out.append(buf, 0, size_t(stream.gcount()));
  }
  out.append(buf, 0, size_t(stream.gcount()));
  return out;
}

// Function to check if benchmark name matches any of the comma-separated filters
bool matches_filter(const std::string& benchmark_name, const std::string& filter) {
  if (filter.empty()) return true;

  // Split filter by comma
  size_t start = 0;
  size_t end = filter.find(',');
  while (end != std::string::npos) {
    std::string token = filter.substr(start, end - start);
    if (benchmark_name.find(token) != std::string::npos) {
      return true;
    }
    start = end + 1;
    end = filter.find(',', start);
  }
  // Check last token
  std::string token = filter.substr(start);
  return benchmark_name.find(token) != std::string::npos;
}

int main(int argc, char *argv[]) {
  // Get the JSON file path from preprocessor or use default
  std::string filename;
#ifdef JSON_FILE
  filename = JSON_FILE;
#else
  filename = "jsonexamples/citm_catalog.json";
#endif

  std::string json_str = read_file(filename);

  // Parse command-line arguments for filter
  std::string filter;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-f" && i + 1 < argc) {
      filter = argv[i + 1];
      printf("# Filter: %s\n", filter.c_str());
      i++;
    }
  }

  // If no filter provided, run all benchmarks
  if (filter.empty()) {
    printf("# Running all benchmarks (use -f <filter> to run specific ones)\n");
  }

  // Benchmarking the parsing
  if (matches_filter("nlohmann", filter)) {
    bench_nlohmann_parsing(json_str);
  }
#ifdef SIMDJSON_COMPETITION_RAPIDJSON
  if (matches_filter("rapidjson", filter)) {
    bench_rapidjson_parsing(json_str);
  }
#endif
#ifdef SIMDJSON_COMPETITION_YYJSON
  if (matches_filter("yyjson", filter)) {
    bench_yyjson_parsing(json_str);
  }
#endif
  if (matches_filter("simdjson_static_reflection", filter)) {
    bench_simdjson_static_reflection_parsing<CitmCatalog>(json_str);
  }
#if SIMDJSON_STATIC_REFLECTION
  if (matches_filter("simdjson_from", filter)) {
    bench_simdjson_from_parsing<CitmCatalog>(json_str);
  }
#endif
#ifdef SIMDJSON_RUST_VERSION
  if (matches_filter("rust", filter)) {
    printf("# Note: Rust/Serde parsing test\n");
    bench_rust_parsing(json_str);
  }
#endif
  return EXIT_SUCCESS;
}