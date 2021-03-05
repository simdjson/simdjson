#ifndef SIMDJSON_ONDEMAND_H
#define SIMDJSON_ONDEMAND_H

namespace simdjson {

using parser          = simdjson::ondemand::parser;
using document        = simdjson::ondemand::document;
using value           = simdjson::ondemand::value;
using object          = simdjson::ondemand::object;
using array           = simdjson::ondemand::array;
using field           = simdjson::ondemand::field;
using raw_json_string = simdjson::ondemand::raw_json_string;
using json_type       = simdjson::ondemand::json_type;

} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_H
