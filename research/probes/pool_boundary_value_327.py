"""Read-only consumed-pool attribution using the unchanged326 exact oracle."""
import argparse
from collections import Counter
import copy
import json
from pathlib import Path
import subprocess
import time
from http_prefix_option_loss_321 import ROOT, load, digest, object_hash, require, write_new, state, close_bridge

ID = "ATTR-POOL-BOUNDARY-OPTION-VALUE-327"
M = ROOT / f"research/holdouts/{ID}.json"
D = ROOT / f"research/evidence/{ID}"
S = ROOT / f"research/evidence/{ID}.summary.json"
P326 = "ATTR-CERTIFIED-PORTFOLIO-REPRESENTATIVES-326"
P325 = "ATTR-INDEPENDENT-COLUMN-RETENTION-325"
P323 = "ATTR-DAY2-POOL-EXACT-VALUE-323"

def singleton(plan, actual):
    """One plan per portfolio: never mix agents from different candidates."""
    require(len(plan) == len(actual["agents"]) == 3, "three-agent scope")
    return [[{"agent": a, "plan": [copy.deepcopy(actions)], "contingencyBundle": 0,
              "terminalCell": actual["agents"][a]["pos"], "terminalFuel": actual["agents"][a]["fuel"],
              "firstVisits": [{"spot": c["spot"], "claimed": True} for c in actual["claims"] if c["agent"] == a]}]
            for a, actions in enumerate(plan)]

def verify_local(m):
    for p, h in m["local_hashes"].items():
        require(digest(ROOT / p) == h, "local frozen drift: " + p)

def freeze():
    from run_http_baseline_314 import Bridge
    m326 = ROOT / f"research/holdouts/{P326}.json"
    old = load(m326); verify_local(old)
    s326 = ROOT / f"research/evidence/{P326}.summary.json"
    require(digest(s326) == "2A9FE9C4A7338DC33494313819F5C541193A7FCAAC463437E15A8959640184F9", "326 summary")
    s325 = ROOT / f"research/evidence/{P325}.summary.json"
    require(digest(s325) == "25B76905EA5E520DD9B2FA65D2E99552CD674361E629DB9998D17C5B50C8B35F", "325 summary")
    m323 = ROOT / f"research/holdouts/{P323}.json"
    verify_local(load(m323))
    s323 = ROOT / f"research/evidence/{P323}.summary.json"
    require(digest(s323) == "9783F5DA1CA4E7DF375EED0ECF2E71CA156A8016E2E87F18424786C9DA976134", "323 summary")
    roots = {c["seed"]: c for c in old["cases"]}
    refs = {r["seed"]: r for r in load(s326)["results"]}
    finals = {r["seed"]: r for r in load(s323)["matches"]}
    cases = []; count = 0; occurrences = 0
    bridge = Bridge(ROOT / "artifacts/research/324/claims_bridge.exe")
    try:
        for row in load(s325)["results"]:
            require(row["qualified"] and row["same_final_pool_ids"], "historical identity")
            seed = row["seed"]; root = roots[seed]
            q = {k: copy.deepcopy(root["request"][k]) for k in ("setup", "state", "ledger")}
            entries = {}
            for pool, values in sorted(row["pools"].items()):
                for index, v in enumerate(values):
                    sid = v["stableId"]; occurrences += 1
                    entry = entries.setdefault(sid, {"plan": json.loads(sid), "memberships": [], "actual": v["actual"]})
                    require(entry["actual"] == v["actual"], "same plan inconsistent outcome")
                    entry["memberships"].append({"pool": pool, "index": index})
            for pool in ("legacy", "merged"):
                witness = refs[seed]["work"]["portfolios"][pool]
                plan = witness["days"][0]["plan"]; sid = json.dumps(plan, separators=(",", ":"))
                checked = bridge.request({**q, "op": "step", "plan": plan})
                require(checked.get("ok") and checked.get("agrees"), "retained control invalid")
                for k in ("score", "ledger", "agents"):
                    require(checked[k] == witness["days"][0][k], "retained control identity")
                entry = entries.setdefault(sid, {"plan": plan, "memberships": [], "actual": checked})
                require(entry["actual"] == checked, "control action identity")
                entry["memberships"].append({"pool": "retained-control-" + pool, "index": 0})
                entry.setdefault("expected_values", []).append({"source": "326-" + pool, "score": witness["score"]})
            for ref in finals[seed]["candidates"]:
                sid = ref["candidate"]["stableId"]
                require(sid in entries, "missing final candidate")
                entries[sid]["audit"] = ref["candidate"]
                entries[sid].setdefault("expected_values", []).append({"source": "323", "score": ref["value"]})
            encoded = {}; plans = {}
            for i, (sid, entry) in enumerate(sorted(entries.items())):
                label = f"p{i:03d}"
                checked = bridge.request({**q, "op": "step", "plan": entry["plan"]})
                require(checked.get("ok") and checked.get("agrees") and checked == entry["actual"], "recorded plan dual identity")
                encoded[label] = singleton(entry["plan"], checked)
                plans[label] = {"stableId": sid, **entry}
            selected = [e for e in plans.values() if e.get("audit", {}).get("selected")]
            require(len(selected) == 1, "selected action identity")
            count += len(plans)
            cases.append({"id": str(seed), "seed": seed, "family": row["family"], "players": row["players"],
                "plans": plans, "selected_value": root["selected"], "selected_today": selected[0]["actual"]["score"],
                "ceiling": root["ceiling"], "request": {**q, "portfolios": encoded, "certificates": []}})
    finally: close_bridge(bridge)
    paths = set(old["local_hashes"]) | set(load(m323)["local_hashes"])
    paths.update(str(p.relative_to(ROOT)).replace('\\', '/') for p in (m326, s326, s325, m323, s323,
        Path(__file__), Path(__file__).with_name("test_pool_boundary_value_327.py")))
    require(len(cases) == 12 and occurrences == 1257, "whole recorded pool coverage")
    write_new(M, {"experiment": ID, "cases": cases, "unique_plans": count, "recorded_occurrences": occurrences,
        "local_hashes": {p: digest(ROOT / p) for p in sorted(paths)}, "vm_hashes": old["vm_hashes"],
        "probe_sha256": old["probe_sha256"], "runner_sha256": digest(Path(__file__)),
        "helper_sha256": digest(Path(__file__).with_name("http_prefix_option_loss_321.py")),
        "bridge": "artifacts/research/314/bridge.exe", "min_mem_available_kib": 4*1024*1024,
        "gate": "All12 and all1257 recorded occurrences; dual-valid all complete suffixes; exact323/326 controls and321 ceiling. Shared-boundary recurrence2cases2families permits distinct invariant design only.",
        "production_change": False, "holdout_authority": False, "score_successor_authorized": False})
    print(json.dumps({"manifest_sha256": digest(M), "cases": 12, "unique_plans": count, "occurrences": occurrences}))

def validate(r, c, mh, ph):
    require(r["id"] == c["id"] and r["manifest_sha256"] == mh and r["probe_sha256"] == ph and
            r["request_sha256"] == object_hash(c["request"]), "result provenance")
    o = r["oracle"]
    require(o.get("ok") and o.get("complete") and o["certificates_checked"] == 0 and
            set(o["portfolios"]) == set(c["plans"]), "whole oracle output")
    for label, p in o["portfolios"].items():
        require(p["complete"] and [d["day"] for d in p["days"]] == [2, 3, 4] and
                p["admissible_combinations"] == p["unique_joint_roots"] == 1 and
                p["incompatible_combinations"] == 0 and p["columns_before"] == 3 and
                p["days"][0]["plan"] == c["plans"][label]["plan"], "singleton action identity")

def execute(manifest, mh, directory, probe):
    import fcntl
    m = load(manifest)
    require(digest(manifest) == mh and digest(Path(__file__)) == m["runner_sha256"], "manifest/runner drift")
    require(digest(Path(__file__).with_name("http_prefix_option_loss_321.py")) == m["helper_sha256"], "helper drift")
    for p, h in m["vm_hashes"].items(): require(digest(p) == h, "VM input drift: " + p)
    require(digest(probe) == m["probe_sha256"] and len(m["cases"]) == 12, "probe/count")
    directory.mkdir(exist_ok=True)
    with (directory / "runner.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        require(not list(directory.glob("*.partial")), "ambiguous partial: audit first")
        require({p.name for p in directory.glob("*.result.json")} <= {c["id"] + ".result.json" for c in m["cases"]}, "unknown result")
        for c in m["cases"]:
            out = directory / (c["id"] + ".result.json")
            if out.exists(): validate(load(out), c, mh, m["probe_sha256"]); continue
            mem = {line.split()[0].rstrip(':'): int(line.split()[1]) for line in Path('/proc/meminfo').read_text().splitlines()}
            require(mem["MemAvailable"] >= m["min_mem_available_kib"], "memory safety")
            partial = directory / (c["id"] + ".partial"); err = directory / (c["id"] + ".stderr")
            require(not err.exists() and not (directory / (c["id"] + ".stdout.json")).exists(), "orphaned case")
            started = time.monotonic()
            with partial.open("x", encoding="utf-8") as stdout, err.open("x", encoding="utf-8") as stderr:
                p = subprocess.run([str(probe)], input=json.dumps(c["request"])+"\n", text=True, stdout=stdout, stderr=stderr)
            require(p.returncode == 0 and err.stat().st_size == 0, "oracle process error")
            r = {"id": c["id"], "manifest_sha256": mh, "probe_sha256": m["probe_sha256"],
                "request_sha256": object_hash(c["request"]), "oracle": load(partial), "elapsed_seconds": time.monotonic()-started}
            validate(r, c, mh, m["probe_sha256"])
            commit = directory / (c["id"] + ".commit.partial"); write_new(commit, r); commit.rename(out)
            partial.rename(directory / (c["id"] + ".stdout.json"))
            print(json.dumps({"event": "case_complete", "id": c["id"], "sha256": digest(out)}), flush=True)
        marker = {"experiment": ID, "manifest_sha256": mh, "cases": 12,
                  "results": {p.name: digest(p) for p in sorted(directory.glob("*.result.json"))}}
        require(len(marker["results"]) == 12, "complete result set")
        if (directory / "run_complete.json").exists(): require(load(directory / "run_complete.json") == marker, "marker drift")
        else: write_new(directory / "run_complete.json", marker)
        print(json.dumps({"event": "run_complete", "cases": 12}), flush=True)

def summarize(manifest, directory, output):
    from run_http_baseline_314 import Bridge
    from day2_pool_value_323 import comparison
    m = load(manifest); verify_local(m); mh = digest(manifest)
    marker = load(directory / "run_complete.json")
    require(marker["cases"] == 12 and marker["manifest_sha256"] == mh, "completion marker")
    require(set(marker["results"]) == {c["id"] + ".result.json" for c in m["cases"]} ==
            {p.name for p in directory.glob("*.result.json")}, "complete root coverage")
    require(not list(directory.glob("*.partial")), "partial evidence")
    bridge = Bridge(ROOT / m["bridge"]); matches = []; dual = 0; controls = 0
    try:
        for c in m["cases"]:
            p = directory / (c["id"] + ".result.json"); require(digest(p) == marker["results"][p.name], "result hash")
            r = load(p); validate(r, c, mh, m["probe_sha256"]); plans = []; pools = {}
            for label, meta in c["plans"].items():
                answer = r["oracle"]["portfolios"][label]
                agents, ledger = c["request"]["state"]["agents"], c["request"]["ledger"]
                for day in answer["days"]:
                    checked = bridge.request({"op": "step", "setup": c["request"]["setup"],
                        "state": state(agents, day["day"]), "ledger": ledger, "plan": day["plan"]})
                    require(checked.get("ok") and checked.get("agrees") and all(checked[k] == day[k] for k in ("score", "agents", "ledger")), "dual suffix reconstruction")
                    agents, ledger = checked["agents"], checked["ledger"]; dual += 1
                value = answer["score"]
                require(value == answer["days"][-1]["score"] and tuple(value) <= tuple(c["ceiling"]), "exact ceiling")
                for expected in meta.get("expected_values", []):
                    require(value == expected["score"], "frozen323/326 exact value mismatch"); controls += 1
                for membership in meta["memberships"]:
                    pools.setdefault(membership["pool"], []).append((label, value))
                plans.append({"label": label, "stableId": meta["stableId"], "memberships": meta["memberships"],
                    "value": value, "versus_selected": comparison(value, c["selected_value"]),
                    "today_versus_selected": comparison(meta["actual"]["score"], c["selected_today"]),
                    "audit": meta.get("audit"), "expected_values": meta.get("expected_values", [])})
            boundaries = {pool: {"best": max((v for _, v in rows), key=tuple), "count": len(rows)} for pool, rows in pools.items()}
            for b in boundaries.values(): b["versus_selected"] = comparison(b["best"], c["selected_value"])
            matches.append({"seed": c["seed"], "family": c["family"], "players": c["players"], "ceiling": c["ceiling"],
                "selected": c["selected_value"], "boundaries": boundaries, "plans": plans})
    finally: close_bridge(bridge)
    require(len(matches) == 12 and dual == 3*m["unique_plans"] and controls == 216, "complete validation coverage")
    stages = {}
    for pool in ("initial-master", "legacy-master", "pre-f0", "final-audit"):
        gains = [r for r in matches if r["boundaries"][pool]["versus_selected"]["result"] == "win"]
        stages[pool] = {"gains": [r["seed"] for r in gains], "families": sorted({r["family"] for r in gains}),
            "distinct_mechanism_design_qualified": len(gains) >= 2 and len({r["family"] for r in gains}) >= 2}
    report = {"experiment": ID, "manifest_sha256": mh, "run_complete_sha256": digest(directory / "run_complete.json"),
        "result_hashes": marker["results"], "roots": 12, "unique_plans": m["unique_plans"],
        "recorded_occurrences": m["recorded_occurrences"], "dual_days": dual, "exact_prior_controls": controls,
        "zero_failure": True, "matches": matches, "stages": stages, "production_change": False,
        "holdout_authority": False, "score_successor_authorized": False,
        "strata": {field: {str(key): {pool: dict(Counter(r["boundaries"][pool]["versus_selected"]["result"]
            for r in matches if r[field] == key)) for pool in stages} for key in sorted({r[field] for r in matches})}
            for field in ("family", "players")}}
    write_new(output, report)
    print(json.dumps({"summary_sha256": digest(output), "roots": 12, "dual_days": dual, "controls": controls, "stages": stages}))
    for r in matches: print(json.dumps({k: v for k, v in r.items() if k != "plans"}))

if __name__ == "__main__":
    p = argparse.ArgumentParser(); p.add_argument("mode", choices=("freeze", "execute", "summarize"))
    p.add_argument("--manifest", type=Path, default=M); p.add_argument("--manifest-sha256")
    p.add_argument("--directory", type=Path, default=D); p.add_argument("--probe", type=Path)
    p.add_argument("--output", type=Path, default=S); a = p.parse_args()
    if a.mode == "freeze": freeze()
    elif a.mode == "execute": execute(a.manifest, a.manifest_sha256, a.directory, a.probe)
    else: summarize(a.manifest, a.directory, a.output)
