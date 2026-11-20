#!/usr/bin/env bash
# clang++-strict equivalent for CI / machines without a preinstalled wrapper.
# Install as `clang++-strict` in PATH, or point CMAKE_CXX_COMPILER at this file.
set -euo pipefail

CXX="${TEMPLATE_STRICT_REAL_CXX:-clang++}"
FLAGS=(-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
       -Wnon-virtual-dtor -Woverloaded-virtual -Werror)
if [ "${STRICT_NO_WERROR:-0}" = "1" ]; then
    FILTERED=()
    for flag in "${FLAGS[@]}"; do
        [ "$flag" = "-Werror" ] || FILTERED+=("$flag")
    done
    FLAGS=("${FILTERED[@]}")
fi

# Older Clang versions reject the flag; probe before adding it.
if printf 'int main(){return 0;}\n' | \
   "$CXX" -x c++ -std=c++20 -Xclang -fexperimental-lifetime-safety \
          -fsyntax-only - >/dev/null 2>&1; then
    FLAGS+=(-Xclang -fexperimental-lifetime-safety)
fi

exec "$CXX" "${FLAGS[@]}" "$@"
