#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path


def parse_record(line: str) -> dict[str, str]:
    fields = line.strip().split(",")
    return dict(field.split("=", 1) for field in fields[1:])


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    data = args.log.read_bytes()
    records = [
        parse_record(line)
        for line in data.decode("utf-8").splitlines()
        if line.startswith("exact_bundle_frontier_attribute,")
    ]
    run_complete = sum(
        line.startswith("run_complete,cases=4")
        for line in data.decode("utf-8").splitlines()
    )
    seeds = [int(record["seed"]) for record in records]
    recovered = [
        int(record["seed"])
        for record in records
        if int(record["master_outcome"]) == 1
    ]
    summary = {
        "experiment": "ATTR-COORDINATED-EXACT-BUNDLE-FRONTIER-310",
        "log_sha256": hashlib.sha256(data).hexdigest().upper(),
        "cases": len(records),
        "unique_seeds": len(set(seeds)),
        "run_complete_markers": run_complete,
        "off_frontier_candidates": sum(
            int(record["off_frontier_candidates"]) for record in records
        ),
        "off_frontier_bundles": sum(
            int(record["off_frontier_bundles"]) for record in records
        ),
        "on_frontier_candidates": [
            int(record["on_frontier_candidates"]) for record in records
        ],
        "on_frontier_bundles": [
            int(record["on_frontier_bundles"]) for record in records
        ],
        "oracle_masks": [record["oracle_mask"] for record in records],
        "master_exact_recoveries": sum(
            int(record["master_exact"]) for record in records
        ),
        "master_outcome_recoveries": len(recovered),
        "recovered_seeds": recovered,
        "master_nodes": [int(record["nodes"]) for record in records],
    }
    summary["gate_pass"] = (
        summary["cases"] == 4
        and summary["unique_seeds"] == 4
        and summary["run_complete_markers"] == 1
        and summary["off_frontier_candidates"] == 0
        and summary["off_frontier_bundles"] == 0
        and summary["master_outcome_recoveries"] >= 2
        and all(count in (1, 2, 3) for count in summary["on_frontier_bundles"])
    )
    args.output.write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
