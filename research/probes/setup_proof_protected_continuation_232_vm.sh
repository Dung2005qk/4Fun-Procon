#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon232-0827
source_sha256=BB8BF0EAA188772FC4101CDBD064621A9B6D799EB98207994E70284952B74AB2
cd "$root"
actual_source="$(sha256sum source.tar.gz | awk '{print toupper($1)}')"
if [[ "$actual_source" != "$source_sha256" ]]; then
    echo "source hash mismatch: $actual_source != $source_sha256" >&2
    exit 2
fi
tar -xzf source.tar.gz
cmake_bin=/home/LMC/.local/bin/cmake
"$cmake_bin" -S . -B build-release -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DUDONSHIELD_BUILD_BENCHMARKS=ON
"$cmake_bin" --build build-release \
    --target udonshield_tests udonshield_historical_tournament \
    -j4
./build-release/udonshield_tests
sha256sum build-release/udonshield_historical_tournament
