#ifndef JSONIFIER_RECIPE_BOOK_DATA_H
#define JSONIFIER_RECIPE_BOOK_DATA_H

#include <jsonifier>
#include "recipe_book_data.h"

template <> struct jsonifier::core<Ingredient> {
  using value_type = Ingredient;
  static constexpr auto parseValue =
      createValue<&value_type::name, &value_type::quantity, &value_type::unit,
                  &value_type::note>();
};

template <> struct jsonifier::core<Nutrition> {
  using value_type = Nutrition;
  static constexpr auto parseValue =
      createValue<&value_type::calories, &value_type::protein_g,
                  &value_type::fat_g, &value_type::carbs_g>();
};

template <> struct jsonifier::core<Author> {
  using value_type = Author;
  static constexpr auto parseValue =
      createValue<&value_type::name, &value_type::handle>();
};

template <> struct jsonifier::core<Variation> {
  using value_type = Variation;
  static constexpr auto parseValue =
      createValue<&value_type::servings, &value_type::scale>();
};

template <> struct jsonifier::core<Recipe> {
  using value_type = Recipe;
  static constexpr auto parseValue = createValue<
      &value_type::id, &value_type::title, &value_type::summary,
      &value_type::cuisine, &value_type::servings, &value_type::prep_minutes,
      &value_type::cook_minutes, &value_type::difficulty, &value_type::rating,
      &value_type::tags, &value_type::ingredients, &value_type::steps,
      &value_type::nutrition, &value_type::photos, &value_type::author,
      &value_type::variations>();
};

#endif
