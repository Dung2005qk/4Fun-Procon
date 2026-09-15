"""Diagnostic oracle-reference decomposition ONLY; never a planner/candidate input."""
import argparse
from collections import Counter
import copy
import json
from pathlib import Path
from joint_response_composition_334 import compose, source, M330, D330, S330
from http_prefix_option_loss_321 import ROOT, load, digest, require, write_new, close_bridge
from run_http_baseline_314 import Bridge
from pool_boundary_value_327 import verify_local
from day2_pool_value_323 import comparison

ID = "ATTR-W1-REFERENCE-COORDINATION-335"
M = ROOT/f"research/holdouts/{ID}.json"
S = ROOT/f"research/evidence/{ID}.summary.json"
D328 = ROOT/"research/evidence/ATTR-DAY1-AUDIT-EXACT-OPTION-328"
S328 = D328.with_suffix(".summary.json")
BRIDGE = "artifacts/research/324/claims_bridge.exe"


def minimum_changes(combinations, threshold, strict=True):
    valid = [r["mask"].bit_count() for r in combinations
             if tuple(r["score"]) > tuple(threshold) or (not strict and r["score"] == threshold)]
    return min(valid) if valid else None


def gate(rows):
    eligible = [r for r in rows if r["better_action"] and r["min_changes_above_single"] is not None]
    return len({r["seed"] for r in eligible}) >= 2 and len({r["family"] for r in eligible}) >= 2


def freeze():
    m = source()
    require(digest(S328) == "78A7337894BA8DDBA038A6191117872280E6F6D0C0A13BBDD34A750A26C12343", "328 summary drift")
    marker = load(D328/"run_complete.json")
    require(marker["cases"] == 12 and load(S328)["result_hashes"] == marker["results"], "328 completion")
    paths = {M330, S330, D330/"run_complete.json", S328, D328/"run_complete.json", Path(__file__),
             ROOT/"research/probes/test_reference_coordination_335.py", ROOT/BRIDGE,
             ROOT/"research/probes/joint_response_composition_334.py",
             ROOT/"research/evidence/ATTR-W1-JOINT-RESPONSE-COMPOSITION-334.summary.json"}
    paths.update(ROOT/p for p in m["local_hashes"]); paths.update(D330.glob("*.result.json"))
    for name, h in marker["results"].items():
        require(digest(D328/name) == h, "328 exact record drift"); paths.add(D328/name)
    # Do not inspect any reference trajectory until these inputs and gate are frozen.
    write_new(M, {"experiment": ID, "cases": m["cases"], "bridge": BRIDGE,
        "local_hashes": {str(p.relative_to(ROOT)).replace("\\", "/"): digest(p) for p in sorted(paths)},
        "gate": "All216 complete with exact controls; classify obstruction on2better-action roots2families only. No oracle-fed candidate or SCORE authority.",
        "production_change": False, "holdout_authority": False})
    print("manifest_sha256="+digest(M))


def run():
    m = load(M); verify_local(m); source(); rows = []; dual = 0; bridge = Bridge(ROOT/m["bridge"])
    try:
        for c in m["cases"]:
            q = c["request"]; out = load(D330/(c["id"]+".result.json"))["output"]
            exact = load(D328/(str(c["seed"])+".result.json"))["oracle"]["portfolios"][c["label"]]
            require(exact["score"] == c["exact_value"] and [d["day"] for d in exact["days"]] == [1,2,3,4], "reference identity")
            require(exact["days"][0]["plan"] == json.loads(c["audit"]["stableId"]) and
                    exact["days"][0]["agents"] == q["state"]["agents"] and exact["days"][0]["ledger"] == q["ledger"], "unchanged current action/root")
            refs = [{"agent": a, "days": exact["days"][1:]} for a in range(3)]; combinations = []
            for mask in range(8):
                plans = compose(q["plans"], refs, mask); state = copy.deepcopy(q["state"]); ledger = copy.deepcopy(q["ledger"]); days = []
                for di, plan in enumerate(plans):
                    step = bridge.request({"op": "step", "setup": q["setup"], "state": state, "ledger": ledger, "plan": plan})
                    require(step.get("ok") and step["agrees"], "reference composition invalid")
                    for a in range(3):
                        expected = refs[a]["days"][di]["agents"][a] if mask & (1 << a) else c["fixed_states"][di][a]
                        require(step["agents"][a] == expected, "reference physical identity")
                    days.append({"day": state["day"], "plan": plan, **step}); dual += 1
                    state = {**state, "day": state["day"]+1, "agents": step["agents"]}; ledger = step["ledger"]
                value = days[-1]["score"]
                require(tuple(value) <= tuple(c["exact_value"]), "reference ceiling")
                if mask == 0: require(value == c["baseline"], "original control")
                if mask == 7: require(value == c["exact_value"], "full reference control")
                if mask in (1,2,4): require(tuple(value) <= tuple(out["responses"][mask.bit_length()-1]["score"]), "330 best response upper contradicted")
                combinations.append({"mask": mask, "score": value, "days": days, "versus_original": comparison(value,c["baseline"])})
            single = max((r["score"] for r in out["responses"]), key=tuple)
            rows.append({"id": c["id"], "seed": c["seed"], "family": c["family"], "players": c["players"],
                "original": c["baseline"], "best_single": single, "exact_value": c["exact_value"],
                "better_action": tuple(c["exact_value"]) > tuple(c["selected_exact_value"]) and c["audit"]["certified"],
                "min_changes_above_single": minimum_changes(combinations,single),
                "min_changes_for_optimum": minimum_changes(combinations,c["exact_value"],False),
                "singleton_comparisons": [{"agent": a, "reference_score": combinations[1<<a]["score"],
                    "best_response_score": out["responses"][a]["score"],
                    "versus_best_response": comparison(combinations[1<<a]["score"],out["responses"][a]["score"]),
                    "same_trajectory": all(exact["days"][d+1]["plan"][a] == out["responses"][a]["days"][d]["plan"][a] for d in range(3))}
                    for a in range(3)], "combinations": combinations})
    finally: close_bridge(bridge)
    require(len(rows) == 27 and dual == 648, "coverage"); verify_local(m)
    report = {"experiment": ID, "complete": True, "roots": 27, "compositions": 216, "dual_days": dual,
        "zero_failure": True, "gate_passed": gate(rows), "minimum_changes_above_single": dict(Counter(str(r["min_changes_above_single"]) for r in rows)),
        "rows": rows, "manifest_sha256": digest(M), "production_change": False, "score_authority": False,
        "authority": "Consumed optimal-reference diagnostics ONLY. Neither this oracle path nor oracle score may be supplied to a production candidate."}
    write_new(S,report)
    print(json.dumps({k:report[k] for k in ("roots","compositions","dual_days","gate_passed","minimum_changes_above_single")}))
    print("summary_sha256="+digest(S))


if __name__ == "__main__":
    p = argparse.ArgumentParser(); p.add_argument("mode",choices=("freeze","run")); args = p.parse_args()
    freeze() if args.mode == "freeze" else run()
