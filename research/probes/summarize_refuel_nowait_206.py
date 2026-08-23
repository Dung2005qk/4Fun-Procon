"""Paired summary for SCORE-REFUEL-NOWAIT-206 (official lexicographic only)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def parse(path):
    cases = {}
    meta = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith("case_begin,"):
            meta = dict(part.split("=", 1) for part in line.split(",")[1:])
        elif line.startswith("result,"):
            fields = dict(part.split("=", 1) for part in line.split(",")[1:])
            key = (fields["suite"], fields["seed"])
            cases[key] = {
                "score": (
                    int(fields["lifetime"]),
                    int(fields["daily"]),
                    int(fields["servings"]),
                ),
                "invalid": int(fields["invalid"]),
                "emergency": int(fields["emergency"]),
                "fuel": fields["fuel_profile"],
                "family": fields["family"],
                "role": fields["role_mode"],
            }
            if meta:
                cases[key]["window"] = meta.get("window", "?")
    return cases


def main(split):
    parent = parse(ROOT / f"research/evidence/SCORE-REFUEL-NOWAIT-206-{split}-parent.log")
    candidate = parse(ROOT / f"research/evidence/SCORE-REFUEL-NOWAIT-206-{split}-candidate.log")
    keys = sorted(set(parent) & set(candidate))
    missing = sorted(set(parent) ^ set(candidate))
    wins = ties = losses = 0
    gain = loss = 0
    tiers = {1: [0, 0], 2: [0, 0], 3: [0, 0]}
    by = {}
    worst = []
    invalid = emergency = 0
    for key in keys:
        p, c = parent[key], candidate[key]
        invalid += c["invalid"]
        emergency += c["emergency"]
        ps, cs = p["score"], c["score"]
        lane = (c["fuel"], c["role"])
        by.setdefault(lane, [0, 0, 0])
        if cs > ps:
            wins += 1
            by[lane][0] += 1
        elif cs == ps:
            ties += 1
            by[lane][1] += 1
        else:
            losses += 1
            by[lane][2] += 1
        if cs != ps:
            tier = 1 if cs[0] != ps[0] else (2 if cs[1] != ps[1] else 3)
            tiers[tier][0 if cs > ps else 1] += 1
            delta = sum(a - b for a, b in ((cs[i], ps[i]) for i in range(3)))
        if cs > ps:
            gain += (cs[0] - ps[0]) * 0 + (cs[2] - ps[2]) if cs[:2] == ps[:2] else 0
        if cs < ps:
            worst.append((tuple(a - b for a, b in zip(cs, ps)), key, c["fuel"], c["family"]))
            loss += (ps[2] - cs[2]) if cs[:2] == ps[:2] else 0
    print(f"split={split} pairs={len(keys)} missing={len(missing)}")
    print(f"W/T/L={wins}/{ties}/{losses} invalid={invalid} emergency={emergency}")
    for tier in (1, 2, 3):
        print(f"tier{tier}: +{tiers[tier][0]} / -{tiers[tier][1]}")
    print(f"tier3 servings gained on wins={gain} lost on losses={loss}")
    for lane in sorted(by):
        w, t, l = by[lane]
        print(f"lane fuel={lane[0]} role={lane[1]}: {w}/{t}/{l}")
    worst.sort()
    for delta, key, fuel, family in worst[:10]:
        print(f"loss {delta} at {key} fuel={fuel} family={family}")
    if missing:
        print("MISSING:", missing[:10])


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "development")
