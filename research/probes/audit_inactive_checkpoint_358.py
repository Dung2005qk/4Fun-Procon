"""Offline audit of ALL completed 357 inactive lanes; never executes a solver."""
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "artifacts/research/357/completed"
sys.path.insert(0, str(PACKAGE / "research/probes"))
from inactive_runtime_aa_342 import LABELS, compare_runs
from run_resource_357 import audit, verify

ID = "ATTR-INACTIVE-CHECKPOINT-CAUSALITY-358"
DATA = PACKAGE / "research/evidence/SCORE-PROTECTED-RESOURCE-MARGINAL-357-development"
EXPECTED = {
    "execution357.json": "F0188694FF3E0042D1B85F8111F06E42AF7F1538A71C6AECBB44F384ED4C102C",
    "research/evidence/SCORE-PROTECTED-RESOURCE-MARGINAL-357-development.summary.json":
        "40D92A3B69923CE551DE5087090B2131FBD70B37A412691E33C00DA589DF11CC",
    "research/evidence/SCORE-PROTECTED-RESOURCE-MARGINAL-357-development/run_complete.json":
        "146E3E25722E2287E6C22C56D46CDD6A139245100448E459321398321556F4CE",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def semantic_state(state):
    return {k: v for k, v in state.items() if k != "endsAt"}


def run():
    for name, digest in EXPECTED.items():
        assert sha(PACKAGE / name) == digest, name
    execution = verify()
    complete = load(DATA / "run_complete.json")
    for name, digest in complete["files"].items():
        assert sha(DATA / name) == digest, name
    inputs = load(PACKAGE / "research/holdouts/SCORE-PROTECTED-RESOURCE-MARGINAL-357.json")
    split = inputs["splits"]["development"]
    all_cases = load(PACKAGE / split["path"])["cases"]
    audit(all_cases, DATA)
    cases = [c for c in all_cases if c["window_ms"] == 5000]
    assert len(cases) == 12
    rows, controls, changed, sides = [], [], [], []
    for case in cases:
        seed = case["seed"]
        cached = {}
        for label, (repeat, side) in LABELS.items():
            prefix = DATA / repeat / side / str(seed)
            events = [json.loads(line) for line in prefix.with_suffix(".replay.jsonl").read_text().splitlines()]
            documents = [e["body"] for e in events if e["kind"] == "decision"]
            refinements = {e["body"]["day"]: e["body"] for e in events if e["kind"] == "protected_slack"}
            resource_frames = [e for e in events if e["kind"] == "resource_marginal"]
            public_flags = [e["body"]["publicContinuationAuthorized"] for e in events
                            if isinstance(e.get("body"), dict) and "publicContinuationAuthorized" in e["body"]]
            assert len(documents) == case["days"]
            assert not resource_frames and not any(public_flags)
            assert all(v == 5 for v in case["setup"]["daySeconds"])
            # Received-state delay can shrink an authoritative five-second window.
            assert all(0 < x["computeWindowMs"] <= x["outerWindowMs"] <= 5000 for x in refinements.values())
            sides.append({"seed": seed, "label": label, "days": len(documents),
                          "resource_frames": len(resource_frames), "public_flags_observed": len(public_flags),
                          "true_public_flags": sum(public_flags)})
            cached[label] = (load(prefix.with_suffix(".transport.json")), documents, refinements)
        pairs = [("AB_A", "parentA", "candidateA"), ("AB_B", "parentB", "candidateB"),
                 ("AA_parent", "parentA", "parentB"), ("AA_candidate", "candidateA", "candidateB")]
        for kind, left, right in pairs:
            ta, da, ra = cached[left]
            tb, db, rb = cached[right]
            comparison = compare_runs(ta, tb)
            row = {"seed": seed, "kind": kind, "left": left, "right": right, **comparison}
            (rows if kind.startswith("AB") else controls).append(row)
            if comparison["trajectory_equal"]:
                continue
            day = comparison["first_divergence"]
            detail = {"seed": seed, "kind": kind, "first_day": day,
                      "scores": [comparison["A"], comparison["B"]]}
            if day == 0:
                detail["first_boundary"] = "role-selection"
            else:
                a, b = da[day - 1], db[day - 1]
                pa, pb = a["decision"]["candidate"]["plan"], b["decision"]["candidate"]["plan"]
                detail.update({
                    "semantic_state_equal": semantic_state(a["state"]) == semantic_state(b["state"]),
                    "ledger_equal": a["ledger"] == b["ledger"],
                    "main_plan_equal": pa == pb,
                    "main_score_equal": a["decision"]["candidate"]["scoreAfterToday"] == b["decision"]["candidate"]["scoreAfterToday"],
                    "main_timing": [a["decision"]["timing"], b["decision"]["timing"]],
                    "main_deadline": [a["decision"]["deadline"], b["decision"]["deadline"]],
                    "main_plan_hashes": [hashlib.sha256(json.dumps(p, sort_keys=True).encode()).hexdigest().upper() for p in (pa, pb)],
                    "protected_refinement": [ra.get(day), rb.get(day)],
                    "first_boundary": "main-decision" if pa != pb else "later-canonical-protected-refinement-or-submission",
                })
            changed.append(detail)
    source = PACKAGE / "source/src/btc_main.cpp"
    lines = source.read_text(encoding="utf-8").splitlines()
    consumers = [{"line": i, "source": line.strip()} for i, line in enumerate(lines, 1)
                 if "resourceMarginal357" in line or "resourceFlag357" in line]
    # Inventory, not a formal compiler/timing noninterference proof.
    assert len(consumers) == 5
    suffix_guards = [x for x in consumers if "if (options.resourceMarginal357" in x["source"]]
    assert len(suffix_guards) == 2 and all("publicContinuationAuthorized" in x["source"] for x in suffix_guards)
    result = {
        "experiment": ID, "source": str(source.relative_to(ROOT)), "source_sha256": sha(source),
        "script_sha256": sha(Path(__file__)), "frozen_inputs": EXPECTED,
        "verified_execution_dependencies": len(execution["hashes"]),
        "verified_completed_files": len(complete["files"]),
        "fixtures": len(cases), "sides": len(sides), "days": sum(s["days"] for s in sides),
        "new_suffix_frames": sum(s["resource_frames"] for s in sides),
        "true_public_flags": sum(s["true_public_flags"] for s in sides),
        "ab_changed": sum(not r["trajectory_equal"] for r in rows),
        "aa_changed": sum(not r["trajectory_equal"] for r in controls),
        "pairs": rows, "controls": controls, "changed": changed, "sides_detail": sides,
        "flag_consumers": consumers,
        "authority": "Offline source/recorded-boundary attribution, not a new run, timing-causation proof or promotion gate",
        "promotion_authorized": False, "holdout_authorized": False,
    }
    output = ROOT / f"research/evidence/{ID}.json"
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps({k: result[k] for k in ["fixtures", "sides", "days", "new_suffix_frames", "ab_changed", "aa_changed"]}))
    print("summary_sha256", sha(output))
    for row in changed:
        print(json.dumps({k: v for k, v in row.items() if k not in ["main_timing", "main_deadline", "protected_refinement"]}))


if __name__ == "__main__":
    run()
