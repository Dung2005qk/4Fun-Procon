#!/usr/bin/env python3
import argparse
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path


TIMERS = (
    "master_dfs_branch_us",
    "master_dfs_plan_us",
    "master_dfs_canonical_us",
    "master_dfs_dedup_us",
    "master_dfs_credit_us",
    "master_dfs_sim_us",
    "master_dfs_reservation_us",
    "master_dfs_validate_us",
    "master_dfs_stock_us",
    "master_dfs_finalize_us",
    "master_dfs_population_us",
)
COUNTERS = (
    "master_dfs_combinations",
    "master_branch_calls",
    "master_valid",
    "master_duplicates",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def fields(line: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for token in line.split(",")[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            result[key] = value
    return result


def blank() -> dict[str, int]:
    return {key: 0 for key in ("master_dfs_us", *TIMERS, *COUNTERS)}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    manifest = Path(args.manifest)
    log = Path(args.log)
    rows = list(csv.DictReader(manifest.open(encoding="utf-8-sig")))
    expected_cases = sum(int(row["count"]) for row in rows)
    case_meta: dict[int, tuple[str, str, int]] = {}
    for row in rows:
        for offset in range(int(row["count"])):
            case_meta[int(row["first_seed"]) + offset] = (
                row["suite"], row["role_mode"], int(row["public_window_ms"])
            )

    completed: set[int] = set()
    results: set[int] = set()
    totals = blank()
    strata: dict[str, dict[str, int]] = defaultdict(blank)
    days = 0
    for raw in log.read_text(encoding="utf-8-sig").splitlines():
        if raw.startswith("result,"):
            results.add(int(fields(raw)["seed"]))
        elif raw.startswith("case_complete,"):
            completed.add(int(fields(raw)["seed"]))
        elif raw.startswith("day_detail,"):
            row = fields(raw)
            seed = int(row["seed"])
            suite, role, window = case_meta[seed]
            values = {key: int(row[key]) for key in totals}
            for key, value in values.items():
                totals[key] += value
            for label in (suite, f"role:{role}", f"window:{window}"):
                for key, value in values.items():
                    strata[label][key] += value
            days += 1

    if len(results) != expected_cases or completed != results:
        raise RuntimeError(
            f"incomplete evidence: expected={expected_cases} results={len(results)} completed={len(completed)}"
        )
    if "run_complete,results=" + str(expected_cases) not in log.read_text(encoding="utf-8-sig"):
        raise RuntimeError("missing run_complete")

    def enrich(values: dict[str, int]) -> dict[str, int | float]:
        accounted = sum(values[key] for key in TIMERS)
        dfs = values["master_dfs_us"]
        enriched: dict[str, int | float] = dict(values)
        enriched["accounted_us"] = accounted
        enriched["residual_us"] = dfs - accounted
        enriched["accounted_pct"] = round(100.0 * accounted / dfs, 3) if dfs else 0.0
        for key in TIMERS:
            enriched[key + "_pct"] = round(100.0 * values[key] / dfs, 3) if dfs else 0.0
        return enriched

    summary = {
        "experiment": "ATTR-MASTER-LEAF-COST-234",
        "manifest_sha256": sha256(manifest),
        "log_sha256": sha256(log),
        "cases": len(results),
        "days": days,
        "totals": enrich(totals),
        "strata": {label: enrich(values) for label, values in sorted(strata.items())},
    }
    Path(args.output).write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
