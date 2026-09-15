"""Full-run oracle-versus-recorded-HTTP attribution. No solver or policy change."""
import argparse
from collections import Counter
import json
from pathlib import Path

from run_http_witness_315 import EXPERIMENT, SEEDS, digest, parse_case, require
from run_http_baseline_314 import ROOT, Bridge, EMPTY_LEDGER
from summarize_http_baseline_314 import safety


def ledger_key(ledger):
    return (tuple(sorted(ledger["brands"])), ledger["totalDailyDistinct"], ledger["totalServings"])


def state_key(agents, ledger, unordered=False):
    values = [(agent["kind"], agent["pos"], agent["fuel"]) for agent in agents]
    return tuple(sorted(values) if unordered else values), ledger_key(ledger)


def audit_classification(matches):
    if not matches:
        return "absent-from-recorded-audit"
    if any(row["selected"] for row in matches):
        return "selected-before-later-divergence"
    if any(row["certified"] for row in matches):
        return "certified-but-unselected"
    if all(row["disposition"] == "not-shortlisted" for row in matches):
        return "present-but-not-shortlisted"
    return "shortlisted-but-uncertified"


def score_list(value):
    return [value[key] for key in ("lifetimeDistinct", "totalDailyDistinct", "totalServings")]


def candidates_matching(bridge, setup, state, ledger, target, candidates):
    matches, unordered_matches = [], []
    target_key = state_key(target["agents"], target["ledger"])
    unordered_key = state_key(target["agents"], target["ledger"], True)
    for candidate in candidates:
        if score_list(candidate["scoreAfterToday"]) != target["score"]:
            continue
        agents = [{"kind": original["kind"], "pos": cell, "fuel": fuel}
                  for original, cell, fuel in zip(state["agents"], candidate["terminalCells"],
                                                  candidate["terminalFuel"], strict=True)]
        if sorted((a["kind"], a["pos"], a["fuel"]) for a in agents) != list(unordered_key[0]):
            continue
        # Audit scores do not carry exact lifetime brand identity. Revalidate
        # the recorded plan rather than accepting equal cardinality as equality.
        checked = bridge.request({"op": "step", "setup": setup, "state": state,
                                  "ledger": ledger, "plan": json.loads(candidate["stableId"])})
        require(checked.get("ok") and checked.get("agrees"), "matching audited plan invalid")
        require(checked["agents"] == agents and checked["score"] == target["score"],
                "audit outcome differs from exact replay")
        if state_key(checked["agents"], checked["ledger"], True) == unordered_key:
            unordered_matches.append(candidate)
        if state_key(checked["agents"], checked["ledger"]) == target_key:
            matches.append(candidate)
    return matches, unordered_matches


def analyze_case(spec, setup, traces, replay_path, transport_path, bridge):
    telemetry = safety(replay_path)
    events = [json.loads(line) for line in replay_path.read_text().splitlines() if line]
    decisions = [event["body"] for event in events if event["kind"] == "decision"]
    transport = json.loads(transport_path.read_text())
    require(transport["setup"] == setup and transport["failure"] is None and
            len(transport["actions"]) == 4, "frozen transport/fixture mismatch")
    generated = bridge.request({"op": "fixture", **spec})
    require(generated.get("ok") and generated["setup"] == setup, "fixture generator mismatch")
    agents = [{"kind": 0, "pos": cell, "fuel": setup["fuelLimits"]} for cell in setup["agents"]]
    ledger = dict(EMPTY_LEDGER)
    http_agents, http_ledger = agents, ledger
    days = []
    first_physical = first_unordered = None
    for trace, action, decision in zip(traces, transport["actions"], decisions, strict=True):
        day = trace["day"]
        require(action["wire_day"] == day - 1 and action["state"]["day"] == day - 1,
                "HTTP day identity mismatch")
        require(state_key(action["state"]["agents"], http_ledger) == state_key(http_agents, http_ledger),
                "HTTP transition chain mismatch")
        require(action["state"]["others"] == [] and action["state"]["traffics"] == [],
                "non-roadless/non-isolated input cannot use this witness replay")
        shared = state_key(agents, ledger) == state_key(http_agents, http_ledger)
        unordered_shared = state_key(agents, ledger, True) == state_key(http_agents, http_ledger, True)
        state = {"day": day, "endsAt": action["state"]["endsAt"] // 1000,
                 "agents": agents, "others": [], "traffics": []}
        oracle = bridge.request({"op": "step", "setup": setup, "state": state,
                                 "ledger": ledger, "plan": trace["plan"]})
        require(oracle.get("ok") and oracle.get("agrees"), "oracle witness failed dual validation")
        require(oracle["score"] == trace["score"], "oracle cumulative trace differs from exact replay")
        http_state = dict(action["state"], day=day, endsAt=state["endsAt"])
        http = bridge.request({"op": "step", "setup": setup, "state": http_state,
                               "ledger": http_ledger, "plan": action["plan"]})
        require(http == action["validated"], "recorded HTTP outcome changed under dual replay")
        same = state_key(oracle["agents"], oracle["ledger"]) == state_key(http["agents"], http["ledger"])
        unordered_same = state_key(oracle["agents"], oracle["ledger"], True) == state_key(http["agents"], http["ledger"], True)
        if not same and first_physical is None:
            first_physical = day
        if not unordered_same and first_unordered is None:
            first_unordered = day
        audit_shared = (shared and decision["state"]["day"] == day and
            state_key(decision["state"]["agents"], decision["ledger"]) == state_key(agents, ledger))
        matches, permutation_matches = [], []
        if audit_shared:
            matches, permutation_matches = candidates_matching(bridge, setup, state, ledger, oracle,
                                                               decision["decision"]["audit"]["candidates"])
        days.append({"day": day, "shared_physical_input": shared,
            "shared_permutation_invariant_input": unordered_shared, "shared_decision_input": audit_shared,
            "same_output": same, "same_unordered_output": unordered_same,
            "oracle_score": oracle["score"], "http_score": http["score"],
            "oracle_agents": oracle["agents"], "http_agents": http["agents"],
            "oracle_ledger": oracle["ledger"], "http_ledger": http["ledger"],
            "oracle_plan": trace["plan"], "submitted_http_plan": action["plan"],
            "audit_class": audit_classification(matches) if audit_shared else "non-shared-input-not-attributable",
            "matching_audit_outcomes": matches, "permuted_matching_audit_outcomes": permutation_matches,
            "selection_reason": decision["decision"]["audit"]["selectionReason"]})
        agents, ledger = oracle["agents"], oracle["ledger"]
        http_agents, http_ledger = http["agents"], http["ledger"]
    require(days[-1]["oracle_score"] == spec["oracle_score"] and
            days[-1]["http_score"] == spec["http_score"], "final reference mismatch")
    require(first_physical is not None, "expected residual has no divergence")
    boundary = days[first_physical - 1]
    return {**spec, "first_physical_divergence": first_physical,
        "first_permutation_invariant_divergence": first_unordered,
        "boundary_class": boundary["audit_class"], "days": days, "http_telemetry": telemetry}


def summarize(manifest_path, directory):
    manifest = json.loads(manifest_path.read_text())
    require(manifest["experiment"] == EXPERIMENT and tuple(s["seed"] for s in manifest["cases"]) == SEEDS,
            "wrong315 manifest")
    for path, expected in manifest["local_hashes"].items():
        require(digest(ROOT / path) == expected, "frozen input changed: " + path)
    marker = json.loads((directory / "run_complete.json").read_text())
    log = directory / "witnesses.log"
    require(marker == {"cases": 6, "traces": 24, "manifest_sha256": digest(manifest_path),
                        "log_sha256": digest(log)}, "completion marker mismatch")
    require(len(list(directory.glob("*.result"))) == 6 and not list(directory.glob("*.partial")),
            "incomplete result inventory")
    raw = log.read_text()
    require(raw.count("\ncase_complete,") == 6 and raw.endswith("run_complete,cases=6,traces=24\n"),
            "combined log not complete")
    expected_log = ""
    parsed = []
    for spec in manifest["cases"]:
        text = (directory / (str(spec["seed"]) + ".result")).read_text()
        require((directory / (str(spec["seed"]) + ".stderr")).stat().st_size == 0, "nonempty case stderr")
        parsed.append(parse_case(text, spec))
        expected_log += text.rstrip() + "\ncase_complete,seed=" + str(spec["seed"]) + "\n"
    require(raw == expected_log + "run_complete,cases=6,traces=24\n", "combined/per-case mismatch")
    bridge = Bridge(ROOT / manifest["bridge_binary"])
    try:
        results = [analyze_case(spec, setup, traces, ROOT / spec["replay"], ROOT / spec["transport"], bridge)
                   for spec, setup, traces in zip(manifest["cases"], manifest["setups"], parsed, strict=True)]
    finally:
        bridge.close()
        bridge.process.stdout.close()
        bridge.process.stderr.close()
    boundaries = Counter(result["boundary_class"] for result in results)
    recurrent = {key: value for key, value in boundaries.items()
                 if value >= 2 and key != "non-shared-input-not-attributable"}
    return {"experiment": EXPERIMENT, "cases": 6, "oracle_days_dual_validated": 24,
        "http_days_dual_revalidated": 24, "manifest_sha256": digest(manifest_path),
        "log_sha256": digest(log), "boundary_counts": dict(boundaries), "recurrent_boundaries": recurrent,
        "results": results, "next_read_only_source_attribution_authorized": bool(recurrent),
        "score_successor_authorized": False, "holdout_authority": False, "btc_authority": False,
        "limits": "Consumed six known wins only; no prevalence, promotion or local latency inference. "
                  "Final audit absence does not distinguish route supply from master retention. "
                  "No comparison of candidates across different input states. Incidental engine traces ignored."}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = summarize(args.manifest, args.directory)
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps({key: value for key, value in report.items() if key != "results"}, indent=2))


if __name__ == "__main__":
    main()
