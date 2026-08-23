"""SCORE-TERMINAL-PAIR-EXCHANGE-207 paired causal analyzer (same-binary off/on)."""
import re
import sys
from collections import defaultdict


def parse(path):
    cases = {}
    meta = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.strip()
        if line.startswith("case_begin,"):
            meta = dict(kv.split("=", 1) for kv in line[len("case_begin,"):].split(","))
        elif line.startswith("result,"):
            fields = dict(kv.split("=", 1) for kv in line[len("result,"):].split(",")
                          if "=" in kv)
            seed = int(fields["seed"])
            cases[seed] = {
                "suite": fields.get("suite", meta.get("suite", "?")),
                "fuel": fields.get("fuel_profile", meta.get("fuel", "?")),
                "role": fields.get("role_mode", meta.get("role", "?")),
                "window": meta.get("window", "?"),
                "L": int(fields["lifetime"]),
                "D": int(fields["daily"]),
                "Q": int(fields["servings"]),
                "invalid": int(fields.get("invalid", 0)),
                "emergency": int(fields.get("emergency", 0)),
                "pair_acc": int(fields.get("terminal_pair_acceptances", 0)),
                "sparse_strict": int(fields.get("terminal_sparse_strict", 0)),
                "sparse_takeovers": int(fields.get("terminal_sparse_takeovers", 0)),
                "sparse_deadline": int(fields.get("terminal_sparse_deadline", 0)),
            }
    return cases


def score(c):
    return (c["L"], c["D"], c["Q"])


def main():
    off = parse(sys.argv[1])
    on = parse(sys.argv[2])
    seeds = sorted(set(off) & set(on))
    missing = sorted(set(off) ^ set(on))
    if missing:
        print("WARNING unpaired seeds:", missing)
    w = t = l = 0
    dq = 0
    lanes = defaultdict(lambda: [0, 0, 0])
    tails = []
    total_acc_on = 0
    total_acc_off = 0
    inv = eme = 0
    for seed in seeds:
        a, b = off[seed], on[seed]
        total_acc_on += b["pair_acc"]
        total_acc_off += a["pair_acc"]
        inv += a["invalid"] + b["invalid"]
        eme += a["emergency"] + b["emergency"]
        sa, sb = score(a), score(b)
        delta_q = b["Q"] - a["Q"]
        dq += delta_q
        outcome = 1 if sb > sa else (2 if sb < sa else 0)
        for lane in (a["suite"], "fuel:" + a["fuel"], "role:" + a["role"],
                     "window:" + a["window"]):
            lanes[lane][0 if outcome == 1 else (2 if outcome == 2 else 1)] += 1
        if outcome == 1:
            w += 1
        elif outcome == 2:
            l += 1
        else:
            t += 1
        if sb != sa:
            tails.append((seed, a["suite"], a["fuel"], a["role"], a["window"],
                          sa, sb, b["pair_acc"]))
    print("paired=%d W/T/L=%d/%d/%d servingsDelta=%+d" % (len(seeds), w, t, l, dq))
    print("pair_acceptances on-side total=%d (off-side control=%d)"
          % (total_acc_on, total_acc_off))
    print("invalid=%d emergency=%d" % (inv, eme))
    print("\nlanes (W/T/L):")
    for lane in sorted(lanes):
        v = lanes[lane]
        print("  %-28s %d/%d/%d" % (lane, v[0], v[1], v[2]))
    print("\nnon-ties:")
    for row in sorted(tails, key=lambda r: (r[6] > r[5], r[6] < r[5], r[0])):
        seed, suite, fuel, role, window, sa, sb, acc = row
        tag = "WIN " if sb > sa else "LOSS"
        print("  %s seed=%d %s/%s/%s/%s off=%s on=%s acc=%d"
              % (tag, seed, suite, fuel, role, window, sa, sb, acc))
    on_acc_days = sum(1 for s in seeds if on[s]["pair_acc"] > 0)
    print("\ncases with >=1 pair acceptance on on-side: %d/%d"
          % (on_acc_days, len(seeds)))


if __name__ == "__main__":
    main()
