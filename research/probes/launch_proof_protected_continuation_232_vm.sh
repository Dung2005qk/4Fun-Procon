#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon232-0827
binary="$root/build-release/udonshield_historical_tournament"
runner="$root/research/probes/run_proof_protected_continuation_232_resume.sh"
summarizer="$root/research/probes/summarize_proof_protected_continuation_232.py"
manifest="$root/research/holdouts/SCORE-PROOF-PROTECTED-CONTINUATION-232.csv"
logs="$root/logs232"
source_sha256=BB8BF0EAA188772FC4101CDBD064621A9B6D799EB98207994E70284952B74AB2
binary_sha256=90CF7458FD546AFD8E6B9E278EF5CFFDF60A57DE824394A7B0EB0A2361625369
manifest_sha256=42CC8EEEF031C27C85BA7168C2187AEA7A60AED3E60061213743D0497763F670
runner_sha256=12EF0D6738ED73BFD358F255334CDE36234403B53060CD7472365A451C607F5B
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
