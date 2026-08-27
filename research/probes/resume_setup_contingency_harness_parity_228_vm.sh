#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon228-0826
cmake_bin=/home/LMC/.local/lib/python3.10/site-packages/cmake/data/bin/cmake

test -f "$root/buggy226/src/btc_main.cpp"
test -f "$root/cache-faithful/src/btc_main.cpp"
test -f "$root/m-4195.jsonl"

"$cmake_bin" -S "$root/buggy226" -B "$root/buggy226/build228" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
"$cmake_bin" --build "$root/buggy226/build228" --target udonshield_btc

"$cmake_bin" -S "$root/cache-faithful" -B "$root/cache-faithful/build228" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
"$cmake_bin" --build "$root/cache-faithful/build228" --target udonshield_btc

sha256sum \
    "$root/m-4195.jsonl" \
    "$root/buggy226/build228/udonshield_btc" \
    "$root/cache-faithful/build228/udonshield_btc"
