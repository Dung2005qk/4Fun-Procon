"""Fresh selected-W1 prevalence; frozen HTTP baseline then unchanged330 probe.

Loopback synthetic fixtures only. Never applies a probe plan to a live match.
All baseline cases must complete before any certificate attribution is read.
"""
import argparse
from collections import Counter
import copy
import json
from http_prefix_option_loss_321 import ROOT, load, digest, object_hash, require, write_new, close_bridge
from run_http_baseline_314 import Bridge, run_case
from summarize_http_baseline_314 import safety
from pool_boundary_value_327 import verify_local
from day2_pool_value_323 import comparison

ID = "ATTR-W1-HORIZON-PRICING-PREVALENCE-331"
M = ROOT / f"research/holdouts/{ID}.json"
D = ROOT / f"research/evidence/{ID}"
S = ROOT / f"research/evidence/{ID}.summary.json"
FAMILIES = ("three-balanced", "three-duplicate", "three-stock", "three-coverage", "three-fuel", "three-terminal")
BASESEED = 202609070000


def scores(s):
    return [s[k] for k in ("lifetimeDistinct", "totalDailyDistinct", "totalServings")]


def freeze():
    p330 = "research/holdouts/ATTR-W1-HORIZON-PATROL-BEST-RESPONSE-330.json"
    s330 = "research/evidence/ATTR-W1-HORIZON-PATROL-BEST-RESPONSE-330.summary.json"
    require(digest(ROOT/p330) == "B0D75D06A6495E144F33B603B67039EE4320EB5D97CBEB4EDB117AD0092F20AF", "330 manifest")
    require(digest(ROOT/s330) == "3E0EBD44EEB8CDE78C210675E9CD1ADF9A2A5BD5E9B591350AC14A67C217CC18", "330 evidence")
    old = load(ROOT/p330); verify_local(old)
    m314 = "research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json"
    prior = load(ROOT/m314)
    for p, h in prior["hashes"].items():
        require(digest(ROOT/p) == h, "314 frozen drift: " + p)
    paths = set(old["local_hashes"]) | set(prior["hashes"]) | {p330, s330, m314,
        "research/probes/selected_horizon_pricing_331.py", "research/probes/test_selected_horizon_pricing_331.py"}
    cases = [{"seed": BASESEED + 100*f + r, "family": family, "players": 8+(f+r)%3}
             for f, family in enumerate(FAMILIES) for r in range(4)]
    bridge = Bridge(ROOT/prior["bridge_binary"])
    try:
        setups = []
        for c in cases:
            q = bridge.request({"op": "fixture", **c})
            require(q.get("ok"), "fixture generation failed")
            require(q["setup"]["daySeconds"] == [5]*4 and len(q["setup"]["agents"]) == 3, "fixture contract")
            setups.append(q["setup"])
    finally:
        close_bridge(bridge)
    write_new(M, {"experiment": ID, "parent": "c76a8ea", "cases": cases, "setups": setups,
        "local_hashes": {p: digest(ROOT/p) for p in sorted(paths)},
        "btc_binary": prior["btc_binary"], "bridge_binary": prior["bridge_binary"], "probe": old["probe"],
        "expected_baselines": 24, "expected_responses": 72,
        "gate": "24 complete96ACK72transitions then72 independent dual-valid responses; selected certificate gains on4roots2families; zero failure; separate fresh bounded SCORE proposal only.",
        "production_change": False, "holdout_authority": False, "btc_authority": False,
        "scope": "Roadless three-Patrol synthetic HTTP; fixed roles and no opponent agents. Player-count labels do not measure traffic pressure. No oracle input or timing claim."})
    print("manifest_sha256=" + digest(M), flush=True)


def baseline():
    m = load(M); verify_local(m); D.mkdir(exist_ok=False)
    bridge = Bridge(ROOT/m["bridge_binary"])
    try:
        for c, setup in zip(m["cases"], m["setups"], strict=True):
            q = bridge.request({"op": "fixture", **c})
            require(q.get("ok") and q["setup"] == setup, "fixture identity")
            run_case(c, setup, m, D, bridge)
            print("case_complete seed=" + str(c["seed"]), flush=True)
    finally:
        close_bridge(bridge)
    verify_local(m)
    write_new(D/"run_complete.json", {"cases": 24, "manifest_sha256": digest(M),
        "result_hashes": {p.name: digest(p) for p in sorted(D.glob("*.result.json"))}})
    print("baseline_complete cases=24", flush=True)


def validate_complete(m):
    verify_local(m)
    marker = load(D/"run_complete.json")
    expected = {str(c["seed"])+".result.json" for c in m["cases"]}
    require(len(m["cases"]) == 24 and len(expected) == 24 and marker["cases"] == 24 and
            marker["manifest_sha256"] == digest(M), "incomplete baseline")
    require({p.name for p in D.glob("*.result.json")} == expected == set(marker["result_hashes"]), "baseline inventory")
    for c in m["cases"]:
        p = D/str(c["seed"]); result = p.with_suffix(".result.json"); r = load(result)
        require(digest(result) == marker["result_hashes"][result.name], "baseline result changed")
        require(all(r[k] == v for k, v in c.items()) and r["actions"] == 4 and
                r["transitions"] == 3 and r["failure"] is None, "case identity/lifecycle")
        for suffix, key in ((".replay.jsonl", "replay_sha256"), (".transport.json", "transport_sha256"),
                            (".replay-check.txt", "replay_check_sha256")):
            require(digest(p.with_suffix(suffix)) == r[key], "baseline artifact changed")
        require(p.with_suffix(".stderr").stat().st_size == 0, "baseline stderr")
        safety(p.with_suffix(".replay.jsonl"))
    return marker


def selected_context(body, setup, bridge):
    d = body["decision"]; outcomes = d["profile"]["outcomes"]
    require(len(outcomes) == 1 and outcomes[0]["certified"] and not outcomes[0]["lowerBoundOnly"]
            and len(outcomes[0]["futurePlans"]) == 3, "missing selected complete W1")
    require(body["state"]["day"] == 1 and body["state"]["others"] == [] and
            body["state"]["traffics"] == [], "root scope")
    current = bridge.request({"op": "step", "setup": setup, "state": body["state"],
                              "ledger": body["ledger"], "plan": d["candidate"]["plan"]})
    require(current.get("ok") and current["agrees"] and
            current["score"] == scores(d["candidate"]["scoreAfterToday"]), "current action identity")
    q = {"setup": setup, "state": {"day": 2, "endsAt": 0, "agents": current["agents"], "others": [], "traffics": []},
         "ledger": current["ledger"], "plans": outcomes[0]["futurePlans"]}
    state, ledger = q["state"], q["ledger"]; originals = []
    for plan in q["plans"]:
        r = bridge.request({"op": "step", "setup": setup, "state": state, "ledger": ledger, "plan": plan})
        require(r.get("ok") and r["agrees"], "original witness invalid")
        originals.append({"day": state["day"], "plan": plan, **r})
        state = {**state, "day": state["day"]+1, "agents": r["agents"]}; ledger = r["ledger"]
    baseline = scores(outcomes[0]["witnessScore"]); upper = scores(d["profile"]["validUpperBound"])
    require(originals[-1]["score"] == baseline and tuple(baseline) <= tuple(upper), "original score/upper identity")
    return {"request": copy.deepcopy(q), "original_days": originals, "baseline": baseline,
            "valid_upper_bound": upper, "current_plan": d["candidate"]["plan"], "current": current,
            "selected_body_sha256": object_hash(body)}


def validate_response(out, c, bridge):
    require(out.get("ok") and out.get("complete") and out["baseline"] == c["baseline"] and
            [r["agent"] for r in out["responses"]] == [0, 1, 2], "response coverage/control")
    dual = 0
    for r in out["responses"]:
        require(r["complete"] and [d["day"] for d in r["days"]] == [2, 3, 4] and
                tuple(c["baseline"]) <= tuple(r["score"]) <= tuple(c["valid_upper_bound"]), "response score/ceiling")
        require(all(isinstance(r[k], int) and r[k] >= 0 for k in ("memo_states", "day_enumerations", "transitions")), "work fields")
        q = c["request"]; state, ledger = q["state"], q["ledger"]
        for i, day in enumerate(r["days"]):
            require(len(day["plan"]) == 3 and all(day["plan"][a] == q["plans"][i][a]
                    for a in range(3) if a != r["agent"]), "fixed other plan changed")
            checked = bridge.request({"op": "step", "setup": q["setup"], "state": state, "ledger": ledger, "plan": day["plan"]})
            require(checked.get("ok") and checked["agrees"] and all(checked[k] == day[k]
                    for k in ("score", "agents", "ledger")), "dual response identity")
            require(all(checked["agents"][a] == c["original_days"][i]["agents"][a]
                    for a in range(3) if a != r["agent"]), "fixed other state changed")
            state = {**state, "day": state["day"]+1, "agents": checked["agents"]}; ledger = checked["ledger"]; dual += 1
        require(r["score"] == r["days"][-1]["score"], "final ledger score")
    return dual


def analyze():
    m = load(M); validate_complete(m)
    directory = D/"attribution"; directory.mkdir(exist_ok=False)
    bridge = Bridge(ROOT/m["bridge_binary"]); probe = Bridge(ROOT/m["probe"]); dual = 0
    try:
        for spec, setup in zip(m["cases"], m["setups"], strict=True):
            replay = D/(str(spec["seed"])+".replay.jsonl")
            events = [json.loads(line) for line in replay.read_text().splitlines()]
            body = next(e["body"] for e in events if e["kind"] == "decision")
            context = selected_context(body, setup, bridge)
            request_before = object_hash(context["request"])
            out = probe.request(context["request"]); dual += validate_response(out, context, bridge)
            require(object_hash(context["request"]) == request_before, "probe request mutated")
            write_new(directory/(str(spec["seed"])+".result.json"), {"spec": spec, "context": context,
                "output": out, "replay_sha256": digest(replay), "manifest_sha256": digest(M)})
    finally:
        close_bridge(bridge); close_bridge(probe)
    verify_local(m); require(dual == 216, "replacement dual coverage")
    write_new(directory/"run_complete.json", {"roots": 24, "responses": 72, "replacement_dual_days": dual,
        "manifest_sha256": digest(M), "baseline_complete_sha256": digest(D/"run_complete.json"),
        "result_hashes": {p.name: digest(p) for p in sorted(directory.glob("*.result.json"))}})
    print("attribution_complete roots=24 responses=72", flush=True)


def summarize():
    m = load(M); validate_complete(m); directory = D/"attribution"; marker = load(directory/"run_complete.json")
    require(marker["roots"] == 24 and marker["responses"] == 72 and marker["replacement_dual_days"] == 216 and
            marker["manifest_sha256"] == digest(M) and marker["baseline_complete_sha256"] == digest(D/"run_complete.json"), "attribution completion")
    require({p.name for p in directory.glob("*.result.json")} == set(marker["result_hashes"]) ==
            {str(c["seed"])+".result.json" for c in m["cases"]}, "attribution inventory")
    results = []; bridge = Bridge(ROOT/m["bridge_binary"])
    try:
        for spec, setup in zip(m["cases"], m["setups"], strict=True):
            p = directory/(str(spec["seed"])+".result.json"); row = load(p)
            require(digest(p) == marker["result_hashes"][p.name] and row["spec"] == spec and row["manifest_sha256"] == digest(M), "attribution identity")
            replay = D/(str(spec["seed"])+".replay.jsonl")
            require(digest(replay) == row["replay_sha256"], "replay identity")
            body = next(e["body"] for e in map(json.loads, replay.read_text().splitlines()) if e["kind"] == "decision")
            c = selected_context(body, setup, bridge); require(c == row["context"], "selected context changed")
            validate_response(row["output"], c, bridge)
            responses = [{**r, "versus_original": comparison(r["score"], c["baseline"])} for r in row["output"]["responses"]]
            best = max((r["score"] for r in responses), key=tuple)
            results.append({**spec, "fuel": setup["fuelLimits"], "steps": str(setup["daySteps"]),
                "stocks": str([s["stocks"] for s in setup["spots"]]), "brands": len({s["brand"] for s in setup["spots"]}),
                "original": c["baseline"], "best": best, "valid_upper_bound": c["valid_upper_bound"],
                "versus_original": comparison(best, c["baseline"]), "responses": responses,
                "result_sha256": digest(p), "replay_sha256": digest(replay)})
    finally:
        close_bridge(bridge)
    wins = [r for r in results if r["versus_original"]["result"] == "win"]
    families = sorted({r["family"] for r in wins})
    report = {"experiment": ID, "complete": True, "cases": 24, "actions": 96, "transitions": 72,
        "responses": 72, "original_dual_days": 72, "replacement_dual_days": 216,
        "manifest_sha256": digest(M), "run_complete_sha256": digest(directory/"run_complete.json"),
        "result_hashes": marker["result_hashes"], "results": results, "zero_failure": True,
        "wtl": dict(Counter(r["versus_original"]["result"] for r in results)),
        "response_wtl": dict(Counter(p["versus_original"]["result"] for r in results for p in r["responses"])),
        "first_tiers": dict(Counter(str(r["versus_original"]["tier"]) for r in results)),
        "gain_loss_tails": sorted((r["versus_original"] for r in results), key=lambda x: tuple(x["components"])),
        "strata": {f: {str(k): dict(Counter(r["versus_original"]["result"] for r in results if r[f] == k))
                       for k in sorted({r[f] for r in results})} for f in ("family", "players", "fuel", "steps", "stocks", "brands")},
        "qualifying_roots": len(wins), "qualifying_families": families,
        "gate_passed": len(wins) >= 4 and len(families) >= 2,
        "production_change": False, "score_promotion_authority": False, "btc_authority": False,
        "limits": "Selected certificate improvements, NOT played-score improvements or runtime suitability. Roadless three-Patrol fixtures, no opponent agents; player labels are not traffic validation. No oracle values or plans."}
    verify_local(m); write_new(S, report)
    print(json.dumps({k: report[k] for k in ("experiment", "wtl", "response_wtl", "qualifying_roots", "qualifying_families", "gate_passed", "zero_failure")}))
    print("summary_sha256=" + digest(S), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(); parser.add_argument("mode", choices=("freeze", "baseline", "analyze", "summarize", "execute"))
    mode = parser.parse_args().mode
    if mode == "execute":
        baseline(); analyze(); summarize()
    else:
        {"freeze": freeze, "baseline": baseline, "analyze": analyze, "summarize": summarize}[mode]()
