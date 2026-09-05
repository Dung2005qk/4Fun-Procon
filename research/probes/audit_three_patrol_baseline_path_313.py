"""Read-only attribution of the frozen 312 comparison boundary.

This checks named source spans, not C++ execution or score equivalence. The
frozen hashes make every lexical assertion specific to the reviewed snapshot.
"""
import argparse
import hashlib
import json
from pathlib import Path


def sha256(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest().upper()


def span(text: str, start: str, end: str) -> str:
    first = text.index(start)
    return text[first:text.index(end, first + len(start))]


def location(text: str, needle: str) -> int:
    return text.count("\n", 0, text.index(needle)) + 1


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def analyze(texts: dict[str, str], summary: dict) -> dict:
    oracle_path = "research/probes/multi_patrol_oracle.cpp"
    oracle = texts[oracle_path]
    runtime = texts["src/runtime.cpp"]
    host = texts["src/btc_main.cpp"]
    header = texts["include/udon/decision.hpp"]
    decision = texts["src/decision.cpp"]
    baseline = span(oracle, "[[nodiscard]] HeadResult solve_head(",
                    "[[nodiscard]] std::int32_t first_tier(")
    ctor = span(runtime, "MatchSession::MatchSession(",
                "std::vector<RoleAssignment> MatchSession::select_roles_until(")
    direct_constructor = span(baseline, "udon::UdonShieldEngine engine(",
                              "const udon::ExactStepSimulator")
    compact_ctor = "".join(direct_constructor.split())
    require("fixture.config,{},{},udon::RoutePoolSearch::SinglePass,"
            "kHarvestMode,false,kFutureHarvestMode" in compact_ctor,
            "312 direct constructor changed; re-review its policy and calibration")
    require("harvestExtensionMode,true,futureHarvestExtensionMode" in
            "".join(ctor.split()), "MatchSession current-floor policy changed")
    require("requireUndominatedCurrentFloor = false" in header and
            "certifiedPool,\n        requireUndominatedCurrentFloor_" in decision,
            "current-floor argument no longer traces to final selection")
    require("networkFloor{50}" in header and "networkPercent = 10" in header
            and "certificationPercent = 10" in header,
            "default calibration changed")
    calibration = span(host, "udon::DeadlineCalibration btc_http_deadline_calibration()",
                       "class ReplayWriter")
    require("networkFloor = std::chrono::milliseconds{1600}" in calibration
            and "networkPercent = 20" in calibration
            and "certificationPercent = 20" in calibration,
            "HTTP calibration changed")
    require("engine.solve_day(state, ledger, kProductionBudget)" in baseline
            and "engine.record_submitted(decision, elapsed)" in baseline,
            "312 planning/ACK path changed")
    require("refine_" not in baseline and "MatchSession" not in baseline,
            "312 now has a checkpoint path; audit needs revision")
    require("const HeadResult head = solve_head(fixture);" in oracle,
            "312 callsite changed")
    require("$binary\" --manifest \"$manifest\" --split development --only-seed" in
            texts["research/probes/run_three_active_patrol_prevalence_312_vm.sh"],
            "frozen runner invocation changed")
    require("protected-head" not in
            texts["research/probes/run_three_active_patrol_prevalence_312_vm.sh"],
            "runner requests a different path")
    main_protected = span(oracle, "if (fixture.adversarialTraffic) {\n                    if (options.protectedHead)",
                          "if (options.headOnly)")
    require("run_current_protected_against_policy" in main_protected,
            "protected-head branch needs a new scope audit")
    checks = [
        ("refine_wait_detours", "checkpoint WAIT detour refinement"),
        ("refine_midday_chains", "checkpoint midday chain and target-terminal refinement"),
        ("refine_terminal_sparse", "checkpoint terminal sparse/pair refinement"),
        ("planningState.agents = *virtualAgents", "virtual-parent planning state"),
        ("session.acknowledge_submitted(responseTime)", "session ACK lifecycle"),
        ("prospectiveCheckpointLedger", "checkpoint ledger and admission certificate"),
    ]
    for needle, description in checks:
        require(needle in host, f"missing reviewed HTTP path: {description}")
    require("slackRefiner.enableTerminalPairExchange = true" in host and
            "slackRefiner.enableMiddayChainAdoption = true" in host and
            "slackRefiner.enableMiddayTargetTerminalFollowup = true" in host,
            "accepted checkpoint mechanisms changed")
    require(summary["cases"] == 12 and summary["zero_invalid"],
            "312 evidence incomplete or invalid")
    return {
        "source_checks_passed": True,
        "baseline": {
            "observed": "direct UdonShieldEngine with default calibration and current-floor=false",
            "claimed_in_312": "complete accepted HTTP258 protected 5000-ms checkpoint",
            "same_runtime_policy": False,
            "complete_checkpoint_measured": False,
            "production_residual_gap": "unmeasured",
            "score_equivalence": "not established; no new score run in this audit",
        },
        "differences": [
            {"boundary": "current-floor selection", "probe": False,
             "http_session": True,
             "probe_line": location(oracle, "[[nodiscard]] HeadResult solve_head("),
             "runtime_line": location(runtime, "MatchSession::MatchSession("),
             "consumer_line": location(decision, "const std::size_t selected = comparator_.choose(")},
            {"boundary": "deadline calibration",
             "probe": {"network_floor_ms": 50, "network_percent": 10, "certification_percent": 10},
             "http": {"network_floor_ms": 1600, "network_percent": 20, "certification_percent": 20},
             "http_line": location(host, "udon::DeadlineCalibration btc_http_deadline_calibration()")},
            {"boundary": "checkpoint refinement and state lifecycle", "probe": "absent",
             "http": [{"mechanism": description, "line": location(host, needle)}
                      for needle, description in checks]},
        ],
        "protected_head_flag": "only dispatched for adversarialTraffic fixtures; not a repair for the three-Patrol runner",
        "domain_limit": "fixed roadless 8x8 corridor, three all-Patrol roles; six family labels vary brands/stock and day steps, not map topology; player count does not exercise road traffic here",
        "preserved_312_oracle_vs_engine": {
            "wtl": summary["oracle_head_wtl"],
            "first_tiers": summary["first_open_tiers"],
            "serving_gains": [item["gain"] for item in summary["wins"]],
            "zero_invalid": summary["zero_invalid"],
        },
        "verdict": "confirmed-baseline-path-mismatch",
        "score_successor_authorized": False,
        "holdout_open_authorized": False,
        "next_gate": "Revalidate every consumed 312 case on a frozen full checkpoint policy, explicitly accounting for calibration, current-floor policy, all accepted refinement, virtual state, ACK and post-ACK scheduling. Only surviving feasible gaps may drive a new causal mechanism. Keep fixed-role synthetic replay distinct from live BTC/role-selection evidence.",
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    manifest_raw = args.manifest.read_bytes()
    manifest = json.loads(manifest_raw)
    texts: dict[str, str] = {}
    for relative, expected in manifest["inputs"].items():
        raw = (root / relative).read_bytes()
        require(sha256(raw) == expected, f"frozen hash mismatch: {relative}")
        texts[relative] = raw.decode("utf-8-sig").replace("\r\n", "\n")
    summary_path = next(p for p in texts if p.endswith(".summary.json"))
    report = analyze(texts, json.loads(texts[summary_path]))
    report.update({
        "experiment": manifest["experiment"],
        "parent": manifest["parent"],
        "manifest_sha256": sha256(manifest_raw),
        "audit_script_sha256": sha256(Path(__file__).read_bytes()),
        "inputs": manifest["inputs"],
    })
    with args.output.open("x", encoding="utf-8", newline="\n") as output:
        output.write(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps({k: report[k] for k in (
        "experiment", "source_checks_passed", "verdict", "baseline",
        "score_successor_authorized", "holdout_open_authorized")}, indent=2))


if __name__ == "__main__":
    main()
