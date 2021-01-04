#pragma once

#ifdef SIMDJSON_COMPETITION_RAPIDJSON

#include "distinct_user_id.h"

namespace distinct_user_id {

using namespace rapidjson;

template<int F>
class rapidjson_base {
  Document doc{};

public:
  bool run(const padded_string &json, std::vector<uint64_t> &ids) {
    auto &root = doc.Parse<F>(json.data());
    if (root.HasParseError() || !root.IsObject()) { return false; }
    auto statuses = root.FindMember("statuses");
    if (statuses == root.MemberEnd() || !statuses->value.IsArray()) { return false; }
    for (auto &tweet : statuses->value.GetArray()) {
      if (!tweet.IsObject()) { return false; }
      auto user = tweet.FindMember("user");
      if (user == tweet.MemberEnd() || !user->value.IsObject()) { return false; }
      auto id = user->value.FindMember("id");
      if (id == user->value.MemberEnd() || !id->value.IsUint64()) { return false; }
      ids.push_back(id->value.GetUint64());

      auto retweet = tweet.FindMember("retweeted_status");
      if (retweet != tweet.MemberEnd()) {
        if (!retweet->value.IsObject()) { return false; }
        user = retweet->value.FindMember("user");
        if (user == retweet->value.MemberEnd() || !user->value.IsObject()) { return false; }
        id = user->value.FindMember("id");
        if (id == user->value.MemberEnd() || !id->value.IsUint64()) { return false; }
        ids.push_back(id->value.GetUint64());
      }
    }

    return true;
  }
};

class rapidjson : public rapidjson_base<kParseValidateEncodingFlag> {};
BENCHMARK_TEMPLATE(distinct_user_id, rapidjson);

} // namespace partial_tweets

#endif // SIMDJSON_COMPETITION_RAPIDJSON
