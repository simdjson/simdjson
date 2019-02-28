#include <map>
#include "simdjson/simdjerr.h"

const std::map<int, const std::string> errorStrings = {
    {simdjerr::SUCCESS, "No errors"},
    {simdjerr::CAPACITY, "This ParsedJson can't support a document that big"},
    {simdjerr::MEMALLOC, "Error allocating memory, we're most likely out of memory"},
    {simdjerr::TAPE_ERROR, "Something went wrong while writing to the tape"}
};

const std::string& simdjerr::errorMsg(const int errorCode) {
    return errorStrings.at(errorCode);
}