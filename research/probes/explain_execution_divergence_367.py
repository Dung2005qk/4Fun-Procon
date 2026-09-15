"""Derive bounded causal facts from the frozen367 inventory and completed replays."""
from collections import Counter
import json
from pathlib import Path
from audit_execution_divergence_367 import ROOT, SETS, LABELS, sha, load, verify_files

ID = "ATTR-INACTIVE-EXECUTION-DIVERGENCE-367"
INVENTORY_HASH = "D839273028262C905C88A985AA6FFAB700CEFB89E2A71EF080F128BEF4384FD8"
SCORE_KEYS = ("lifetimeDistinct", "totalDailyDistinct", "totalServings")


def score(value):
    return tuple(value[k] for k in SCORE_KEYS)


def singleton_rank_check(decision):
    """Check strictly higher eligible certified quantile, not unlogged tie fields."""
    if len(decision["manifest"]["scenarios"]) != 1:
        return {"status": "multiple-scenarios-not-reconstructible"}
    pool = [r for r in decision["audit"]["candidates"] if r["certified"]]
    assert pool
    assert all(r["finalQuantile50"] == r["finalCertifiedLowerBound"] for r in pool)
    selected = [r for r in pool if r["selected"]]
    assert len(selected) == 1
    best_confidence = max(score(r["finalCertifiedLowerBound"])[0] for r in pool)
    eligible = [r for r in pool if score(r["finalCertifiedLowerBound"])[0] >=
                best_confidence - decision["riskPolicy"]["safetySlack"]]
    reason = decision["audit"]["selectionReason"]
    if reason == "certified-undominated-current-floor":
        floor = max(score(r["scoreAfterToday"]) for r in eligible)
        eligible = [r for r in eligible if score(r["scoreAfterToday"]) == floor]
    elif reason != "certified-undominated":
        return {"status": "unhandled-selection-reason", "reason": reason}
    winner = score(selected[0]["finalQuantile50"])
    better = [r["stableId"] for r in eligible if score(r["finalQuantile50"]) > winner]
    return {"status": "strict-rank-inversion" if better else "no-strict-rank-inversion",
            "eligible": len(eligible), "selected_quantile": winner, "better_count": len(better)}


def stripped_work(decision):
    keys = ("combinationsVisited", "depthFirstCombinationsVisited", "deadlineReached", "searchComplete")
    return {"master": {k: decision["masterDiagnostics"][k] for k in keys},
            "alns": {k: decision["alns"][k] for k in ("iterations", "accepted", "synthesizedRoutes", "poolNovelRoutes")},
            "independent": {k: v for k, v in decision["audit"].items() if k.startswith("independent")}}


def run():
    path = ROOT / "research/evidence" / (ID + ".json")
    assert sha(path) == INVENTORY_HASH
    inventory = load(path)
    result = {"experiment": ID, "inventory_sha256": INVENTORY_HASH,
              "script_sha256": sha(Path(__file__)), "sets": [], "first_boundaries": [],
              "loss_trajectories": [], "singleton_checks": Counter()}
    case_docs = {}
    for name, folder, score_id, phase, count, summary_sha, complete_sha in SETS:
        copy = ROOT / "artifacts/research" / folder
        data = copy / "research/evidence" / (score_id + "-" + phase)
        assert sha(data / "run_complete.json") == complete_sha
        verify_files(data, load(data / "run_complete.json")["files"])
        cases = [c for c in load(copy / "research/holdouts" / (score_id + "-" + phase + ".json"))["cases"] if c["window_ms"] == 5000]
        for case in cases:
            for label, (repeat, side) in LABELS.items():
                prefix = data / repeat / side / str(case["seed"])
                docs = [e["body"] for e in map(json.loads, prefix.with_suffix(".replay.jsonl").read_text().splitlines()) if e["kind"] == "decision"]
                case_docs[name, case["seed"], label] = docs
                for doc in docs:
                    check = singleton_rank_check(doc["decision"])
                    result["singleton_checks"][check["status"]] += 1
                    assert check["status"] != "strict-rank-inversion", (name, case["seed"], label)
        rows = [r for r in inventory["comparisons"] if r["set"] == name]
        result["sets"].append({"name": name, "fixtures": count,
            "AB": dict(Counter(r["comparison_B_vs_A"] for r in rows if r["kind"].startswith("AB"))),
            "AA": dict(Counter(r["comparison_B_vs_A"] for r in rows if r["kind"].startswith("AA"))),
            "AB_trajectory_differences": sum(not r["trajectory_equal"] for r in rows if r["kind"].startswith("AB")),
            "AA_trajectory_differences": sum(not r["trajectory_equal"] for r in rows if r["kind"].startswith("AA"))})
    for row in inventory["differences"]:
        if row["first_day"] == 0:
            continue
        labels = (row["left"], row["right"])
        docs = [case_docs[row["set"], row["seed"], l][row["first_day"] - 1] for l in labels]
        detail = {k: row[k] for k in ("set", "seed", "kind", "first_day", "boundary", "delta", "state_equal", "ledger_equal", "post_F0_pool_equal")}
        detail["manifest_equal"] = docs[0]["decision"]["manifest"] == docs[1]["decision"]["manifest"]
        detail["work"] = {l: stripped_work(doc["decision"]) for l, doc in zip(labels, docs)}
        detail["scenario_counts"] = [len(d["decision"]["manifest"]["scenarios"]) for d in docs]
        detail["selected_membership"] = [row["left_selected_in_right"]["boundary"], row["right_selected_in_left"]["boundary"]]
        detail["selected_fuel_reserve"] = {l: d["decision"]["candidate"]["terminalSlack"]["patrolFuelReserve"] for l, d in zip(labels, docs)}
        detail["selected_f0_scores_in_both_runs"] = {}
        for label, d in zip(labels, docs):
            sid = d["decision"]["candidate"]["stableId"]
            detail["selected_f0_scores_in_both_runs"][label] = [
                [r["provisionalLowerBound"] for r in doc["decision"]["audit"]["candidates"] if r["stableId"] == sid]
                for doc in docs]
        result["first_boundaries"].append(detail)
        if row["set"] == "366HOLDOUT" and row["kind"] == "AB_B" and row["comparison_B_vs_A"] == "loss":
            trajectory = {"seed": row["seed"], "days": []}
            for day, (a, b) in enumerate(zip(row["all_days"][labels[0]], row["all_days"][labels[1]]), 1):
                trajectory["days"].append({"day": day,
                    "main_plan_equal": a["main_plan_hash"] == b["main_plan_hash"],
                    "submitted_plan_equal": a["submitted_plan_hash"] == b["submitted_plan_hash"],
                    "ledger": [a["validated"]["ledger"], b["validated"]["ledger"]],
                    "main_scores": [a["main_score"], b["main_score"]],
                    "protected_scores": [[x["protected"]["refinedDailyDistinct"], x["protected"]["refinedServings"]] for x in (a, b)],
                    "agents_equal": a["validated"]["agents"] == b["validated"]["agents"],
                    "road_footprints_equal": a["validated"]["road_footprint"] == b["validated"]["road_footprint"]})
            result["loss_trajectories"].append(trajectory)
    result["source_hashes"] = {s: sha(ROOT / s) for s in (
        "src/decision.cpp", "src/slack_refiner.cpp", "src/planner.cpp", "include/udon/planner.hpp",
        "artifacts/research/357/completed/source/src/decision.cpp",
        "artifacts/research/357/completed/source/src/slack_refiner.cpp")}
    assert (ROOT / "src/decision.cpp").read_text() == (ROOT / "artifacts/research/357/completed/source/src/decision.cpp").read_text()
    canonical_refiner = (ROOT / "src/slack_refiner.cpp").read_text()
    research_refiner = (ROOT / "artifacts/research/357/completed/source/src/slack_refiner.cpp").read_text()
    resource_boundary = "ResourceMarginalResult ProtectedSlackRefiner::refine_resource_marginal("
    assert research_refiner.count(resource_boundary) == 1
    unchanged_prefix = research_refiner.split(resource_boundary)[0]
    assert canonical_refiner == unchanged_prefix + "} // namespace udon\n"
    result["source_equivalence_scope"] = "decision.cpp exact normalized text; complete canonical slack_refiner prefix before added research resource method"
    out = ROOT / "research/evidence" / (ID + "-explanation.json")
    with out.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps(result["sets"]))
    print(json.dumps(result["singleton_checks"]))
    print("sha256", sha(out))
    for row in result["loss_trajectories"]:
        print(json.dumps(row))


if __name__ == "__main__":
    run()
