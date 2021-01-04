// TODO: Finish adding ESP8266 support to the demo. There are
// apparently issues using C++ with the old esp8266 sdk and
// platformIO won't use the new one. it can be done using the arduino
// framework but that's not my first choice.
#if defined ARDUINO || defined ESP_PLATFORM || defined ESP8266
#define IOT
#endif
#define LEXSOURCE_CAPTURE_SIZE 25
// there sure are a lot of ways to say "data.json"!:
#if defined ARDUINO
// arduino needs filenames in 8.3 format!
#define FILE_PATH "/data.jso"
#elif defined USELITTLEFS
#ifdef ESP_PLATFORM
#define FILE_PATH "/spiffs/data.json"
#else
#define FILE_PATH "/data.json"
#endif
#elif defined ESP_PLATFORM
// ESP32 FreeRTOS uses posix style mount points
#define FILE_PATH "/sdcard/data.jso"
#elif defined __linux__
#define FILE_PATH "./bulk.json"
#elif defined _WIN32
#define FILE_PATH "bulk.json"
#endif
#define SEASON_INDEX 7
#define EPISODE_INDEX 0

#include <stdio.h>
#if !defined ARDUINO
#include <chrono>
#endif
#if defined ARDUINO
#include <Arduino.h>
#if USELITTLEFS
#include <FS.h>
#include <LittleFS.h>
#else
#include <SD.h>
#endif
#endif
#if defined ESP_PLATFORM
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <sys/types.h>
#include "driver/gpio.h"
#endif
#ifndef IOT
#include "MemoryMappedLexSource.hpp"
#define MMAP
#endif
#include "JsonReader.hpp"
using namespace lex;
using namespace json;
void scratch();
void benchmarks();
void readAllIds(LexSource &fls);
void skipToSeasonIndexAndEpisodeIndex(LexSource &ls, int si, int ei);
void extractEpisodes(LexSource &fls, bool silent = false);
void showExtraction(LexSource &fls, bool silent = false);
void parseEpisodes(LexSource &fls, bool silent = false);
void countEpisodes(LexSource &fls);
void readStatus(LexSource &fls);
void printExtraction(MemoryPool &pool, const JsonExtractor &extraction, int depth);
void printSEFmt(long long s, long long e);
void printSpeed(double bps);
// some Arduinos don't have printf so we have an alternative
void print(const char *v);
void print(long long v);
void print(double v);
void print(int v);
void println();
// get the microseconds
unsigned long long getus();

void scratch()
{
    JsonElement showName;
    JsonElement createdByName;
    JsonElement createdById;
    JsonExtractor createdByNameExtractor(&createdByName);
    JsonExtractor createdByIdExtractor(&createdById);
    JsonExtractor createdByEntryFieldExtractors[]{createdByNameExtractor, createdByIdExtractor};
    const char *createdByEntryFields[] = {"name", "id"};
    JsonExtractor createdByEntryExtractor(createdByEntryFields, 2, createdByEntryFieldExtractors);
    JsonExtractor createdByEntryExtractors[]{createdByEntryExtractor};
    size_t createdByArrayIndexExtractorIndices[]{0};
    JsonExtractor createdByArrayIndexExtractor(createdByArrayIndexExtractorIndices, 1, createdByEntryExtractors);
    JsonExtractor showNameExtractor(&showName);
    const char *showFields[] = {"name", "created_by"};
    JsonExtractor showExtractors[]{showNameExtractor, createdByArrayIndexExtractor};
    JsonExtractor showExtractor(showFields, 2, showExtractors);

    // Arduino framework uses its own API for
    // accessing files and such
#ifdef ARDUINO
    ArduinoLexSource<256> fls;
    File file;
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif
    if (!fls.begin(file))
        return;
#else
    StaticFileLexSource<256> fls;
    fls.open(FILE_PATH);
#endif
#ifndef IOT
    StaticMemoryPool<8192> pool;
#else
    StaticMemoryPool<2048> pool;
#endif
    JsonReader jr(fls);
    if (!jr.extract(pool, showExtractor))
    {
        print("Extraction failed");
        println();
        if (jr.hasError())
        {
            print(jr.value());
            println();
        }
    }
    else
    {
        printExtraction(pool, showExtractor, 0);
    }
#ifndef ARDUINO
    fls.close();
#else
    file.close();
#endif
}
void benchmarks()
{
#ifdef MMAP
    StaticMemoryMappedLexSource<LEXSOURCE_CAPTURE_SIZE> mmls;
#endif
    // this isn't used for IOT devices but we keep it around so
    // i didn't have to conditionally compile it out
    StaticSZLexSource<LEXSOURCE_CAPTURE_SIZE> szls;

    // Arduino doesn't use fopen() to access files.
#ifndef ARDUINO
    StaticFileLexSource<LEXSOURCE_CAPTURE_SIZE> fls;
#else
    ArduinoLexSource<LEXSOURCE_CAPTURE_SIZE> fls;
#endif
    int nodes = 0;
    char *szdata = nullptr;
#ifndef IOT
    // don't even try to load this into memory
    // on an arduino or ESP device!
    FILE *pf = fopen(FILE_PATH, "rb");
    fseek(pf, 0L, SEEK_END);
    // use system heap via dynamic memory pool since we don't know how much size we have.
    DynamicMemoryPool readPool((size_t)ftell(pf));
    rewind(pf);
    szdata = (char *)readPool.alloc(readPool.capacity());
    if (nullptr != pf)
    {
        szdata[fread(szdata, 1, readPool.capacity(), pf)] = 0;
    }
    fclose(pf);
#endif
    double ms;
    unsigned long long ustart, ustop;
    print("Extract episodes - JSONPath equivelent: $..episodes[*].season_number,episode_number,name");
    println();
    if (nullptr != szdata)
    {
        szls.attach(szdata);
        //ustart = getus();
        ustart = getus();
        extractEpisodes(szls, true);
        //ustop = getus();
        ustop = getus();
        //ms = (ustop-ustart)/1000.0;
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }

#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    extractEpisodes(mmls, true);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
    mmls.close();
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
#ifdef ARDUINO
    File file;
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif
    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    ustart = getus();
    extractEpisodes(fls, true);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();

    print("Episode count - JSONPath equivelent: $..episodes[*].length()");
    println();
    if (nullptr != szdata)
    {
        szls.attach(szdata);
        ustart = getus();
        countEpisodes(szls);
        ustop = getus();
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }
#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    countEpisodes(mmls);
    ustop = getus();
    mmls.close();
    ms = (ustop - ustart) / 1000.0;
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
#ifdef ARDUINO
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif

    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    ustart = getus();
    countEpisodes(fls);
    ustop = getus();
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    ms = (ustop - ustart) / 1000.0;
    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();

    print("Read id fields - JSONPath equivelent: $..id");
    println();
    if (nullptr != szdata)
    {
        szls.attach(szdata);
        ustart = getus();
        readAllIds(szls);
        ustop = getus();
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }
#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    readAllIds(mmls);
    ustop = getus();
    mmls.close();
    ms = (ustop - ustart) / 1000.0;
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
#ifdef ARDUINO
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif

    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    ustart = getus();
    readAllIds(fls);
    ustop = getus();
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    ms = (ustop - ustart) / 1000.0;
    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();

    print("Skip to season/episode by index - JSONPath equivelent $.seasons[");
    print(SEASON_INDEX);
    print("].episodes[");
    print(EPISODE_INDEX);
    print("].name");
    println();
    if (nullptr != szdata)
    {
        szls.attach(szdata);
        ustart = getus();
        skipToSeasonIndexAndEpisodeIndex(szls, SEASON_INDEX, EPISODE_INDEX);
        ustop = getus();
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }
#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    skipToSeasonIndexAndEpisodeIndex(mmls, SEASON_INDEX, EPISODE_INDEX);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
    mmls.close();
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
#ifdef ARDUINO
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif

    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    ustart = getus();
    skipToSeasonIndexAndEpisodeIndex(fls, SEASON_INDEX, EPISODE_INDEX);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();

    print("Read the entire document");
    println();
    if (nullptr != szdata)
    {
        nodes = 0;
        szls.attach(szdata);
        JsonReader jr(szls);
        ustart = getus();
        while (jr.read())
            ++nodes;
        ustop = getus();
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tRead ");
        print(nodes);
        print(" nodes");
        println();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }
    nodes = 0;
#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    JsonReader jr1(mmls);
    ustart = getus();
    while (jr1.read())
        ++nodes;
    ustop = getus();
    mmls.close();
    ms = (ustop - ustart) / 1000.0;
    print("\tRead ");
    print(nodes);
    print(" nodes");
    println();
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
    nodes = 0;
#ifdef ARDUINO
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif

    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    JsonReader jr2(fls);
    ustart = getus();
    while (jr2.read())
        ++nodes;
    ustop = getus();
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    ms = (ustop - ustart) / 1000.0;
    print("\tRead ");
    print(nodes);
    print(" nodes");
    println();

    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();

    print("Structured skip of entire document");
    println();
    if (nullptr != szdata)
    {
        szls.attach(szdata);
        JsonReader jr(szls);
        ustart = getus();
        jr.skipSubtree();
        ustop = getus();
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }
#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    JsonReader jr3(mmls);
    ustart = getus();
    jr3.skipSubtree();
    ustop = getus();
    mmls.close();
    ms = (ustop - ustart) / 1000.0;
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
#ifdef ARDUINO
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif
    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    JsonReader jr4(fls);
    ustart = getus();
    jr4.skipSubtree();
    ustop = getus();
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    ms = (ustop - ustart) / 1000.0;
    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();

    print("Episode parsing - JSONPath equivelent: $..episodes[*]");
    println();
    if (nullptr != szdata)
    {
        szls.attach(szdata);
        ustart = getus();
        parseEpisodes(szls, true);
        ustop = getus();
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }
#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    parseEpisodes(mmls, true);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
    mmls.close();
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
#ifdef ARDUINO
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif
    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    ustart = getus();
    parseEpisodes(fls, true);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();

    print("Read status - JSONPath equivelent: $.status");
    println();
    if (nullptr != szdata)
    {
        szls.attach(szdata);
        ustart = getus();
        readStatus(szls);
        ustop = getus();
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }
#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    readStatus(mmls);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
    mmls.close();
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
#ifdef ARDUINO
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif
    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    ustart = getus();
    readStatus(fls);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();

    print("Extract root - JSONPath equivelent: $.id,name,created_by[0].name,created_by[0].profile_path,number_of_episodes,last_episode_to_air.name");
    println();
    if (nullptr != szdata)
    {
        szls.attach(szdata);
        ustart = getus();
        showExtraction(szls, true);
        ustop = getus();
        ms = (ustop - ustart) / 1000.0;
        szls.detach();
        print("\tMemory: ");
        printSpeed(szls.position() / (ms / 1000.0));
        println();
    }
#ifdef MMAP
    mmls.open(FILE_PATH);
    ustart = getus();
    showExtraction(mmls, true);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
    mmls.close();
    print("\tMemory Mapped: ");
    printSpeed(mmls.position() / (ms / 1000.0));
    println();
#endif
#ifdef ARDUINO
#ifdef USELITTLEFS
    file = SPIFFS.open(FILE_PATH,"r");
#else
    file = SD.open(FILE_PATH);
#endif

    fls.begin(file);
#else
    fls.open(FILE_PATH);
#endif
    ustart = getus();
    showExtraction(fls, true);
    ustop = getus();
    ms = (ustop - ustart) / 1000.0;
#ifdef ARDUINO
    if (file)
        file.close();
#else
    fls.close();
#endif
    print("\tFile: ");
    printSpeed(fls.position() / (ms / 1000.0));
    println();
    println();
}
unsigned long long getus()
{
#ifdef ARDUINO
    return micros();
#else
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
}
void skipToSeasonIndexAndEpisodeIndex(LexSource &fls, int si, int ei)
{
    JsonReader jr(fls);
    // JSONPath $.seasons[@si].episodes[@ei].name
    if (jr.skipToFieldValue("seasons", JsonReader::Siblings))
    {
        if (jr.skipToIndex(si))
        {
            // the depth param is ignored unless axis = JsonReader::Descendants, in which case it is required
            unsigned long int depth = 0;
            int axis = JsonReader::Siblings;
            if (jr.skipToFieldValue("episodes", axis, &depth))
            {
                if (jr.skipToIndex(ei))
                {
                    unsigned long int depth2 = 0;
                    // this code can handle streaming name fields:
                    if (jr.skipToFieldValue("name", axis, &depth2))
                    {
                        print("\tFound \"");
                        print(jr.value());
                        while (JsonReader::ValuePart == jr.nodeType())
                        {
                            print(jr.value());
                            if (!jr.read())
                                break;
                        }
                        print("\"");
                        println();
                    }
                }
            }
        }
    }
    if (jr.hasError())
    {
        // report the error
        print("\tError (");
        print((int)jr.error());
        print("): ");
        print(jr.value());
        return;
    }
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
    print("\tRead ");
    print(count);
    print(" id fields");
    println();
}

void printSEFmt(int s, int e)
{
    char sz[16];
    sprintf(sz, "S%02dE%02d", s, e);
    print(sz);
}
void extractEpisodes(LexSource &fls, bool silent)
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
                print("\t\t");
                print(episodes);
                print(". ");
                print("Extraction failed");
                println();

                break;
            }

            if (!silent)
            {
                print("\t\t");
                print(episodes);
                print(". ");
                printSEFmt((int)seasonNumber.integer(), (int)episodeNumber.integer());
                print(" ");
                print(name.string());
                println();
            }
        }
    }
    if (jr.hasError())
    {
        // report the error
        print("\tError (");
        print((int)jr.error());
        print("): ");
        print(jr.value());
        println();
        return;
    }
    else
    {
        print("\tExtracted ");
        print(episodes);
        print(" episodes using ");
        print((long long)maxUsedPool);
        print(" bytes of the pool");
        println();
    }
}

void parseEpisodes(LexSource &fls, bool silent)
{
    // big and slow! included for completeness
    // there are better ways to do almost everything
    // see performance info output

    // it won't run on devices without some RAM to spare

    DynamicMemoryPool pool(5200); // we also need space to hold the JSON output if silent=false
    if (0 == pool.capacity())
    {
        print("\tNot enough RAM to complete the operation");
        println();
        return;
    }
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

            // read and extract from the next array element
            JsonElement e;
            if (!jr.parseSubtree(pool, &e))
                break;
            if (!silent)
            {
                print(e.toString(pool));
                println();
            }
        }
    }
    if (jr.hasError())
    {
        // report the error
        print("\tError (");
        print((int)jr.error());
        print("): ");
        print(jr.value());
        return;
    }
    else
    {
        print("\tParsed ");
        print(episodes);
        print(" episodes using ");
        print((long long)maxUsedPool);
        print(" bytes of the pool");
        println();
    }
}

void readStatus(LexSource &fls)
{
    // open the file
    // create the reader
    JsonReader jr(fls);

    // we start on nodeType() Initial, but that's sort of a pass through node. skipToField()
    // and skipToFieldValue() simply read through when it finds it, so our first level is
    // actually the root object.
    // look for the field named "name" on this level of the heirarchy.
    // then read its value which comes right after it.
    if (jr.skipToFieldValue("status", JsonReader::Siblings))
    {
        // print the string
        print("\tstatus: ");
        // begin reading the value. if nodeType() is Value this is the only read necessary
        print(jr.value());
        // if it streamed our value, make sure we print all of it
        while (jr.read() && JsonReader::ValuePart == jr.nodeType())
        {
            print(jr.value());
        }
        println();
    }
    else if (jr.hasError())
    {
        // report the error
        print("\tError (");
        print((int)jr.error());
        print("): ");
        print(jr.value());
        return;
    }
}
void countEpisodes(LexSource &fls)
{

    int episodes = 0;
    JsonReader jr(fls);

    while (jr.skipToFieldValue("episodes", JsonReader::Forward))
    {
        // move past the open Array node
        if (!jr.read())
            break;
        while (!jr.hasError() && JsonReader::EndArray != jr.nodeType())
        {
            // skip the next array element
            if (!jr.skipSubtree())
            {
                break;
            }
            ++episodes;
        }
    }
    if (jr.hasError())
    {
        // report the error
        print("\tError (");
        print((int)jr.error());
        print("): ");
        print(jr.value());
        return;
    }
    else
    {
        print("\tFound ");
        print(episodes);
        print(" episodes");
        println();
    }
}
void showExtraction(LexSource &ls, bool silent)
{
    //JSONPath is roughly $.id,name,created_by[0].name,created_by[0].profile_path,number_of_episodes,last_episode_to_air.name
    DynamicMemoryPool pool(256);
    if (0 == pool.capacity())
    {
        print("\tNot enough memory to perform the operation");
        println();
        return;
    }
    // create nested the extraction query

    // we want name and credit_id from the created_by array's objects
    JsonElement creditName;
    JsonElement creditProfilePath;
    // we also want id, name, and number_of_episodes to air from the root object
    JsonElement id;
    JsonElement name;
    JsonElement numberOfEpisodes;
    // we want the last episode to air's name.
    JsonElement lastEpisodeToAirName;

    // create nested the extraction query for $.created_by.name and $.created_by.profile_path
    const char *createdByFields[] = {"name", "profile_path"};
    JsonExtractor createdByExtractions[] = {JsonExtractor(&creditName), JsonExtractor(&creditProfilePath)};
    JsonExtractor createdByExtraction(createdByFields, 2, createdByExtractions);
    // we want the first index of the created_by array: $.created_by[0]
    JsonExtractor createdByArrayExtractions[]{createdByExtraction};
    size_t createdByArrayIndices[] = {0};
    JsonExtractor createdByArrayExtraction(createdByArrayIndices, 1, createdByArrayExtractions);
    // we want the name off of last_episode_to_air, like $.last_episode_to_air.name
    const char *lastEpisodeFields[] = {"name"};
    JsonExtractor lastEpisodeExtractions[] = {JsonExtractor(&lastEpisodeToAirName)};
    JsonExtractor lastEpisodeExtraction(lastEpisodeFields, 1, lastEpisodeExtractions);
    // we want id,name, and created by from the root
    // $.id, $.name, $.created_by, $.number_of_episodes and $.last_episode_to_air
    const char *showFields[] = {"id", "name", "created_by", "number_of_episodes", "last_episode_to_air"};
    JsonExtractor showExtractions[] = {
        JsonExtractor(&id),
        JsonExtractor(&name),
        createdByArrayExtraction,
        JsonExtractor(&numberOfEpisodes),
        lastEpisodeExtraction};
    JsonExtractor showExtraction(showFields, 5, showExtractions);

    JsonReader jsonReader(ls);

    if (jsonReader.extract(pool, showExtraction))
    {
        if (!silent)
            printExtraction(pool, showExtraction, 0);
        print("\tUsed ");
        print((int)pool.used());
        print(" bytes of the pool");
        println();
    }
    else
    {
        print("extract() failed.");
        println();
    }
}
void printExtraction(MemoryPool &pool, const JsonExtractor &extraction, int depth)
{
    if (0 == extraction.count)
    {
        if (nullptr == extraction.presult)
        {
            print("(#error NULL result)");
            println();
            return;
        }
        switch (extraction.presult->type())
        {
        case JsonElement::Null:
            print("<null>");
            println();
            break;

        case JsonElement::String:
            print(extraction.presult->string());
            println();
            break;

        case JsonElement::Real:
            print(extraction.presult->real());
            println();
            break;

        case JsonElement::Integer:
            print(extraction.presult->integer());
            println();
            break;

        case JsonReader::Boolean:
            print(extraction.presult->boolean() ? "true" : "false");
            println();
            break;
        case JsonElement::Undefined:
            print("<no result>");
            println();
            break;

        default:
            print(extraction.presult->toString(pool));
            println();
            break;
        }

        return;
    }
    if (nullptr != extraction.pfields)
    {
        if (0 < extraction.count)
        {
            JsonExtractor q = extraction.pchildren[0];
            print(extraction.pfields[0]);
            print(": ");
            printExtraction(pool, extraction.pchildren[0], depth + 1);
        }
        for (size_t i = 1; i < extraction.count; ++i)
        {
            JsonExtractor q = extraction.pchildren[i];
            for (int j = 0; j < depth; ++j)
                print("  ");
            print(extraction.pfields[i]);
            print(": ");
            printExtraction(pool, extraction.pchildren[i], depth + 1);
        }
    }
    else if (nullptr != extraction.pindices)
    {
        if (0 < extraction.count)
        {
            print("[");
            print((int)extraction.pindices[0]);
            print("]: ");
            printExtraction(pool, extraction.pchildren[0], depth + 1);
        }
        for (size_t i = 1; i < extraction.count; ++i)
        {
            JsonExtractor q = extraction.pchildren[i];
            for (int j = 0; j < depth; ++j)
                print("  ");
            print("[");
            print((int)extraction.pindices[i]);
            print("]: ");
            printExtraction(pool,q, depth + 1);
        }
    }
}
void print(const char *v)
{
#ifdef ARDUINO
    Serial.print(v);
#else
    printf("%s", v);
#endif
}
void print(long long v)
{
#ifdef ARDUINO
    Serial.print((long)v);
#else
    printf("%lli", v);
#endif
}
void print(int v)
{
#ifdef ARDUINO
    Serial.print(v);
#else
    printf("%d", v);
#endif
}
void print(double v)
{
#ifdef ARDUINO
    static char buf[32];
    dtostrf(v, 31, 6, buf);
    print(buf);
#else
    printf("%f", v);
#endif
}
void println()
{
    print("\r\n");
}
void printSpeed(double bps)
{
    if (bps >= (1024.0 * 1024.0 * 1024.0))
    {
        print(bps / (1024.0 * 1024.0 * 1024.0));
        print("GB/s");
        return;
    }
    if (bps >= (1024.0 * 1024.0))
    {
        print(bps / (1024.0 * 1024.0));
        print("MB/s");
        return;
    }
    if (bps >= 1024)
    {
        print(bps / 1024.0);
        print("kB/s");
        return;
    }
    print(bps);
    print("bytes/s");
}

#ifdef ESP_PLATFORM
extern "C"
{
    void app_main(void);
}
// ALTER THESE TO YOUR PIN CONFIGS
#define SDSPI_SLOT_CONFIG_DEVKIT()   \
    {                                \
        .gpio_miso = GPIO_NUM_19,    \
        .gpio_mosi = GPIO_NUM_23,    \
        .gpio_sck = GPIO_NUM_18,     \
        .gpio_cs = GPIO_NUM_5,       \
        .gpio_cd = SDSPI_SLOT_NO_CD, \
        .gpio_wp = SDSPI_SLOT_NO_WP, \
        .gpio_int = GPIO_NUM_NC,     \
        .dma_channel = 1             \
    }
// ALTER THIS TO USE HSPI INSTEAD OF VSPI
// ALSO ENSURE THE CARD SPEED.
#define SDSPI_HOST_VSPI()                             \
    {                                                 \
        .flags = SDMMC_HOST_FLAG_SPI,                 \
        .slot = VSPI_HOST,                            \
        .max_freq_khz = SDMMC_FREQ_HIGHSPEED,         \
        .io_voltage = 3.3f,                           \
        .init = &sdspi_host_init,                     \
        .set_bus_width = NULL,                        \
        .get_bus_width = NULL,                        \
        .set_bus_ddr_mode = NULL,                     \
        .set_card_clk = &sdspi_host_set_card_clk,     \
        .do_transaction = &sdspi_host_do_transaction, \
        .deinit = &sdspi_host_deinit,                 \
        .io_int_enable = &sdspi_host_io_int_enable,   \
        .io_int_wait = &sdspi_host_io_int_wait,       \
        .command_timeout_ms = 0,                      \
    }
    //spi_get_freq_limit().
void app_main()
{


    sdmmc_host_t host = SDSPI_HOST_VSPI();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEVKIT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 2,
        .allocation_unit_size = 16 * 1024};
    sdmmc_card_t *card;
    if(ESP_OK!=esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card)) {
        print("Error mounting SD");
        println();
        while(true); //halt
    }

    benchmarks();
    //scratch();
    esp_vfs_fat_sdmmc_unmount();
    return;
}
#elif !defined ARDUINO
int main(int argc, char **argv)
{
    benchmarks();
    //scratch();
    return 0;
}
#else
void setup();
void loop();

void loop() {}
void setup()
{
    // wire an SD reader to your
    // first SPI bus and the standard
    // CS for your board
    //
    // put data.json on it but
    // name it "data.jso" due
    // to 8.3 filename limits
    // on some platforms
    Serial.begin(115200);
#if USELITTLEFS
    if(!SPIFFS.begin()) {
        Serial.println("Unable to mount SPIFFS");
        while (true)
            ; // halt
    }
#else
    if (!SD.begin())
    {
        Serial.println("Unable to mount SD");
        while (true)
            ; // halt
    }
#endif
    benchmarks();
    //scratch();
}
#endif