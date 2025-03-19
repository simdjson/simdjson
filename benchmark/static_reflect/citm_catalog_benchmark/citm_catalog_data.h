#ifndef CITM_CATALOG_DATA_H
#define CITM_CATALOG_DATA_H

#include <string>
#include <vector>
#include <map>

struct Area {
  int64_t id;
  std::string name;
  int64_t parent;
  std::vector<int64_t> childAreas;
  bool operator==(const Area &other) const = default;
};

struct AudienceSubCategory {
  int64_t id;
  std::string name;
  int64_t parent;
  bool operator==(const AudienceSubCategory &other) const = default;
};

struct Event {
  int64_t id;
  std::string name;
  std::string description;
  int64_t subTopic;
  int64_t topic;
  std::vector<int64_t> audience;
  bool operator==(const Event &other) const = default;
};

struct Performance {
  int64_t id;
  std::string name;
  int64_t event;
  std::string start;
  int64_t venueCode;
  bool operator==(const Performance &other) const = default;
};

struct SeatCategory {
  int64_t id;
  std::string name;
  std::vector<int64_t> areas;
  bool operator==(const SeatCategory &other) const = default;
};

struct SubTopic {
  int64_t id;
  std::string name;
  int64_t parent;
  bool operator==(const SubTopic &other) const = default;
};

struct Topic {
  int64_t id;
  std::string name;
  bool operator==(const Topic &other) const = default;
};

struct Venue {
  int64_t id;
  std::string name;
  int64_t address;
  bool operator==(const Venue &other) const = default;
};

struct CitmCatalog {
  std::vector<std::pair<std::string, Area>> areas;
  std::vector<std::pair<std::string, AudienceSubCategory>> audienceSubCategory;
  std::vector<std::pair<std::string, Event>> events;
  std::vector<std::pair<std::string, Performance>> performances;
  std::vector<std::pair<std::string, SeatCategory>> seatCategory;
  std::vector<std::pair<std::string, SubTopic>> subTopic;
  std::vector<std::pair<std::string, Topic>> topic;
  std::vector<std::pair<std::string, Venue>> venue;

  bool operator==(const CitmCatalog &other) const = default;
};

#endif
