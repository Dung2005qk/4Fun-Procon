#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def parse_record(line: str) -> tuple[str, dict[str, str]]:
    parts = line.strip().split(",")
    return parts[0], dict(part.split("=", 1) for part in parts[1:] if "=" in part)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    raw = args.log.read_bytes()
    records: dict[int, dict[str, dict[str, str]]] = {}
    run_complete = 0
    for line in raw.decode("utf-8").splitlines():
        kind, fields = parse_record(line)
        if kind == "run_complete":
            run_complete += 1
            continue
        if kind not in {
            "current_exact_fuel_attribute",
            "f0_upper_attribute",
            "profile_attribute",
            "attribute",
        }:
            continue
        seed = int(fields["seed"])
        records.setdefault(seed, {})[kind] = fields

    expected = {10500000, 10500001, 10500100, 10500101}
    if set(records) != expected or run_complete != 1:
        raise SystemExit(
            f"incomplete 309 evidence: seeds={sorted(records)} run_complete={run_complete}"
        )
    required = {
        "current_exact_fuel_attribute",
        "f0_upper_attribute",
        "profile_attribute",
        "attribute",
    }
    classifications: dict[str, int] = {}
    cases: list[dict[str, object]] = []
    for seed in sorted(records):
        case = records[seed]
        if set(case) != required:
            raise SystemExit(f"seed {seed} missing records: {sorted(required - set(case))}")
        stage = case["attribute"]
        f0 = case["f0_upper_attribute"]
        profile = case["profile_attribute"]
        route_mask = stage["merged_mask"]
        master_outcome = int(stage["merged_master_outcome"])
        f0_present = int(f0["oracle_current"])
        if route_mask != "111":
            classification = "route-supply"
        elif master_outcome == 0:
            classification = "master-composition-retention"
        elif f0_present == 0:
            classification = "f0-retention"
        elif seed == 10500101:
            classification = "profile-shortlist"
        else:
            classification = "post-f0-unclassified"
        classifications[classification] = classifications.get(classification, 0) + 1
        cases.append(
            {
                "seed": seed,
                "classification": classification,
                "legacy12_mask": stage["legacy12_mask"],
                "expanded16_mask": stage["expanded16_mask"],
                "merged_mask": route_mask,
                "wide32_mask": stage["wide32_mask"],
                "wide64_mask": stage["wide64_mask"],
                "merged_master_outcome": master_outcome,
                "wide_master_outcome": int(stage["wide_master_outcome"]),
                "augmented_master_outcome": int(stage["augmented_master_outcome"]),
                "wide_augmented_outcome": int(stage["wide_augmented_outcome"]),
                "first_master_cap": int(stage["first_master_cap"]),
                "f0_current": f0_present,
                "f0_upper": int(f0["oracle_upper"]),
                "exact_certified": int(profile["exact_certified"]),
                "exact_dominates_parent": int(profile["exact_dominates_parent"]),
                "parent_dominates_exact": int(profile["parent_dominates_exact"]),
            }
        )

    summary = {
        "experiment": "ATTR-THREE-ACTIVE-PATROL-PIPELINE-309",
        "cases": len(cases),
        "run_complete": run_complete,
        "classifications": classifications,
        "case_records": cases,
        "log_sha256": hashlib.sha256(raw).hexdigest().upper(),
    }
    args.output.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
