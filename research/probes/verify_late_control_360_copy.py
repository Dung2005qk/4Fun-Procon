"""Independent, read-only verification of the exact downloaded 360 contract."""
import hashlib
import json
from pathlib import Path
import re

from late_control_360 import source_contract, verify_package

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / "artifacts/research/360/completed"


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def guarded_reachability(text, start):
    instructions = {}
    for line in text.splitlines():
        m = re.match(r"^\s*([0-9a-f]+):\t[0-9a-f ]+\t([a-z][a-z0-9]*)\s*(.*)$", line)
        if m:
            instructions[int(m[1], 16)] = (m[2], m[3])
    ordered = sorted(instructions)
    following = dict(zip(ordered, ordered[1:]))
    edges = {}
    for at, (op, args) in instructions.items():
        targets = [] if op.startswith("ret") else [following.get(at)]
        if op == "call" and any(name in args for name in (
            "__throw_", "__cxa_throw", "__stack_chk_fail", "_ZSt9terminatev", "abort@plt")):
            targets = []  # C++/libc noreturn entries; not normal fall-through.
        if op.startswith("j"):
            assert not args.startswith("*")
            m = re.match(r"([0-9a-f]+)\b", args)
            assert m, (at, args)
            jump = int(m[1], 16)
            targets = [jump] if op == "jmp" else targets + [jump]
        edges[at] = [x for x in targets if x in instructions]

    def reachable(cut=None):
        pending, seen = [start], set()
        while pending:
            at = pending.pop()
            if at in seen:
                continue
            seen.add(at)
            pending.extend(to for to in edges[at] if (at, to) != cut)
        return start + 0x567A in seen

    assert reachable(), "vacuous CFG"
    # Public authorization true edge and nonterminal fall-through are EACH necessary.
    assert not reachable((start + 0x4D62, start + 0x564C))
    assert not reachable((start + 0x566D, start + 0x5673))
    return "normal-flow CFG; calls return normally; exception paths terminate this solve"


def verify():
    package = ROOT / "artifacts/research/357/completed"
    old = verify_package(package)
    source = source_contract(package, WORK)
    expected = {
        "execution360.json": "47488178393C4578A1D28795CA2EFDC9329F0B6BE96A75967774446F28FC68F9",
        "result360.json": "F61EB2B622550443556D56B1975F011C46A9385ED9F1C1003889E1C608449808",
        "stage360.json": "72E56A5A5195FCD5D5864456B962465222A87165B9882B57631B3A2CC57CAAF9",
    }
    for p, h in expected.items():
        assert sha(WORK / p) == h, p
    assert sha(WORK.parent / "complete.tar.gz") == "B4C42C4AC44CC9AA82083DD03495A29B318350A5C43E3B294517E2FC6611A3D3"
    execution, result = load(WORK / "execution360.json"), load(WORK / "result360.json")
    for manifest, key in ((load(WORK / "stage360.json"), "hashes"), (execution, "hashes"), (result, "files")):
        for p, h in manifest[key].items():
            assert sha(WORK / p) == h, p
    assert {p.relative_to(WORK).as_posix() for p in (WORK / "contracts").rglob("*") if p.is_file()} == set(result["files"])
    rows = []
    for spec in load(WORK / "input360.json")["cases"]:
        for control in spec["controls"]:
            side = "candidate" if control == "1" else "parent"
            prefix = WORK / "contracts" / spec["name"] / side / str(spec["case"]["seed"])
            r = load(prefix.with_suffix(".result.json"))
            assert r["actions"] == 4 and r["transitions"] == 3 and r["failure"] is None
            assert prefix.with_suffix(".stderr").stat().st_size == 0
            events = [json.loads(s) for s in prefix.with_suffix(".replay.jsonl").read_text().splitlines()]
            decisions = [e for e in events if e["kind"] == "decision"]
            actions = [e for e in events if e["kind"] == "actions"]
            assert len(decisions) == len(actions) == 4
            trace = prefix.with_suffix(".control.txt")
            reads = [int(s) for s in trace.read_text().splitlines()] if trace.exists() else []
            days = []
            for t in reads:
                matched = [i + 1 for i in range(4) if decisions[i]["atUnixMs"] <= t <= actions[i]["atUnixMs"]]
                assert len(matched) == 1
                days += matched
            assert days == spec["expected_read_days"]
            check = prefix.with_suffix(".replay-check.txt").read_text()
            assert "resume accepted_days=4 last_wire_day=3" in check
            rows.append(dict(case=spec["name"], control=control, read_days=days, actions=4, transitions=3, stderr_bytes=0))
    assert result["rows"] == rows and result["actions"] == 16 and result["transitions"] == 12
    # Same object for both links; all non-interposition compile/link arguments exact.
    commands = execution["commands"]
    assert commands[2][5:-2] == commands[3][5:-2][:1] + commands[3][5:-2][2:3]
    assert commands[2][5] == commands[3][5] == "host360.o"
    offsets = []
    for name, call in (("btc360-plain", "getenv@plt"), ("btc360-probe", "__wrap_getenv")):
        text = (WORK / (name + ".disasm.txt")).read_text()
        start = int(re.search(r"^([0-9a-f]+) <_ZN12_GLOBAL__N_18run_httpERKNS_14RuntimeOptionsE>:", text, re.M)[1], 16)
        sites = re.findall(r"^\s*([0-9a-f]+):.*\bcall\s+[0-9a-f]+ <" + call + r">", text, re.M)
        assert len(sites) == 2  # token reader and the sole late non-secret treatment reader
        offsets.append([int(x, 16) - start for x in sites])
        assert not re.search(r"\bjmp\s+\*", text), "unresolved indirect normal-flow edge"
        guarded_reachability(text, start)
    assert offsets[0] == offsets[1] == [0x127, 0x567A]
    return dict(verified=True, dependencies357=len(old["hashes"]), files360=len(result["files"]),
                source=source, matches=4, actions=16, transitions=12, rows=rows,
                treatment_call_relative_offset="0x567a", interposed_and_plain_same_host_object=True,
                public_and_nonterminal_edges_each_dominate_reader_in_normal_flow=True)


if __name__ == "__main__":
    print(json.dumps(verify(), indent=2))
