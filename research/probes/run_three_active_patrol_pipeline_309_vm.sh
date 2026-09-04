#!/usr/bin/env bash
set -euo pipefail

binary=${BINARY:?BINARY is required}
manifest=${MANIFEST:?MANIFEST is required}
source_file=${SOURCE_FILE:?SOURCE_FILE is required}
output_dir=${OUTPUT_DIR:?OUTPUT_DIR is required}
expected_binary=${BINARY_SHA256:?BINARY_SHA256 is required}
expected_manifest=${MANIFEST_SHA256:?MANIFEST_SHA256 is required}
expected_source=${SOURCE_SHA256:?SOURCE_SHA256 is required}

test "$(sha256sum "$binary" | awk '{print toupper($1)}')" = "$expected_binary"
test "$(sha256sum "$manifest" | awk '{print toupper($1)}')" = "$expected_manifest"
test "$(sha256sum "$source_file" | awk '{print toupper($1)}')" = "$expected_source"

mkdir -p "$output_dir"
lock_dir="$output_dir/.runner.lock"
if ! mkdir "$lock_dir"; then
    echo "another 309 runner already owns $lock_dir" >&2
    exit 73
fi
trap 'rmdir "$lock_dir"' EXIT
printf '%s\n' "$$" >"$output_dir/runner.pid"

run_case() {
    local seed=$1
    local oracle=$2
    local parent=$3
    local partial="$output_dir/$seed.partial"
    local result="$output_dir/$seed.result"
    local stderr_partial="$output_dir/$seed.stderr.partial"
    local stderr_final="$output_dir/$seed.stderr"
    if [[ -f "$result" ]]; then
        return
    fi
    rm -f "$partial" "$stderr_partial"
    "$binary" \
        --manifest "$manifest" \
        --split development \
        --only-seed "$seed" \
        --inspect-plan "$oracle" \
        --inspect-parent-plan "$parent" \
        >"$partial" 2>"$stderr_partial"
    test "$(grep -c '^current_exact_fuel_attribute,' "$partial")" -eq 1
    test "$(grep -c '^f0_upper_attribute,' "$partial")" -eq 1
    test "$(grep -c '^profile_attribute,' "$partial")" -eq 1
    test "$(grep -c '^attribute,' "$partial")" -eq 1
    if [[ -s "$stderr_partial" ]]; then
        mv "$stderr_partial" "$stderr_final"
    else
        rm -f "$stderr_partial" "$stderr_final"
    fi
    mv "$partial" "$result"
}

run_case 10500000 '2.-16|5.5.5.5.5.-8|5.5.5.5.5.-8' \
    '2.2.2.-12|5.5.-14|5.5.5.5.5.5.-6' &
run_case 10500001 '2.-14|5.5.5.5.5.-6|5.5.5.5.5.-6' \
    '2.2.2.-10|5.5.-12|5.5.5.5.5.5.-4' &
run_case 10500100 '2.-15|5.5.5.5.5.-7|5.5.5.5.5.-7' \
    '2.2.2.2.2.-7|5.5.5.5.5.-7|-17' &
run_case 10500101 '2.-16|5.5.5.5.5.-8|5.5.5.5.5.-8' \
    '2.2.2.2.2.-8|5.5.5.5.5.-8|-18' &
wait

combined_partial="$output_dir/development.log.partial"
combined="$output_dir/development.log"
: >"$combined_partial"
for seed in 10500000 10500001 10500100 10500101; do
    test -f "$output_dir/$seed.result"
    cat "$output_dir/$seed.result" >>"$combined_partial"
done
test "$(grep -c '^current_exact_fuel_attribute,' "$combined_partial")" -eq 4
test "$(grep -c '^f0_upper_attribute,' "$combined_partial")" -eq 4
test "$(grep -c '^profile_attribute,' "$combined_partial")" -eq 4
test "$(grep -c '^attribute,' "$combined_partial")" -eq 4
printf 'run_complete,cases=4\n' >>"$combined_partial"
mv "$combined_partial" "$combined"

