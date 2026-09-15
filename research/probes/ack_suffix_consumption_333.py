"""Read-only attribution of consumed332 ACK/suffix boundaries. No planner calls."""
import argparse
from collections import Counter, defaultdict
import copy
import json
from pathlib import Path
from run_horizon_score_332 import M as M332, D as D332, S as S332, load, verify, validate_side, write_new
from run_http_baseline_314 import ROOT, Bridge, digest
from summarize_http_baseline_314 import require

ID = "ATTR-ACK-W1-SUFFIX-CONSUMPTION-333"
M = ROOT / f"research/holdouts/{ID}.json"
S = ROOT / f"research/evidence/{ID}.summary.json"
KEYS = ("lifetimeDistinct", "totalDailyDistinct", "totalServings")


def score(value):
    return [value[k] for k in KEYS]


def plan_id(plan):
    return json.dumps(plan, separators=(",", ":"))


def cache_has(checkpoint, witness):
    return any(c.get("certifiedSuffix") == {
        "certified": witness["certified"], "lowerBoundOnly": witness["lowerBoundOnly"],
        "score": witness["witnessScore"], "futurePlans": witness["futurePlans"]}
        and c["plan"] == witness["futurePlans"][0]
        for c in checkpoint["cachedContingencies"])


def classify(row):
    if tuple(row["next_certificate"]) >= tuple(row["witness_score"]):
        return "equal-or-better-certified-alternative" if not row["plan_matches"] else "direct-consumption"
    if not row["current_floor_eligible"]:
        return "current-day-floor-conflict"
    if not row["cache_at_ack"]:
        return "missing-ack-cache"
    if not row["cache_before_next"]:
        return "lost-during-idle-cache"
    if row["audit_record"] is None:
        return "absent-from-post-F0-audit"
    if row["audit_record"]["disposition"] == "not-shortlisted":
        return "absent-from-W1-shortlist"
    return "lower-selected-certificate"


def gate(rows):
    lost = [r for r in rows if r["current_floor_eligible"] and r["cache_at_ack"]
            and tuple(r["witness_score"]) > tuple(r["next_certificate"])]
    return len({r["seed"] for r in lost}) >= 2 and len({r["family"] for r in lost}) >= 2


def complete332():
    m = load(M332); verify(m); report = load(S332); marker = load(D332/"run_complete.json")
    require(digest(S332) == "0BEA828803DF4EC44BAE8316DA11F9B16C674D9197E1EEBBF363902AA609D982", "332 summary drift")
    require(digest(D332/"run_complete.json") == "577DDA5E467DC04F125E0144EDD1ABFBC448E7DE5F6932793DA158A333F879B8", "332 marker drift")
    require(report["complete"] and report["pairs"] == marker["pairs"] == 24 and not report["gate_passed"], "332 verdict")
    cases = load(ROOT/m["development"])["cases"]
    for c in cases:
        pair = D332/(str(c["spec"]["seed"])+".pair_complete.json")
        require(marker["pair_hashes"][pair.name] == digest(pair), "pair marker drift")
        for side in ("parent", "candidate"):
            validate_side(c, side)
            require(load(pair)["results"][side] == digest(D332/side/(str(c["spec"]["seed"])+".result.json")), "side hash")
    return m, cases, report


def freeze():
    m, cases, report = complete332()
    paths = {M332, S332, D332/"run_complete.json", ROOT/m["development"], ROOT/m["bridge_binary"], Path(__file__),
             ROOT/"research/probes/test_ack_suffix_consumption_333.py", ROOT/"research/probes/run_horizon_score_332.py",
             ROOT/"research/probes/run_http_baseline_314.py", ROOT/"research/probes/summarize_http_baseline_314.py"}
    for p in ("src/decision.cpp", "src/btc_main.cpp", "src/runtime.cpp", "tests/test_main.cpp"):
        paths.add(ROOT/p)
    for side in ("parent", "candidate"):
        paths.update((D332/side).iterdir())
    write_new(M, {"experiment": ID, "parent": "c76a8ea", "cases": cases, "bridge": m["bridge_binary"],
        "hashes": {str(p.relative_to(ROOT)).replace("\\", "/"): digest(p) for p in sorted(paths)},
        "expected_boundaries": 144, "production_change": False, "holdout_authority": False,
        "gate": "All144 dual-valid; exact same-state retained certificate strictly better than next certificate, current floor eligible, on2roots2families permits separate consistency proposal only."})
    print("manifest_sha256="+digest(M))


def check_step(bridge, setup, state, ledger, plan):
    r = bridge.request({"op": "step", "setup": setup, "state": state, "ledger": ledger, "plan": plan})
    require(r.get("ok") and r["agrees"], "dual validation failed")
    return r


def analyze():
    m = load(M); verify(m); _, cases, previous = complete332()
    rows = []; dual_days = 0; bridge = Bridge(ROOT/m["bridge"])
    try:
        for c in cases:
            seed = c["spec"]["seed"]
            for side in ("parent", "candidate"):
                p = D332/side/str(seed)
                events = [json.loads(line) for line in p.with_suffix(".replay.jsonl").read_text().splitlines()]
                indexed = [(i, e["body"]) for i, e in enumerate(events) if e["kind"] == "decision"]
                transport = load(p.with_suffix(".transport.json")); result = load(p.with_suffix(".result.json"))
                require(len(indexed) == 4, "decision coverage")
                for di in range(3):
                    ei, body = indexed[di]; ni, nb = indexed[di+1]; d = body["decision"]; nd = nb["decision"]
                    require(body["state"]["day"] == di+1 and nb["state"]["day"] == di+2, "day identity")
                    require(not body["state"]["traffics"] and not body["state"]["others"], "roadless scope")
                    require(d["candidate"]["plan"] == transport["actions"][di]["plan"], "actual submitted plan")
                    require(len(d["profile"]["outcomes"]) == 1, "deterministic witness count")
                    w = d["profile"]["outcomes"][0]
                    require(w["certified"] and not w["lowerBoundOnly"] and len(w["futurePlans"]) == 3-di, "complete W1")
                    current = check_step(bridge, c["setup"], body["state"], body["ledger"], d["candidate"]["plan"])
                    dual_days += 1
                    require(current["agents"] == nb["state"]["agents"] and current["ledger"] == nb["ledger"], "ACK state/ledger drift")
                    checkpoints = [e["body"] for e in events[ei+1:ni] if e["kind"] == "session_checkpoint"
                                   and e["body"]["acceptedDay"] == di+1]
                    require(checkpoints, "missing checkpoint evidence")
                    state, ledger = copy.deepcopy(nb["state"]), copy.deepcopy(nb["ledger"]); days = []
                    for plan in w["futurePlans"]:
                        step = check_step(bridge, c["setup"], state, ledger, plan); dual_days += 1
                        days.append({"day": state["day"], "plan": plan, **step})
                        state = {**state, "day": state["day"]+1, "agents": step["agents"]}; ledger = step["ledger"]
                    require(days[-1]["score"] == score(w["witnessScore"]) == score(w["score"]), "suffix certificate score")
                    wid = plan_id(w["futurePlans"][0]); audit = [a for a in nd["audit"]["candidates"] if a["stableId"] == wid]
                    require(len(audit) <= 1, "duplicate candidate ID")
                    row = {**c["spec"], "side": side, "after_day": di+1,
                        "replay_sha256": result["replay_sha256"], "witness_score": days[-1]["score"],
                        "witness_first_day_score": days[0]["score"], "next_certificate": score(nd["profile"]["certifiedLowerBound"]),
                        "next_current_score": score(nd["candidate"]["scoreAfterToday"]), "actual_final_score": result["http_score"],
                        "selected_pricing_improvements": d["profile"].get("horizonPricing", {}).get("improvements", 0),
                        "cache_at_ack": cache_has(checkpoints[0], w), "cache_before_next": cache_has(checkpoints[-1], w),
                        "checkpoint_count": len(checkpoints), "cache_repair": nd["cacheRepair"],
                        "plan_matches": wid == nd["candidate"]["stableId"], "audit_record": audit[0] if audit else None,
                        "current_floor_eligible": tuple(days[0]["score"]) >= tuple(score(nd["candidate"]["scoreAfterToday"])),
                        "validated_suffix": days}
                    row["classification"] = classify(row)
                    row["realized_below_witness"] = tuple(row["actual_final_score"]) < tuple(row["witness_score"])
                    rows.append(row)
    finally:
        bridge.close(); bridge.process.stdout.close(); bridge.process.stderr.close()
    require(len(rows) == 144 and dual_days == 432, "exact attribution coverage")
    verify(m)
    write_new(S, {"experiment": ID, "complete": True, "boundaries": len(rows), "dual_days": dual_days,
        "gate_passed": gate(rows), "zero_state_ledger_or_validation_failure": True,
        "classifications": dict(Counter(r["classification"] for r in rows)),
        "improved_classifications": dict(Counter(r["classification"] for r in rows if r["selected_pricing_improvements"])),
        "strata": {k: {str(v): dict(Counter(r["classification"] for r in rows if r[k] == v))
                    for v in sorted({r[k] for r in rows})} for k in ("side", "family", "players", "after_day")},
        "rows": rows, "manifest_sha256": digest(M), "source332_summary_sha256": digest(S332),
        "authority": "Consumed local synthetic read-only attribution; no new score/promotional evidence and no holdout opened."})
    print(json.dumps({"boundaries": len(rows), "dual_days": dual_days, "gate": gate(rows),
        "classes": dict(Counter(r["classification"] for r in rows)), "summary_sha256": digest(S)}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(); parser.add_argument("mode", choices=("freeze", "analyze"))
    args = parser.parse_args(); freeze() if args.mode == "freeze" else analyze()
