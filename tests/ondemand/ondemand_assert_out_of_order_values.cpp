#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "simdjson.h"

using namespace simdjson;
using namespace simdjson::builtin;

// This ensures the compiler can't rearrange them into the proper order (which causes it to work!)
simdjson_never_inline int check_point(simdjson_result<ondemand::value> xval, simdjson_result<ondemand::value> yval) {
  // Verify the expected release behavior
  error_code error;
  uint64_t x = 0;
  if ((error = xval.get(x))) { std::cerr << "error getting x: " << error << std::endl; }
  else if (x != 2) { std::cerr << "expected x to (wrongly) be 2, was " << x << std::endl; }
  uint64_t y = 0;
  if ((error = yval.get(y))) { std::cerr << "error getting y: " << error << std::endl; }
  else if (y != 3) { std::cerr << "expected y to (wrongly) be 3, was " << y << std::endl; }
  return 0;
}

int test_check_point() {
  auto json = R"(
    {
      "x": 1,
      "y": 2 3
  )"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  return check_point(doc["x"], doc["y"]);
}

bool fork_failed_with_assert() {
  int status = 0;
  wait(&status);
  return WIFSIGNALED(status);
}

int main(void) {

  // To verify that the program asserts (which sends a signal), we fork a new process and use wait()
  // to check the resulting error code. If it's a signal error code, we consider the
  // test to have passed.
  // From https://stackoverflow.com/a/33694733

  pid_t pid = fork();
  if (pid == -1) { std::cerr << "fork failed" << std::endl; return 1; }
  if (pid) {
    // Parent - wait child and interpret its result
    return fork_failed_with_assert() ? 0 : 1;
  } else {
    test_check_point();
  }
}
