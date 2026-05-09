#ifndef GLAZE_TWITTER_DATA_H
#define GLAZE_TWITTER_DATA_H

#include "twitter_data.h"
#include <glaze/glaze.hpp>
#include <stdexcept>
#include <string>

// Glaze auto-reflects aggregate types whose field names already match JSON
// keys (snake_case here matches the JSON), so no glz::meta is required.

inline TwitterData glaze_deserialize(const std::string &json_str) {
    TwitterData data;
    constexpr glz::opts opts{.error_on_unknown_keys = false};
    auto err = glz::read<opts>(data, json_str);
    if (err) {
        throw std::runtime_error("glaze parse error: " + glz::format_error(err, json_str));
    }
    return data;
}

inline std::string glaze_serialize(const TwitterData &data) {
    std::string out;
    auto err = glz::write_json(data, out);
    if (err) {
        throw std::runtime_error("glaze write error");
    }
    return out;
}

#endif // GLAZE_TWITTER_DATA_H
