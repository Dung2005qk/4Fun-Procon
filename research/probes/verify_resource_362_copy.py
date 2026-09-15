"""Read-only verification and complete-result reporting for the exact 362 copy.

No solver execution, partial-result analysis, source edit, or sealed setup read.
The frozen summarizer is reused unchanged; only its filesystem/host verification
adapter is mapped to independently verified local copies.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
COPY = ROOT / "artifacts/research/362/completed"
STAGE = ROOT / "artifacts/research/362/stage"
OLD = ROOT / "artifacts/research/357/completed"
ID = "SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362"
DATA = COPY / "research/evidence" / (ID + "-development")


def load(p):
    return json.loads(p.read_text(encoding="utf-8"))


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest().upper()


def verify_copy():
    expected = {
        "input362.json": "09FF94371D108006FAB7E3E774A25EC3167E500516372A5D31FDB95118CCD853",
        "execution362.json": "586AEAC11D9D0EBD7221FF9E59EE7D1BD558E6F268D5BD58DD73DF7240C5E11E",
        "stage362.json": "B17A4190C8BFFFEEEEDF6CB01B3EA614B95BF3B23A69908FAF2E7EDC3A4681EF",
        "score_resource_362.py": "E50B50BC5AF560687AD32598CDC4F8FD04341F0AA742F8EEDC400A9D2E48FD7A",
        f"research/evidence/{ID}-development.summary.json": "D0C8AB6AEB8E27C5A607AC6C31FB5A63F41A288632B38E599F6E29FA8170F7F4",
        f"research/evidence/{ID}-development/run_complete.json": "24A5574453D6FD284CABABFBD764479AD65A68AED06E1C298923193DE7CD4791",
    }
    assert sha(COPY.parent / "complete-development.tar.gz") == "142BBB9604113E731ABF1941628B7740A98204E24B6949A39272A5238B317236"
    for name, h in expected.items():
        assert sha(COPY / name) == h, name
    execution = load(COPY / "execution362.json")
    for name, h in execution["hashes"].items():
        p = COPY / name
        if not p.exists():
            p = STAGE / name
        assert sha(p) == h, name
    from verify_late_control_360_copy import verify as boundary360
    from verify_resume_control_361_copy import verify as boundary361
    assert boundary360()["verified"] and boundary361()["verified"]
    complete = load(DATA / "run_complete.json")
    actual = {p.relative_to(DATA).as_posix() for p in DATA.rglob("*") if p.is_file() and p.name != "run_complete.json"}
    assert actual == set(complete["files"])
    for name, h in complete["files"].items():
        assert sha(DATA / name) == h, name
    results = list(DATA.glob("*/*/*.result.json"))
    assert len(results) == len(list(DATA.glob("*/*/*.certificate.json"))) == 144
    assert len(list(DATA.glob("*.fixture_complete.json"))) == 36
    rows = [load(p) for p in results]
    assert sum(r["actions"] for r in rows) == 912
    assert sum(r["transitions"] for r in rows) == 768
    assert all(r["failure"] is None for r in rows)
    assert all(p.stat().st_size == 0 for p in DATA.rglob("*.stderr"))
    assert (COPY / "runner362.stderr").stat().st_size == 0
    return execution


def run(report=False):
    # Imported 357 dependencies are the frozen downloaded copies, not a new runner.
    sys.path.insert(0, str(OLD / "research/probes"))
    spec = importlib.util.spec_from_file_location("frozen362_copy", COPY / "score_resource_362.py")
    s = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(s)
    s.WORK = COPY
    s.EXEC = COPY / "execution362.json"
    s.INPUT = COPY / "input362.json"
    s.verify = verify_copy
    s.summarize("development", check=True)
    summary = load(COPY / "research/evidence" / (ID + "-development.summary.json"))
    rows = summary["rows"]
    inactive = [r for r in rows if r["window_ms"] == 5000]
    detail = {
        "verified": True, "files": 1044, "results": 144, "fixtures": 36,
        "acks": 912, "transitions": 768, "summary_sha256": sha(COPY / "research/evidence" / (ID + "-development.summary.json")),
        "completion_sha256": sha(DATA / "run_complete.json"),
        "archive_sha256": sha(COPY.parent / "complete-development.tar.gz"),
        "gate": summary["gate"], "summary": summary["summary"],
        "active": summary["active_only_summary"], "inactive": s.f.aggregate(inactive),
        "repeats": summary["repeats"], "strata": summary["strata"],
        "per_repeat_strata": {repeat: {k: {str(v): s.f.aggregate([r for r in rows if r["repeat"] == repeat and r[k] == v])
            for v in sorted({r[k] for r in rows})} for k in s.f.STRATA} for repeat in ("A", "B")},
        "losses": [{k: v for k, v in r.items() if k != "certificate"} for r in rows if r["comparison_B_vs_A"] == "loss"],
        "gains": [{k: v for k, v in r.items() if k != "certificate"} for r in rows if r["comparison_B_vs_A"] == "win"],
        "inactive_trajectory_mismatches": summary["independent5000_trajectory_mismatches"],
        "inactive_score_mismatches": summary["independent5000_score_mismatches"],
        "aa_inactive": {side: {"pairs": len(rr := [r for r in summary["same_binary_controls"] if r["window_ms"] == 5000 and r["side_binary"] == side]),
            "trajectory_mismatches": sum(not r["trajectory_equal"] for r in rr), "score_mismatches": sum(r["A"] != r["B"] for r in rr)} for side in ("parent", "candidate")},
        "work": {}, "first_inactive_divergences": [],
    }
    frames = [x for r in rows for x in r["certificate"]["frames"]]
    for key in ("entered", "takeover", "deadline", "failure", "queries", "settled", "routes", "evaluated", "valid", "certified"):
        detail["work"][key] = sum(x[key] for x in frames)
    for r in inactive:
        if r["trajectory_equal"]:
            continue
        day = r["first_divergence"]
        out = {"seed": r["seed"], "repeat": r["repeat"], "first_day": day, "A": r["A"], "B": r["B"]}
        if day == 0:
            out["boundary"] = "roles"
        else:
            ds, refs = [], []
            for side in ("parent", "candidate"):
                p = DATA / r["repeat"] / side / f"{r['seed']}.replay.jsonl"
                events = [json.loads(line) for line in p.read_text().splitlines()]
                ds.append([e["body"] for e in events if e["kind"] == "decision"][day - 1])
                refs.append(next(e["body"] for e in events if e["kind"] == "protected_slack" and e["body"]["day"] == day))
            a, b = ds
            out.update(state_equal={k: v for k, v in a["state"].items() if k != "endsAt"} == {k: v for k, v in b["state"].items() if k != "endsAt"},
                ledger_equal=a["ledger"] == b["ledger"], main_plan_equal=a["decision"]["candidate"]["plan"] == b["decision"]["candidate"]["plan"],
                main_scores=[x["decision"]["candidate"]["scoreAfterToday"] for x in ds], main_timing=[x["decision"]["timing"] for x in ds], protected=refs)
            out["boundary"] = "protected-refinement-or-submission" if out["main_plan_equal"] else "main-decision"
        detail["first_inactive_divergences"].append(out)
    if report:
        p = ROOT / "research/evidence" / (ID + "-development-audit.json")
        with p.open("x", encoding="utf-8", newline="\n") as stream:
            json.dump(detail, stream, indent=2, sort_keys=True)
            stream.write("\n")
        print("audit_sha256", sha(p))
    print(json.dumps({k: detail[k] for k in ("verified", "acks", "transitions", "aa_inactive", "work")}))
    for r in detail["first_inactive_divergences"]:
        print(json.dumps({k: v for k, v in r.items() if k not in ("main_timing", "protected")}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", action="store_true")
    run(parser.parse_args().report)
