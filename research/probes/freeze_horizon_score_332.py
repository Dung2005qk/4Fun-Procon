"""Freeze SCORE332 splits before candidate edits; never execute held-out cases."""
import json
from http_prefix_option_loss_321 import ROOT, load, digest, write_new, require, close_bridge
from run_http_baseline_314 import Bridge
from pool_boundary_value_327 import verify_local

ID = "SCORE-W1-HORIZON-RESOURCE-PRICING-332"
FAMILIES = ("three-balanced", "three-duplicate", "three-stock", "three-coverage", "three-fuel", "three-terminal")


def main():
    prior_path = ROOT/"research/holdouts/ATTR-W1-HORIZON-PRICING-PREVALENCE-331.json"
    prior = load(prior_path); verify_local(prior)
    summary_path = ROOT/"research/evidence/ATTR-W1-HORIZON-PRICING-PREVALENCE-331.summary.json"
    require(digest(summary_path) == "66C50E076C00109BABD5E42A122EDD8804F1102C763F2FC36710F023982307BB", "331 summary identity")
    summary = load(summary_path)
    require(summary["gate_passed"] and summary["zero_failure"] and summary["complete"], "331 not qualified")
    bridge = Bridge(ROOT/prior["bridge_binary"])
    splits = {}
    try:
        for phase, base, count in (("development", 202609071000, 4), ("holdout", 202609072000, 9)):
            cases = []
            for f, family in enumerate(FAMILIES):
                for r in range(count):
                    spec = {"seed": base+100*f+r, "family": family, "players": 8+(f+r)%3}
                    fixture = bridge.request({"op": "fixture", **spec})
                    require(fixture.get("ok") and fixture["setup"]["daySeconds"] == [5]*4, "fixture contract")
                    cases.append({"spec": spec, "setup": fixture["setup"],
                                  "order": ["parent", "candidate"] if (f+r)%2 == 0 else ["candidate", "parent"]})
            path = ROOT/f"research/holdouts/{ID}-{phase}.json"
            write_new(path, {"experiment": ID, "phase": phase, "cases": cases,
                "authority": "Synthetic roadless fixed three-Patrol causal score screen; not complete protected matrix or BTC."})
            splits[phase] = {"path": str(path.relative_to(ROOT)).replace("\\", "/"), "sha256": digest(path), "pairs": len(cases)}
    finally:
        close_bridge(bridge)
    paths = {str(prior_path.relative_to(ROOT)).replace("\\", "/"),
             str(summary_path.relative_to(ROOT)).replace("\\", "/"),
             "research/probes/freeze_horizon_score_332.py", "CMakeLists.txt",
             "research/probes/run_http_baseline_314.py", "research/probes/summarize_http_baseline_314.py",
             prior["btc_binary"], prior["bridge_binary"], "build-release/udon_shield.lib"}
    paths |= {str(p.relative_to(ROOT)).replace("\\", "/") for folder in ("src", "include", "tests", "strategies/blank_slate")
              for p in (ROOT/folder).rglob("*") if p.suffix in (".cpp", ".hpp")}
    manifest = {"experiment": ID, "parent_commit": "c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c",
        "splits": splits, "parent_hashes": {p: digest(ROOT/p) for p in sorted(paths)},
        "parent_btc_binary": prior["btc_binary"], "bridge_binary": prior["bridge_binary"],
        "mechanism": "Bounded one-Patrol whole-horizon best response inside completed W1; same canonical sparse day search; all other witness plans fixed; independent agents from original witness; strict complete dual-valid improvement only.",
        "proof_domain": "All Patrol, no roads, existing sparse numerical domain with at most32 spots; no seed/family/opponent routing. Existing full W1 unchanged elsewhere.",
        "limits_per_witness": {"wall_ms": 100, "settled_labels": 32768, "memo_states": 8192,
            "dp_transitions": 262144, "stored_plan_actions": 262144,
            "created_day_labels": 196609, "day_queries": 1024,
            "deadline": "minimum of existing candidate certification deadline and invocation+100ms; no main/role/network/public-window budget increase"},
        "exhaustion": "Retain complete original W1 byte-identical; no partial memo witness or invalid plan; no optimum claim. Charge search, allocation, reconstruction and validation, not only settled labels.",
        "development_gate": "All24 paired matches complete with4ACK3transitions per side, zero safety. Candidate must improve actual final official score on at least4 activated pairs across at least2 families; gains exceed losses, total tier3 gain at least twice total tier3 loss; no tier1/2 loss; worst tier3 loss at most1; no family has negative net tier3; inactive cutoff differences are excluded from claimed mechanism benefit and must be separately investigated. Full tails and all cases reported.",
        "holdout_gate": "Open once only after development qualifies and candidate/binary/runner/metrics frozen. All54 pairs complete, zero safety; actual final-score gains on at least6 activated pairs across at least3 families, gains exceed losses, tier3 gains at least twice tier3 losses, zero tier1/2 loss, worst tier3 loss at most1, no family negative net tier3. No threshold/cap/seed/stratum changes after opening.",
        "protected_gates": "Before any promotion require full fresh parent/lane-champion protected matrix: 5000ms main, registered10000/15000 public windows, low/default/high fuel, generated8 and BTC-like32 maps, fixed/native roles,4/5/10days,6 traffic families with8/9/10teams, exact simulator/validator; inactive paths preserve semantics. Freeze detailed protected inputs before candidate measurement. Full production integration equivalence, tests and real BTC target-host lifecycle/deadline/telemetry required. Synthetic HTTP timing has no performance authority.",
        "stage": "splits-frozen-before-candidate; candidate runner and protected manifest not frozen; no measured SCORE run authorized yet",
        "production_change": False, "sealed_holdout_opened": False}
    path = ROOT/f"research/holdouts/{ID}.json"; write_new(path, manifest)
    print(json.dumps({"manifest_sha256": digest(path), "splits": splits}, indent=2))


if __name__ == "__main__":
    main()
