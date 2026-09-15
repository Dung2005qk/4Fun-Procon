"""Finite frozen-byte/source audit and guard-model contract; never runs a solver."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "artifacts/research/357/completed"
ID = "ATTR-LATE-CONTROL-ISOLATION-359"
OBJDUMP = Path("C:/mingw64/bin/objdump.exe")
SYMBOL = "_ZN12_GLOBAL__N_18run_httpERKNS_14RuntimeOptionsE"
EXPECTED = {
    "execution357.json": "F0188694FF3E0042D1B85F8111F06E42AF7F1538A71C6AECBB44F384ED4C102C",
    "build/udonshield_btc": "083B134AD9E0AFE847DDF8E95B7E4352F5F3E3CA5E6F281D3D83DB2B7961144D",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def digest_text(value):
    return hashlib.sha256(value.encode()).hexdigest().upper()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def guard_contract():
    rows = []
    for authorized, nonterminal, deadline, treatment in itertools.product((False, True), repeat=4):
        # Actual source predicates are pinned and checked below before this model.
        old_opt = treatment and authorized and nonterminal and not deadline
        old_telemetry = treatment and authorized and nonterminal
        reads = []

        def late_read():
            reads.append("treatment")
            return treatment

        observed = late_read() if authorized and nonterminal else False
        new_opt = observed and not deadline
        new_telemetry = observed
        assert (old_opt, old_telemetry) == (new_opt, new_telemetry)
        assert len(reads) == int(authorized and nonterminal)
        rows.append(dict(authorized=authorized, nonterminal=nonterminal,
                         deadline=deadline, treatment=treatment, optimizer=old_opt,
                         telemetry=old_telemetry, late_read_count=len(reads)))

    # Mutation controls prove this small checker distinguishes the forbidden changes.
    mutants = {
        "eager_read_in_inactive_lane": any(not r["authorized"] for r in rows),
        "drop_authorization": any((r["treatment"] and r["nonterminal"] and not r["deadline"]) != r["optimizer"] for r in rows),
        "drop_terminal_guard": any((r["treatment"] and r["authorized"] and not r["deadline"]) != r["optimizer"] for r in rows),
        "drop_deadline_guard": any((r["treatment"] and r["authorized"] and r["nonterminal"]) != r["optimizer"] for r in rows),
        "suppress_deadline_telemetry": any(r["optimizer"] != r["telemetry"] for r in rows),
    }
    assert all(mutants.values())
    return {"rows": rows, "mutations_detected": mutants,
            "scope": "Boolean model only; not a compiled or timed runtime equivalence test"}


def audit():
    for name, expected in EXPECTED.items():
        assert sha(PACKAGE / name) == expected, name
    attribution = ROOT / "research/evidence/ATTR-INACTIVE-CHECKPOINT-CAUSALITY-358.json"
    assert sha(attribution) == "773E1C9B783C79764C1C7BF3767C30CCC143B61DA219DFDF29ED1FB52D504400"
    execution = read_json(PACKAGE / "execution357.json")
    verified = {}
    for name, expected in execution["hashes"].items():
        assert sha(PACKAGE / name) == expected, name
        verified[name] = expected
    source = PACKAGE / "source/src/btc_main.cpp"
    text = source.read_text(encoding="utf-8")
    normalized = re.sub(r"\s+", " ", text)
    assert "struct RuntimeOptions { bool resourceMarginal357 = false;" in normalized
    optimizer = "if (options.resourceMarginal357 && publicContinuationAuthorized && state.dayNumber < config.day_count() && !publicContinuation.diagnostics.deadlineReached)"
    telemetry = "if (options.resourceMarginal357 && publicContinuationAuthorized && state.dayNumber < config.day_count())"
    assert normalized.count(optimizer) == normalized.count(telemetry) == 1
    assert text.count('std::getenv("UDON_RESOURCE_MARGINAL_357")') == 1
    assert len(re.findall(r"\bresourceMarginal357\b", text)) == 4
    source_reads = [{"line": n, "text": line.strip()} for n, line in enumerate(text.splitlines(), 1)
                    if "resourceMarginal357" in line or "resourceFlag357" in line]

    command = [str(OBJDUMP), "-d", "--no-show-raw-insn", "--disassemble=" + SYMBOL,
               "artifacts/research/357/completed/build/udonshield_btc"]
    disassembly = subprocess.check_output(command, cwd=ROOT, text=True)
    instructions = {}
    for line in disassembly.splitlines():
        match = re.match(r"\s*([0-9a-f]+):\s+(.*)", line)
        if match:
            instructions[int(match[1], 16)] = match[2]
    assert "mov    %rdi,-0x4580(%rbp)" == instructions[0x3A1A1]
    witnesses = []
    for pointer_load, flag_cmp, zero_jump, auth_cmp, auth_jump, join in (
        (0x3EEBE, 0x3EEC5, 0x3EEC8, 0x3EECA, 0x3EED1, 0x3EED7),
        (0x3F062, 0x3F069, 0x3F06C, 0x3F072, 0x3F079, 0x3D2AE),
    ):
        assert instructions[pointer_load] == "mov    -0x4580(%rbp),%rax"
        assert instructions[flag_cmp] == "cmpb   $0x0,(%rax)"
        assert instructions[zero_jump].startswith("je     " + format(join, "x") + " ")
        assert instructions[auth_cmp] == "cmpb   $0x0,-0x4678(%rbp)"
        assert instructions[auth_jump].split()[0] in ("je", "jne")
        if flag_cmp == 0x3F069:
            assert instructions[auth_jump].startswith("je     " + format(join, "x") + " ")
        witnesses.append({"instructions": {hex(a): instructions[a] for a in
                          (pointer_load, flag_cmp, zero_jump, auth_cmp, auth_jump)},
                          "scope": "Treatment byte controls whether the next comparison executes; no cycle or score causality claim"})
    return {
        "experiment": ID, "complete": True, "solver_invocations": 0,
        "frozen_dependencies_verified": len(verified), "frozen_dependencies": verified,
        "analysis_sha256": sha(Path(__file__)), "attribution358_sha256": sha(attribution),
        "objdump_sha256": sha(OBJDUMP), "command": command,
        "disassembly_sha256": digest_text(disassembly), "source_reads": source_reads,
        "machine_witnesses": witnesses, "guard_model": guard_contract(),
        "frozen357_inactive_instruction_identity": False,
        "new_compiled_noninterference_proven": False,
        "357_verdict": "inconclusive; no holdout or gate change",
        "next": "Separately register a late-control contract implementation; no SCORE until proof obligations are met",
        "limitations": ["No whole-program taint analysis", "No causal attribution of observed score variation",
                        "Equal semantic state is not equal clock/worker history",
                        "New source/model design is not yet compiled or validated"],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    report = audit()
    path = ROOT / f"research/evidence/{ID}.json"
    if args.check:
        assert read_json(path) == report, "audit drift"
    else:
        with path.open("x", encoding="utf-8", newline="\n") as output:
            json.dump(report, output, indent=2, sort_keys=True)
            output.write("\n")
    print(json.dumps({"complete": True, "sha256": sha(path),
                      "dependencies": report["frozen_dependencies_verified"],
                      "truth_table_rows": len(report["guard_model"]["rows"]),
                      "mutation_controls": len(report["guard_model"]["mutations_detected"]),
                      "new_compiled_proof": False}))
