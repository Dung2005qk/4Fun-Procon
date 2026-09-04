#!/usr/bin/env python3
import argparse
import csv
import json
import math
from collections import Counter, defaultdict
from pathlib import Path


ENVELOPES = ("today", "candidate", "viability", "absolute")
NON_ABSOLUTE = ENVELOPES[:-1]


def fields(line: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for token in line.rstrip("\n").split(",")[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            result[key] = value
    return result


def score(value: str) -> tuple[int, int, int]:
    parts = tuple(int(part) for part in value.split("/"))
    if len(parts) != 3:
        raise RuntimeError(f"invalid score triple: {value}")
    return parts


def breadth(roots: list[dict[str, object]]) -> dict[str, list[object]]:
    return {
        "suites": sorted({str(root["suite"]) for root in roots}),
        "fuel_profiles": sorted({str(root["fuel_profile"]) for root in roots}),
        "players": sorted({int(root["players"]) for root in roots}),
        "role_modes": sorted({str(root["role_mode"]) for root in roots}),
        "public_windows_ms": sorted({int(root["public_window_ms"]) for root in roots}),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--phase", choices=("development", "holdout"), required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--log", required=True)
    args = parser.parse_args()

    manifest_rows = list(csv.DictReader(Path(args.manifest).open(encoding="utf-8-sig")))
    metadata: dict[int, dict[str, str]] = {}
    for row in manifest_rows:
        if row["split"] != args.phase:
            continue
        for offset in range(int(row["count"])):
            seed = int(row["first_seed"]) + offset
            if seed in metadata:
                raise RuntimeError(f"duplicate seed in manifest: {seed}")
            metadata[seed] = row

    results: dict[int, dict[str, str]] = {}
    details: dict[int, list[dict[str, str]]] = defaultdict(list)
    completed: set[int] = set()
    run_complete = False
    for line in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("result,"):
            row = fields(line)
            seed = int(row["seed"])
            if seed in results:
                raise RuntimeError(f"duplicate result seed: {seed}")
            results[seed] = row
        elif line.startswith("day_detail,"):
            row = fields(line)
            details[int(row["seed"])].append(row)
        elif line.startswith("case_complete,"):
            completed.add(int(fields(line)["seed"]))
        elif line.startswith("run_complete,"):
            run_complete = True

    if not run_complete:
        raise RuntimeError("log has no run_complete marker")
    expected = set(metadata)
    if set(results) != expected or completed != expected or set(details) != expected:
        raise RuntimeError(
            f"atomic evidence mismatch: expected={len(expected)} results={len(results)} "
            f"completed={len(completed)} details={len(details)}"
        )

    safety_keys = sorted({
        key
        for result in results.values()
        for key in result
        if key in {"invalid", "emergency", "invalid_optimality_envelopes"}
        or "failure" in key
        or key.endswith("_failures")
        or key.endswith("_invalid")
    })
    safety = {key: sum(int(result.get(key, "0")) for result in results.values())
              for key in safety_keys}

    tier_counts = {name: Counter() for name in ENVELOPES}
    component_gap_sums = {name: [0, 0, 0] for name in ENVELOPES}
    invalid_envelopes = 0
    roots: list[dict[str, object]] = []
    for seed in sorted(details):
        meta = metadata[seed]
        result = results[seed]
        expected_days = sum(int(value) for value in result["today_open_tiers"].split("/"))
        if len(details[seed]) != expected_days:
            raise RuntimeError(
                f"day telemetry mismatch for {seed}: {len(details[seed])} != {expected_days}"
            )
        for day in details[seed]:
            root: dict[str, object] = {
                "seed": seed,
                "day": int(day["day"]),
                "suite": meta["suite"],
                "fuel_profile": meta["fuel_profile"],
                "players": int(meta["players"]),
                "role_mode": meta["role_mode"],
                "public_window_ms": int(meta["public_window_ms"]),
                "portfolio_search_complete": int(day["portfolio_search_complete"]),
                "viability_deadline": int(day["viability_deadline"]),
            }
            for name in ENVELOPES:
                open_tier = int(day[f"{name}_open_tier"])
                valid = int(day[f"{name}_valid_envelope"])
                lower = score(day[f"{name}_lower"])
                upper = score(day[f"{name}_upper"])
                gaps = score(day[f"{name}_gap"])
                if open_tier not in range(4):
                    raise RuntimeError(f"invalid {name} tier {open_tier} for {seed}/day{day['day']}")
                if any(value < 0 for value in gaps):
                    raise RuntimeError(f"negative {name} gap for {seed}/day{day['day']}")
                expected_open = next((index + 1 for index, value in enumerate(gaps) if value), 0)
                if expected_open != open_tier:
                    raise RuntimeError(f"inconsistent {name} tier for {seed}/day{day['day']}")
                if valid != int(upper >= lower):
                    raise RuntimeError(f"inconsistent {name} envelope validity for {seed}/day{day['day']}")
                tier_counts[name][open_tier] += 1
                for index, value in enumerate(gaps):
                    component_gap_sums[name][index] += value
                invalid_envelopes += 0 if valid else 1
                root[f"{name}_open_tier"] = open_tier
            roots.append(root)

    high_tier_roots = [
        root for root in roots
        if any(int(root[f"{name}_open_tier"]) in (1, 2) for name in NON_ABSOLUTE)
    ]
    tier3_roots = [
        root for root in roots
        if any(int(root[f"{name}_open_tier"]) == 3 for name in NON_ABSOLUTE)
    ]
    only_absolute_roots = [
        root for root in roots
        if int(root["absolute_open_tier"]) > 0
        and all(int(root[f"{name}_open_tier"]) == 0 for name in NON_ABSOLUTE)
    ]
    high_breadth = breadth(high_tier_roots)
    tier3_breadth = breadth(tier3_roots)
    high_gate = (
        len(high_tier_roots) >= 4
        and len(high_breadth["suites"]) >= 2
        and len(high_breadth["fuel_profiles"]) >= 2
        and len(high_breadth["players"]) >= 2
        and set(high_breadth["role_modes"]) == {"deadline", "fixed"}
    )
    tier3_threshold = math.ceil(0.25 * len(roots))
    tier3_gate = (
        len(tier3_roots) >= tier3_threshold
        and len(tier3_breadth["suites"]) >= 3
        and len(tier3_breadth["fuel_profiles"]) >= 3
        and set(tier3_breadth["role_modes"]) == {"deadline", "fixed"}
    )
    safety_ok = invalid_envelopes == 0 and all(value == 0 for value in safety.values())
    gate_pass = safety_ok and (high_gate or tier3_gate)

    summary = {
        "experiment": "ATTR-OPTIMALITY-ENVELOPE-PREVALENCE-304",
        "phase": args.phase,
        "cases": len(results),
        "day_roots": len(roots),
        "tier_counts_closed_tier1_tier2_tier3": {
            name: [tier_counts[name][tier] for tier in range(4)]
            for name in ENVELOPES
        },
        "component_gap_sums": component_gap_sums,
        "portfolio_search_complete_days": sum(
            int(root["portfolio_search_complete"]) for root in roots
        ),
        "viability_deadline_days": sum(int(root["viability_deadline"]) for root in roots),
        "invalid_envelopes": invalid_envelopes,
        "safety": safety,
        "high_tier_non_absolute": {
            "roots": len(high_tier_roots),
            "breadth": high_breadth,
            "gate": high_gate,
        },
        "tier3_non_absolute": {
            "roots": len(tier3_roots),
            "threshold": tier3_threshold,
            "breadth": tier3_breadth,
            "gate": tier3_gate,
        },
        "only_absolute_open_roots": len(only_absolute_roots),
        "safety_gate": safety_ok,
        "gate_pass": gate_pass,
    }
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
