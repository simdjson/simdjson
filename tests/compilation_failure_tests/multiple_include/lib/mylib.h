#ifndef MYLIB_H
#define MYLIB_H

#include <simdjson.h>
#include <string>

simdjson::padded_string minify_json(const simdjson::padded_string& json);

#endif // MYLIB_H