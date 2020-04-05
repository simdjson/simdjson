#include "simdjson.h"
#include "simdjson.cpp"

bool error_messages_in_correct_order() {
  std::cout << "Running " << __func__ << std::endl;
  using namespace simdjson;
  using namespace simdjson::internal;
  using namespace std;
  if ((sizeof(error_codes)/sizeof(error_code_info)) != NUM_ERROR_CODES) {
    cerr << "error_codes does not have all codes in error_code enum (or too many)" << endl;
    return false;
  }
  for (int i=0; i<NUM_ERROR_CODES; i++) {
    if (error_codes[i].code != i) {
      cerr << "Error " << int(error_codes[i].code) << " at wrong position (" << i << "): " << error_codes[i].message << endl;
      return false;
    }
  }
  return true;
}

int main(void) {
  if (error_messages_in_correct_order()) {
    std::cout << "All static checks successful." << std::endl;
    return 0;
  }
  std::cerr << "Failed static checks." << std::endl;
  return 1;
}