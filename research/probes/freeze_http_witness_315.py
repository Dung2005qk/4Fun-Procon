"""Freeze six consumed development witnesses; do not read the sealed312 CSV."""
import csv
import json
from pathlib import Path
from run_http_baseline_314 import ROOT, digest
from run_http_witness_315 import EXPERIMENT, SEEDS, require
from summarize_http_baseline_314 import summarize


def main():
    baseline_path = ROOT / "research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json"
    baseline_dir = ROOT / "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314"
    require(digest(baseline_path) == "FE31DDCA3C0419AD2279E0C4E076AC77D030E332039998ABBAC8FF906E940CB7", "314 manifest changed")
    baseline = json.loads(baseline_path.read_text())
    report = summarize(baseline_path, baseline_dir)
    cases, setups = [], []
    paths = list(baseline["hashes"]) + [str(baseline_path.relative_to(ROOT)).replace("\\", "/"),
        "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314.summary.json",
        "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314/run_complete.json"]
    for spec, setup, result in zip(baseline["cases"], baseline["setups"], report["results"], strict=True):
        if spec["seed"] not in SEEDS:
            continue
        require(result["oracle_vs_http"] == "win", "selected case is not a consumed oracle win")
        prefix = "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314/" + str(spec["seed"])
        cases.append({**spec, "http_score": result["http_score"],
            "replay": prefix + ".replay.jsonl", "transport": prefix + ".transport.json"})
        setups.append(setup)
        paths.extend(prefix + suffix for suffix in (".replay.jsonl", ".transport.json", ".result.json",
                                                    ".replay-check.txt", ".stderr"))
    require(tuple(case["seed"] for case in cases) == SEEDS, "case set mismatch")
    for path, expected in baseline["hashes"].items():
        require(digest(ROOT / path) == expected, "314 frozen input changed: " + path)
    adapter = ROOT / "research/holdouts/ATTR-THREE-PATROL-HTTP-WITNESS-315-development.csv"
    with adapter.open("x", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(["experiment_id", "split", "family", "fuel_profile", "players", "horizon",
            "first_seed", "count", "active_agents", "total_agents", "spot_count", "role_mode", "oracle_scope"])
        for case in cases:
            writer.writerow(["CEILING-THREE-ACTIVE-PATROL-LOW-FUEL-PREVALENCE-312", "development",
                case["family"], "low", case["players"], 4, case["seed"], 1, 3, 3, 5, "all-patrol",
                "complete-three-active-patrol-roadless-full-match-dp"])
    script_names = ["run_http_witness_315.py", "summarize_http_witness_315.py",
                    "test_http_witness_315.py", "freeze_http_witness_315.py"]
    script_paths = ["research/probes/" + name for name in script_names]
    paths.extend(script_paths + [str(adapter.relative_to(ROOT)).replace("\\", "/")])
    manifest = {"experiment": EXPERIMENT, "parent": "c76a8ea", "cases": cases, "setups": setups,
        "adapter": adapter.name, "bridge_binary": baseline["bridge_binary"],
        "oracle_binary": {"path": "/home/LMC/udon312-0905/build/udonshield_multi_patrol_oracle",
            "sha256": "9348B01F5D1A48C3B2958FDDF6878518C50C4143065EE801FFD7F4BCCF1977FB"},
        "oracle_source": {"path": "/home/LMC/udon312-0905/research/probes/multi_patrol_oracle.cpp",
            "sha256": "5979B702FB175B63077CCBB250B6E55B5B974ECA80543B3B7C105123F94C1BCE"},
        "local_hashes": {path: digest(ROOT / path) for path in sorted(set(paths))},
        "deployment_hashes": {name: digest(ROOT / path) for name, path in zip(script_names, script_paths, strict=True)},
        "scope": "Consumed development-only witness recovery, not new prevalence evidence or holdout. "
                 "Original312 experiment id retained only for frozen fixture loader; incidental engine baseline ignored.",
        "gate": "All6 atomic witnesses24 dual-valid days; exact312 final score and314 fixture/replay; "
                "physical and permutation-invariant divergence with exact brand ledger; "
                "audit membership only at shared input; recurrent>=2 permits read-only attribution only.",
        "production_changes": False, "score_promotion_authority": False, "holdout_authority": False}
    manifest["deployment_hashes"][adapter.name] = digest(adapter)
    output = ROOT / ("research/holdouts/" + EXPERIMENT + ".json")
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(manifest, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps({"manifest": str(output), "sha256": digest(output), "cases": len(cases)}, indent=2))


if __name__ == "__main__":
    main()
