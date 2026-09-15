"""Compose consumed complete one-Patrol trajectories; no solver or new routes."""
import argparse
from collections import Counter
import copy
import json
from pathlib import Path
from horizon_patrol_response_330 import M as M330, D as D330, S as S330, validate
from run_http_baseline_314 import ROOT, Bridge, digest
from http_prefix_option_loss_321 import load, require, write_new, close_bridge
from pool_boundary_value_327 import verify_local
from day2_pool_value_323 import comparison

ID = "ATTR-W1-JOINT-RESPONSE-COMPOSITION-334"
M = ROOT/f"research/holdouts/{ID}.json"
S = ROOT/f"research/evidence/{ID}.summary.json"


def compose(original, responses, mask):
    require(type(mask) is int and 0 <= mask < 8 and len(responses) == 3, "composition domain")
    require([r["agent"] for r in responses] == [0,1,2] and all(len(r["days"]) == len(original) for r in responses), "response coverage")
    require(all(len(p) == 3 for p in original), "Patrol count")
    return [[copy.deepcopy(responses[a]["days"][d]["plan"][a] if mask & (1 << a) else plan[a])
             for a in range(3)] for d, plan in enumerate(original)]


def gate(rows):
    qualifying = [r for r in rows if tuple(r["best"]) > tuple(r["best_single"]) and r["better_action"]]
    return len({r["seed"] for r in qualifying}) >= 2 and len({r["family"] for r in qualifying}) >= 2


def source():
    require(digest(M330) == "B0D75D06A6495E144F33B603B67039EE4320EB5D97CBEB4EDB117AD0092F20AF", "330 manifest drift")
    require(digest(S330) == "3E0EBD44EEB8CDE78C210675E9CD1ADF9A2A5BD5E9B591350AC14A67C217CC18", "330 summary drift")
    m = load(M330); verify_local(m); marker = load(D330/"run_complete.json")
    require(len(m["cases"]) == marker["requests"] == 27 and marker["responses"] == 81 and marker["manifest_sha256"] == digest(M330), "330 complete")
    require({p.name for p in D330.glob("*.result.json")} == set(marker["result_hashes"]), "330 inventory")
    for name, expected in marker["result_hashes"].items(): require(digest(D330/name) == expected, "330 result drift")
    return m


def freeze():
    m = source()
    paths = {M330, S330, D330/"run_complete.json", Path(__file__),
             ROOT/"research/probes/test_joint_response_composition_334.py",
             ROOT/"research/evidence/ATTR-ACK-W1-SUFFIX-CONSUMPTION-333.summary.json"}
    paths.update(D330.glob("*.result.json")); paths.update(ROOT/p for p in m["local_hashes"])
    write_new(M, {"experiment": ID, "parent": "c76a8ea", "cases": m["cases"], "bridge": m["bridge"],
        "local_hashes": {str(p.relative_to(ROOT)).replace("\\", "/"): digest(p) for p in sorted(paths)},
        "expected_roots": 27, "expected_compositions": 216, "expected_composition_dual_days": 648,
        "gate": "Joint-best above best-single on2already-shortlisted better-action roots2families with zero failure permits fresh separate prevalence/design only.",
        "production_change": False, "holdout_authority": False})
    print("manifest_sha256="+digest(M))


def run():
    m = load(M); verify_local(m); source(); rows = []; dual = control_dual = 0
    bridge = Bridge(ROOT/m["bridge"])
    try:
        for c in m["cases"]:
            recorded = load(D330/(c["id"]+".result.json")); out = recorded["output"]
            require(recorded["request"] == c["request"] and recorded["manifest_sha256"] == digest(M330), "request identity")
            control_dual += validate(out, c, bridge)
            q = c["request"]; combinations = []
            for mask in range(8):
                plans = compose(q["plans"], out["responses"], mask)
                state, ledger = copy.deepcopy(q["state"]), copy.deepcopy(q["ledger"]); days = []
                for di, plan in enumerate(plans):
                    step = bridge.request({"op": "step", "setup": q["setup"], "state": state, "ledger": ledger, "plan": plan})
                    require(step.get("ok") and step["agrees"], "composition dual validity")
                    for a in range(3):
                        expected = out["responses"][a]["days"][di]["agents"][a] if mask & (1 << a) else c["fixed_states"][di][a]
                        require(step["agents"][a] == expected, "physical independence contradiction")
                    days.append({"day": state["day"], "plan": plan, **step}); dual += 1
                    state = {**state, "day": state["day"]+1, "agents": step["agents"]}; ledger = step["ledger"]
                value = days[-1]["score"]
                require(tuple(value) <= tuple(c["exact_value"]), "exceeded frozen conditional optimum")
                if mask == 0: require(value == c["baseline"], "original control")
                if mask in (1,2,4):
                    response = out["responses"][mask.bit_length()-1]
                    require(value == response["score"] and plans == [d["plan"] for d in response["days"]], "singleton control")
                combinations.append({"mask": mask, "score": value, "days": days, "versus_original": comparison(value, c["baseline"])})
            best = max((p["score"] for p in combinations), key=tuple)
            single = max((r["score"] for r in out["responses"]), key=tuple)
            require(tuple(best) >= tuple(single), "protected singleton lost")
            rows.append({"id": c["id"], "seed": c["seed"], "family": c["family"], "players": c["players"],
                "original": c["baseline"], "best_single": single, "best": best, "audit": c["audit"],
                "exact_value": c["exact_value"], "selected_exact_value": c["selected_exact_value"],
                "better_action": tuple(c["exact_value"]) > tuple(c["selected_exact_value"]) and c["audit"]["certified"],
                "best_vs_single": comparison(best, single), "best_vs_original": comparison(best, c["baseline"]),
                "compositions": combinations, "source_result_sha256": digest(D330/(c["id"]+".result.json"))})
    finally: close_bridge(bridge)
    require(len(rows) == 27 and dual == 648 and control_dual == 243, "complete coverage")
    verify_local(m)
    report = {"experiment": ID, "complete": True, "roots": 27, "compositions": 216, "dual_days": dual,
        "control_dual_days": control_dual, "zero_validity_identity_or_ceiling_failure": True,
        "gate_passed": gate(rows), "best_vs_single": dict(Counter(r["best_vs_single"]["result"] for r in rows)),
        "all_compositions_vs_original": dict(Counter(p["versus_original"]["result"] for r in rows for p in r["compositions"])),
        "strata": {k: {str(v): dict(Counter(r["best_vs_single"]["result"] for r in rows if r[k] == v))
                    for v in sorted({r[k] for r in rows})} for k in ("family", "players")},
        "rows": rows, "manifest_sha256": digest(M), "production_change": False, "score_promotion_authority": False}
    write_new(S, report)
    print(json.dumps({k: report[k] for k in ("roots", "compositions", "dual_days", "gate_passed", "best_vs_single", "all_compositions_vs_original")}))
    print("summary_sha256="+digest(S))


if __name__ == "__main__":
    p = argparse.ArgumentParser(); p.add_argument("mode", choices=("freeze", "run")); args = p.parse_args()
    freeze() if args.mode == "freeze" else run()
