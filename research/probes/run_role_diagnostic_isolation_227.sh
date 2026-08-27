#!/usr/bin/env bash
set -euo pipefail

root=${1:?usage: run_role_diagnostic_isolation_227.sh ROOT}
manifest="$root/PERF-ROLE-DIAGNOSTIC-ISOLATION-227-v2.csv"
logs="$root/logs227"
mkdir -p "$logs"

expected_manifest_sha=d526d86c5a9d9762b1fe873a2421bd5a51146edc627f3b30e775128abddf1a8b
actual_manifest_sha=$(sha256sum "$manifest" | awk '{print $1}')
if [[ "$actual_manifest_sha" != "$expected_manifest_sha" ]]; then
    echo "manifest hash mismatch: $actual_manifest_sha" >&2
    exit 2
fi

run_case() {
    local binary=$1 label=$2 suite=$3 seed=$4 budget_ms=$5 role_ms=$6
    local role_mode=$7 fuel_profile=$8 spot_count=$9
    local output="$logs/${label}-${suite}-${seed}.result"
    local partial="$logs/${label}-${suite}-${seed}.partial"

    if [[ -f "$output" ]]; then
        [[ $(grep -c '^result,' "$output") -eq 1 ]] || {
            echo "invalid completed case: $output" >&2
            exit 3
        }
        return
    fi
    if [[ -f "$partial" ]]; then
        mv "$partial" "$partial.aborted.$(date -u +%Y%m%dT%H%M%SZ)"
    fi

    "$binary" \
        --version "$label" \
        --track role-diagnostic-isolation \
        --suite "$suite" \
        --first-seed "$seed" \
        --seeds 1 \
        --budget-ms "$budget_ms" \
        --role-ms "$role_ms" \
        --role-mode "$role_mode" \
        --fuel-profile "$fuel_profile" \
        --spot-count "$spot_count" \
        --short-role-fallback 1 >"$partial"

    [[ $(grep -c '^result,' "$partial") -eq 1 ]] || {
        echo "case produced no unique result: $label/$suite/$seed" >&2
        exit 4
    }
    mv "$partial" "$output"
}

run_side() {
    local label=$1 binary=$2
    [[ -x "$binary" ]] || { echo "missing binary: $binary" >&2; exit 5; }

    while IFS=, read -r phase suite first_seed seed_count budget_ms role_ms role_mode fuel_profile spot_count purpose; do
        [[ "$phase" == "holdout" ]] || continue
        local last_seed=$((first_seed + seed_count - 1))
        for seed in $(seq "$first_seed" "$last_seed"); do
            run_case "$binary" "$label" "$suite" "$seed" "$budget_ms" "$role_ms" \
                "$role_mode" "$fuel_profile" "$spot_count"
        done
    done < <(tail -n +2 "$manifest" | tr -d '\r')

    local completed
    completed=$(find "$logs" -maxdepth 1 -type f -name "${label}-*.result" | wc -l)
    [[ "$completed" -eq 40 ]] || {
        echo "$label completed $completed/40" >&2
        exit 6
    }
    date -u +%Y-%m-%dT%H:%M:%SZ >"$logs/${label}.run_complete"
}

run_side candidate-227 "$root/candidate/historical_tournament"
run_side e8bf766 "$root/e8bf766/historical_tournament"
run_side baebad8 "$root/baebad8/historical_tournament"
date -u +%Y-%m-%dT%H:%M:%SZ >"$logs/all.run_complete"
