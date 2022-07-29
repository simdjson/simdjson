#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "top_tweet.h"

namespace top_tweet {

using namespace rapidjson;

struct rapidjson_base {
  using StringType=std::string_view;

  Document doc{};

  bool run(Document &root, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    result.retweet_count = -1;

    // Loop over the tweets
    if (root.HasParseError() || !root.IsObject()) { return false; }
    const auto &statuses = root.FindMember("statuses");
    if (statuses == root.MemberEnd() || !statuses->value.IsArray()) { return false; }
    for (const Value &tweet : statuses->value.GetArray()) {
      if (!tweet.IsObject()) { return false; }

      // Check if this tweet has a higher retweet count than the current top tweet
      const auto &retweet_count_json = tweet.FindMember("retweet_count");
      if (retweet_count_json == tweet.MemberEnd() || !retweet_count_json->value.IsInt64()) { return false; }
      int64_t retweet_count = retweet_count_json->value.GetInt64();
      if (retweet_count <= max_retweet_count && retweet_count >= result.retweet_count) {
        result.retweet_count = retweet_count;

        // TODO I can't figure out if there's a way to keep the Value to use outside the loop ...

        // Get text and screen_name of top tweet
        const auto &text = tweet.FindMember("text");
        if (text == tweet.MemberEnd() || !text->value.IsString()) { return false; }
        result.text = { text->value.GetString(), text->value.GetStringLength() };

        const auto &user = tweet.FindMember("user");
        if (user == tweet.MemberEnd() || !user->value.IsObject()) { return false; }
        const auto &screen_name = user->value.FindMember("screen_name");
        if (screen_name == user->value.MemberEnd() || !screen_name->value.IsString()) { return false; }
        result.screen_name = { screen_name->value.GetString(), screen_name->value.GetStringLength() };

      }
    }

    return result.retweet_count != -1;
  }
};

struct rapidjson : rapidjson_base {
  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    return rapidjson_base::run(doc.Parse<kParseValidateEncodingFlag>(json.data()), max_retweet_count, result);
  }
};
BENCHMARK_TEMPLATE(top_tweet, rapidjson)->UseManualTime();
#if SIMDJSON_COMPETITION_ONDEMAND_INSITU
struct rapidjson_insitu : rapidjson_base {
  bool run(simdjson::padded_string &json, int64_t max_retweet_count, top_tweet_result<StringType> &result) {
    return rapidjson_base::run(doc.ParseInsitu<kParseValidateEncodingFlag|kParseInsituFlag>(json.data()), max_retweet_count, result);
  }
};
BENCHMARK_TEMPLATE(top_tweet, rapidjson_insitu)->UseManualTime();
#endif // SIMDJSON_COMPETITION_ONDEMAND_INSITU
} // namespace top_tweet

#endif // SIMDJSON_COMPETITION_RAPIDJSON
