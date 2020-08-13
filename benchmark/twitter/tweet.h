#ifndef TWEET_H
#define TWEET_H

#include "simdjson.h"
#include "twitter_user.h"

namespace twitter {

// {
//   "statuses": [
//     {
//       "created_at": "Sun Aug 31 00:29:15 +0000 2014",
//       "id": 505874924095815700,
//       "text": "@aym0566x \n\n名前:前田あゆみ\n第一印象:なんか怖っ！\n今の印象:とりあえずキモい。噛み合わない\n好きなところ:ぶすでキモいとこ😋✨✨\n思い出:んーーー、ありすぎ😊❤️\nLINE交換できる？:あぁ……ごめん✋\nトプ画をみて:照れますがな😘✨\n一言:お前は一生もんのダチ💖",
//       "in_reply_to_status_id": null,
//       "user": {
//         "id": 1186275104,
//         "screen_name": "ayuu0123"
//       },
//       "retweet_count": 0,
//       "favorite_count": 0
//     }
//   ]
// }

struct tweet {
  std::string_view created_at{};
  uint64_t id{};
  std::string_view text{};
  uint64_t in_reply_to_status_id{};
  twitter_user user{};
  uint64_t retweet_count{};
  uint64_t favorite_count{};
};

} // namespace twitter

#endif // TWEET_H