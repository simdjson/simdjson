#include <map>
#include "simdjson/simdjson.h"

const std::map<int, const std::string> errorStrings = {
    {simdjson::SUCCESS, "No errors"},
    {simdjson::CAPACITY, "This ParsedJson can't support a document that big"},
    {simdjson::MEMALLOC, "Error allocating memory, we're most likely out of memory"},
    {simdjson::TAPE_ERROR, "Something went wrong while writing to the tape"}
};

const std::string& simdjson::errorMsg(const int errorCode) {
    return errorStrings.at(errorCode);
}