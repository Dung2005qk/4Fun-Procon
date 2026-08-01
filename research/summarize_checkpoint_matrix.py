from __future__ import annotations

import csv
import math
import sys
from collections import defaultdict
from pathlib import Path


def parse_result(line: str) -> dict[str, str]:
    fields = line.strip().split(",")
    if not fields or fields[0] != "result":
        raise ValueError(f"unexpected result line: {line!r}")
    row: dict[str, str] = {}
    for field in fields[1:]:
        key, value = field.split("=", 1)
        row[key] = value
    return row


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return int(row["lifetime"]), int(row["daily"]), int(row["servings"])


def sign_p(wins: int, losses: int) -> float:
    trials = wins + losses
    if trials == 0:
        return 1.0
    tail = min(wins, losses)
    mass = sum(math.comb(trials, count) for count in range(tail + 1))
    return min(1.0, 2.0 * mass / (2**trials))


def percentile(values: list[int], percent: int) -> int:
    if not values:
        return 0
    values = sorted(values)
    index = min(len(values) - 1, (percent * len(values) + 99) // 100 - 1)
    return values[index]


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: summarize_checkpoint_matrix.py RUN_DIRECTORY")
    run_directory = Path(sys.argv[1]).resolve()
    checkpoints = list(csv.DictReader((run_directory / "CHECKPOINTS.csv").open(encoding="utf-8")))
    matrix = {
        row["lane"]: row
        for row in csv.DictReader((run_directory / "MATRIX.csv").open(encoding="utf-8"))
    }
    rows: list[dict[str, str]] = []
    for path in sorted(run_directory.glob("*.txt")):
        rows.extend(
            parse_result(line)
            for line in path.read_text(encoding="utf-8-sig").splitlines()
            if line.strip()
        )

    grouped: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        grouped[row["track"]].append(row)

    lines = [
        "# Checkpoint Matrix Summary",
        "",
        "Scores are compared lexicographically. Local latency is diagnostic only.",
        "",
    ]
    verdict_failed = False
    version_order = [row["label"] for row in checkpoints]
    for lane in matrix:
        lane_rows = grouped.get(lane, [])
        if not lane_rows:
            continue
        reference = {
            row["seed"]: row for row in lane_rows if row["version"] == "current"
        }
        lines.extend(
            [
                f"## {lane}",
                "",
                f"Verdict class: `{matrix[lane]['verdict_class']}`; budget: "
                f"`{matrix[lane]['budget_ms']} ms`.",
                "",
                "| Version | W/T/L vs current | Sign p | Score sum | Invalid | Emergency | Complete days | Deadline days | p95 |",
                "|---|---:|---:|---:|---:|---:|---:|---:|---:|",
            ]
        )
        for version in version_order:
            version_rows = [row for row in lane_rows if row["version"] == version]
            if not version_rows:
                continue
            wins = ties = losses = 0
            totals = [0, 0, 0]
            for row in version_rows:
                reference_row = reference[row["seed"]]
                row_score = score(row)
                reference_score = score(reference_row)
                wins += row_score > reference_score
                ties += row_score == reference_score
                losses += row_score < reference_score
                for index, value in enumerate(row_score):
                    totals[index] += value
            invalid = sum(int(row["invalid"]) for row in version_rows)
            emergency = sum(int(row["emergency"]) for row in version_rows)
            complete = sum(int(row["search_complete_days"]) for row in version_rows)
            deadline = sum(int(row["search_deadline_days"]) for row in version_rows)
            p95 = percentile([int(row["p95_ms"]) for row in version_rows], 95)
            lines.append(
                f"| `{version}` | {wins}/{ties}/{losses} | {sign_p(wins, losses):.4f} | "
                f"{totals[0]}/{totals[1]}/{totals[2]} | {invalid} | {emergency} | "
                f"{complete} | {deadline} | {p95} ms |"
            )
            if matrix[lane]["verdict_class"] == "primary" and (invalid or emergency):
                verdict_failed = True
        lines.append("")

    lines.extend(
        [
            "## Gate",
            "",
            "- Primary lanes reject any checkpoint with invalid or emergency output.",
            "- Score promotion requires paired official-budget evidence; diagnostic lanes cannot promote or block it alone.",
            f"- Mechanical validity gate: `{'FAIL' if verdict_failed else 'PASS'}`.",
            "",
        ]
    )
    report = run_directory / "SUMMARY.md"
    report.write_text("\n".join(lines), encoding="utf-8")
    print(report)


if __name__ == "__main__":
    main()
