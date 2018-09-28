#include <iostream>

#include "jsonparser/jsonioutil.h"
#include "jsonparser/jsonminifier.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>\n";
    exit(1);
  }
  std::pair<u8 *, size_t> p = get_corpus(argv[argc - 1]);
  size_t outlength =
      jsonminify(p.first, p.second, p.first);
  p.first[outlength] = '\0';
  printf("%s",p.first);
  free(p.first);
}
