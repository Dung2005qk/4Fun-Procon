"""Finite offline attribution on every completed 362 inactive lane.

Never executes a solver. CandidateAuditRecord is POST-F0-admission evidence:
absence there cannot distinguish non-generation from pre-F0 pruning.
"""
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
COPY = ROOT / "artifacts/research/362/completed"
OLD = ROOT / "artifacts/research/357/completed"
ID = "ATTR-INACTIVE-CANDIDATE-AVAILABILITY-363"
SCORE_ID = "SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362"
DATA = COPY / "research/evidence" / (SCORE_ID + "-development")
sys.path.insert(0, str(OLD / "research/probes"))
from inactive_runtime_aa_342 import LABELS, compare_runs


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def identity(value):
    raw = value if isinstance(value, str) else json.dumps(value, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(raw.encode()).hexdigest().upper()


def semantic(state):
    return {k: v for k, v in state.items() if k != "endsAt"}


def record(row):
    return {"identity_sha256": identity(row["stableId"]),
            **{k: v for k, v in row.items() if k != "stableId"}}


def inventory(document):
    d = document["decision"]
    pool = d["audit"]["candidates"]
    ids = [r["stableId"] for r in pool]
    assert len(ids) == len(set(ids)), "duplicate audited identities"
    selected = [r for r in pool if r.get("selected")]
    assert len(selected) == 1 and selected[0]["stableId"] == d["candidate"]["stableId"]
    return {"selected": record(selected[0]), "pool": [record(r) for r in pool],
            "profile_quantiles": d["profile"]["quantiles"],
            "profile_certified_lower": d["profile"]["certifiedLowerBound"],
            "timing": d["timing"], "deadline": d["deadline"],
            "selection_reason": d["audit"]["selectionReason"]}


def membership(selected, other):
    sid = selected["decision"]["candidate"]["stableId"]
    found = [r for r in other["decision"]["audit"]["candidates"] if r["stableId"] == sid]
    assert len(found) <= 1
    if not found:
        return {"boundary": "absent-from-post-F0-audit-generation-or-pruning-unresolved"}
    row = found[0]
    disposition = row["disposition"]
    boundary = ("present-not-W1-shortlisted" if disposition == "not-shortlisted" else
                "W1-incomplete-or-invalid" if disposition == "w1-deadline-or-invalid" else
                "certified-final-selection" if row["certified"] else "other-recorded-disposition")
    return {"boundary": boundary, "record": record(row)}


def run():
    expected = {
        COPY / "research/evidence" / (SCORE_ID + "-development.summary.json"):
            "D0C8AB6AEB8E27C5A607AC6C31FB5A63F41A288632B38E599F6E29FA8170F7F4",
        DATA / "run_complete.json": "24A5574453D6FD284CABABFBD764479AD65A68AED06E1C298923193DE7CD4791",
        ROOT / "research/evidence" / (SCORE_ID + "-development-audit.json"):
            "5DC4771F0BD6B028989E624C734F5F0A326FEBE9D21916B13B15244186608393",
        OLD / "source/src/decision.cpp": "FC8D6FF94B8C5863AD134A92050D35E9E068C0AE2800A264226B05D883050BB6",
        ROOT / "src/decision.cpp": "150AA97731A73A86BE30C2CE1C7542F00D92173BD49B988E0B22D5BE901AA2DA",
    }
    for p, h in expected.items():
        assert sha(p) == h, str(p)
    assert (ROOT / "src/decision.cpp").read_text() == (OLD / "source/src/decision.cpp").read_text()
    complete = load(DATA / "run_complete.json")
    assert len(complete["files"]) == 1044
    for name, h in complete["files"].items():
        assert sha(DATA / name) == h, name
    cases = load(COPY / "research/holdouts" / (SCORE_ID + "-development.json"))["cases"]
    cases = [c for c in cases if c["window_ms"] == 5000]
    assert len(cases) == 12
    days, comparisons, differences = [], [], []
    for c in cases:
        cached = {}
        for label, (repeat, side) in LABELS.items():
            prefix = DATA / repeat / side / str(c["seed"])
            events = [json.loads(line) for line in prefix.with_suffix(".replay.jsonl").read_text().splitlines()]
            assert not any(e["kind"] == "resource_marginal" for e in events)
            docs = [e["body"] for e in events if e["kind"] == "decision"]
            refs = {e["body"]["day"]: e["body"] for e in events if e["kind"] == "protected_slack"}
            assert len(docs) == c["days"]
            cached[label] = (load(prefix.with_suffix(".transport.json")), docs, refs)
            for day, doc in enumerate(docs, 1):
                inv = inventory(doc)
                days.append({"seed": c["seed"], "label": label, "day": day,
                    "main_plan_hash": identity(doc["decision"]["candidate"]["plan"]),
                    "selected": inv["selected"], "candidate_count": len(inv["pool"]),
                    "pool_identity_hash": identity(sorted(r["identity_sha256"] for r in inv["pool"])),
                    "dispositions": dict(Counter(r["disposition"] for r in inv["pool"]))})
        for kind, left, right in (("AB_A", "parentA", "candidateA"), ("AB_B", "parentB", "candidateB"),
                                   ("AA_parent", "parentA", "parentB"), ("AA_candidate", "candidateA", "candidateB")):
            ta, da, ra = cached[left]
            tb, db, rb = cached[right]
            pair = compare_runs(ta, tb)
            first_main = next((i + 1 for i, (a, b) in enumerate(zip(da, db, strict=True))
                               if a["decision"]["candidate"]["plan"] != b["decision"]["candidate"]["plan"]), None)
            comparisons.append({"seed": c["seed"], "kind": kind, **pair, "first_main_difference": first_main})
            if pair["trajectory_equal"]:
                continue
            day = pair["first_divergence"]
            out = {"seed": c["seed"], "kind": kind, "left": left, "right": right,
                   "first_day": day, "score_A": pair["A"], "score_B": pair["B"],
                   "roles_equal": pair["roles_equal"], "first_main_difference": first_main}
            if day == 0:
                out["boundary"] = "roles"
            else:
                a, b = da[day - 1], db[day - 1]
                main_equal = a["decision"]["candidate"]["plan"] == b["decision"]["candidate"]["plan"]
                out.update(state_equal=semantic(a["state"]) == semantic(b["state"]), ledger_equal=a["ledger"] == b["ledger"],
                           main_equal=main_equal, boundary="post-main-protected-or-submission" if main_equal else "main-selection",
                           protected=[ra[day], rb[day]])
                if not main_equal:
                    ia, ib = inventory(a), inventory(b)
                    out.update(inventories={left: ia, right: ib}, left_selected_in_right=membership(a, b),
                               right_selected_in_left=membership(b, a),
                               post_F0_pool_equal={r["stableId"] for r in a["decision"]["audit"]["candidates"]} ==
                                                  {r["stableId"] for r in b["decision"]["audit"]["candidates"]})
            differences.append(out)
    result = {"experiment": ID, "fixtures": 12, "sides": 48, "days": len(days),
              "pairs": len(comparisons), "differences": differences, "comparisons": comparisons,
              "all_days": days, "verified_files": 1044, "script_sha256": sha(Path(__file__)),
              "inputs": {str(p.relative_to(ROOT)): h for p, h in expected.items()},
              "audit_boundary": "After F0 candidate admission, before W1 shortlist. Absence does not prove missing generation.",
              "clock_causation_proven": False, "counterfactual_suffix_dominance_proven": False,
              "production_or_holdout_authorized": False}
    path = ROOT / "research/evidence" / (ID + ".json")
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, sort_keys=True, indent=2)
        stream.write("\n")
    print(json.dumps({k: result[k] for k in ("fixtures", "sides", "days", "pairs", "verified_files")}))
    print("result_sha256", sha(path))
    for row in differences:
        print(json.dumps({k: v for k, v in row.items() if k not in ("inventories", "protected")}))


if __name__ == "__main__":
    run()
