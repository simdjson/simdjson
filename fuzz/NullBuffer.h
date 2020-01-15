
#pragma once

#include <iostream>

// from https://stackoverflow.com/a/8244052
class NulStreambuf : public std::streambuf {
  char dummyBuffer[64];

protected:
  virtual int overflow(int c) override final{
    setp(dummyBuffer, dummyBuffer + sizeof(dummyBuffer));
    return (c == traits_type::eof()) ? '\0' : c;
  }
};

class NulOStream final : private NulStreambuf, public std::ostream {
public:
  NulOStream() : std::ostream(this) {}
  NulStreambuf *rdbuf() { return this; }
};
