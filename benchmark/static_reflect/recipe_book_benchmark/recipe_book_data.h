#ifndef RECIPE_BOOK_DATA_H
#define RECIPE_BOOK_DATA_H

#include <optional>
#include <string>
#include <vector>

struct Ingredient {
  std::string name;
  double quantity;
  std::string unit;
  std::optional<std::string> note;
};

struct Nutrition {
  int calories;
  double protein_g;
  double fat_g;
  double carbs_g;
};

struct Author {
  std::string name;
  std::string handle;
};

struct Variation {
  int servings;
  std::vector<double> scale;
};

struct Recipe {
  std::string id;
  std::string title;
  std::optional<std::string> summary;
  std::optional<std::string> cuisine;
  std::optional<int> servings;
  std::optional<int> prep_minutes;
  std::optional<int> cook_minutes;
  std::optional<std::string> difficulty;
  std::optional<double> rating;
  std::optional<std::vector<std::string>> tags;
  std::vector<Ingredient> ingredients;
  std::vector<std::string> steps;
  std::optional<Nutrition> nutrition;
  std::optional<std::vector<std::string>> photos;
  std::optional<Author> author;
  std::optional<std::vector<Variation>> variations;
};
using RecipeBook = std::vector<Recipe>;

#endif // RECIPE_BOOK_DATA_H
