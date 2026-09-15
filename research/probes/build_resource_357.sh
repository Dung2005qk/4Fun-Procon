#!/usr/bin/env bash
set -euo pipefail
cd /home/LMC/udon357-0909
test "$(hostname)" = udon-f0-240-0829
python3 -c "import sys;sys.path.insert(0,'research/probes');from refinement_cost_353 import ROOT,load,sha,require;m=load(ROOT/'package357.manifest.json');[require(sha(ROOT/p)==h,'package drift '+p) for p,h in m['hashes'].items()]"
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DUDONSHIELD_BUILD_BENCHMARKS=OFF
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
g++ -std=c++20 -O3 -DNDEBUG -pthread -I source/include research/probes/resource_contract_357.cpp build/libudon_shield.a -o resource-contract357
./resource-contract357 < artifacts/research/354/input.json > unit357.json
python3 research/probes/run_resource_357.py contracts
python3 research/probes/run_resource_357.py freeze
python3 research/probes/run_resource_357.py run --phase development
python3 research/probes/run_resource_357.py summarize --phase development --check
