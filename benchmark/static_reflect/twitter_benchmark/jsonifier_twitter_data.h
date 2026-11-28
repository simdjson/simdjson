#ifndef JSONIFIER_TWITTER_DATA_H
#define JSONIFIER_TWITTER_DATA_H

#include <jsonifier>
#include "twitter_data.h"

template <> struct jsonifier::core<User> {
  using value_type = User;
  static constexpr auto parseValue =
      createValue<&value_type::id, &value_type::name, &value_type::screen_name,
                  &value_type::location, &value_type::description,
                  &value_type::followers_count, &value_type::friends_count,
                  &value_type::verified, &value_type::statuses_count>();
};

template <> struct jsonifier::core<Status> {
  using value_type = Status;
  static constexpr auto parseValue =
      createValue<&value_type::created_at, &value_type::id, &value_type::text,
                  &value_type::user, &value_type::retweet_count,
                  &value_type::favorite_count>();
};

template <> struct jsonifier::core<TwitterData> {
  using value_type = TwitterData;
  static constexpr auto parseValue = createValue<&value_type::statuses>();
};

#endif
