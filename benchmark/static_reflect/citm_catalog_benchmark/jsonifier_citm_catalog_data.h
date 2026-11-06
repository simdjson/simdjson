#ifndef JSONIFIER_CITM_CATALOG_DATA_H
#define JSONIFIER_CITM_CATALOG_DATA_H

#include <jsonifier>
#include "citm_catalog_data.h"

template <> struct jsonifier::core<CITMPrice> {
  using value_type = CITMPrice;
  static constexpr auto parseValue =
      createValue<&value_type::amount, &value_type::audienceSubCategoryId,
                  &value_type::seatCategoryId>();
};

template <> struct jsonifier::core<CITMArea> {
  using value_type = CITMArea;
  static constexpr auto parseValue =
      createValue<&value_type::areaId, &value_type::blockIds>();
};

template <> struct jsonifier::core<CITMSeatCategory> {
  using value_type = CITMSeatCategory;
  static constexpr auto parseValue =
      createValue<&value_type::areas, &value_type::seatCategoryId>();
};

template <> struct jsonifier::core<CITMPerformance> {
  using value_type = CITMPerformance;
  static constexpr auto parseValue =
      createValue<&value_type::eventId, &value_type::id, &value_type::logo,
                  &value_type::name, &value_type::prices,
                  &value_type::seatCategories, &value_type::seatMapImage,
                  &value_type::start, &value_type::venueCode>();
};

template <> struct jsonifier::core<CITMEvent> {
  using value_type = CITMEvent;
  static constexpr auto parseValue =
      createValue<&value_type::description, &value_type::id, &value_type::logo,
                  &value_type::name, &value_type::subTopicIds,
                  &value_type::subjectCode, &value_type::subtitle,
                  &value_type::topicIds>();
};

template <> struct jsonifier::core<CitmCatalog> {
  using value_type = CitmCatalog;
  static constexpr auto parseValue =
      createValue<&value_type::events, &value_type::performances>();
};

#endif
