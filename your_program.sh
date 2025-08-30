#!/bin/sh


set -e # Exit early if any commands fail

#
# - Edit this to change how your program compiles locally
# - Edit compile.sh to change how your program compiles remotely
(
  cd "$(dirname "$0")" # Ensure compile steps are run within the repository directory
  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
  cmake --build ./build
)
exec $(dirname $0)/build/bittorrent "$@"
