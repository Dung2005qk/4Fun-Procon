"""Finalize isolated candidate, full protected inputs and executable closure before SCORE."""
import difflib
import json
import subprocess
import sys
from run_horizon_score_332 import ROOT, ID, M, load, digest, write_new, require
from run_http_baseline_314 import Bridge


def rel(p): return str(p.relative_to(ROOT)).replace("\\", "/")


def main():
    require(not M.exists(), "execution closure already frozen")
    base_path = ROOT/f"research/holdouts/{ID}.json"; base = load(base_path)
    require(digest(base_path) == "BC29A422CCCBD97AE457B8D5AAF2B362E8D10C6E5396F0739A65CF3F55D8ABF9", "initial freeze identity")
    for p,h in base["parent_hashes"].items(): require(digest(ROOT/p)==h, "parent changed: "+p)
    for entry in base["splits"].values(): require(digest(ROOT/entry["path"])==entry["sha256"], "split changed")
    require(subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()==base["parent_commit"], "HEAD changed")
    # Test evidence is reproducible, create-only. This runs no fresh match.
    tests = subprocess.run([sys.executable, "-m", "unittest", "test_bounded_horizon_332",
        "test_horizon_score_runner_332", "test_http_baseline_314"], cwd=ROOT/"research/probes", capture_output=True, text=True)
    cache = (ROOT/"build-release/CMakeCache.txt").read_text().splitlines()
    cmake = next(s.split("=",1)[1] for s in cache if s.startswith("CMAKE_COMMAND:INTERNAL="))
    unit = subprocess.run([str(__import__("pathlib").Path(cmake).with_name("ctest.exe")), "--test-dir",
        "artifacts/research/332/build", "--output-on-failure"], cwd=ROOT, capture_output=True, text=True)
    require(tests.returncode==0 and unit.returncode==0, "pre-freeze contract/unit failure")
    test_path = ROOT/f"research/evidence/{ID}-preflight.json"
    write_new(test_path, {"tests": tests.stdout+tests.stderr, "unit": unit.stdout+unit.stderr,
        "no_fresh_score_run": True, "old_caller_modes":5, "consumed330_controls":27,
        "authority":"Correctness only, not performance or promotion."})

    bridge = Bridge(ROOT/"artifacts/research/332/protected-fixture.exe")
    protected = []
    try:
        for family in range(6):
            for fuel_index, fuel in enumerate(("low", "default", "high")):
                for size_index, side in enumerate((8,32)):
                    for window_index, window in enumerate((5000,10000,15000)):
                        serial = len(protected)
                        seed = 202609073000 - 202609073000%6 + 6*serial + family
                        q = {"seed":seed, "side":side, "fuel":fuel, "window_ms":window,
                            "days":(4,5,10)[(family+fuel_index+window_index)%3],
                            "players":8+(family+window_index)%3,
                            "roadless":(family+fuel_index+size_index)%2==0}
                        r = bridge.request(q); require(r.get("ok"), "protected fixture validation: "+str(r))
                        protected.append({**q, **r, "role":"fixed-all-Patrol" if (family+window_index)%2==0 else "native",
                            "order":["parent","candidate"] if serial%2==0 else ["candidate","parent"]})
    finally: bridge.close(); bridge.process.stdout.close(); bridge.process.stderr.close()
    require(len(protected)==108 and len({r["family"] for r in protected})==6, "protected coverage")
    protected_path = ROOT/f"research/holdouts/{ID}-protected.json"
    write_new(protected_path, {"experiment":ID,"cases":protected, "pairs":108,
        "authority":"Pre-frozen development protection, not the sealed primary holdout; generator exports only and never solves.",
        "traffic":"For day d sum own exact submitted road footprint plus frozen external aggregate footprints at d-1 and d-2; divide by players before applying match thresholds. Day1 smooth. Not human opponents.",
        "execution_contract":"Use actual MatchSession/HTTP lifecycle per side and public deadlines; fixed all-Patrol or native role selection within5000ms. Freeze runner/equivalence/transport tests before running. Do not substitute isolated historical engine solve.",
        "gate":"Complete108 pairs, zero safety/validity failure. No lifetime or daily component loss, worst serving loss at most1, no negative serving net in any family/fuel/size/role/horizon/window/player stratum. Report all score and plan/state/ledger differences, including inactive cutoff differences; these require attribution rather than being credited to the new mechanism. Unchanged shared daily callers must remain operation-equivalent. Primary fresh positive-gain gate and sealed holdout still mandatory; no signature from protection alone.",
        "lane_champions":"Direct canonical parent c76a8ea/accepted258 at all lanes; previous rejected330/331 research probes are not lane champions. Compare any additional actually retained champion if current provenance shows one before promotion."})

    source = ROOT/"artifacts/research/332/source"
    files = [source/"CMakeLists.txt"] + [p for folder in ("src","include","strategies/blank_slate","tests")
        for p in (source/folder).rglob("*") if p.suffix in (".cpp",".hpp")]
    changes=[]; diff=[]
    for p in sorted(files):
        key=str(p.relative_to(source)).replace("\\","/"); parent=ROOT/key
        before=parent.read_text(encoding="utf-8").splitlines(keepends=True) if parent.exists() else []
        after=p.read_text(encoding="utf-8").splitlines(keepends=True)
        if before!=after:
            changes.append(key)
            diff.extend(difflib.unified_diff(before,after,fromfile="a/"+key if parent.exists() else "/dev/null",tofile="b/"+key))
    expected={"CMakeLists.txt","include/udon/orienteering.hpp","include/udon/decision.hpp","include/udon/horizon_pricing.hpp",
              "src/orienteering.cpp","src/decision.cpp","src/audit.cpp","src/horizon_pricing.cpp"}
    require(set(changes)==expected, "unexpected candidate source scope")
    patch_path=ROOT/f"research/evidence/{ID}-candidate.patch"
    with patch_path.open("x",encoding="utf-8",newline="\n") as stream:stream.write("".join(diff))
    paths=set(base["parent_hashes"])|{rel(p) for p in files}|{
        rel(base_path), base["splits"]["development"]["path"],base["splits"]["holdout"]["path"],
        rel(protected_path),rel(test_path),rel(patch_path),
        "artifacts/research/332/build/udonshield_btc.exe","artifacts/research/332/build/udon_shield.lib",
        "artifacts/research/332/build/udonshield_tests.exe","artifacts/research/332/build/CMakeCache.txt",
        "artifacts/research/332/probe.exe","artifacts/research/332/parent-probe.exe","artifacts/research/332/protected-fixture.exe",
        "research/probes/run_horizon_score_332.py","research/probes/freeze_horizon_execution_332.py",
        "research/probes/protected_fixture_332.cpp","research/probes/bounded_horizon_probe_332.cpp",
        "research/probes/test_bounded_horizon_332.py","research/probes/test_horizon_score_runner_332.py",
        "research/probes/test_http_baseline_314.py","research/probes/test_http_prefix_oracle_321.py",
        "research/probes/http_prefix_option_loss_321.py","old/harness/historical_tournament.cpp"}
    write_new(M,{"experiment":ID,"initial_manifest":rel(base_path),"development":base["splits"]["development"]["path"],
        "holdout":base["splits"]["holdout"],"protected":rel(protected_path),
        "parent_binary":base["parent_btc_binary"],"candidate_binary":"artifacts/research/332/build/udonshield_btc.exe",
        "bridge_binary":base["bridge_binary"],"hashes":{p:digest(ROOT/p) for p in sorted(paths)},
        "changes":changes,"stage":"candidate frozen; development run authorized; holdout unopened; production unchanged",
        "protected_prerequisite":"Detailed108 setups/traffic/roles/public windows frozen before candidate measurement; runner still requires validation before protected execution.",
        "resource_floor_bytes":768*1024*1024,"automatic_holdout_open":False})
    print(json.dumps({"execution_sha256":digest(M),"candidate_sha256":digest(ROOT/"artifacts/research/332/build/udonshield_btc.exe"),
        "protected_sha256":digest(protected_path),"patch_sha256":digest(patch_path),"preflight_sha256":digest(test_path)},indent=2))


if __name__=="__main__": main()
