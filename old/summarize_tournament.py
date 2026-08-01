from __future__ import annotations

import csv
import math
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent
RAW = ROOT / "results" / "raw"
REPORT = ROOT / "results" / "HISTORICAL_TOURNAMENT.md"


def parse_result(line: str) -> dict[str, str]:
    fields = line.strip().split(",")
    if not fields or fields[0] != "result":
        raise ValueError(f"unexpected result line: {line!r}")
    result: dict[str, str] = {}
    for field in fields[1:]:
        key, value = field.split("=", 1)
        result[key] = value
    return result


def load(patterns: list[str]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for pattern in patterns:
        for path in sorted(RAW.glob(pattern)):
            rows.extend(
                parse_result(line)
                for line in path.read_text(encoding="utf-8-sig").splitlines()
                if line.strip()
            )
    return rows


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return int(row["lifetime"]), int(row["daily"]), int(row["servings"])


def compare(left: dict[str, str], right: dict[str, str]) -> int:
    return (score(left) > score(right)) - (score(left) < score(right))


def sign_p(wins: int, losses: int) -> float:
    trials = wins + losses
    if trials == 0:
        return 1.0
    tail = min(wins, losses)
    probability = sum(math.comb(trials, count) for count in range(tail + 1))
    return min(1.0, 2.0 * probability / (2**trials))


def percentile(values: list[int], percent: int) -> int:
    if not values:
        return 0
    ordered = sorted(values)
    index = min(
        len(ordered) - 1,
        (percent * len(ordered) + 99) // 100 - 1,
    )
    return ordered[index]


def version_order() -> list[str]:
    with (ROOT / "CHECKPOINTS.csv").open(encoding="utf-8", newline="") as source:
        return [row["label"] for row in csv.DictReader(source)]


def row_key(row: dict[str, str]) -> tuple[str, str]:
    return row["track"], row["seed"]


def summary_table(
    rows: list[dict[str, str]],
    versions: list[str],
    reference: str = "current",
) -> list[str]:
    by_version: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        by_version[row["version"]].append(row)
    reference_rows = {
        row_key(row): row for row in by_version.get(reference, [])
    }
    lines = [
        "| Version | W/T/L vs current | Sign p | Sum score | Invalid | Emergency | Fixture-p95 |",
        "|---|---:|---:|---:|---:|---:|---:|",
    ]
    for version in versions:
        version_rows = by_version.get(version, [])
        if not version_rows:
            continue
        wins = ties = losses = 0
        for row in version_rows:
            order = compare(row, reference_rows[row_key(row)])
            wins += order > 0
            ties += order == 0
            losses += order < 0
        totals = tuple(
            sum(score(row)[index] for row in version_rows)
            for index in range(3)
        )
        invalid = sum(int(row["invalid"]) for row in version_rows)
        emergency = sum(int(row["emergency"]) for row in version_rows)
        fixture_p95 = percentile(
            [int(row["p95_ms"]) for row in version_rows],
            95,
        )
        lines.append(
            f"| `{version}` | {wins}/{ties}/{losses} | "
            f"{sign_p(wins, losses):.4f} | "
            f"{totals[0]}/{totals[1]}/{totals[2]} | "
            f"{invalid} | {emergency} | {fixture_p95} ms |"
        )
    return lines


def family_table(
    rows: list[dict[str, str]],
    left_version: str,
    right_version: str,
) -> list[str]:
    left = {
        row_key(row): row for row in rows if row["version"] == left_version
    }
    right = {
        row_key(row): row for row in rows if row["version"] == right_version
    }
    grouped: dict[str, list[tuple[dict[str, str], dict[str, str]]]] = defaultdict(list)
    for key, left_row in left.items():
        grouped[left_row["family"]].append((left_row, right[key]))
    lines = [
        "| Family | Current vs baseline W/T/L | Score delta current-baseline |",
        "|---|---:|---:|",
    ]
    for family in sorted(grouped):
        wins = ties = losses = 0
        delta = [0, 0, 0]
        for baseline_row, current_row in grouped[family]:
            order = compare(current_row, baseline_row)
            wins += order > 0
            ties += order == 0
            losses += order < 0
            for index, value in enumerate(score(current_row)):
                delta[index] += value - score(baseline_row)[index]
        lines.append(
            f"| {family} | {wins}/{ties}/{losses} | "
            f"{delta[0]:+d}/{delta[1]:+d}/{delta[2]:+d} |"
        )
    return lines


def baseline_current_native(rows: list[dict[str, str]]) -> str:
    baseline = {
        row_key(row): row for row in rows if row["version"] == "baseline-btc"
    }
    current = {
        row_key(row): row for row in rows if row["version"] == "current"
    }
    wins = ties = losses = 0
    deltas = [0, 0, 0]
    for key, baseline_row in baseline.items():
        current_row = current[key]
        order = compare(current_row, baseline_row)
        wins += order > 0
        ties += order == 0
        losses += order < 0
        for index, value in enumerate(score(current_row)):
            deltas[index] += value - score(baseline_row)[index]
    return (
        f"HEAD vs baseline: **{wins}/{ties}/{losses}**, "
        f"sign-test `p={sign_p(wins, losses):.4f}`, aggregate delta "
        f"`{deltas[0]:+d}/{deltas[1]:+d}/{deltas[2]:+d}`."
    )


def main() -> None:
    versions = version_order()
    primary = load(["fixed-official-*.txt", "fixed-future-*.txt"])
    native_all = load(["native-official-*.txt"])
    native_baseline_current = load(
        [
            "native-official-baseline-btc.txt",
            "native-official-current.txt",
            "native-future-baseline-btc.txt",
            "native-future-current.txt",
        ]
    )
    stress_fixed = load(["fixed-heldout-*.txt"])
    stress_native = load(["general-heldout-*.txt"])
    btc_large = load(["btc-large-*.txt"])
    btc_highfuel = load(["btc-highfuel-*.txt"])

    lines = [
        "# Historical UDON-SHIELD tournament",
        "",
        "Generated 2026-07-31 from the frozen raw files in `old/results/raw`.",
        "",
        "## Verdict",
        "",
        "- The current checkpoint is not universally stronger at every deadline.",
        "- At the production-like 2500 ms planner lane, every checkpoint from "
        "`02df79d` through HEAD is score-identical on all 120 fixed-role maps.",
        "- Against the frozen BTC baseline, HEAD is directionally better at "
        "2500 ms, but this 120-map sample is not statistically decisive.",
        "- At 500 ms, the old baseline is decisively stronger because the modern "
        "engine deliberately suppresses expensive stages under short deadlines.",
        "- No tested checkpoint produced an invalid plan or emergency day.",
        "",
        "## Protocol",
        "",
        "- One version-neutral C++ harness linked independently against each Git snapshot.",
        "- Same map generator, seeds, opponent traffic, own two-day traffic feedback, "
        "official lexicographic scoring, exact simulator and independent validator.",
        "- `fixed` lanes use the same one-tanker mask to isolate the day planner.",
        "- `native` lanes use deterministic exhaustive role selection to remove local "
        "wall-clock noise from pre-match rollout.",
        "- Local latency is diagnostic only; BTC remains authoritative for runtime gates.",
        "",
        "## Primary: fixed roles, 2500 ms, 120 maps",
        "",
        *summary_table(primary, versions),
        "",
        "### Family result: HEAD versus baseline",
        "",
        *family_table(primary, "baseline-btc", "current"),
        "",
        "## Native roles, 2500 ms",
        "",
        "Thirty maps include all eight checkpoints:",
        "",
        *summary_table(native_all, versions),
        "",
        "The independent 30-map future window was run for baseline and HEAD only. "
        + baseline_current_native(native_baseline_current),
        "",
        "## Stress: fixed roles, 500 ms, 60 maps",
        "",
        *summary_table(stress_fixed, versions),
        "",
        "## Stress: native roles, 500 ms, 60 maps",
        "",
        *summary_table(stress_native, versions),
        "",
        "## BTC-scale 32x32 probes",
        "",
        "Default fuel, three ten-day maps:",
        "",
        *summary_table(btc_large, versions),
        "",
        "High fuel, three ten-day maps:",
        "",
        *summary_table(btc_highfuel, versions),
        "",
        "All BTC-scale checkpoints reached identical lifetime and daily tiers. "
        "Tier-3 servings near the 2500 ms cutoff varied across repeated local runs, "
        "so those small tail differences are not a promotion verdict.",
        "",
        "## Interpretation",
        "",
        "1. The large improvement happened by `02df79d`; later accepted commits mostly "
        "improve correctness, proof bounds, final-day edge cases, and live runtime wiring.",
        "2. `6f84a06` is safer and more truthful about proof gaps, but it is not a "
        "general-score upgrade over `02df79d` on this closed tournament.",
        "3. The frozen baseline remains the best short-deadline policy. A production "
        "fallback may reuse its scheduling profile only after a separate semantic and "
        "BTC deadline gate; this tournament does not authorize that change.",
        "4. Further research must start from the measured open gap: preserve modern "
        "correctness while recovering short-deadline quality. No unrelated axis should "
        "be reopened first.",
        "",
    ]
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text("\n".join(lines), encoding="utf-8")
    print(REPORT)


if __name__ == "__main__":
    main()
