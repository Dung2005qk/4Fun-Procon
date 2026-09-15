"""Complete synthetic capture and offline source membership; no solver policy."""
import argparse
from collections import Counter
import itertools
import json
from pathlib import Path
from run_http_baseline_314 import ROOT, Bridge, digest, run_case
from summarize_http_baseline_314 import safety, summarize as summarize_lifecycle
from http_prefix_option_loss_321 import load, require, write_new, close_bridge
from summarize_http_witness_315 import state_key
from build_pre_f0_capture_324 import HOOKS, transform

ID = "ATTR-HTTP-DAY2-PRE-F0-CAPTURE-324"
M = ROOT / f"research/holdouts/{ID}.json"
D = ROOT / f"research/evidence/{ID}"
S = ROOT / f"research/evidence/{ID}.summary.json"
B314 = "research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json"
D314 = "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314"
M322 = "research/holdouts/ATTR-DAY2-OPTION-PROVENANCE-322.json"
S323 = "research/evidence/ATTR-DAY2-POOL-EXACT-VALUE-323.summary.json"

def verify(m):
    for path, expected in m["hashes"].items():
        require(digest(ROOT / path) == expected, "frozen input drift: " + path)

def freeze():
    baseline = load(ROOT / B314)
    verify(baseline)
    require(digest(ROOT / M322) == "4F8CF6E24F9F1F2E01568C4F04A9AF09C00DCB1AF16DECF397323AA2C32F9A2B", "322 hash")
    require(digest(ROOT / S323) == "9783F5DA1CA4E7DF375EED0ECF2E71CA156A8016E2E87F18424786C9DA976134", "323 hash")
    paths = set(baseline["hashes"]) | {B314, M322, S323,
        "artifacts/research/324/capture_btc.exe", "artifacts/research/324/claims_bridge.exe",
        "artifacts/research/324/source-roundtrip.json",
        *[f"research/probes/{n}" for n in ("pre_f0_capture_324.hpp", "build_pre_f0_capture_324.py",
            "claims_bridge_324.cpp", "pre_f0_attribution_324.py", "test_pre_f0_attribution_324.py",
            "http_prefix_option_loss_321.py", "summarize_http_witness_315.py")],
        *[f"artifacts/research/324/{n}.capture324.cpp" for n in HOOKS]}
    for name, hooks in HOOKS.items():
        original = (ROOT / f"src/{name}.cpp").read_bytes().replace(b"\r\n", b"\n").decode()
        require(transform(original, hooks) == (ROOT / f"artifacts/research/324/{name}.capture324.cpp").read_text(), "source transform changed")
    for case in baseline["cases"]:
        for suffix in (".transport.json", ".replay.jsonl", ".result.json", ".replay-check.txt"):
            paths.add(f"{D314}/{case['seed']}{suffix}")
    m = {**baseline, "experiment": ID, "btc_binary": "artifacts/research/324/capture_btc.exe",
        "claims_bridge": "artifacts/research/324/claims_bridge.exe",
        "hashes": {p: digest(ROOT / p) for p in sorted(paths)},
        "gate": "Complete12 captures48actions36transitions; same day1 prefix and day2 root+selected/submitted action as314; "
                "dual-replay all recorded pools and isolated columns; no favorable reruns; only source attribution, no promotion.",
        "limits": "Copying observations can perturb wall-clock cutoffs. Unqualified traces cannot explain historical gaps. "
                  "One optimum-witness membership is not a proof that all optima are absent. Consumed narrow development only.",
        "production_change": False, "holdout_authority": False}
    write_new(M, m)
    print(json.dumps({"manifest_sha256": digest(M), "cases": len(m["cases"]), "hashes":len(m["hashes"])}))

def run():
    m = load(M); verify(m)
    D.mkdir(parents=True, exist_ok=False)
    bridge = Bridge(ROOT / m["bridge_binary"])
    try:
        for spec, setup in zip(m["cases"], m["setups"], strict=True):
            require(bridge.request({"op":"fixture", **spec})["setup"] == setup, "fixture identity")
        for spec, setup in zip(m["cases"], m["setups"], strict=True):
            run_case(spec, setup, m, D, bridge)
            print(f"case_complete seed={spec['seed']}", flush=True)
        verify(m)
        write_new(D / "run_complete.json", {"cases":12, "manifest_sha256":digest(M)})
        print("run_complete cases=12", flush=True)
    finally: close_bridge(bridge)

def decisions(path):
    return [e["body"] for line in path.read_text().splitlines()
            if (e := json.loads(line))["kind"] == "decision"]

def qualifying(old, new, old_dec, new_dec):
    # endsAt changes by construction; score-bearing state/ledger may not.
    checks = {
        "day1_plan": old["actions"][0]["plan"] == new["actions"][0]["plan"],
        "day1_outcome": old["actions"][0]["validated"] == new["actions"][0]["validated"],
        "day2_root": state_key(old_dec["state"]["agents"], old_dec["ledger"]) ==
                     state_key(new_dec["state"]["agents"], new_dec["ledger"]),
        "day2_selected": old_dec["decision"]["candidate"]["stableId"] == new_dec["decision"]["candidate"]["stableId"],
        "day2_submitted": old["actions"][1]["plan"] == new["actions"][1]["plan"],
    }
    return checks, all(checks.values())

def same_outcome(a, b, unordered=False):
    return state_key(a["agents"], a["ledger"], unordered) == state_key(b["agents"], b["ledger"], unordered)

def feature(checked, agent):
    a = checked["agents"][agent]
    return (a["kind"], a["pos"], a["fuel"],
            tuple(sorted(c["spot"] for c in checked["claims"] if c["agent"] == agent)))

def checked_step(bridge, request, plan):
    checked = bridge.request({**request, "op":"step", "plan":plan})
    require(checked.get("ok") and checked.get("agrees"), "dual-invalid recorded plan: " + str(checked))
    return checked

def pool_rows(pool, bridge, request, target):
    rows = []
    for candidate in pool:
        require(json.loads(candidate["stableId"]) == candidate["plan"], "pool plan identity")
        checked = checked_step(bridge, request, candidate["plan"])
        for key in ("agents", "score", "claims"):
            if key in candidate: require(checked[key] == candidate[key], "recorded " + key + " mismatch")
        if "valid" in candidate: require(candidate["valid"], "invalid captured candidate")
        rows.append({"stableId":candidate["stableId"], "actual":checked,
            "physical_match":same_outcome(checked,target), "permuted_match":same_outcome(checked,target,True)})
    return rows

def columns_rows(columns, bridge, request, target):
    require(len(columns) == 3, "three Patrol scope")
    steps = request["setup"]["daySteps"][1]
    by_agent, mismatches = [], []
    for agent, values in enumerate(columns):
        rows = []
        for column in values:
            require(column["agent"] == agent and len(column["plan"]) == 1, "column ownership")
            plan = [[-steps] for _ in range(3)]
            plan[agent] = column["plan"][0]
            checked = checked_step(bridge, request, plan)
            actual_feature = feature(checked, agent)
            expected_claims = sorted(v["spot"] for v in column["firstVisits"] if v["claimed"])
            observed_claims = list(actual_feature[3])
            mismatch = {"claims":expected_claims != observed_claims,
                "terminalCell":column["terminalCell"] != checked["agents"][agent]["pos"],
                "terminalFuel":column["terminalFuel"] != checked["agents"][agent]["fuel"]}
            if any(mismatch.values()): mismatches.append({"agent":agent,"columnId":column["columnId"],**mismatch})
            rows.append({"column":column, "actual":checked, "feature":actual_feature,
                "optimal_agent_matches":[i for i in range(3) if actual_feature == feature(target,i)]})
        by_agent.append(rows)
    # A complete feature matching is independently simulated as a TEAM. Do not
    # infer stock/refuel correctness from independent column validity alone.
    assignments = []
    for mapping in itertools.permutations(range(3)):
        selected = [next((r for r in by_agent[a] if mapping[a] in r["optimal_agent_matches"]),None) for a in range(3)]
        if all(selected):
            plan = [r["column"]["plan"][0] for r in selected]
            actual = checked_step(bridge, request, plan)
            require(same_outcome(actual,target,True), "feature match failed joint stock/state replay")
            assignments.append({"mapping":mapping,"columnIds":[r["column"]["columnId"] for r in selected],
                                "plan":plan,"actual":actual,"physical_match":same_outcome(actual,target)})
    return {"by_agent":by_agent,"optimal_feature_assignments":assignments,"metadata_mismatches":mismatches}

def summarize():
    m = load(M); verify(m)
    lifecycle = summarize_lifecycle(M,D)  # Reject incomplete evidence BEFORE inspecting telemetry.
    old_cases = {c["seed"]:c for c in load(ROOT/M322)["cases"]}
    bridge = Bridge(ROOT/m["claims_bridge"])
    results = []
    try:
        for spec in m["cases"]:
            seed = spec["seed"]
            new_transport = load(D/f"{seed}.transport.json")
            old_transport = load(ROOT/f"{D314}/{seed}.transport.json")
            new_decisions = decisions(D/f"{seed}.replay.jsonl")
            old_decisions = decisions(ROOT/f"{D314}/{seed}.replay.jsonl")
            require(len(new_decisions)==4 and [b["capture324"]["active"] for b in new_decisions]==[False,True,False,False], "capture lifecycle")
            body = new_decisions[1]; capture = body["capture324"]
            require(capture["day"]==2 and state_key(capture["agents"],capture["ledger"]) ==
                    state_key(body["state"]["agents"],body["ledger"]), "collector root mismatch")
            require(set(capture["portfolios"])=={"legacy","merged"} and
                    set(capture["pools"])=={"initial-master","legacy-master","pre-f0"}, "missing source boundary")
            checks, qualified = qualifying(old_transport,new_transport,old_decisions[1],body)
            final_pool_equal = [c["stableId"] for c in old_decisions[1]["decision"]["audit"]["candidates"]] == [c["stableId"] for c in body["decision"]["audit"]["candidates"]]
            row = {"seed":seed,"family":spec["family"],"players":spec["players"],"qualification_checks":checks,
                "qualified":qualified,"same_final_pool_ids":final_pool_equal,"capture":capture}
            # Still validate every raw plan if drifted, but do NOT compare to a
            # historical optimum whose root no longer matches.
            reference = old_cases[seed]
            request = {"setup":reference["request"]["setup"],"state":body["state"],"ledger":body["ledger"]}
            target_plan = reference["oracle"]["days"][0]["plan"] if qualified else new_transport["actions"][1]["plan"]
            target = checked_step(bridge,request,target_plan)
            if qualified:
                require(same_outcome(target,reference["oracle"]["days"][0]), "conditional witness root mismatch")
            row["target"] = target
            row["target_is_historical_optimal_witness"] = qualified
            row["portfolios"] = {label:columns_rows(value,bridge,request,target) for label,value in capture["portfolios"].items()}
            row["pools"] = {label:pool_rows(value,bridge,request,target) for label,value in capture["pools"].items()}
            final = [{"stableId":c["stableId"],"plan":json.loads(c["stableId"])} for c in body["decision"]["audit"]["candidates"]]
            row["pools"]["final-audit"] = pool_rows(final,bridge,request,target)
            row["historical_day2_loss"] = reference["option_loss"]
            results.append(row)
    finally: close_bridge(bridge)
    metadata = sum(len(p["metadata_mismatches"]) for r in results for p in r["portfolios"].values())
    report = {"experiment":ID,"manifest_sha256":digest(M),"cases":12,"actions":48,"transitions":36,
        "qualified":sum(r["qualified"] for r in results),"same_final_pool_ids":sum(r["same_final_pool_ids"] for r in results),
        "dual_validated_team_plans":sum(len(p) for r in results for p in r["pools"].values()),
        "dual_validated_columns":sum(len(a) for r in results for p in r["portfolios"].values() for a in p["by_agent"]),
        "column_metadata_mismatches":metadata,"zero_dual_failure":True,"zero_safety_failure":True,
        "results":results,"lifecycle":lifecycle,"score_successor_authorized":False,"production_change":False,
        "limits":m["limits"], "strata":{family:dict(Counter(str(r["qualified"]) for r in results if r["family"]==family))
            for family in sorted({r["family"] for r in results})}}
    write_new(S,report)
    print(json.dumps({k:v for k,v in report.items() if k not in ("results","lifecycle","strata")},indent=2))
    for r in results:
        print(json.dumps({"seed":r["seed"],"qualified":r["qualified"],"final_pool_equal":r["same_final_pool_ids"],
            "column_witness_assignment_counts":{k:len(v["optimal_feature_assignments"]) for k,v in r["portfolios"].items()},
            "pool_witness_counts":{k:sum(c["permuted_match"] for c in v) for k,v in r["pools"].items()}}))
    print("summary_sha256 " + digest(S))

if __name__ == "__main__":
    p=argparse.ArgumentParser();p.add_argument("mode",choices=("freeze","run","summarize"))
    {"freeze":freeze,"run":run,"summarize":summarize}[p.parse_args().mode]()
