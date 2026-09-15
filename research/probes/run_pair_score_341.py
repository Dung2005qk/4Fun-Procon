"""Frozen actual HTTP paired screen; no partial-score interpretation or holdout run."""
import argparse
from collections import Counter, defaultdict
import ctypes
import json
from pathlib import Path
from run_http_baseline_314 import ROOT, Bridge, digest, run_case
from summarize_http_baseline_314 import safety, compare, require

ID = "SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341"
M = ROOT / f"research/holdouts/{ID}-execution.json"
D = ROOT / f"research/evidence/{ID}-development"
S = ROOT / f"research/evidence/{ID}-development.summary.json"
LIMITS = {"settledLabels": 321024, "createdLabels": 196609, "storedActions": 262144,
          "memoStates": 8192, "transitions": 262144, "dayQueries": 1024}
FIELDS = ("calls", "supported", "completed", "improvements", "exhausted", "failures", *LIMITS)


def load(p): return json.loads(Path(p).read_text(encoding="utf-8"))


def write_new(p, value):
    with Path(p).open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, sort_keys=True); stream.write("\n")


def verify(m):
    for p, h in m["hashes"].items(): require(digest(ROOT/p) == h, "frozen drift: " + p)


def memory_available():
    class Memory(ctypes.Structure):
        _fields_ = [("length", ctypes.c_ulong), ("load", ctypes.c_ulong),
                    *[(n, ctypes.c_ulonglong) for n in ("totalPhys", "availPhys", "totalPage", "availPage",
                                                        "totalVirtual", "availVirtual", "extended")]]
    m = Memory(); m.length = ctypes.sizeof(m)
    require(ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m)), "memory query failed")
    return m.availPhys


def check_stats(d):
    require(set(d) == set(FIELDS) and all(type(v) is int and v >= 0 for v in d.values()), "pricing counters missing/invalid")
    require(d["improvements"] <= d["completed"] <= d["supported"] <= d["calls"], "pricing count ordering")
    require(d["completed"] + d["exhausted"] + d["failures"] == d["supported"], "unfinished pricing outcome")
    require(d["failures"] == 0, "pricing validity/score/fixed-state failure")
    for k, cap in LIMITS.items(): require(d[k] <= cap*d["supported"], "pricing bound exceeded: " + k)


def decisions(replay):
    return [e["body"]["decision"] for e in
            (json.loads(line) for line in Path(replay).read_text().splitlines()) if e["kind"] == "decision"]


def pricing_safety(replay):
    for day in decisions(replay):
        check_stats(day["profile"]["horizonPricing"])
        for candidate in day["audit"]["candidates"]: check_stats(candidate["horizonPricing"])


def validate_side(c, side, directory=D):
    prefix = directory/side/str(c["spec"]["seed"])
    row = load(prefix.with_suffix(".result.json"))
    require(row["kind"] == "case_complete" and row["failure"] is None and row["actions"] == 4 and row["transitions"] == 3, "incomplete result")
    require(all(row[k] == v for k, v in c["spec"].items()), "wrong case identity")
    for suffix, key in ((".replay.jsonl", "replay_sha256"), (".transport.json", "transport_sha256"),
                        (".replay-check.txt", "replay_check_sha256")):
        require(digest(prefix.with_suffix(suffix)) == row[key], "result hash mismatch")
    require(prefix.with_suffix(".stderr").stat().st_size == 0, "child stderr")
    safety(prefix.with_suffix(".replay.jsonl"))
    if side == "candidate": pricing_safety(prefix.with_suffix(".replay.jsonl"))
    return row


def run(resume=False):
    m = load(M); verify(m)
    cases = load(ROOT/m["development"])["cases"]
    require(len(cases) == 24, "wrong development coverage")
    if D.exists():
        require(resume and not (D/"run_complete.json").exists(), "existing run; no blind duplicate")
    else:
        D.mkdir(); (D/"parent").mkdir(); (D/"candidate").mkdir()
    expected = {str(c["spec"]["seed"]) for c in cases}
    for side in ("parent", "candidate"):
        require(all(p.name.split(".")[0] in expected for p in (D/side).iterdir()), "foreign case evidence")
        for c in cases:
            prefix = D/side/str(c["spec"]["seed"])
            if prefix.with_suffix(".result.json").exists(): validate_side(c, side)
            else: require(not list((D/side).glob(prefix.name+".*")), "ambiguous partial case; recovery needs audit")
    bridge = Bridge(ROOT/m["bridge_binary"])
    try:
        for i, c in enumerate(cases):
            for side in c["order"]:
                prefix = D/side/str(c["spec"]["seed"])
                if prefix.with_suffix(".result.json").exists(): continue
                require(memory_available() >= 1024*1024*1024, "available RAM below frozen 1024MiB safety floor")
                run_case(c["spec"], c["setup"], {"experiment": ID, "btc_binary": m[side+"_binary"]}, D/side, bridge)
                validate_side(c, side)  # Operational fields only, never compare partial scores.
                print(f"side_complete pair={i+1} side={side}", flush=True)
            marker = D/(str(c["spec"]["seed"])+".pair_complete.json")
            hashes = {side: digest((D/side/str(c["spec"]["seed"])).with_suffix(".result.json")) for side in c["order"]}
            row = {"seed": c["spec"]["seed"], "order": c["order"], "results": hashes}
            if marker.exists(): require(load(marker) == row, "pair marker mismatch")
            else: write_new(marker, row)
            print(f"pair_complete count={i+1}", flush=True)
    finally: bridge.close(); bridge.process.stdout.close(); bridge.process.stderr.close()
    verify(m)
    write_new(D/"run_complete.json", {"pairs": 24, "results": 48, "actions": 192, "transitions": 144,
        "execution_sha256": digest(M), "pair_hashes": {p.name: digest(p) for p in sorted(D.glob("*.pair_complete.json"))}})
    print("run_complete pairs=24 results=48", flush=True)
    summarize()


def measured_day(day, next_plan):
    all_stats = [c["horizonPricing"] for c in day["audit"]["candidates"]]
    total = {k: sum(s[k] for s in all_stats) for k in FIELDS}
    selected = day["profile"]["horizonPricing"]
    outcomes = day["profile"]["outcomes"]
    return {"day": day["dayNumber"], "all_candidates": total, "selected": selected,
            "chosen_plan": day["candidate"]["plan"], "certified_score": day["profile"]["certifiedLowerBound"],
            "improved_witness_first_plan_matches_next_submission": bool(selected["improvements"] and
                any(o["futurePlans"] and o["futurePlans"][0] == next_plan for o in outcomes)),
            "work_by_candidate": all_stats}


def gate(rows, minimum_wins=4, minimum_families=2):
    wins = [r for r in rows if r["comparison"] == "win" and r["activated_before_divergence"]]
    losses = [r for r in rows if r["comparison"] == "loss"]
    family_net = defaultdict(int)
    for r in rows: family_net[r["family"]] += r["delta"][2]
    return bool(len(wins) >= minimum_wins and len({r["family"] for r in wins}) >= minimum_families and
        len(wins) > len(losses) and sum(max(0,r["delta"][2]) for r in rows) >= 2*sum(max(0,-r["delta"][2]) for r in rows) and
        all(r["delta"][0] >= 0 and r["delta"][1] >= 0 and r["delta"][2] >= -1 for r in rows) and
        all(v >= 0 for v in family_net.values()) and
        all(r["comparison"] == "tie" or r["activated_before_divergence"] for r in rows))


def summarize():
    m = load(M); verify(m); cases = load(ROOT/m["development"])["cases"]
    marker = load(D/"run_complete.json")
    require(marker["execution_sha256"] == digest(M) and marker["pairs"] == len(cases) == 24, "completion identity")
    require(len(list(D.glob("*.pair_complete.json"))) == 24 and
            all(len(list((D/side).glob("*.result.json"))) == 24 for side in ("parent", "candidate")), "wrong exact file counts")
    require(marker["pair_hashes"] == {p.name: digest(p) for p in sorted(D.glob("*.pair_complete.json"))}, "pair marker drift")
    rows = []
    for c in cases:
        seed = c["spec"]["seed"]
        pair = load(D/(str(seed)+".pair_complete.json"))
        results = {side: validate_side(c, side) for side in ("parent", "candidate")}
        for side in results: require(pair["results"][side] == digest((D/side/str(seed)).with_suffix(".result.json")), "side drift")
        replay = {side: (D/side/str(seed)).with_suffix(".replay.jsonl") for side in results}
        dd = decisions(replay["candidate"])
        plans = {side: [d["plan"] for d in load((D/side/str(seed)).with_suffix(".transport.json"))["actions"]] for side in results}
        telem = [measured_day(d, plans["candidate"][i+1] if i < 3 else None) for i,d in enumerate(dd)]
        divergence = next((i+1 for i in range(4) if plans["parent"][i] != plans["candidate"][i]), None)
        activation = next((d["day"] for d in telem if d["all_candidates"]["improvements"]), None)
        a, b = results["candidate"]["http_score"], results["parent"]["http_score"]
        outcome, tier, difference = compare(a, b)
        rows.append({**c["spec"], "order": c["order"], "parent": b, "candidate": a,
            "comparison": outcome, "first_tier": tier, "first_tier_difference": difference,
            "delta": [x-y for x,y in zip(a,b)], "first_plan_divergence_day": divergence,
            "first_certificate_gain_day": activation,
            "activated_before_divergence": bool(activation is not None and divergence is not None and activation <= divergence),
            "pricing_days": telem, "parent_replay_sha256": results["parent"]["replay_sha256"],
            "candidate_replay_sha256": results["candidate"]["replay_sha256"],
            "safety": {side: safety(replay[side]) for side in results},
            "fuel_capacity": c["setup"]["fuelLimits"], "day_steps": c["setup"]["daySteps"],
            "stocks": [s["stocks"] for s in c["setup"]["spots"]], "brands": len({s["brand"] for s in c["setup"]["spots"]}),
            "steps_stratum": str(c["setup"]["daySteps"]), "stock_stratum": str([s["stocks"] for s in c["setup"]["spots"]]),
            "role": "fixed-three-Patrol", "map": "8x8-roadless-tiny-reachable-support"})
    report = {"experiment": ID, "phase": "development", "complete": True, "pairs": 24,
        "actions": 192, "transitions": 144, "zero_safety_failure": True, "gate_passed": gate(rows),
        "wtl": dict(Counter(r["comparison"] for r in rows)), "rows": rows,
        "first_tiers": dict(Counter(str(r["first_tier"]) for r in rows)),
        "tier_delta": [sum(r["delta"][i] for r in rows) for i in range(3)],
        "strata": {k: {str(v): dict(Counter(r["comparison"] for r in rows if r[k] == v))
                    for v in sorted({r[k] for r in rows})} for k in ("family", "players", "fuel_capacity", "role", "map", "brands", "steps_stratum", "stock_stratum")},
        "inactive_differences": [r["seed"] for r in rows if r["comparison"] != "tie" and not r["activated_before_divergence"]],
        "run_complete_sha256": digest(D/"run_complete.json"), "execution_sha256": digest(M),
        "authority": "Local synthetic closed-loop screening, not BTC, performance, full protected matrix or promotion. No automatic holdout opening."}
    write_new(S, report)
    print(json.dumps({k:report[k] for k in ("complete", "pairs", "wtl", "tier_delta", "gate_passed", "inactive_differences")}))
    print("summary_sha256="+digest(S), flush=True)


if __name__ == "__main__":
    p = argparse.ArgumentParser(); p.add_argument("mode", choices=("run", "summarize")); p.add_argument("--resume", action="store_true")
    args = p.parse_args()
    run(args.resume) if args.mode == "run" else summarize()
