#ifndef TWITTER_USER_H
#define TWITTER_USER_H

#include "simdjson.h"

namespace twitter {

struct twitter_user {
  uint64_t id{};
  std::string_view screen_name{};
};

} // namespace twitter

#endif // TWITTER_USER_H