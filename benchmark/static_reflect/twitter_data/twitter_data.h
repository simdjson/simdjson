#ifndef TWITTER_DATA_H
#define TWITTER_DATA_H

#include <string>
#include <vector>

struct User {
  int64_t id;
  std::string id_str;
  std::string name;
  std::string screen_name;
  std::string location;
  std::string description;
  bool verified;
  int64_t followers_count;
  int64_t friends_count;
  int64_t statuses_count;
  bool operator<=>(const User &other) const = default;
};

struct Hashtag {
  std::string text;
  int64_t indices_start;
  int64_t indices_end;
  bool operator<=>(const Hashtag &other) const = default;
};

struct Url {
  std::string url;
  std::string expanded_url;
  std::string display_url;
  int64_t indices_start;
  int64_t indices_end;
  bool operator<=>(const Url &other) const = default;
};

struct UserMention {
  int64_t id;
  std::string name;
  std::string screen_name;
  int64_t indices_start;
  int64_t indices_end;
  bool operator<=>(const UserMention &other) const = default;
};

struct Entities {
  std::vector<Hashtag> hashtags;
  std::vector<Url> urls;
  std::vector<UserMention> user_mentions;
  bool operator==(const Entities &other) const = default;
};

struct Status {
  std::string created_at;
  int64_t id;
  std::string text;
  User user;
  Entities entities;
  int64_t retweet_count;
  int64_t favorite_count;
  bool favorited;
  bool retweeted;
  bool operator==(const Status &other) const = default;
};

struct TwitterData {
  std::vector<Status> statuses;
  bool operator==(const TwitterData &other) const = default;
};

#endif