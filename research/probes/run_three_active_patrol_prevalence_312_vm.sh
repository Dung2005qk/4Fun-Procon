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
    echo "another 312 runner already owns $lock_dir" >&2
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
        test "$(grep -c "^case,seed=$seed," "$result")" -eq 1
        test "$(grep -c "^case_complete,seed=$seed$" "$result")" -eq 1
        return
    fi
    if [[ -e "$partial" || -e "$stderr_partial" ]]; then
        echo "ambiguous partial evidence exists for seed $seed" >&2
        exit 74
    fi
    set +e
    "$binary" --manifest "$manifest" --split development --only-seed "$seed" \
        >"$partial" 2>"$stderr_partial"
    local status=$?
    set -e
    if [[ "$status" -ne 0 ]]; then
        [[ -s "$stderr_partial" ]] && mv "$stderr_partial" "$stderr_final"
        echo "oracle seed $seed exited with code $status" >&2
        exit "$status"
    fi
    test "$(grep -c "^case,seed=$seed," "$partial")" -eq 1
    test "$(grep -c '^summary,' "$partial")" -eq 1
    printf 'case_complete,seed=%s\n' "$seed" >>"$partial"
    if [[ -s "$stderr_partial" ]]; then
        mv "$stderr_partial" "$stderr_final"
        echo "nonempty stderr for seed $seed" >&2
        exit 75
    fi
    rm -f "$stderr_partial" "$stderr_final"
    mv "$partial" "$result"
}

for seed in "${seeds[@]}"; do
    run_seed "$seed"
done

combined_partial="$output_dir/development.log.partial"
combined="$output_dir/development.log"
if [[ -e "$combined_partial" ]]; then
    echo "ambiguous combined partial evidence exists" >&2
    exit 76
fi
: >"$combined_partial"
for seed in "${seeds[@]}"; do
    test -f "$output_dir/$seed.result"
    grep "^case,seed=$seed," "$output_dir/$seed.result" >>"$combined_partial"
    grep "^case_complete,seed=$seed$" "$output_dir/$seed.result" >>"$combined_partial"
done
test "$(grep -c '^case,' "$combined_partial")" -eq 12
test "$(grep -c '^case_complete,' "$combined_partial")" -eq 12
printf 'run_complete,cases=12\n' >>"$combined_partial"
mv "$combined_partial" "$combined"
