#ifndef NLOHMANN_TWITTER_DATA_H
#define NLOHMANN_TWITTER_DATA_H

#include "twitter_data.h"
#include <nlohmann/json.hpp>

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

void to_json(nlohmann::json &j, const Hashtag &h) {
  j = nlohmann::json{{"text", h.text},
                     {"indices_start", h.indices_start},
                     {"indices_end", h.indices_end}};
}

void to_json(nlohmann::json &j, const Url &u) {
  j = nlohmann::json{{"url", u.url},
                     {"expanded_url", u.expanded_url},
                     {"display_url", u.display_url},
                     {"indices_start", u.indices_start},
                     {"indices_end", u.indices_end}};
}

void to_json(nlohmann::json &j, const UserMention &um) {
  j = nlohmann::json{{"id", um.id},
                     {"name", um.name},
                     {"screen_name", um.screen_name},
                     {"indices_start", um.indices_start},
                     {"indices_end", um.indices_end}};
}

void to_json(nlohmann::json &j, const Entities &e) {
  j = nlohmann::json{{"hashtags", e.hashtags},
                     {"urls", e.urls},
                     {"user_mentions", e.user_mentions}};
}

void to_json(nlohmann::json &j, const Status &s) {
  j = nlohmann::json{{"created_at", s.created_at},
                     {"id", s.id},
                     {"text", s.text},
                     {"user", s.user},
                     {"entities", s.entities},
                     {"retweet_count", s.retweet_count},
                     {"favorite_count", s.favorite_count},
                     {"favorited", s.favorited},
                     {"retweeted", s.retweeted}};
}


std::string nlohmann_serialize(const std::vector<Hashtag>& v) {
  nlohmann::json a = nlohmann::json::array();
  for(const Hashtag & h : v) {
    a.push_back(nlohmann::json{{"text", h.text},
                     {"indices_start", h.indices_start},
                     {"indices_end", h.indices_end}});
  }
  return a.dump();
}
std::string nlohmann_serialize(const std::vector<Url>& v) {
  nlohmann::json a = nlohmann::json::array();
  for(const Url & u : v) {
    a.push_back(nlohmann::json{{"url", u.url},
                     {"expanded_url", u.expanded_url},
                     {"display_url", u.display_url},
                     {"indices_start", u.indices_start},
                     {"indices_end", u.indices_end}});
  }
  return a.dump();
}
std::string nlohmann_serialize(const std::vector<UserMention>& v) {
  nlohmann::json a = nlohmann::json::array();
  for(const UserMention & um : v) {
    a.push_back(nlohmann::json{{"id", um.id},
                     {"name", um.name},
                     {"screen_name", um.screen_name},
                     {"indices_start", um.indices_start},
                     {"indices_end", um.indices_end}});
  }
  return a.dump();
}

std::string nlohmann_serialize(const std::vector<Status>& v) {
  nlohmann::json a = nlohmann::json::array();
  for(const Status & s : v) {
    a.push_back(nlohmann::json{{"created_at", s.created_at},
                     {"id", s.id},
                     {"text", s.text},
                     {"user", s.user},
                     {"entities", s.entities},
                     {"retweet_count", s.retweet_count},
                     {"favorite_count", s.favorite_count},
                     {"favorited", s.favorited},
                     {"retweeted", s.retweeted}});
  }
  return a.dump();
}

void to_json(nlohmann::json &j, const TwitterData &t) {
  j = nlohmann::json{{"statuses", t.statuses}};
}

std::string nlohmann_serialize(const TwitterData &data) {
  return nlohmann_serialize(data.statuses);
}

#endif // NLOHMANN_TWITTER_DATA_H