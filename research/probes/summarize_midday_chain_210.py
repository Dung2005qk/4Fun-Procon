"""SCORE-MIDDAY-CHAIN-ADOPTION-210 paired causal analyzer (same-binary off/on).

Both sides run the accepted production configuration (terminal pair ON);
they differ only in --midday-chain 0|1, so any paired score difference is
causally attributable to the mid-day deep-chain lane.
"""
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
                "acc": int(fields.get("midday_chain_acceptances", 0)),
                "pair_acc": int(fields.get("midday_pair_acceptances", 0)),
                "target_acc": int(fields.get("midday_target_acceptances", 0)),
                "takeovers": int(fields.get("midday_takeovers", 0)),
                "routes": int(fields.get("midday_routes", 0)),
                "valid_plans": int(fields.get("midday_valid", 0)),
                "deadline_days": int(fields.get("midday_deadline_days", 0)),
                "failure_days": int(fields.get("midday_failure_days", 0)),
                "mean_ms": int(fields.get("mean_ms", 0)),
                "max_ms": int(fields.get("max_ms", 0)),
            }
    return cases


def score(c):
    return (c["L"], c["D"], c["Q"])


def main():
    # Optional 3rd arg selects the mechanism-specific acceptance field. This
    # is required whenever both sides already run the accepted chain prefix.
    cond_key = "acc"
    if len(sys.argv) > 3 and sys.argv[3] == "--cond=pair":
        cond_key = "pair_acc"
    elif len(sys.argv) > 3 and sys.argv[3] == "--cond=target":
        cond_key = "target_acc"
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
    inv = eme = fail = 0
    acc_w = acc_t = acc_l = 0
    for seed in seeds:
        a, b = off[seed], on[seed]
        total_acc_on += b["acc"]
        total_acc_off += a["acc"]
        inv += a["invalid"] + b["invalid"]
        eme += a["emergency"] + b["emergency"]
        fail += a["failure_days"] + b["failure_days"]
        sa, sb = score(a), score(b)
        dq += b["Q"] - a["Q"]
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
        conditioned = b[cond_key] > 0 if cond_key != "acc" else (
            b["acc"] > 0 or b["takeovers"] > 0)
        if conditioned:
            if outcome == 1:
                acc_w += 1
            elif outcome == 2:
                acc_l += 1
            else:
                acc_t += 1
        if sb != sa:
            tails.append((seed, a["suite"], a["fuel"], a["role"], a["window"],
                          sa, sb, b[cond_key], b["takeovers"]))
    print("paired=%d W/T/L=%d/%d/%d servingsDelta=%+d" % (len(seeds), w, t, l, dq))
    print("midday acceptances on-side total=%d (off-side=%d)"
          % (total_acc_on, total_acc_off))
    pair_on = sum(on[s]["pair_acc"] for s in seeds)
    pair_off = sum(off[s]["pair_acc"] for s in seeds)
    print("midday PAIR acceptances on-side total=%d (off-side=%d)"
          % (pair_on, pair_off))
    target_on = sum(on[s]["target_acc"] for s in seeds)
    target_off = sum(off[s]["target_acc"] for s in seeds)
    print("midday TARGET acceptances on-side total=%d (off-side=%d)"
          % (target_on, target_off))
    print("acceptance-conditional (%s) W/T/L=%d/%d/%d"
          % (cond_key, acc_w, acc_t, acc_l))
    print("invalid=%d emergency=%d lane_failures=%d" % (inv, eme, fail))
    on_mean = sum(on[s]["mean_ms"] for s in seeds) / max(1, len(seeds))
    off_mean = sum(off[s]["mean_ms"] for s in seeds) / max(1, len(seeds))
    on_max = max(on[s]["max_ms"] for s in seeds) if seeds else 0
    off_max = max(off[s]["max_ms"] for s in seeds) if seeds else 0
    print("runtime parity: mean_ms off=%.0f on=%.0f, max_ms off=%d on=%d"
          % (off_mean, on_mean, off_max, on_max))
    print("\nlanes (W/T/L):")
    for lane in sorted(lanes):
        v = lanes[lane]
        print("  %-28s %d/%d/%d" % (lane, v[0], v[1], v[2]))
    print("\nnon-ties:")
    for row in sorted(tails, key=lambda r: (r[6] > r[5], r[6] < r[5], r[0])):
        seed, suite, fuel, role, window, sa, sb, acc, tak = row
        tag = "WIN " if sb > sa else "LOSS"
        print("  %s seed=%d %s/%s/%s/%s off=%s on=%s cond_acc=%d takeovers=%d"
              % (tag, seed, suite, fuel, role, window, sa, sb, acc, tak))
    on_acc_cases = sum(1 for s in seeds if on[s][cond_key] > 0)
    print("\ncases with >=1 conditioned acceptance on on-side: %d/%d"
          % (on_acc_cases, len(seeds)))


if __name__ == "__main__":
    main()
