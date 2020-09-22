#include "simdjson.h"
#include "FuzzUtils.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

struct FuzzData {
    std::string_view json_pointer;
    std::string_view json_doc;
};

/**
 * @brief split split fuzz data into a pointer and a document
 * @param Data
 * @param Size
 * @return
 */
FuzzData split(const char *Data, size_t Size) {

    using namespace std::literals;
    constexpr auto sep="\n~~~\n"sv;

    std::string_view all(Data,Size);
    auto pos=all.find(sep);
    if(pos==std::string_view::npos) {
        //not found.
        return FuzzData{std::string_view{},all};
    } else {
        return FuzzData{std::string_view{all.substr(0,pos)},all.substr(pos+sep.size())};
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    // Split data into two strings, json pointer and the document string.
    // Might end up with none, either or both being empty, important for
    // covering edge cases such as https://github.com/simdjson/simdjson/issues/1142
    // Inputs missing the separator line will get an empty json pointer
    // but the all the input put in the document string. This means
    // test data from other fuzzers that take json input works for this fuzzer
    // as well.
    const auto fd=split(as_chars(Data),Size);

    simdjson::dom::parser parser;

    // parse without exceptions, for speed
    auto res=parser.parse(fd.json_doc.data(),fd.json_doc.size());
    if(res.error())
        return 0;

    simdjson::dom::element root;
    if(res.get(root))
        return 0;

    auto maybe_leaf=root.at_pointer(fd.json_pointer);
    if(maybe_leaf.error())
        return 0;

    simdjson::dom::element leaf;
    if(maybe_leaf.get(leaf))
        return 0;

    std::string_view sv;
    if(leaf.get_string().get(sv))
        return 0;
    return 0;
}
