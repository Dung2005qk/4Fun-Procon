#!/usr/bin/env python3
"""Read-only causality audit for the consumed experiment-304 envelopes."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from collections import Counter
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def fields(line: str) -> dict[str, str]:
    return dict(item.split("=", 1) for item in line.rstrip().split(",")[1:])


def score(value: str) -> tuple[int, int, int]:
    result = tuple(int(part) for part in value.split("/"))
    if len(result) != 3:
        raise ValueError(f"bad score triple: {value}")
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inputs", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    root = Path.cwd()
    input_rows = list(csv.DictReader(args.inputs.open(encoding="utf-8-sig")))
    if len(input_rows) != 7:
        raise ValueError(f"expected seven frozen inputs, got {len(input_rows)}")
    resolved: dict[str, Path] = {}
    for row in input_rows:
        if row["experiment_id"] != "ATTR-OPTIMALITY-ENVELOPE-CAUSALITY-305":
            raise ValueError("wrong attribution experiment")
        path = root / row["input_path"]
        actual = sha256(path)
        if actual != row["input_sha256"]:
            raise ValueError(f"hash mismatch for {path}: {actual}")
        resolved[row["input_kind"]] = path

    roots: list[dict[str, str]] = []
    split_counts: dict[str, dict[str, int]] = {}
    for split in ("development", "holdout"):
        log = resolved[f"{split}-log"]
        run_complete = 0
        results = 0
        completions = 0
        split_roots: list[dict[str, str]] = []
        for line in log.read_text(encoding="utf-8-sig").splitlines():
            if line.startswith("result,"):
                results += 1
            elif line.startswith("day_detail,"):
                row = fields(line)
                row["split"] = split
                split_roots.append(row)
            elif line.startswith("case_complete,"):
                completions += 1
            elif line.startswith("run_complete,"):
                run_complete += 1
        expected_results = 12 if split == "development" else 24
        if results != expected_results or completions != expected_results or run_complete != 1:
            raise ValueError(
                f"bad {split} structure: results={results} completions={completions} "
                f"run_complete={run_complete}"
            )
        split_counts[split] = {
            "cases": results,
            "day_roots": len(split_roots),
        }
        roots.extend(split_roots)

    maximum_day = Counter()
    for row in roots:
        key = f"{row['split']}:{row['seed']}"
        maximum_day[key] = max(maximum_day[key], int(row["day"]))

    candidate_by_terminal = Counter()
    today_by_completion = Counter()
    equality = Counter()
    for row in roots:
        key = f"{row['split']}:{row['seed']}"
        terminal = int(row["day"]) == maximum_day[key]
        candidate_open = int(row["candidate_open_tier"]) != 0
        candidate_by_terminal[
            f"{'terminal' if terminal else 'nonterminal'}:{'open' if candidate_open else 'closed'}"
        ] += 1

        complete = int(row["portfolio_search_complete"]) == 1
        today_open = int(row["today_open_tier"]) != 0
        today_by_completion[
            f"{'complete' if complete else 'incomplete'}:{'open' if today_open else 'closed'}"
        ] += 1

        candidate_upper = score(row["candidate_upper"])
        viability_upper = score(row["viability_upper"])
        absolute_upper = score(row["absolute_upper"])
        equality["candidate_equals_viability"] += candidate_upper == viability_upper
        equality["candidate_equals_absolute"] += candidate_upper == absolute_upper
        equality["viability_equals_absolute"] += viability_upper == absolute_upper

    nonterminal_roots = sum(
        value for key, value in candidate_by_terminal.items()
        if key.startswith("nonterminal:")
    )
    terminal_roots = len(roots) - nonterminal_roots
    candidate_structural = (
        candidate_by_terminal["nonterminal:open"] == nonterminal_roots
        and candidate_by_terminal["nonterminal:closed"] == 0
        and candidate_by_terminal["terminal:closed"] == terminal_roots
        and candidate_by_terminal["terminal:open"] == 0
    )

    decision_source = resolved["decision-source"].read_text(encoding="utf-8")
    planner_source = resolved["master-source"].read_text(encoding="utf-8")
    source_checks = {
        "candidate_upper_calls_fast_viability": (
            "return analyzer.analyze(futureState, futureLedger, deadline).upperBound;"
            in decision_source
        ),
        "viability_daily_relaxation": (
            "config_.brand_count() * remainingDays" in decision_source
            and "perDayServings * remainingDays" in decision_source
        ),
        "today_upper_is_master_optimistic_bound": (
            "result.diagnostics.optimisticUpperBound" in decision_source
            and "diagnostics.optimisticUpperBound = exactMetadata" in planner_source
        ),
    }
    sources_ok = all(source_checks.values())

    report = {
        "experiment": "ATTR-OPTIMALITY-ENVELOPE-CAUSALITY-305",
        "input_manifest_sha256": sha256(args.inputs),
        "split_counts": split_counts,
        "total_day_roots": len(roots),
        "candidate_by_terminal": dict(sorted(candidate_by_terminal.items())),
        "candidate_gap_exactly_nonterminal": candidate_structural,
        "today_by_portfolio_completion": dict(sorted(today_by_completion.items())),
        "upper_bound_equalities": dict(equality),
        "source_checks": source_checks,
        "source_classification": {
            "candidate_horizon": "next-state fast-viability resource relaxation, not a feasible witness",
            "viability_horizon": "per-brand reachability plus agent-day/stock capacity relaxation, not a joint plan",
            "today_portfolio": "master optimistic relaxation over generated columns, not an emitted challenger",
        },
        "exact_feasible_challenger_records": 0,
        "sources_ok": sources_ok,
        "score_successor_authorized": False,
        "verdict": (
            "closed-noncausal-bound-telemetry"
            if candidate_structural and sources_ok
            else "inconclusive"
        ),
        "next_authorized_axis": (
            "fresh exact-oracle counterexample with three active Patrols on official 8/9/10-team configs"
        ),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
