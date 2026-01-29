#ifndef TWITTER_DATA_H
#define TWITTER_DATA_H

#include <string>
#include <vector>

// Simplified Twitter structures for benchmarking

struct User {
  uint64_t id;
  std::string name;
  std::string screen_name;
  std::string location;
  std::string description;
  bool verified;
  uint64_t followers_count;
  uint64_t friends_count;
  uint64_t statuses_count;
};

struct Status {
  std::string created_at;
  uint64_t id;
  std::string text;
  User user;
  uint64_t retweet_count;
  uint64_t favorite_count;
};

struct TwitterData {
  std::vector<Status> statuses;
};

#endif // TWITTER_DATA_H