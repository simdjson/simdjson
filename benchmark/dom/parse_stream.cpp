#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <vector>

#include "simdjson.h"

#define NB_ITERATION 20
#define MIN_BATCH_SIZE 10000
#define MAX_BATCH_SIZE 10000000

bool test_baseline = false;
bool test_per_batch = true;
bool test_best_batch = false;

bool compare(std::pair<size_t, double> i, std::pair<size_t, double> j) {
  return i.second > j.second;
}

int main(int argc, char *argv[]) {

  if (argc <= 1) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    exit(1);
  }
  const char *filename = argv[1];
  auto v = simdjson::padded_string::load(filename);
  if (v.error()) {
    std::cerr << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  const simdjson::padded_string& p = v.value_unsafe();
  if (test_baseline) {
    std::wclog << "Baseline: Getline + normal parse... " << std::endl;
    std::cout << "Gigabytes/second\t"
              << "Nb of documents parsed" << std::endl;
    for (auto i = 0; i < 3; i++) {
      // Actual test
      simdjson::dom::parser parser;
      simdjson::error_code alloc_error = parser.allocate(p.size());
      if (alloc_error) {
        std::cerr << alloc_error << std::endl;
        return EXIT_FAILURE;
      }
      std::istringstream ss(std::string(p.data(), p.size()));

      auto start = std::chrono::steady_clock::now();
      int count = 0;
      std::string line;
      int parse_res = simdjson::SUCCESS;
      while (getline(ss, line)) {
        // TODO we're likely triggering simdjson's padding reallocation here. Is
        // that intentional?
        parser.parse(line);
        count++;
      }

      auto end = std::chrono::steady_clock::now();

      std::chrono::duration<double> secs = end - start;
      double speedinGBs = static_cast<double>(p.size()) /
                          (static_cast<double>(secs.count()) * 1000000000.0);
      std::cout << speedinGBs << "\t\t\t\t" << count << std::endl;

      if (parse_res != simdjson::SUCCESS) {
        std::cerr << "Parsing failed" << std::endl;
        exit(1);
      }
    }
  }

  std::map<size_t, double> batch_size_res;
  if (test_per_batch) {
    std::wclog << "parse_many: Speed per batch_size... from " << MIN_BATCH_SIZE
               << " bytes to " << MAX_BATCH_SIZE << " bytes..." << std::endl;
    std::cout << "Batch Size\t"
              << "Gigabytes/second\t"
              << "Nb of documents parsed" << std::endl;
    for (size_t i = MIN_BATCH_SIZE; i <= MAX_BATCH_SIZE;
         i += (MAX_BATCH_SIZE - MIN_BATCH_SIZE) / 100) {
      batch_size_res.insert(std::pair<size_t, double>(i, 0));
      int count;
      for (size_t j = 0; j < 5; j++) {
        // Actual test
        simdjson::dom::parser parser;
        simdjson::error_code error;

        auto start = std::chrono::steady_clock::now();
        count = 0;
        simdjson::dom::document_stream docs;
        if ((error = parser.parse_many(p, i).get(docs))) {
          std::wcerr << "Parsing failed with: " << error << std::endl;
          exit(1);
        }
        for (auto result : docs) {
          error = result.error();
          if (error) {
            std::wcerr << "Parsing failed with: " << error << std::endl;
            exit(1);
          }
          count++;
        }
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> secs = end - start;
        double speedinGBs = static_cast<double>(p.size()) /
                            (static_cast<double>(secs.count()) * 1000000000.0);
        if (speedinGBs > batch_size_res.at(i))
          batch_size_res[i] = speedinGBs;
      }
      std::cout << i << "\t\t" << std::fixed << std::setprecision(3)
                << batch_size_res.at(i) << "\t\t\t\t" << count << std::endl;
    }
  }
  size_t optimal_batch_size{};
  double best_speed{};
  if (test_per_batch) {
    std::pair<size_t, double> best_results;
    best_results =
        (*min_element(batch_size_res.begin(), batch_size_res.end(), compare));
    optimal_batch_size = best_results.first;
    best_speed = best_results.second;
  } else {
    optimal_batch_size = MIN_BATCH_SIZE;
  }
  std::wclog << "Seemingly optimal batch_size: " << optimal_batch_size << "..."
             << std::endl;
  std::wclog << "Best speed: " << best_speed << "..." << std::endl;

  if (test_best_batch) {
    std::wclog << "Starting speed test... Best of " << NB_ITERATION
               << " iterations..." << std::endl;
    std::vector<double> res;
    for (int i = 0; i < NB_ITERATION; i++) {

      // Actual test
      simdjson::dom::parser parser;
      simdjson::error_code error;

      auto start = std::chrono::steady_clock::now();
      // This includes allocation of the parser
      simdjson::dom::document_stream docs;
      if ((error = parser.parse_many(p, optimal_batch_size).get(docs))) {
        std::wcerr << "Parsing failed with: " << error << std::endl;
        exit(1);
      }
      for (auto result : docs) {
        error = result.error();
        if (error) {
          std::wcerr << "Parsing failed with: " << error << std::endl;
          exit(1);
        }
      }
      auto end = std::chrono::steady_clock::now();

      std::chrono::duration<double> secs = end - start;
      res.push_back(secs.count());
    }

    double min_result = *min_element(res.begin(), res.end());
    double speedinGBs =
        static_cast<double>(p.size()) / (min_result * 1000000000.0);

    std::cout << "Min:  " << min_result << " bytes read: " << p.size()
              << " Gigabytes/second: " << speedinGBs << std::endl;
  }
#ifdef SIMDJSON_THREADS_ENABLED
  // Multithreading probably does not help matters for small files (less than 10
  // MB).
  if (p.size() < 10000000) {
    std::cout << std::endl;

    std::cout << "Warning: your file is small and the performance results are "
                 "probably meaningless"
              << std::endl;
    std::cout << "as far as multithreaded performance goes." << std::endl;

    std::cout << std::endl;

    std::cout
        << "Try to concatenate the file with itself to generate a large one."
        << std::endl;
    std::cout << "In bash: " << std::endl;
    std::cout << "for i in {1..1000}; do cat '" << filename
              << "' >> bar.ndjson; done" << std::endl;
    std::cout << argv[0] << " bar.ndjson" << std::endl;
  }
#endif

  return 0;
}
