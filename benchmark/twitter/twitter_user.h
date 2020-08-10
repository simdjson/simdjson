#ifndef TWITTER_USER_H
#define TWITTER_USER_H

#include "simdjson.h"

SIMDJSON_TARGET_HASWELL

namespace twitter {

struct twitter_user {
  uint64_t id{};
  std::string_view screen_name{};
};

} // namespace twitter

SIMDJSON_UNTARGET_REGION

#endif // TWITTER_USER_H