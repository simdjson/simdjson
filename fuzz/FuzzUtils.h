#ifndef SIMDJSON_FUZZUTILS_H
#define SIMDJSON_FUZZUTILS_H

#include <cstdint>
#include <vector>
#include <string_view>
#include <cstring> //memcpy

// view data as a byte pointer
template <typename T> inline const std::uint8_t* as_bytes(const T* data) {
  return static_cast<const std::uint8_t*>(static_cast<const void*>(data));
}

// view data as a char pointer
template <typename T> inline const char* as_chars(const T* data) {
  return static_cast<const char*>(static_cast<const void*>(data));
}




// Splits the input into strings, using a four byte separator which is human
// readable. Makes for nicer debugging of fuzz data.
// See https://github.com/google/fuzzing/blob/master/docs/split-inputs.md#magic-separator
// for background. Note: don't use memmem, it is not standard C++.
inline std::vector<std::string_view> split(const char* Data, size_t Size) {

  std::vector<std::string_view> ret;

    using namespace std::literals;
    constexpr auto sep="\n~~\n"sv;

    std::string_view all(Data,Size);
    auto pos=all.find(sep);
    while(pos!=std::string_view::npos) {
      ret.push_back(all.substr(0,pos));
      all=all.substr(pos+sep.size());
      pos=all.find(sep);
    }
    ret.push_back(all);
    return ret;
}

// Generic helper to split fuzz data into usable parts, like ints etc.
// Note that it does not throw, instead it sets the data pointer to null
// if the input is exhausted.
struct FuzzData {
  // data may not be null, even if size is zero.
  FuzzData(const uint8_t* data,
           size_t size) : Data(data),Size(size){}

  ///range is inclusive
  template<int Min, int Max>
  int getInt() {
    static_assert (Min<Max,"min must be <max");

    // make this constexpr, can't overflow because that is UB and is forbidden
    // in constexpr evaluation
    constexpr int range=(Max-Min)+1;
    constexpr unsigned int urange=range;

    // don't use std::uniform_int_distribution, we don't want to pay for
    // over consumption of random data. Accept the slightly non-uniform distribution.
    if(range<256)
      return Min+static_cast<int>(get<uint8_t>()%urange);
    if(range<65536)
      return Min+static_cast<int>(get<uint16_t>()%urange);

    return Min+static_cast<int>(get<uint32_t>()%urange);
  }

  template<typename T>
  T get() {
    const auto Nbytes=sizeof(T);
    T ret{};
    if(Size<Nbytes) {
      //don't throw, signal with null instead.
      Data=nullptr;
      Size=0;
      return ret;
    }
    std::memcpy(&ret,Data,Nbytes);
    Data+=Nbytes;
    Size-=Nbytes;
    return ret;
  }

  // gets a string view with length in [Min,Max]
  template<int Min, int Max>
  std::string_view get_stringview() {
    static_assert (Min>=0,"Min must be positive");
    const int len=getInt<Min,Max>();
    const unsigned int ulen=static_cast<unsigned int>(len);
    if(ulen<Size) {
      std::string_view ret(chardata(),ulen);
      Data+=len;
      Size-=ulen;
      return ret;
    }

    //mark that there is too little data to fulfill the request
    Data=nullptr;
    Size=0;

    return {};
  }

  // consumes the rest of the data as a string view
  std::string_view remainder_as_stringview() {
    std::string_view ret{chardata(),Size};
    Data+=Size;
    Size=0;
    return ret;
  }

  // split the remainder of the data into string views,
  std::vector<std::string_view> splitIntoStrings() {
    std::vector<std::string_view> ret;
    if(Size>0) {
      ret=split(chardata(),Size);
      // all data consumed.
      Data+=Size;
      Size=0;
    }
    return ret;
  }

  //are we good?
  explicit operator bool() const { return Data!=nullptr;}

  //we are a URBG
  // https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator
  //The type G satisfies UniformRandomBitGenerator if    Given
  //   T, the type named by G::result_type
  //    g, a value of type G
  //
  //  The following expressions must be valid and have their specified effects
  //  Expression 	Return type 	Requirements
  //  G::result_type 	T 	T is an unsigned integer type
  using result_type=uint8_t;
  //  G::min() 	T 	Returns the smallest value that G's operator() may return. The value is strictly less than G::max(). The function must be constexpr.
  static constexpr result_type min() {return 0;}
  //  G::max() 	T 	Returns the largest value that G's operator() may return. The value is strictly greater than G::min(). The function must be constexpr.
  static constexpr result_type max() {return 255;}
  //  g() 	T 	Returns a value in the closed interval [G::min(), G::max()]. Has amortized constant complexity.
  result_type operator()() {
    if(Size==0) {
      // return something varying, otherwise uniform_int_distribution may get
      // stuck
      return failcount++;
    }
    const result_type ret=Data[0];
    Data++;
    Size--;
    return ret;
  }
  // returns a pointer to data as const char* to avoid those cstyle casts
  const char* chardata() const {return static_cast<const char*>(static_cast<const void*>(Data));}
  // members
  const uint8_t* Data;
  size_t Size;
  uint8_t failcount=0;
};


#endif // SIMDJSON_FUZZUTILS_H
