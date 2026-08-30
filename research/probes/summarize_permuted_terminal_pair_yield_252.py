#!/usr/bin/env python3
import argparse
import csv
import json
from collections import Counter
from pathlib import Path


def fields(line: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for token in line.rstrip("\n").split(",")[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            result[key] = value
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--log", required=True)
    args = parser.parse_args()

    manifest = list(csv.DictReader(Path(args.manifest).open(encoding="utf-8-sig")))
    metadata: dict[tuple[str, int], dict[str, str]] = {}
    for row in manifest:
        for offset in range(int(row["count"])):
            metadata[row["suite"], int(row["first_seed"]) + offset] = row

    cases: dict[tuple[str, int], dict[str, str]] = {}
    run_complete = 0
    case_complete: set[tuple[str, int]] = set()
    for line in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("result,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if key in cases:
                raise RuntimeError(f"duplicate result {key}")
            cases[key] = row
        elif line.startswith("case_complete,"):
            row = fields(line)
            case_complete.add((row["suite"], int(row["seed"])))
        elif line.startswith("run_complete,"):
            run_complete += 1

    expected = set(metadata)
    if set(cases) != expected or case_complete != expected or run_complete != 1:
        raise RuntimeError(
            f"atomic evidence mismatch results={len(cases)} complete={len(case_complete)} "
            f"expected={len(expected)} run_complete={run_complete}"
        )

    yielded = [key for key, row in cases.items() if int(row["permuted_probe_certified"]) > 0]
    yielded_players = sorted({metadata[key]["players"] for key in yielded})
    yielded_suites = sorted({key[0] for key in yielded})
    yielded_roles = sorted({metadata[key]["role_mode"] for key in yielded})
    safety_keys = sorted({
        key for row in cases.values() for key in row
        if key in {"invalid", "emergency"} or "failure" in key or key.endswith("_invalid")
    })
    safety = {key: sum(int(row.get(key, "0")) for row in cases.values()) for key in safety_keys}
    mutation_failures = sum(
        int(row["permuted_probe_mutation_failures"]) for row in cases.values()
    )
    strata = {
        "players": Counter(metadata[key]["players"] for key in yielded),
        "suite": Counter(key[0] for key in yielded),
        "role": Counter(metadata[key]["role_mode"] for key in yielded),
        "fuel": Counter(metadata[key]["fuel_profile"] for key in yielded),
    }
    qualified = (
        len(yielded) >= 3
        and len(yielded_players) >= 2
        and len(yielded_suites) >= 2
        and set(yielded_roles) == {"deadline", "fixed"}
        and all(value == 0 for value in safety.values())
        and mutation_failures == 0
    )

    def summed(key: str) -> int:
        return sum(int(row.get(key, "0")) for row in cases.values())

    summary = {
        "experiment": "ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252",
        "results": len(cases),
        "yielded_matches": len(yielded),
        "yielded_days": summed("permuted_probe_yield_days"),
        "yielded_players": yielded_players,
        "yielded_suites": yielded_suites,
        "yielded_roles": yielded_roles,
        "yielded_cases": [{"suite": suite, "seed": seed} for suite, seed in yielded],
        "yield_strata": {key: dict(sorted(value.items())) for key, value in strata.items()},
        "work": {
            "tasks": summed("permuted_probe_tasks"),
            "routes": summed("permuted_probe_routes"),
            "cross_pairs": summed("permuted_probe_cross_pairs"),
            "valid": summed("permuted_probe_valid"),
            "certified": summed("permuted_probe_certified"),
            "lifetime_gain": summed("permuted_probe_lifetime_gain"),
            "daily_gain": summed("permuted_probe_daily_gain"),
            "serving_gain": summed("permuted_probe_serving_gain"),
        },
        "safety": safety,
        "mutation_failures": mutation_failures,
        "qualified_for_score_successor": qualified,
    }
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
