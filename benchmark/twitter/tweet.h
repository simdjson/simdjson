#ifndef TWEET_H
#define TWEET_H

#include "simdjson.h"
#include "twitter_user.h"

namespace twitter {

struct tweet {
  uint64_t id{};
  std::string_view text{};
  std::string_view created_at{};
  uint64_t in_reply_to_status_id{};
  uint64_t retweet_count{};
  uint64_t favorite_count{};
  twitter_user user{};
};

} // namespace twitter

#endif // TWEET_H