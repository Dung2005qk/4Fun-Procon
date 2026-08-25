"""Paired analyzer for SCORE-ROLE-SHORT-HORIZON-221 dev/holdout A/B logs.

Usage: python summarize_role_short_horizon_221.py off.log on.log
Pairs harness `result,...` lines by seed, compares official lexicographic
score (lifetime, daily, servings), reports W/T/L for the flag-on side, every
tier-1/tier-2 difference, per-seed servings deltas, and role-mask changes.
"""
import re
import sys


def parse(path):
    rows = {}
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if not line.startswith("result,"):
                continue
            fields = dict(
                part.split("=", 1)
                for part in line.strip().split(",")[1:]
                if "=" in part
            )
            seed = int(fields["seed"])
            rows[seed] = {
                "lifetime": int(fields["lifetime"]),
                "daily": int(fields["daily"]),
                "servings": int(fields["servings"]),
                "invalid": int(fields["invalid"]),
                "emergency": int(fields["emergency"]),
                "role_mask": int(fields["role_mask"]),
                "fixture": fields.get("fixture", "?"),
            }
    return rows


def main():
    off = parse(sys.argv[1])
    on = parse(sys.argv[2])
    seeds = sorted(set(off) & set(on))
    missing = sorted(set(off) ^ set(on))
    if missing:
        print(f"WARNING unpaired seeds: {missing}")
    wins = ties = losses = 0
    tier_diffs = []
    serving_delta = 0
    mask_changes = []
    for seed in seeds:
        a, b = off[seed], on[seed]
        ka = (a["lifetime"], a["daily"], a["servings"])
        kb = (b["lifetime"], b["daily"], b["servings"])
        if kb > ka:
            wins += 1
        elif kb < ka:
            losses += 1
        else:
            ties += 1
        if (b["lifetime"], b["daily"]) != (a["lifetime"], a["daily"]):
            tier_diffs.append((seed, ka, kb, a["fixture"]))
        serving_delta += b["servings"] - a["servings"]
        if a["role_mask"] != b["role_mask"]:
            mask_changes.append((seed, a["role_mask"], b["role_mask"], ka, kb))
        flags = []
        if b["invalid"] or b["emergency"]:
            flags.append("ON-SIDE INVALID/EMERGENCY")
        marker = " <-- " + ",".join(flags) if flags else ""
        print(f"seed={seed} off={ka} on={kb} mask {a['role_mask']}->{b['role_mask']} {a['fixture']}{marker}")
    print()
    print(f"paired={len(seeds)} W/T/L(on)={wins}/{ties}/{losses} net_servings={serving_delta:+d}")
    print(f"role_mask_changes={len(mask_changes)}")
    for seed, ma, mb, ka, kb in mask_changes:
        print(f"  seed={seed} mask {ma}->{mb} score {ka}->{kb}")
    if tier_diffs:
        print("TIER-1/TIER-2 DIFFERENCES:")
        for seed, ka, kb, fixture in tier_diffs:
            print(f"  seed={seed} {ka}->{kb} {fixture}")
    else:
        print("no tier-1/tier-2 differences")


if __name__ == "__main__":
    main()
