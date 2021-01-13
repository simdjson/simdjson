#pragma once
#include "simdjson.h"

namespace partial_tweets {

template<typename StringType=std::string_view>
struct twitter_user {
  uint64_t id{};
  StringType screen_name{};

  template<typename OtherStringType>
  bool operator==(const twitter_user<OtherStringType> &other) const {
    return id == other.id &&
           screen_name == other.screen_name;
  }
};

} // namespace partial_tweets
