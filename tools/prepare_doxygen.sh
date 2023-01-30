#!/bin/sh
set -e

PACKAGE_URL="https://github.com/jothepro/doxygen-awesome-css.git"
PACKAGE_VERSION="v2.1.0"

BASE_DIR=$(pwd)
THEME_DIR="$BASE_DIR/theme"
WORKSPACE=$(mktemp -d 2> /dev/null || mktemp -d -t 'tmp')
cleanup () {
  EXIT_CODE=$?
  [ -d "$WORKSPACE" ] && rm -rf "$WORKSPACE"
  exit $EXIT_CODE
}

trap cleanup INT TERM EXIT

cd "$WORKSPACE"
git clone --depth=1 --branch "$PACKAGE_VERSION" "$PACKAGE_URL" theme
rm -rf "$THEME_DIR"
mv "$WORKSPACE/theme" "$BASE_DIR"
