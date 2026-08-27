#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon231-0827
binary="$root/build-release/udonshield_historical_tournament"
runner="$root/research/probes/run_actual_ack_transition_anchor_231_resume.sh"
logs="$root/logs231"
binary_sha256=27C72E463DFBA48E12212C4964805182A59D73AE5B8AD875415790423B41D4D5
source_sha256=CC5237AC45F11748E7691A173215A842B0F9B8D39B6E5FCAB0123438026A3E15
runner_sha256=5F982006F410D3AE888C74633BB5A7F0BDBD7D4ACB59B1E44120E8453E79894F
manifest_sha256=F7E6DB5791C2FBEB7D9F60B29525004712CB77F9C6E64D5CB8A42D975A644B04

cd "$root"
actual_binary="$(sha256sum "$binary" | awk '{print toupper($1)}')"
actual_runner="$(sha256sum "$runner" | awk '{print toupper($1)}')"
actual_manifest="$(sha256sum research/holdouts/SCORE-ACTUAL-ACK-TRANSITION-ANCHOR-231.csv | awk '{print toupper($1)}')"
if [[ "$actual_binary" != "$binary_sha256" ]]; then
    echo "binary hash mismatch: $actual_binary != $binary_sha256" >&2
    exit 2
fi
if [[ "$actual_runner" != "$runner_sha256" ]]; then
    echo "runner hash mismatch: $actual_runner != $runner_sha256" >&2
    exit 2
fi
if [[ "$actual_manifest" != "$manifest_sha256" ]]; then
    echo "manifest hash mismatch: $actual_manifest != $manifest_sha256" >&2
    exit 2
fi
if [[ "${SOURCE_SHA256:-}" != "$source_sha256" ]]; then
    echo "source archive hash is not frozen in the launch environment" >&2
    exit 2
fi

export BINARY_SHA256="$binary_sha256"
"$runner" "$binary" development off "$logs"
"$runner" "$binary" development on "$logs"
