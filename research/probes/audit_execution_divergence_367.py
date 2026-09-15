"""Offline complete-evidence forensic audit; never launches a solver or reads P2."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research/probes"))
from audit_candidate_availability_363 import identity, inventory, membership, semantic
from inactive_runtime_aa_342 import LABELS, compare_runs

ID = "ATTR-INACTIVE-EXECUTION-DIVERGENCE-367"
SETS = (
    ("362DEV", "362/completed", "SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362", "development", 12,
     "D0C8AB6AEB8E27C5A607AC6C31FB5A63F41A288632B38E599F6E29FA8170F7F4",
     "24A5574453D6FD284CABABFBD764479AD65A68AED06E1C298923193DE7CD4791"),
    ("366DEV", "366/completed", "SCORE-CAUSAL-RESOURCE-QUALIFICATION-366", "development", 12,
     "5C31F1FFE142675563171FCA5FC6A7C432F08BD42BE0D9ED5EE541241AA0A376",
     "62E584047290605FE72C83B0AE4129BD36EA7E789866F324B56F481038CA8562"),
    ("366HOLDOUT", "366/completed-holdout", "SCORE-CAUSAL-RESOURCE-QUALIFICATION-366", "holdout", 18,
     "88E7FF81955C7AFAC838D430508809B8B21C2AC00C9906E7BA6E3994D9EF00B3",
     "EE02542F975343DC8E202C343EFD1F6EC57C2685F714994A076CEB71F65291A4"),
)
PAIRS = (("AB_A", "parentA", "candidateA"), ("AB_B", "parentB", "candidateB"),
         ("AA_parent", "parentA", "parentB"), ("AA_candidate", "candidateA", "candidateB"))


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest().upper()


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def verify_files(directory, files):
    for name, expected in files.items():
        path = (directory / name).resolve()
        if not path.is_relative_to(directory.resolve()):
            raise ValueError("foreign evidence path")
        if sha(path) != expected:
            raise ValueError("evidence hash mismatch: " + name)


def day_detail(doc, ref, action):
    decision = doc["decision"]
    return {"main_plan_hash": identity(decision["candidate"]["plan"]),
            "submitted_plan_hash": identity(action["plan"]),
            "main_score": decision["candidate"].get("scoreAfterToday"),
            "validated": action["validated"],
            "timing": decision["timing"], "deadline": decision["deadline"],
            "protected": ref,
            "audit": {k: v for k, v in decision["audit"].items() if k != "candidates"}}


def run():
    result = {"experiment": ID, "sets": [], "comparisons": [], "differences": [],
              "script_sha256": sha(Path(__file__)), "solver_executed": False,
              "protected_read": False, "past_clock_events_reconstructed": False}
    for name, folder, score_id, phase, expected_count, summary_sha, complete_sha in SETS:
        copy = ROOT / "artifacts/research" / folder
        data = copy / "research/evidence" / (score_id + "-" + phase)
        summary = data.with_suffix(".summary.json")
        complete = data / "run_complete.json"
        assert sha(summary) == summary_sha and sha(complete) == complete_sha
        marker = load(complete)
        verify_files(data, marker["files"])
        case_path = copy / "research/holdouts" / (score_id + "-" + phase + ".json")
        cases = [c for c in load(case_path)["cases"] if c["window_ms"] == 5000]
        assert len(cases) == expected_count
        meta = {"name": name, "fixtures": len(cases), "sides": 4 * len(cases),
                "summary_sha256": summary_sha, "completion_sha256": complete_sha,
                "cases_sha256": sha(case_path), "verified_files": len(marker["files"]),
                "days": 0}
        for case in cases:
            runs = {}
            for label, (repeat, side) in LABELS.items():
                prefix = data / repeat / side / str(case["seed"])
                events = [json.loads(line) for line in prefix.with_suffix(".replay.jsonl").read_text().splitlines()]
                assert not any(e["kind"] == "resource_marginal" for e in events)
                docs = [e["body"] for e in events if e["kind"] == "decision"]
                refs = {e["body"]["day"]: e["body"] for e in events if e["kind"] == "protected_slack"}
                transport = load(prefix.with_suffix(".transport.json"))
                assert len(docs) == len(transport["actions"]) == case["days"]
                assert len(refs) == case["days"]
                assert case["setup"]["daySeconds"] == [5] * case["days"]
                assert all(0 < r["computeWindowMs"] <= r["outerWindowMs"] <= 5000 for r in refs.values())
                assert not any(e["kind"] == "public_continuation" and e["body"].get("publicContinuationAuthorized") for e in events)
                runs[label] = (transport, docs, refs)
                meta["days"] += len(docs)
            for kind, left, right in PAIRS:
                ta, da, ra = runs[left]
                tb, db, rb = runs[right]
                pair = compare_runs(ta, tb)
                common = {"set": name, "seed": case["seed"], "kind": kind,
                          "left": left, "right": right, **pair}
                result["comparisons"].append(common)
                if pair["trajectory_equal"]:
                    continue
                day = pair["first_divergence"]
                out = {**common, "first_day": day,
                       "strata": {k: v for k, v in case.items() if k != "setup"},
                       "replays": [str((data / LABELS[l][0] / LABELS[l][1] / (str(case["seed"]) + ".replay.jsonl")).relative_to(ROOT)) for l in (left, right)]}
                if day == 0:
                    out["boundary"] = "roles"
                else:
                    a, b = da[day - 1], db[day - 1]
                    main_equal = a["decision"]["candidate"]["plan"] == b["decision"]["candidate"]["plan"]
                    out.update(state_equal=semantic(a["state"]) == semantic(b["state"]),
                               ledger_equal=a["ledger"] == b["ledger"], main_equal=main_equal,
                               boundary="post-main-protected-or-submission" if main_equal else "main-selection",
                               inventories={left: inventory(a), right: inventory(b)},
                               left_selected_in_right=membership(a, b), right_selected_in_left=membership(b, a),
                               post_F0_pool_equal={r["stableId"] for r in a["decision"]["audit"]["candidates"]} ==
                                                 {r["stableId"] for r in b["decision"]["audit"]["candidates"]},
                               first_day_detail={left: day_detail(a, ra[day], ta["actions"][day - 1]),
                                                 right: day_detail(b, rb[day], tb["actions"][day - 1])},
                               all_days={l: [day_detail(d, refs[i + 1], t["actions"][i]) for i, d in enumerate(docs)]
                                         for l, (t, docs, refs) in ((left, runs[left]), (right, runs[right]))})
                    if not out["state_equal"] or not out["ledger_equal"]:
                        out["boundary"] = "incoming-state-or-ledger"
                result["differences"].append(out)
        result["sets"].append(meta)
    result["counts"] = {"fixtures": sum(x["fixtures"] for x in result["sets"]),
                        "sides": sum(x["sides"] for x in result["sets"]),
                        "days": sum(x["days"] for x in result["sets"]),
                        "pairs": len(result["comparisons"]), "divergences": len(result["differences"]),
                        "boundaries": dict(Counter(r["boundary"] for r in result["differences"]))}
    out = ROOT / "research/evidence" / (ID + ".json")
    with out.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, sort_keys=True, indent=2)
        stream.write("\n")
    print(json.dumps(result["counts"]))
    print("result_sha256", sha(out))
    for row in result["differences"]:
        print(json.dumps({k: row.get(k) for k in ("set", "seed", "kind", "first_day", "boundary", "delta", "post_F0_pool_equal")}))


if __name__ == "__main__":
    run()
