#include <string>
#include <chrono>
#include <simdjson.h>
#include <simdjson.cpp>
#include <MemoryMappedLexSource.hpp>
#include <JsonReader.hpp>
using namespace json;
using namespace lex;
using namespace simdjson;
using namespace simdjson::builtin; // for ondemand
int64_t getus() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}
void readAllIds(LexSource &fls)
{
    JsonReader jr(fls);
    long long count = 0;
    while (jr.skipToFieldValue("id", JsonReader::Forward))
    {

        // read the while value so we're not cheating
        while (jr.nodeType() == JsonReader::ValuePart)
            jr.read();
        ++count;
    }
}
void readAllIds(ondemand::object& root) {
    // TODO: figure out how to recursively look for ids
    while(!root.find_field("id"_padded)) {
        printf("id\r\n");
    }
}
void extractEpisodes(LexSource &fls)
{
    // we don't need nearly this much. see the profile info output
    StaticMemoryPool<256> pool;
    JsonElement seasonNumber;
    JsonElement episodeNumber;
    JsonElement name;
    const char *fields[] = {"season_number", "episode_number", "name"};
    JsonExtractor children[] = {JsonExtractor(&seasonNumber), JsonExtractor(&episodeNumber), JsonExtractor(&name)};
    JsonExtractor extraction(fields, 3, children);
    JsonReader jr(fls);
    unsigned long long maxUsedPool = 0;
    int episodes = 0;
    while (jr.skipToFieldValue("episodes", JsonReader::Forward))
    {
        if (!jr.read())
            break;
        while (!jr.hasError() && JsonReader::EndArray != jr.nodeType())
        {

            // we keep track of the max pool we use
            if (pool.used() > maxUsedPool)
                maxUsedPool = pool.used();
            // we don't need to keep the pool data between calls here
            pool.freeAll();

            ++episodes;
            //if(jr.nodeType()==JsonReader::EndObject || jr.nodeType()==JsonReader::EndArray) jr.read();
            // read and extract from the next array element
            if (!jr.extract(pool, extraction))
            {
                printf("\t\t%d. (extraction failed!)\r\n",episodes);
                break;
            }
        }
    }
    if (jr.hasError())
    {
        // report the error
        printf("\tError (%d): %s\r\n",(int)jr.error(),jr.value());
        return;
    }
}

void getEpisodeData(ondemand::object& root) {
    auto seasons = root["seasons"].get_array();
    auto sit = seasons.begin();
    while(sit!=seasons.end()) {
        auto season = *sit;
        auto episodes = season["episodes"].get_array();
        auto eit = episodes.begin();
        while(eit!=episodes.end()) {
            auto episode = *eit;
            auto season_number = episode["season_number"].get_int64();
            auto episode_number = episode["episode_number"].get_int64();
            auto name = episode["name"];
            ++eit;
        }
        ++sit;
    }


}

int main(int cargs, char** argsv) {
    std::cout << "simdjson is using: " << active_implementation->name() << std::endl;
    int64_t start,stop;
    start = getus();
    ondemand::parser parser;
    padded_string json = padded_string::load("bulk.json");
    ondemand::document show = parser.iterate(json);
    auto str1=std::string_view(show["status"]);
    stop = getus();
    double time1 = (stop-start)/(1000.0);
    std::cout <<"simdjson took " << time1 << " ms to find " << str1 <<std::endl;
    start = getus();
    StaticMemoryMappedLexSource<256> mmls;
    mmls.open("./bulk.json");
    JsonReader jr(mmls);
    std::string str2;
    if(jr.skipToFieldValue("status",JsonReader::Siblings))
        str2 = jr.value();
    else
        str2 = "{not found}";
    mmls.close();
    stop = getus();
    double time2 = (stop-start)/(1000.0);
    std::cout <<"JSON(C++) took " << time2 << " ms to find " << str2 <<std::endl;

    start = getus();
    json = padded_string::load("bulk.json");
    show = parser.iterate(json);
    ondemand::object root = show.get_object();
    getEpisodeData(root);
    stop = getus();
    time1 = (stop-start)/(1000.0);
    std::cout <<"simdjson took " << time1 << " ms to extract episode data" <<std::endl;

    start = getus();
    mmls.open("./bulk.json");
    extractEpisodes(mmls);
    mmls.close();
    stop = getus();
    time2 = (stop-start)/(1000.0);
    std::cout <<"JSON(C++) took " << time2 << " ms to extract episode data" << std::endl;

    start = getus();
    json = padded_string::load("bulk.json");
    show = parser.iterate(json);
    root = show.get_object();
    //readAllIds(root);
    stop = getus();
    time1 = (stop-start)/(1000.0);
    //std::cout <<"simdjson took " << time1 << " ms to read all ids" <<std::endl;
    std::cout << "simdjson read all ids not yet implemented. time is for parsing only";

    start = getus();
    mmls.open("./bulk.json");
    readAllIds(mmls);
    mmls.close();
    stop = getus();
    time2 = (stop-start)/(1000.0);
    std::cout <<"JSON(C++) took " << time2 << " ms to read all ids" << std::endl;

    return 0;

}