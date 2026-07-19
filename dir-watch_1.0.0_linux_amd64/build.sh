#!/usr/bin/env bash

set -Eeuo pipefail

readonly ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly PACKAGE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly DEBROOT="$PACKAGE_DIR/root"
readonly PACKAGE_NAME="dir-watch"
readonly VERSION="1.0.0"
readonly ARCH="amd64"

cd "$ROOT_DIR"

mkdir -p "$DEBROOT/usr/local/bin"

g++ -std=c++17 -O2 main.cpp -o "$DEBROOT/usr/local/bin/dir-watch-bin"
ln -sfn dir-watch-bin "$DEBROOT/usr/local/bin/dir-watch"

dpkg-deb --root-owner-group --build "$DEBROOT" "${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
