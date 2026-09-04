#!/usr/bin/env bash
set -euo pipefail

binary=${BINARY:?BINARY is required}
manifest=${MANIFEST:?MANIFEST is required}
source_file=${SOURCE_FILE:?SOURCE_FILE is required}
output_dir=${OUTPUT_DIR:?OUTPUT_DIR is required}
expected_binary=${BINARY_SHA256:?BINARY_SHA256 is required}
expected_manifest=${MANIFEST_SHA256:?MANIFEST_SHA256 is required}
expected_source=${SOURCE_SHA256:?SOURCE_SHA256 is required}
parallelism=${PARALLELISM:-3}

test "$(sha256sum "$binary" | awk '{print toupper($1)}')" = "$expected_binary"
test "$(sha256sum "$manifest" | awk '{print toupper($1)}')" = "$expected_manifest"
test "$(sha256sum "$source_file" | awk '{print toupper($1)}')" = "$expected_source"
test "$parallelism" -ge 1

mkdir -p "$output_dir"
lock_dir="$output_dir/.runner.lock"
if ! mkdir "$lock_dir"; then
    echo "another 306 runner already owns $lock_dir" >&2
    exit 73
fi
trap 'rmdir "$lock_dir"' EXIT
printf '%s\n' "$$" >"$output_dir/runner.pid"
mapfile -t seeds < <(
    awk -F, '$2 == "development" { for (i = 0; i < $8; ++i) print $7 + i }' "$manifest"
)
test "${#seeds[@]}" -eq 12

run_seed() {
    local seed=$1
    local result="$output_dir/$seed.result"
    local partial="$output_dir/$seed.partial"
    local stderr_partial="$output_dir/$seed.stderr.partial"
    local stderr_final="$output_dir/$seed.stderr"
    if [[ -f "$result" ]]; then
        return
    fi
    rm -f "$partial" "$stderr_partial"
    "$binary" --manifest "$manifest" --split development --only-seed "$seed" \
        >"$partial" 2>"$stderr_partial"
    test "$(grep -c '^case,' "$partial")" -eq 1
    test "$(grep -c '^summary,' "$partial")" -eq 1
    if [[ -s "$stderr_partial" ]]; then
        mv "$stderr_partial" "$stderr_final"
    else
        rm -f "$stderr_partial" "$stderr_final"
    fi
    mv "$partial" "$result"
}

for seed in "${seeds[@]}"; do
    run_seed "$seed" &
    while [[ "$(jobs -rp | wc -l)" -ge "$parallelism" ]]; do
        wait -n
    done
done
wait

combined_partial="$output_dir/development.log.partial"
combined="$output_dir/development.log"
: >"$combined_partial"
for seed in "${seeds[@]}"; do
    test -f "$output_dir/$seed.result"
    grep '^case,' "$output_dir/$seed.result" >>"$combined_partial"
done
test "$(grep -c '^case,' "$combined_partial")" -eq 12
printf 'run_complete,cases=12\n' >>"$combined_partial"
mv "$combined_partial" "$combined"
