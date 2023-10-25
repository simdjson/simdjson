#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace json_package_tests {
using namespace std;
bool baby() {
    TEST_START();
  auto json = R"({
  "name": "Daniel",
  "age": 42
})"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));

  simdjson::ondemand::object main_object;
  ASSERT_SUCCESS(doc.get_object().get(main_object));
  std::string name;
  ASSERT_SUCCESS(main_object["name"].get_string(name));
  ASSERT_EQUAL(name, "Daniel");
  uint64_t age;
  ASSERT_SUCCESS(main_object["age"].get_uint64().get(age));
  ASSERT_EQUAL(age, 42);
  TEST_SUCCEED();
}

bool thirtysecondsofcode() {
  TEST_START();
  auto json = R"({
  "name": "30-seconds-of-code",
  "private": true,
  "version": "10.0.0",
  "description": "30 seconds of code website.",
  "exports": "./index.js",
  "author": "chalarangelo",
  "type": "module",
  "devDependencies": {
    "@jsiqle/core": "^3.0.0",
    "astro": "^3.2.0",
    "chalk": "^5.3.0",
    "eslint": "^8.50.0",
    "eslint-config-prettier": "^9.0.0",
    "front-matter": "^4.0.2",
    "fs-extra": "^11.1.1",
    "glob": "^10.3.10",
    "hast-util-to-html": "^9.0.0",
    "js-yaml": "^4.1.0",
    "mdast-util-to-hast": "^13.0.2",
    "prettier": "^3.0.3",
    "prettier-plugin-astro": "^0.12.0",
    "prismjs": "^1.29.0",
    "remark": "^15.0.1",
    "remark-gfm": "^4.0.0",
    "sass": "^1.68.0",
    "sharp": "^0.32.6",
    "unist-util-select": "^5.0.0",
    "unist-util-visit": "^5.0.0",
    "unist-util-visit-parents": "^6.0.1",
    "webfonts-generator": "^0.4.0"
  },
  "imports": {
    "#blocks/*": "./src/blocks/*.js",
    "#components/*": "./src/components/*.astro",
    "#layouts/*": "./src/layouts/*.astro",
    "#settings/*": "./src/settings/*.js",
    "#prefabs": "./src/prefabs/index.js",
    "#utils": "./src/utils/index.js",
    "#utils/search": "./src/utils/search.js"
  },
  "scripts": {
    "predev": "NODE_ENV=development node ./src/scripts/develop.js",
    "dev": "astro dev --port 8000",
    "start": "astro dev --port 8000",
    "prebuild": "NODE_ENV=production node ./src/scripts/build.js",
    "build": "astro build",
    "preview": "astro preview --port 9000",
    "watch": "NODE_ENV=development node ./src/scripts/watch.js",
    "console": "NODE_ENV=production node ./src/scripts/console.js",
    "create": "NODE_ENV=production node ./src/scripts/create.js",
    "icons": "NODE_ENV=production node ./src/scripts/icons.js",
    "manifest": "NODE_ENV=production node ./src/scripts/manifest.js"
  },
  "license": "MIT",
  "repository": {
    "type": "git",
    "url": "https://github.com/30-seconds/30-seconds-of-code"
  },
  "bugs": {
    "url": "https://github.com/30-seconds/30-seconds-of-code/issues"
  },
  "browserslist": [
    "> 0.5% and last 4 versions and not dead and not ie>0 and not op_mini all and not and_uc>0 and not edge<79"
  ],
  "engines": {
    "node": ">=18.14.2"
  }
})"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));

  simdjson::ondemand::object main_object;
  ASSERT_SUCCESS(doc.get_object().get(main_object));

  simdjson::ondemand::raw_json_string key;
  simdjson::ondemand::value value;

  for (auto field : main_object) {
    // Throw error if getting key or value fails.
    ASSERT_SUCCESS(field.key().get(key));
    ASSERT_SUCCESS(field.value().get(value));

    if (key == "name") {
      std::string name;
      ASSERT_SUCCESS(value.get_string(name));
      ASSERT_EQUAL(name, "30-seconds-of-code");
    } else if (key == "main") {
      std::string main;
      ASSERT_SUCCESS(value.get_string(main));
      // unused
    } else if (key == "exports") {
      simdjson::ondemand::json_type exports_type;
      if (!value.type().get(exports_type)) {
        std::string_view exports;
        switch (exports_type) {
        case simdjson::ondemand::json_type::object: {
          simdjson::ondemand::object exports_object;
          if (!value.get_object().get(exports_object) &&
              !exports_object.raw_json().get(exports)) {
            // unused
          }
          break;
        }
        case simdjson::ondemand::json_type::array: {
          simdjson::ondemand::array exports_array;
          if (!value.get_array().get(exports_array) &&
              !exports_array.raw_json().get(exports)) {
            // unused
          }
          break;
        }
        case simdjson::ondemand::json_type::string: {
          if (!value.get_string().get(exports)) {
            ASSERT_EQUAL(exports, "./index.js");
          }
          break;
        }
        default:
          break;
        }
      }
    } else if (key == "imports") {
      simdjson::ondemand::json_type imports_type;
      if (!value.type().get(imports_type)) {
        std::string_view imports;
        switch (imports_type) {
        case simdjson::ondemand::json_type::object: {
          simdjson::ondemand::object imports_object;
          if (!value.get_object().get(imports_object) &&
              !imports_object.raw_json().get(imports)) {
            ASSERT_EQUAL(imports, R"({
    "#blocks/*": "./src/blocks/*.js",
    "#components/*": "./src/components/*.astro",
    "#layouts/*": "./src/layouts/*.astro",
    "#settings/*": "./src/settings/*.js",
    "#prefabs": "./src/prefabs/index.js",
    "#utils": "./src/utils/index.js",
    "#utils/search": "./src/utils/search.js"
  })");
          }
          break;
        }
        case simdjson::ondemand::json_type::array: {
          simdjson::ondemand::array imports_array;
          if (!value.get_array().get(imports_array) &&
              !imports_array.raw_json().get(imports)) {
            // unused
          }
          break;
        }
        case simdjson::ondemand::json_type::string: {
          if (!value.get_string().get(imports)) {
            // unused
          }
          break;
        }
        default:
          break;
        }
      }
    } else if (key == "type") {
      std::string_view type;
      if (!value.get_string().get(type) &&
          (type == "commonjs" || type == "module")) {
        ASSERT_EQUAL(type, "module");
      }
    }
  }
  TEST_SUCCEED();
}

bool run() { return thirtysecondsofcode() && baby(); }

} // namespace json_package_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, json_package_tests::run);
}
