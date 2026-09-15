#!/usr/bin/env bash
set -euo pipefail
cd /home/LMC/udon341-0907
test "$(hostname)" = udon-f0-240-0829
test "$(sha256sum source-linux.tar | cut -d ' ' -f 1)" = 8e70ecd0eba0489c315e00655438c3cc0e2d774f6953e6864858c3b02fa8bd47
test ! -e source
mkdir source
tar -xf source-linux.tar -C source
test "$(sha256sum source/src/horizon_pricing.cpp | cut -d ' ' -f 1)" = ce67231797eb97e78ad07dfa7fc4ae72d4766b3b38690b65d9a6b5a338e7ebec
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DUDONSHIELD_BUILD_BENCHMARKS=OFF
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
g++ -std=c++20 -O3 -DNDEBUG -I source/include bounded_pair_probe_340.cpp build/libudon_shield.a -o probe
python3 pair_vm_contract_341.py run --manifest SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341-vm-contract.json --binary ./probe --source bounded_pair_probe_340.cpp --library build/libudon_shield.a --output SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341-vm-result.json
sha256sum source-linux.tar source/src/horizon_pricing.cpp build/udonshield_btc probe build/libudon_shield.a
