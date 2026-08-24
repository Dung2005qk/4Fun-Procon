import json
import glob
import os
from collections import defaultdict

# Global next-day status transition counts on real BTC matches, plus the
# subset where our own team contributed zero stops to the road in the
# 2-day window (opponent-driven statuses only).

def own_footprints_by_day(path):
    """Own road footprint per day from the decision candidate simulation."""
    footprints = {}
    for line in open(path, encoding="utf-8"):
        o = json.loads(line)
        if o.get("kind") != "decision":
            continue
        d = o["body"]["decision"]
        sim = d.get("candidate", {}).get("simulation", {})
        fp = sim.get("roadFootprint")
        if fp is not None:
            footprints[d["dayNumber"]] = fp
    return footprints

matrix = defaultdict(int)
oppmatrix = defaultdict(int)
per_match = {}
for path in sorted(glob.glob(r"C:\Users\LMC\Desktop\4Fun\artifacts\btc\m-*.jsonl")):
    name = os.path.basename(path).replace(".jsonl", "")
    days = []
    players = None
    for line in open(path, encoding="utf-8"):
        o = json.loads(line)
        if o.get("kind") == "setup":
            players = o["body"].get("players")
        if o.get("kind") == "day_state":
            b = o["body"]
            days.append((b["day"], {t["pos"]: t["status"] for t in b["traffics"]}))
    if len(days) < 3:
        continue
    own = own_footprints_by_day(path)
    local = defaultdict(int)
    for (d0, t0), (d1, t1) in zip(days, days[1:]):
        for pos, s0 in t0.items():
            s1 = t1.get(pos, 0)
            matrix[(s0, s1)] += 1
            local[(s0, s1)] += 1
            # our own contribution to the 2-day window ending before day d1's
            # statuses: own footprints of the two completed days before d1.
            contrib = 0
            for day in (d1 - 1, d1):  # day_state day fields are 0-based prior day markers
                fp = own.get(day)
                if fp and pos < len(fp):
                    contrib += fp[pos]
            if contrib == 0:
                oppmatrix[(s0, s1)] += 1
    stay_busy = sum(local[(s, t)] for s in (1, 2) for t in (1, 2))
    from_busy = sum(local[(s, t)] for s in (1, 2) for t in (0, 1, 2))
    per_match[name] = (players, from_busy, stay_busy)

def show(m, title):
    print(title)
    total = sum(m.values())
    for s0 in (0, 1, 2):
        row = [m[(s0, s1)] for s1 in (0, 1, 2)]
        n = sum(row)
        if n:
            print("  from %d: %s  (P stay>=busy: %.2f)" %
                  (s0, row, (row[1] + row[2]) / n if s0 else 0.0))
    print("  total transitions:", total)

show(matrix, "ALL road-day transitions (status d -> d+1):")
show(oppmatrix, "OPPONENT-ONLY (our stops == 0 in window):")
print()
print("per match: players, busy/jam road-days, of which stayed busy/jam next day")
for k, v in per_match.items():
    if v[1]:
        print("  %s: players=%s persist %d/%d = %.2f" % (k, v[0], v[2], v[1], v[2] / v[1]))
