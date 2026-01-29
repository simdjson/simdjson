#ifndef NLOHMANN_TWITTER_DATA_H
#define NLOHMANN_TWITTER_DATA_H

#include "twitter_data.h"
#include <nlohmann/json.hpp>

// Serialization functions for nlohmann
void to_json(nlohmann::json &j, const User &u) {
  j = nlohmann::json{{"id", u.id},
                     {"name", u.name},
                     {"screen_name", u.screen_name},
                     {"location", u.location},
                     {"description", u.description},
                     {"verified", u.verified},
                     {"followers_count", u.followers_count},
                     {"friends_count", u.friends_count},
                     {"statuses_count", u.statuses_count}};
}

void to_json(nlohmann::json &j, const Status &s) {
  j = nlohmann::json{{"created_at", s.created_at},
                     {"id", s.id},
                     {"text", s.text},
                     {"user", s.user},
                     {"retweet_count", s.retweet_count},
                     {"favorite_count", s.favorite_count}};
}

void to_json(nlohmann::json &j, const TwitterData &t) {
  j = nlohmann::json{{"statuses", t.statuses}};
}

// Deserialization functions for nlohmann
void from_json(const nlohmann::json &j, User &u) {
  j.at("id").get_to(u.id);
  j.at("name").get_to(u.name);
  j.at("screen_name").get_to(u.screen_name);
  j.at("location").get_to(u.location);
  j.at("description").get_to(u.description);
  j.at("verified").get_to(u.verified);
  j.at("followers_count").get_to(u.followers_count);
  j.at("friends_count").get_to(u.friends_count);
  j.at("statuses_count").get_to(u.statuses_count);
}

void from_json(const nlohmann::json &j, Status &s) {
  j.at("created_at").get_to(s.created_at);
  j.at("id").get_to(s.id);
  j.at("text").get_to(s.text);
  j.at("user").get_to(s.user);
  j.at("retweet_count").get_to(s.retweet_count);
  j.at("favorite_count").get_to(s.favorite_count);
}

void from_json(const nlohmann::json &j, TwitterData &t) {
  j.at("statuses").get_to(t.statuses);
}

// Helper functions for benchmarking
std::string nlohmann_serialize(const TwitterData &data) {
  nlohmann::json j = data;
  return j.dump();
}

TwitterData nlohmann_deserialize(const std::string &json_str) {
  nlohmann::json j = nlohmann::json::parse(json_str);
  return j.get<TwitterData>();
}

#endif // NLOHMANN_TWITTER_DATA_H