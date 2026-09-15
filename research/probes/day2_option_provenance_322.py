"""Full recorded day2 pool attribution. No planning or oracle invocation."""
import argparse
from collections import Counter
import json
from http_prefix_option_loss_321 import ROOT, load, digest, require, write_new, close_bridge, state
from run_http_baseline_314 import Bridge
from summarize_http_witness_315 import state_key, score_list, audit_classification

ID = "ATTR-DAY2-OPTION-PROVENANCE-322"
M = ROOT / f"research/holdouts/{ID}.json"
S = ROOT / f"research/evidence/{ID}.summary.json"
M321 = "research/holdouts/ATTR-HTTP-PREFIX-OPTION-LOSS-321.json"
S321 = "research/evidence/ATTR-HTTP-PREFIX-OPTION-LOSS-321.summary.json"
D321 = "research/evidence/ATTR-HTTP-PREFIX-OPTION-LOSS-321"
D314 = "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314"

def permutation(source, target):
    def key(a): return a["kind"], a["pos"], a["fuel"]
    require(Counter(map(key, source)) == Counter(map(key, target)), "not permutation equivalent")
    unused = list(range(len(target)))
    mapping = []
    for a in source:
        index = next(i for i in unused if key(a) == key(target[i]))
        unused.remove(index)
        mapping.append(index)
    return mapping

def freeze():
    parent, summary = load(ROOT / M321), load(ROOT / S321)
    require(summary["manifest_sha256"] == digest(ROOT / M321) and summary["roots"] == 36
            and summary["zero_failure"] and summary["monotonicity_pass"], "321 incomplete")
    for path, expected in parent["local_hashes"].items():
        require(digest(ROOT / path) == expected, "321 input drift: " + path)
    paths = set(parent["local_hashes"]) | {M321, S321, D321 + "/run_complete.json",
        "research/probes/day2_option_provenance_322.py", "research/probes/test_day2_option_provenance_322.py",
        "research/probes/summarize_http_witness_315.py", "research/probes/run_http_witness_315.py"}
    matches = {r["seed"]: r for r in summary["matches"]}
    cases = []
    for root in parent["cases"]:
        if root["after_day"] != 1:
            continue
        seed = root["seed"]
        result_path = f"{D321}/{seed}-d1.result.json"
        require(digest(ROOT / result_path) == summary["result_hashes"][f"{seed}-d1.result.json"], "321 result hash")
        paths.add(result_path)
        oracle = load(ROOT / result_path)["oracle"]
        replay = ROOT / f"{D314}/{seed}.replay.jsonl"
        bodies = [json.loads(line)["body"] for line in replay.read_text().splitlines()
                  if json.loads(line)["kind"] == "decision"]
        require(len(bodies) == 4, "day2 replay coverage")
        body = bodies[1]
        request = root["request"]
        require(body["state"]["day"] == 2 and state_key(body["state"]["agents"], body["ledger"]) ==
                state_key(request["state"]["agents"], request["ledger"]), "actual day2 root mismatch")
        scenario = body["decision"]["manifest"]["scenarios"]
        require(len(scenario) == 1 and scenario[0]["class"] == "deterministic-no-road"
                and scenario[0]["weight"] == 10000 and scenario[0]["jointFeasible"], "scope mismatch")
        audit = body["decision"]["audit"]["candidates"]
        require(sum(c["selected"] for c in audit) == 1 and len({c["stableId"] for c in audit}) == len(audit), "pool identity")
        selected = next(c for c in audit if c["selected"])
        require(selected["stableId"] == body["decision"]["candidate"]["stableId"] and
                json.loads(selected["stableId"]) == body["decision"]["candidate"]["plan"], "selected provenance")
        action = load(ROOT / f"{D314}/{seed}.transport.json")["actions"][1]
        cases.append({"seed": seed, "family": root["family"], "players": root["players"],
            "request": request, "oracle": oracle, "audit": audit, "submitted": action,
            "selected_id": selected["stableId"], "option_loss": matches[seed]["days"][1],
            "conditioned_values": matches[seed]["conditional_values"],
            "selection_reason": body["decision"]["audit"]["selectionReason"]})
    require(len(cases) == 12, "root count")
    write_new(M, {"experiment": ID, "cases": cases, "parent": "c76a8ea", "bridge": parent["bridge"],
        "hashes": {p: digest(ROOT / p) for p in sorted(paths)},
        "gate": "All12 actual day2 roots and all audited candidates dual-valid. Two loss roots across2 families "
                "with optimal-equivalent outcomes permit ranking/certification source attribution only.",
        "limitation": "Audit contains post-F0-admission candidates only. A single optimal witness does not enumerate all optimal outcomes.",
        "score_successor_authorized": False, "production_change": False})
    print(json.dumps({"manifest_sha256": digest(M), "roots": 12, "candidates": sum(len(c["audit"]) for c in cases)}))

def analyze(case, bridge):
    request, target = case["request"], case["oracle"]["days"][0]
    require(target["day"] == 2, "optimal current day")
    oracle_step = bridge.request({"op": "step", **request, "plan": target["plan"]})
    require(oracle_step["ok"] and oracle_step["agrees"] and
            all(oracle_step[k] == target[k] for k in ("agents", "ledger", "score")), "oracle first step mismatch")
    submitted = bridge.request({"op": "step", **request, "plan": case["submitted"]["plan"]})
    require(submitted == case["submitted"]["validated"], "submitted action mismatch")
    physical, unordered, rows = [], [], []
    suffix_checks = 0
    for index, candidate in enumerate(case["audit"]):
        checked = bridge.request({"op": "step", **request, "plan": json.loads(candidate["stableId"])})
        require(checked["ok"] and checked["agrees"] and checked["score"] == score_list(candidate["scoreAfterToday"])
                and [a["pos"] for a in checked["agents"]] == candidate["terminalCells"]
                and [a["fuel"] for a in checked["agents"]] == candidate["terminalFuel"], "audited candidate mismatch")
        same = state_key(checked["agents"], checked["ledger"]) == state_key(target["agents"], target["ledger"])
        permuted = state_key(checked["agents"], checked["ledger"], True) == state_key(target["agents"], target["ledger"], True)
        if same:
            physical.append(candidate)
        if permuted:
            unordered.append(candidate)
            mapping = permutation(target["agents"], checked["agents"])
            agents, ledger = checked["agents"], checked["ledger"]
            for day in case["oracle"]["days"][1:]:
                plan = [None] * 3
                expected_agents = [None] * 3
                for source, dest in enumerate(mapping):
                    plan[dest], expected_agents[dest] = day["plan"][source], day["agents"][source]
                step = bridge.request({"op": "step", "setup": request["setup"], "state": state(agents, day["day"]),
                                       "ledger": ledger, "plan": plan})
                require(step["ok"] and step["agrees"] and step["agents"] == expected_agents
                        and step["ledger"] == day["ledger"] and step["score"] == day["score"], "mapped optimum suffix invalid")
                agents, ledger = step["agents"], step["ledger"]
                suffix_checks += 1
        rows.append({"index": index, "candidate": candidate, "actual": checked,
                     "physical_optimal_witness_match": same, "permutation_optimal_witness_match": permuted})
    return {"seed": case["seed"], "family": case["family"], "players": case["players"],
        "actual_day2_loss": case["option_loss"], "oracle_total": case["oracle"]["score"],
        "physical_class": audit_classification(physical), "permutation_class": audit_classification(unordered),
        "physical_match_count": len(physical), "permutation_match_count": len(unordered),
        "selected_matches_submitted": json.loads(case["selected_id"]) == case["submitted"]["plan"],
        "oracle_day2": oracle_step, "submitted_day2": submitted,
        "mapped_suffix_dual_days": suffix_checks, "candidates": rows}

def run():
    m = load(M)
    for path, expected in m["hashes"].items():
        require(digest(ROOT / path) == expected, "frozen hash drift: " + path)
    bridge = Bridge(ROOT / m["bridge"])
    try:
        results = [analyze(case, bridge) for case in m["cases"]]
    finally:
        close_bridge(bridge)
    affected = [r for r in results if r["actual_day2_loss"]["first_tier"]]
    matching = [r for r in affected if r["permutation_match_count"]]
    gate = len(matching) >= 2 and len({r["family"] for r in matching}) >= 2
    report = {"experiment": ID, "manifest_sha256": digest(M), "roots": len(results), "results": results,
        "audited_candidates": sum(len(r["candidates"]) for r in results), "zero_failure": True,
        "mapped_suffix_dual_days": sum(r["mapped_suffix_dual_days"] for r in results),
        "loss_root_physical_classes": dict(Counter(r["physical_class"] for r in affected)),
        "loss_root_permutation_classes": dict(Counter(r["permutation_class"] for r in affected)),
        "ranking_source_attribution_authorized": gate, "score_successor_authorized": False,
        "strata": {field: {key: dict(Counter(r["permutation_class"] for r in results if str(r[field]) == key))
                           for key in sorted({str(r[field]) for r in results})} for field in ("family", "players")},
        "limitation": m["limitation"], "production_change": False}
    write_new(S, report)
    print(json.dumps({k: v for k, v in report.items() if k not in ("results", "strata")}, indent=2))
    print("summary_sha256 " + digest(S))

if __name__ == "__main__":
    p = argparse.ArgumentParser();p.add_argument("mode", choices=("freeze", "run"))
    freeze() if p.parse_args().mode == "freeze" else run()
