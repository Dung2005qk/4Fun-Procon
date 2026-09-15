"""Fresh selected-W1 pair prevalence. Synthetic HTTP, never official BTC.

Run all canonical baselines before reading attribution; keep all probe outputs
out of the HTTP session. Memory waits are case-boundary exits, not busy polling.
"""
import argparse
from collections import Counter
import copy
import ctypes
import json
from pathlib import Path

from http_prefix_option_loss_321 import ROOT, load, digest, object_hash, require, write_new, close_bridge
from run_http_baseline_314 import Bridge, run_case
from summarize_http_baseline_314 import safety
from selected_horizon_pricing_331 import FAMILIES, selected_context, validate_response
from pair_resource_response_336 import validate as validate_pair
from pool_boundary_value_327 import verify_local
from day2_pool_value_323 import comparison

ID = "ATTR-W1-PAIR-PRICING-PREVALENCE-337"
M = ROOT / f"research/holdouts/{ID}.json"
D = ROOT / f"research/evidence/{ID}"
S = ROOT / f"research/evidence/{ID}.summary.json"
BASE = 20260907337000
COUNT = 24
MIN_FREE = 1024 * 1024 * 1024
SUFFIXES = (".result.json", ".replay.jsonl", ".transport.json", ".replay-check.txt", ".stdout", ".stderr")


class MemoryWait(RuntimeError):
    pass


def memory_guard():
    class Status(ctypes.Structure):
        _fields_ = [("length", ctypes.c_ulong), ("load", ctypes.c_ulong)] + [
            (name, ctypes.c_ulonglong) for name in
            ("total", "available", "page_total", "page_available", "virtual_total", "virtual_available", "extended")]
    status = Status(); status.length = ctypes.sizeof(status)
    require(ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)), "memory status unavailable")
    if status.available < MIN_FREE:
        raise MemoryWait(f"case-boundary memory wait: available_bytes={status.available} minimum={MIN_FREE}")


def gameplay_hash(setup):
    q = copy.deepcopy(setup)
    for key in ("startsAt", "players"):
        q.pop(key, None)
    q["spots"] = sorted(q["spots"], key=lambda x: x["pos"])
    return object_hash(q)


def assert_fresh(setups, consumed):
    hashes = [gameplay_hash(q) for q in setups]
    require(len(hashes) == len(set(hashes)), "duplicate fresh gameplay setup")
    require(not (set(hashes) & {gameplay_hash(q) for q in consumed}), "consumed gameplay setup reused")
    return hashes


def freeze():
    p336 = ROOT / "research/holdouts/ATTR-W1-PAIR-RESOURCE-RESPONSE-336.json"
    s336 = ROOT / "research/evidence/ATTR-W1-PAIR-RESOURCE-RESPONSE-336.summary.json"
    require(digest(p336) == "15D3C19EADC348818298EEB2F7A8486B57EA7099A1CAFB7A75BACB07C8AC6B89", "336 manifest")
    require(digest(s336) == "DB8127F31E4E5167D3177B9908FD62F6AF6409CC50D8743DB4C640C98E8C32B2", "336 summary")
    prior = load(p336); verify_local(prior)
    require(load(s336)["gate_passed"], "336 gate")
    p314 = ROOT / "research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json"
    p331 = ROOT / "research/holdouts/ATTR-W1-HORIZON-PRICING-PREVALENCE-331.json"
    p332dev = ROOT / "research/holdouts/SCORE-W1-HORIZON-RESOURCE-PRICING-332-development.json"
    old = load(p314)
    for p, h in old["hashes"].items(): require(digest(ROOT/p) == h, "314 dependency drift: " + p)
    verify_local(load(p331))
    require(digest(p332dev) == "A918A3AFF74C80CB2A44BB873F2845EC48A7600E9BBA8A3EF9A76A886D09F923", "consumed332dev drift")
    consumed = old["setups"] + load(p331)["setups"] + [c["setup"] for c in load(p332dev)["cases"]]
    specs = [{"seed": BASE + 100*f + r, "family": family, "players": 8+(f+r)%3}
             for f, family in enumerate(FAMILIES) for r in range(4)]
    bridge = Bridge(ROOT/old["bridge_binary"])
    try:
        setups = []
        for spec in specs:
            q = bridge.request({"op": "fixture", **spec})
            require(q.get("ok") and q["setup"]["daySeconds"] == [5]*4, "fixture contract")
            setups.append(q["setup"])
    finally: close_bridge(bridge)
    setup_hashes = assert_fresh(setups, consumed)
    paths = {ROOT/p for p in prior["local_hashes"]} | {ROOT/p for p in old["hashes"]} | {
        p336, s336, p314, p331, p332dev, Path(__file__),
        ROOT/"research/probes/selected_horizon_pricing_331.py", ROOT/"research/probes/test_pair_pricing_prevalence_337.py"}
    write_new(M, {"experiment": ID, "parent": "c76a8ea", "cases": specs, "setups": setups,
        "gameplay_hashes": setup_hashes, "consumed_gameplay_hashes": sorted({gameplay_hash(q) for q in consumed}),
        "local_hashes": {str(p.relative_to(ROOT)).replace("\\", "/"): digest(p) for p in sorted(paths)},
        "btc_binary": old["btc_binary"], "bridge_binary": old["bridge_binary"],
        "single_probe": "artifacts/research/330/probe.exe", "pair_probe": "artifacts/research/336/probe.exe",
        "memo_limit": 250000, "transition_limit": 30000000, "minimum_free_bytes": MIN_FREE,
        "expected_cases": COUNT, "single_responses": 72, "pair_responses": 72, "replacement_dual_days": 432,
        "gate": "All24baselines96ACK72transitions then72single72pair432dual-days zero failure. Pair best strictly above single best on4roots2families. Separate fresh bounded SCORE design ONLY.",
        "production_change": False, "holdout_authority": False, "scope": "Fixed roadless3Patrol tiny corridor four days; no opponents, no traffic or performance authority."})
    print("manifest_sha256=" + digest(M), flush=True)


def validate_baseline_result(spec, directory):
    prefix = directory / str(spec["seed"])
    r = load(prefix.with_suffix(".result.json"))
    require(all(r[k] == v for k, v in spec.items()) and r["actions"] == 4 and
            r["transitions"] == 3 and r["failure"] is None, "baseline identity/lifecycle")
    for suffix, key in ((".replay.jsonl", "replay_sha256"), (".transport.json", "transport_sha256"),
                        (".replay-check.txt", "replay_check_sha256")):
        require(digest(prefix.with_suffix(suffix)) == r[key], "baseline artifact drift")
    require(prefix.with_suffix(".stderr").stat().st_size == 0, "baseline stderr")
    safety(prefix.with_suffix(".replay.jsonl"))


def baseline_inventory(m, directory):
    """A complete contiguous prefix only; any unfinished HTTP artifact blocks resume."""
    allowed = {str(c["seed"])+s for c in m["cases"] for s in SUFFIXES} | {"run_complete.json"}
    require(all(p.name in allowed and p.is_file() for p in directory.iterdir()), "unknown baseline evidence")
    completed = []; missing_seen = False
    for c in m["cases"]:
        files = {p.name for p in directory.glob(str(c["seed"])+".*")}
        result = str(c["seed"])+".result.json"
        if result in files:
            require(not missing_seen and files == {str(c["seed"])+s for s in SUFFIXES}, "noncontiguous/incomplete companion set")
            validate_baseline_result(c, directory); completed.append(c["seed"])
        else:
            require(not files, "ambiguous partial HTTP case; do not duplicate accepted days")
            missing_seen = True
    return completed


def complete_baseline(m):
    directory = D/"baseline"; done = baseline_inventory(m, directory); mark = load(directory/"run_complete.json")
    expected = {str(c["seed"])+".result.json" for c in m["cases"]}
    require(len(done) == COUNT and mark["cases"] == COUNT and mark["manifest_sha256"] == digest(M) and
            set(mark["result_hashes"]) == expected, "baseline completion")
    require(all(digest(directory/p) == h for p, h in mark["result_hashes"].items()), "baseline result drift")
    return mark


def baseline(m):
    directory = D/"baseline"; directory.mkdir(exist_ok=True)
    done = set(baseline_inventory(m, directory))
    if (directory/"run_complete.json").exists(): complete_baseline(m); return
    bridge = Bridge(ROOT/m["bridge_binary"])
    try:
        for spec, setup in zip(m["cases"], m["setups"], strict=True):
            if spec["seed"] in done: continue
            memory_guard()
            q = bridge.request({"op": "fixture", **spec})
            require(q.get("ok") and q["setup"] == setup, "frozen fixture identity")
            run_case(spec, setup, m, directory, bridge)
            print(f"case_complete baseline_count={len(done)+1}", flush=True); done.add(spec["seed"])
    finally: close_bridge(bridge)
    verify_local(m)
    write_new(directory/"run_complete.json", {"cases": COUNT, "manifest_sha256": digest(M),
        "result_hashes": {p.name: digest(p) for p in sorted(directory.glob("*.result.json"))}})


def context_for(spec, setup, bridge):
    replay = D/"baseline"/(str(spec["seed"])+".replay.jsonl")
    body = next(e["body"] for e in map(json.loads, replay.read_text().splitlines()) if e["kind"] == "decision")
    return selected_context(body, setup, bridge), digest(replay)


def validate_both(single, pair, context, bridge):
    dual = validate_response(single, context, bridge)
    control = {"request": context["request"], "baseline": context["baseline"],
        "exact_value": context["valid_upper_bound"],  # sound bound, NOT an oracle value
        "fixed_states": [d["agents"] for d in context["original_days"]]}
    dual += validate_pair(pair, control, bridge, single["responses"])
    return dual


def attribute(m):
    complete_baseline(m)
    directory = D/"attribution"; directory.mkdir(exist_ok=True)
    expected = {str(c["seed"])+".result.json" for c in m["cases"]}
    require(all(p.is_file() and p.name in expected | {"run_complete.json"} for p in directory.iterdir()), "partial/unknown attribution evidence")
    if (directory/"run_complete.json").exists(): return
    bridge = Bridge(ROOT/m["bridge_binary"]); single = Bridge(ROOT/m["single_probe"]); pair = Bridge(ROOT/m["pair_probe"])
    dual = 0
    try:
        for i, (spec, setup) in enumerate(zip(m["cases"], m["setups"], strict=True)):
            path = directory/(str(spec["seed"])+".result.json")
            memory_guard(); context, replay_hash = context_for(spec, setup, bridge)
            if path.exists():
                row = load(path)
                require(row["spec"] == spec and row["context"] == context and row["replay_sha256"] == replay_hash and
                        row["manifest_sha256"] == digest(M), "attribution resume identity")
                a, b = row["single"], row["pair"]
            else:
                before = object_hash(context["request"])
                a = single.request(context["request"])
                b = pair.request({**context["request"], "memo_limit": m["memo_limit"], "transition_limit": m["transition_limit"]})
                require(object_hash(context["request"]) == before, "probe input mutated")
                if not all(o.get("ok") and o.get("complete") for o in (a, b)):
                    write_new(directory/(str(spec["seed"])+".incomplete.json"), {"spec": spec, "single": a, "pair": b})
                    raise RuntimeError("incomplete capability; preserve evidence, never raise caps")
                validate_both(a, b, context, bridge)
                write_new(path, {"spec": spec, "context": context, "single": a, "pair": b,
                    "replay_sha256": replay_hash, "manifest_sha256": digest(M)})
            dual += validate_both(a, b, context, bridge)
            print(f"case_complete attribution_count={i+1}", flush=True)
    finally:
        for b in (bridge, single, pair): close_bridge(b)
    require(dual == 432, "attribution dual coverage"); verify_local(m)
    write_new(directory/"run_complete.json", {"cases": COUNT, "single_responses": 72, "pair_responses": 72,
        "dual_days": dual, "manifest_sha256": digest(M), "baseline_complete_sha256": digest(D/"baseline/run_complete.json"),
        "result_hashes": {p.name: digest(p) for p in sorted(directory.glob("*.result.json"))}})


def summarize(m):
    complete_baseline(m); directory = D/"attribution"; mark = load(directory/"run_complete.json")
    expected = {str(c["seed"])+".result.json" for c in m["cases"]}
    require(mark["cases"] == COUNT and mark["single_responses"] == mark["pair_responses"] == 72 and
            mark["dual_days"] == 432 and mark["manifest_sha256"] == digest(M) and
            mark["baseline_complete_sha256"] == digest(D/"baseline/run_complete.json"), "attribution completion")
    require(set(mark["result_hashes"]) == expected == {p.name for p in directory.glob("*.result.json")} and
            not list(directory.glob("*.incomplete.json")), "attribution inventory")
    rows = []; bridge = Bridge(ROOT/m["bridge_binary"])
    try:
        for spec, setup in zip(m["cases"], m["setups"], strict=True):
            path = directory/(str(spec["seed"])+".result.json"); row = load(path)
            require(digest(path) == mark["result_hashes"][path.name] and row["spec"] == spec and
                    row["manifest_sha256"] == digest(M), "result provenance")
            context, replay_hash = context_for(spec, setup, bridge)
            require(row["context"] == context and row["replay_sha256"] == replay_hash, "context provenance")
            validate_both(row["single"], row["pair"], context, bridge)
            single = max((r["score"] for r in row["single"]["responses"]), key=tuple)
            pair = max((r["score"] for r in row["pair"]["responses"]), key=tuple)
            rows.append({**spec, "fuel": setup["fuelLimits"], "steps": str(setup["daySteps"]),
                "stocks": str([s["stocks"] for s in setup["spots"]]), "brands": len({s["brand"] for s in setup["spots"]}),
                "original": context["baseline"], "single": single, "pair": pair,
                "versus_single": comparison(pair, single), "versus_original": comparison(pair, context["baseline"]),
                "single_work": [{k: r[k] for k in ("agent", "memo_states", "day_enumerations", "transitions")} for r in row["single"]["responses"]],
                "pair_work": [{k: r[k] for k in ("pair", "memo_states", "day_enumerations", "transitions")} for r in row["pair"]["responses"]],
                "result_sha256": digest(path), "replay_sha256": replay_hash})
    finally: close_bridge(bridge)
    wins = [r for r in rows if r["versus_single"]["result"] == "win"]
    report = {"experiment": ID, "complete": True, "cases": COUNT, "valid_ack": 96, "transitions": 72,
        "single_responses": 72, "pair_responses": 72, "replacement_dual_days": 432, "original_dual_days": 72,
        "zero_failure": True, "gate_passed": len(wins) >= 4 and len({r["family"] for r in wins}) >= 2,
        "versus_single": dict(Counter(r["versus_single"]["result"] for r in rows)),
        "versus_original": dict(Counter(r["versus_original"]["result"] for r in rows)),
        "first_tiers": dict(Counter(str(r["versus_single"]["tier"]) for r in rows)),
        "gain_loss_tails": sorted((r["versus_single"] for r in rows), key=lambda x: tuple(x["components"])),
        "strata": {k: {str(v): dict(Counter(r["versus_single"]["result"] for r in rows if r[k] == v))
                       for v in sorted({r[k] for r in rows})} for k in ("family", "players", "fuel", "steps", "stocks", "brands")},
        "rows": rows, "manifest_sha256": digest(M), "run_complete_sha256": digest(directory/"run_complete.json"),
        "result_hashes": mark["result_hashes"], "production_change": False, "score_promotion_authority": False,
        "limits": "Selected certificate prevalence only; all original matches unchanged. Tiny roadless fixed3Patrol; player labels do not validate traffic. No timing/performance/BTC authority."}
    verify_local(m); write_new(S, report)
    print(json.dumps({k: report[k] for k in ("complete", "cases", "versus_single", "versus_original", "gate_passed", "zero_failure")}))
    print("summary_sha256=" + digest(S), flush=True)


def execute(resume=False):
    m = load(M); verify_local(m)
    require(m["minimum_free_bytes"] == MIN_FREE and m["expected_cases"] == COUNT, "runtime policy drift")
    if S.exists(): raise RuntimeError("already summarized; never rerun")
    if D.exists(): require(resume, "existing evidence requires verified --resume")
    memory_guard(); D.mkdir(exist_ok=resume)
    baseline(m); attribute(m); summarize(m)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(); parser.add_argument("mode", choices=("freeze", "execute", "summarize")); parser.add_argument("--resume", action="store_true")
    args = parser.parse_args()
    try:
        if args.mode == "freeze": freeze()
        elif args.mode == "execute": execute(args.resume)
        else: summarize(load(M))
    except MemoryWait as error:
        print(str(error), flush=True)
        raise SystemExit(75)
