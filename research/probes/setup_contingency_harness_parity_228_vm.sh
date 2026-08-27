#!/usr/bin/env bash
set -euo pipefail

source_root=/home/LMC/udon227-0826/candidate
root=/home/LMC/udon228-0826

if [[ -e "$root" ]]; then
    echo "refusing to overwrite existing $root" >&2
    exit 2
fi

mkdir -p "$root"
cp -a "$source_root" "$root/buggy226"
cp -a "$source_root" "$root/cache-faithful"
cp /home/LMC/m-4195.jsonl "$root/m-4195.jsonl"
cp /home/LMC/run_contingency_harness_parity_228_vm.sh "$root/run.sh"
cp /home/LMC/ATTR-CONTINGENCY-HARNESS-PARITY-228.patch "$root/fix.patch"

patch -d "$root/cache-faithful" -p1 < "$root/fix.patch"

cmake -S "$root/buggy226" -B "$root/buggy226/build228" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build "$root/buggy226/build228" --target udonshield_btc

cmake -S "$root/cache-faithful" -B "$root/cache-faithful/build228" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build "$root/cache-faithful/build228" --target udonshield_btc

sha256sum \
    "$root/m-4195.jsonl" \
    "$root/buggy226/build228/udonshield_btc" \
    "$root/cache-faithful/build228/udonshield_btc"
