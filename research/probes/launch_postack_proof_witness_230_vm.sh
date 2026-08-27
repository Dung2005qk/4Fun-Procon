#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon230-0826
binary="$root/build-release/udonshield_historical_tournament"
runner="$root/research/probes/run_postack_proof_witness_230_resume.sh"
logs="$root/logs230"
binary_sha256=DC1D79F7B80D50989E0155348D3C04E90BF7179AF2720DB4DA6CF94F536479BB

cd "$root"
actual="$(sha256sum "$binary" | awk '{print toupper($1)}')"
if [[ "$actual" != "$binary_sha256" ]]; then
    echo "binary hash mismatch: $actual != $binary_sha256" >&2
    exit 2
fi
export BINARY_SHA256="$binary_sha256"
"$runner" "$binary" development off "$logs"
"$runner" "$binary" development on "$logs"
