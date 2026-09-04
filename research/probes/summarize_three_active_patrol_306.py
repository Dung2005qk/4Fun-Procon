#!/usr/bin/env python3
import argparse
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path


def parse_fields(line: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for item in line.rstrip().split(",")[1:]:
        key, value = item.split("=", 1)
        fields[key] = value
    return fields


def score(text: str) -> tuple[int, int, int]:
    values = tuple(int(value) for value in text.split("/"))
    if len(values) != 3:
        raise ValueError(f"invalid official score: {text}")
    return values


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    raw = args.log.read_bytes()
    lines = raw.decode("utf-8-sig").splitlines()
    cases = [parse_fields(line) for line in lines if line.startswith("case,")]
    complete = [line for line in lines if line == "run_complete,cases=12"]
    if len(cases) != 12 or len(complete) != 1:
        raise SystemExit(
            f"expected 12 cases and one run_complete; got {len(cases)} and {len(complete)}"
        )

    invalid = [case for case in cases if case["oracle_valid"] != "1" or case["head_valid"] != "1"]
    if invalid:
        raise SystemExit(f"invalid oracle/head cases: {len(invalid)}")

    outcomes = Counter()
    tiers = Counter()
    families: dict[str, Counter] = defaultdict(Counter)
    fuels: dict[str, Counter] = defaultdict(Counter)
    players: dict[str, Counter] = defaultdict(Counter)
    wins: list[dict[str, object]] = []
    maximum_frontier = 0
    for case in cases:
        oracle = score(case["oracle"])
        head = score(case["head"])
        outcome = "win" if oracle > head else ("loss" if oracle < head else "tie")
        outcomes[outcome] += 1
        families[case["family"]][outcome] += 1
        fuels[case["fuel"]][outcome] += 1
        players[case["players"]][outcome] += 1
        maximum_frontier = max(maximum_frontier, int(case["max_frontier"]))
        if outcome == "win":
            tier = int(case["tier"])
            tiers[str(tier)] += 1
            wins.append(
                {
                    "seed": int(case["seed"]),
                    "family": case["family"],
                    "fuel": case["fuel"],
                    "players": int(case["players"]),
                    "oracle": oracle,
                    "head": head,
                    "first_tier": tier,
                    "gain": int(case["gain"]),
                }
            )

    report = {
        "experiment": "CEILING-THREE-ACTIVE-PATROL-306",
        "phase": "development",
        "log_sha256": hashlib.sha256(raw).hexdigest().upper(),
        "cases": len(cases),
        "oracle_head_wtl": {
            "wins": outcomes["win"],
            "ties": outcomes["tie"],
            "losses": outcomes["loss"],
        },
        "first_open_tiers": dict(sorted(tiers.items())),
        "maximum_exact_frontier": maximum_frontier,
        "wins": wins,
        "strata": {
            "family": {key: dict(value) for key, value in sorted(families.items())},
            "fuel": {key: dict(value) for key, value in sorted(fuels.items())},
            "players": {key: dict(value) for key, value in sorted(players.items())},
        },
        "zero_invalid": True,
        "holdout_open_authorized": False,
        "causal_attribution_authorized": outcomes["win"] > 0,
        "score_successor_authorized": False,
        "verdict": "exact-gap-found-attribution-required" if outcomes["win"] else "closed-no-exact-gap",
    }
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
