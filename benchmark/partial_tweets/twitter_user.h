#pragma once
#include "simdjson.h"

namespace partial_tweets {

struct twitter_user {
  uint64_t id{};
  std::string_view screen_name{};

  bool operator==(const twitter_user &other) const {
    return id == other.id &&
           screen_name == other.screen_name;
  }
};

} // namespace partial_tweets
