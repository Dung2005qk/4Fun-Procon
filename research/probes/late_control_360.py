"""Contract-only host build and four tiny full-lifecycle tests; never SCORE."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ID = "CONTRACT-LATE-TREATMENT-BOUNDARY-360"
ROOT = Path(__file__).resolve().parents[2]
LOCAL_PACKAGE = ROOT / "artifacts/research/357/completed"
VM_PACKAGE = Path("/home/LMC/udon357-0909")
VM_WORK = Path("/home/LMC/udon360-0909")
FROZEN_EXEC = "F0188694FF3E0042D1B85F8111F06E42AF7F1538A71C6AECBB44F384ED4C102C"
SYMBOL = "_ZN12_GLOBAL__N_18run_httpERKNS_14RuntimeOptionsE"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write(path, value):
    with path.open("x", encoding="utf-8", newline="\n") as f:
        json.dump(value, f, indent=2, sort_keys=True)
        f.write("\n")


def verify_package(package):
    assert sha(package / "execution357.json") == FROZEN_EXEC
    manifest = load(package / "execution357.json")
    for path, expected in manifest["hashes"].items():
        assert sha(package / path) == expected, path
    return manifest


def source_contract(package, work):
    old = (package / "source/src/btc_main.cpp").read_text()
    new = (work / "btc_main.cpp").read_text()
    assert new.count('std::getenv("UDON_RESOURCE_MARGINAL_357")') == 1
    assert "resourceMarginal357" not in new
    assert new.count("resourceMarginalControl360") == 4
    reader = new.index('const char* resourceFlag360 = std::getenv(')
    assert new.rfind("publicSlackRefiner.refine_midday_chains(", 0, reader) > new.index("checkpointPlan = submittedPlan;")
    assert new.index("resourceMarginalControl360 = false") > new.index("checkpointSimulation = submittedSimulation;")
    # Reverse precisely the preregistered control-only edit. The ENTIRE host,
    # including original prefix, optimizer body, diagnostics and ACK tail, must match.
    reconstructed = new.replace("struct RuntimeOptions {\n", "struct RuntimeOptions {\n    bool resourceMarginal357 = false;\n", 1)
    startup = ('    // Isolated same-binary research control; never read in main/role search.\n'
               '    const char* resourceFlag357 = std::getenv("UDON_RESOURCE_MARGINAL_357");\n'
               '    options.resourceMarginal357 = resourceFlag357 != nullptr && std::string_view(resourceFlag357) == "1";\n')
    reconstructed = reconstructed.replace("    RuntimeOptions options;\n", "    RuntimeOptions options;\n" + startup, 1)
    reconstructed = reconstructed.replace("                bool resourceMarginalControl360 = false;\n", "", 1)
    begin = reconstructed.index("                    // Contract360:")
    end = reconstructed.index("                        const auto remaining357", begin)
    reconstructed = reconstructed[:begin] + (
        "                    if (options.resourceMarginal357 && publicContinuationAuthorized &&\n"
        "                        state.dayNumber < config.day_count() &&\n"
        "                        !publicContinuation.diagnostics.deadlineReached) {\n") + reconstructed[end:]
    reconstructed = reconstructed.replace("                if (resourceMarginalControl360) {\n",
        "                if (options.resourceMarginal357 && publicContinuationAuthorized && state.dayNumber < config.day_count()) {\n", 1)
    assert reconstructed == old, "change outside the permitted control-only edit"
    return {"entire_host_reverse_edit_equal": True, "late_reader_line": new[:reader].count("\n") + 1}


def stage():
    work = ROOT / "artifacts/research/360"
    verify_package(LOCAL_PACKAGE)
    boundary = source_contract(LOCAL_PACKAGE, work)
    sys.path.insert(0, str(LOCAL_PACKAGE / "research/probes"))
    from test_protected_http_341 import contract_case
    cases = []
    for name, role, windows in (("inactive", "fixed-all-Patrol", [5]*4),
                                 ("mixed", "native", [5,10,5,10])):
        case = copy.deepcopy(contract_case(role))
        case["setup"]["daySeconds"] = windows
        cases.append({"name": name, "case": case, "controls": ["0", "1"],
                      "expected_read_days": [] if name == "inactive" else [2]})
    write(work / "input360.json", {"experiment": ID, "source_contract": boundary, "cases": cases,
          "frozen357_execution": FROZEN_EXEC, "authority": "contract only; not score qualification"})
    files = ["btc_main.cpp", "treatment_reader_360.cpp", "late_control_360.py", "input360.json", "preregistration.md"]
    write(work / "stage360.json", {"hashes": {p: sha(work / p) for p in files}})
    print("staged360", sha(work / "stage360.json"))


def verify_stage():
    assert subprocess.check_output(["hostname"], text=True).strip() == "udon-f0-240-0829"
    verify_package(VM_PACKAGE)
    for p, expected in load(VM_WORK / "stage360.json")["hashes"].items():
        assert sha(VM_WORK / p) == expected, p
    return source_contract(VM_PACKAGE, VM_WORK)


def build():
    boundary = verify_stage()
    compiler = "/usr/bin/c++"
    common = [compiler, "-std=c++20", "-O3", "-DNDEBUG", "-pthread"]
    commands = [
        common + ["-I", str(VM_PACKAGE / "source/include"), "-c", "btc_main.cpp", "-o", "host360.o"],
        common + ["-c", "treatment_reader_360.cpp", "-o", "reader360.o"],
        common + ["host360.o", str(VM_PACKAGE / "build/libudon_shield.a"), "-o", "btc360-plain"],
        common + ["host360.o", "reader360.o", str(VM_PACKAGE / "build/libudon_shield.a"), "-Wl,--wrap=getenv", "-o", "btc360-probe"],
    ]
    for path in ("host360.o", "reader360.o", "btc360-plain", "btc360-probe"):
        assert not (VM_WORK / path).exists(), "never overwrite a build artifact"
    for command in commands:
        subprocess.run(command, cwd=VM_WORK, check=True)
    generated = ["host360.o", "reader360.o", "btc360-plain", "btc360-probe"]
    for binary in ("btc360-plain", "btc360-probe"):
        text = subprocess.check_output(["objdump", "-d", "--disassemble=" + SYMBOL, binary], cwd=VM_WORK, text=True)
        assert SYMBOL in text
        with (VM_WORK / (binary + ".disasm.txt")).open("x") as f:
            f.write(text)
        generated.append(binary + ".disasm.txt")
    write(VM_WORK / "execution360.json", {"commands": commands, "source_contract": boundary,
          "compiler_sha256": sha(Path(compiler)), "compiler_version": subprocess.check_output([compiler, "--version"], text=True),
          "hashes": {**load(VM_WORK / "stage360.json")["hashes"], **{p: sha(VM_WORK / p) for p in generated}},
          "frozen357_execution": FROZEN_EXEC})
    print("execution360_frozen", sha(VM_WORK / "execution360.json"), flush=True)


def verify_execution():
    verify_stage()
    for p, expected in load(VM_WORK / "execution360.json")["hashes"].items():
        assert sha(VM_WORK / p) == expected, p


def summarize():
    verify_execution()
    rows = []
    for spec in load(VM_WORK / "input360.json")["cases"]:
        for control in spec["controls"]:
            side = "candidate" if control == "1" else "parent"
            p = VM_WORK / "contracts" / spec["name"] / side / str(spec["case"]["seed"])
            result = load(p.with_suffix(".result.json"))
            assert result["actions"] == 4 and result["transitions"] == 3 and result["failure"] is None
            assert p.with_suffix(".stderr").stat().st_size == 0
            events = [json.loads(x) for x in p.with_suffix(".replay.jsonl").read_text().splitlines()]
            states = [e for e in events if e["kind"] == "day_state"]
            decisions = [e for e in events if e["kind"] == "decision"]
            actions = [e for e in events if e["kind"] == "actions"]
            trace = p.with_suffix(".control.txt")
            stamps = [int(x) for x in trace.read_text().splitlines()] if trace.exists() else []
            days = []
            for stamp in stamps:
                candidates = [i+1 for i, s in enumerate(states)
                              if decisions[i]["atUnixMs"] <= stamp <= actions[i]["atUnixMs"]]
                assert len(candidates) == 1, "treatment observed outside a decision/submission interval"
                days.extend(candidates)
            assert days == spec["expected_read_days"], (spec["name"], control, days)
            rows.append({"case": spec["name"], "control": control, "read_days": days,
                         "actions": 4, "transitions": 3, "stderr_bytes": 0})
    return {"experiment": ID, "complete": True, "matches": 4, "actions": 16, "transitions": 12,
            "rows": rows, "execution_sha256": sha(VM_WORK / "execution360.json"),
            "files": {p.relative_to(VM_WORK).as_posix(): sha(p) for p in sorted((VM_WORK / "contracts").rglob("*")) if p.is_file()},
            "scope": "finite instrumented compiled lifecycle contract; no promotion or independent-clock equivalence claim"}


def run():
    verify_execution()
    assert not (VM_WORK / "contracts").exists(), "no duplicate/ambiguous contract rerun"
    sys.path.insert(0, str(VM_PACKAGE / "research/probes"))
    import run_resource_357 as frozen
    rpc = frozen.rpc
    rpc.ID = ID
    with rpc.BridgeContext() as bridge:
        for spec in load(VM_WORK / "input360.json")["cases"]:
            for control in spec["controls"]:
                side = "candidate" if control == "1" else "parent"
                p = VM_WORK / "contracts" / spec["name"] / side / str(spec["case"]["seed"])
                p.parent.mkdir(parents=True)
                os.environ["UDON_RESOURCE_MARGINAL_357"] = control
                os.environ["UDON_360_TRACE"] = str(p.with_suffix(".control.txt"))
                rpc.run_one(spec["case"], VM_WORK / "btc360-probe", p, bridge, side)
                print("contract_complete", spec["name"], side, flush=True)
    report = summarize()
    write(VM_WORK / "result360.json", report)
    print("complete360", sha(VM_WORK / "result360.json"), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("stage", "build", "run", "check"))
    mode = parser.parse_args().mode
    if mode == "stage": stage()
    elif mode == "build": build()
    elif mode == "run": run()
    else:
        assert load(VM_WORK / "result360.json") == summarize(), "contract evidence drift"
        print("verified360", sha(VM_WORK / "result360.json"))
