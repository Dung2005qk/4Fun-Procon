#!/usr/bin/env bash
set -euo pipefail
cd /home/LMC/udon352-0908
test "$(hostname)" = udon-f0-240-0829
python3 -c "import sys; sys.path.insert(0,'research/probes'); from process_runtime_352 import verify_file_manifest,ROOT; verify_file_manifest(ROOT/'package352.manifest.json')"
python3 research/probes/test_portable_352.py
for side in parent candidate; do
    cmake -S "$side" -B "build-$side" -G Ninja -DCMAKE_BUILD_TYPE=Release -DUDONSHIELD_BUILD_BENCHMARKS=OFF
    cmake --build "build-$side" --parallel 2
    ctest --test-dir "build-$side" --output-on-failure
done
g++ -std=c++20 -O3 -DNDEBUG -I parent/include research/probes/protected_http_bridge_341.cpp build-parent/libudon_shield.a -o bridge352
python3 research/probes/process_runtime_352.py contracts
python3 research/probes/process_runtime_352.py freeze
python3 research/probes/process_runtime_352.py run
python3 research/probes/process_runtime_352.py summarize --check
