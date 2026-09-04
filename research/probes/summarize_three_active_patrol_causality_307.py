#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path


def fields(line: str) -> dict[str, str]:
    prefix = line.split(",oracle_id=", 1)[0]
    return dict(part.split("=", 1) for part in prefix.split(",")[1:])


def score(text: str) -> tuple[int, int, int]:
    values = tuple(int(value) for value in text.split("/"))
    if len(values) != 3:
        raise ValueError(f"invalid score: {text}")
    return values


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    raw = args.log.read_bytes()
    lines = raw.decode("utf-8").splitlines()
    if lines.count("run_complete,cases=4,traces=16") != 1:
        raise SystemExit("missing unique run_complete marker")

    cases: dict[int, dict[str, str]] = {}
    traces: dict[int, list[dict[str, str]]] = {}
    for line in lines:
        if line.startswith("case,"):
            row = fields(line)
            seed = int(row["seed"])
            if seed in cases:
                raise SystemExit(f"duplicate case {seed}")
            cases[seed] = row
        elif line.startswith("trace,"):
            row = fields(line)
            traces.setdefault(int(row["seed"]), []).append(row)
    if len(cases) != 4 or sum(map(len, traces.values())) != 16:
        raise SystemExit("expected four cases and sixteen traces")

    results = []
    classifications: dict[str, int] = {}
    causal_days: dict[str, int] = {}
    for seed in sorted(cases):
        case = cases[seed]
        rows = sorted(traces.get(seed, []), key=lambda row: int(row["day"]))
        if len(rows) != 4:
            raise SystemExit(f"seed {seed} does not have four traces")
        if score(case["oracle"]) <= score(case["head"]):
            raise SystemExit(f"seed {seed} is not an oracle win")
        causal = next(
            (
                row
                for row in rows
                if row["oracle_score"] != row["head_score"]
                or row["oracle_agents"] != row["head_agents"]
            ),
            None,
        )
        if causal is None:
            raise SystemExit(f"seed {seed} has no causal divergence")
        day = int(causal["day"])
        matches = int(causal["oracle_audit_matches"])
        classification = "candidate-supply" if matches == 0 else "selection"
        classifications[classification] = classifications.get(classification, 0) + 1
        causal_days[str(day)] = causal_days.get(str(day), 0) + 1
        results.append(
            {
                "seed": seed,
                "family": case["family"],
                "gain": int(case["gain"]),
                "causal_day": day,
                "classification": classification,
                "oracle_audit_matches": matches,
                "audit_candidates": int(causal["audit_candidates"]),
                "oracle_score_at_causal_day": list(score(causal["oracle_score"])),
                "head_score_at_causal_day": list(score(causal["head_score"])),
                "oracle_agents": causal["oracle_agents"],
                "head_agents": causal["head_agents"],
                "oracle_plan": causal["oracle_plan"],
                "head_plan": causal["head_plan"],
            }
        )

    recurrent = max(classifications.values(), default=0) >= 2
    summary = {
        "experiment": "ATTR-THREE-ACTIVE-PATROL-CAUSALITY-307",
        "phase": "development-attribution",
        "cases": len(cases),
        "traces": sum(map(len, traces.values())),
        "log_sha256": hashlib.sha256(raw).hexdigest().upper(),
        "classifications": classifications,
        "causal_days": causal_days,
        "recurrent_mechanism": recurrent,
        "score_successor_authorized": recurrent,
        "holdout_open_authorized": False,
        "results": results,
    }
    args.output.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
