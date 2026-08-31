#!/usr/bin/env bash
# Builds build/ (Release) and/or build-debug/ (Debug, ASan+UBSan) with
# Homebrew LLVM. Usage: ./build.sh [release|debug|both]
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

CXX=/opt/homebrew/opt/llvm/bin/clang++

case "${1:-both}" in
  release) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$CXX" && cmake --build build -j ;;
  debug) cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="$CXX" && cmake --build build-debug -j ;;
  both)
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$CXX" && cmake --build build -j
    cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="$CXX" && cmake --build build-debug -j
    ;;
  *) echo "usage: $0 [release|debug|both]" >&2; exit 1 ;;
esac
