#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon232-v2-0827
binary="$root/build-release/udonshield_historical_tournament"
runner="$root/research/probes/run_proof_protected_continuation_232_v2_resume.sh"
summarizer="$root/research/probes/summarize_proof_protected_continuation_232.py"
manifest="$root/research/holdouts/SCORE-PROOF-PROTECTED-CONTINUATION-232-v2.csv"
logs="$root/logs232-v2"
source_sha256=B4985EE90EF5D0B6CE2F6919C7D60DACB29EF871E0452D3BEC5D2A461D68F1CF
binary_sha256=705B524677D2A639AE7FD7776F00607410BF307AE345DA1961AD93B5A3B23701
manifest_sha256=6EF6FFA72F5CADA334D5A0C22C845DED03C1522FC743AFCD416F44B404C40513
runner_sha256=28354D9FBE2CFD82B95E2C5A9A70E87828F25478FCF26640B8DD41523165D46B
summarizer_sha256=C535F66D605D410AD4ED72DB2D6708B948F2AF0F8B07961CBE00F90B4212EA2F

cd "$root"
check_hash() {
    local path="$1" expected="$2" label="$3"
    local actual
    actual="$(sha256sum "$path" | awk '{print toupper($1)}')"
    if [[ "$actual" != "$expected" ]]; then
        echo "$label hash mismatch: $actual != $expected" >&2
        exit 2
    fi
}
check_hash source.tar.gz "$source_sha256" source
check_hash "$binary" "$binary_sha256" binary
check_hash "$manifest" "$manifest_sha256" manifest
check_hash "$runner" "$runner_sha256" runner
check_hash "$summarizer" "$summarizer_sha256" summarizer

export BINARY_SHA256="$binary_sha256"
bash "$runner" "$binary" development-control off "$logs"
bash "$runner" "$binary" development-control on "$logs"
bash "$runner" "$binary" development off "$logs"
bash "$runner" "$binary" development on "$logs"
