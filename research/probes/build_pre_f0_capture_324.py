"""Insert observation-only hooks in NEW mechanical copies; prove roundtrip."""
from pathlib import Path
import json
from run_http_baseline_314 import ROOT, digest

OUT = ROOT / "artifacts/research/324"
HOOKS = {
    "decision": [
        ('#include "udon/decision.hpp"\n', '#include "pre_f0_capture_324.hpp"\n'),
        ('DecisionResult UdonShieldEngine::solve_day(\n    const DayState& state,\n    const MatchLedger& ledger,\n    std::chrono::milliseconds available) {\n', '    capture324::begin(state, ledger);\n'),
        ('    const RoutePortfolio legacyPortfolio = portfolio;\n', '    capture324::portfolio("legacy", legacyPortfolio);\n'),
        ('    result.timing.columnGeneration = columnGenerationDuration;\n', '    capture324::portfolio("merged", portfolio);\n'),
        ('    merge_master_diagnostics(result.diagnostics, initialDiagnostics);\n',
         '    capture324::pool("initial-master", generatedCandidates);\n    capture324::pool("legacy-master", legacyCandidates);\n'),
        ('    const bool deterministicNoRoad = config_.roadCells.empty();\n', '    capture324::pool("pre-f0", candidates);\n'),
    ],
    "audit": [
        ('#include "udon/audit.hpp"\n', '#include "pre_f0_capture_324.hpp"\n'),
        ('    result.emplace("decision", JsonValue(std::move(decisionObject)));\n',
         '    result.emplace("capture324", capture324::serialize(config, state, ledger));\n'),
    ],
}

def transform(source, hooks):
    output = source
    blocks = []
    for index, (anchor, body) in enumerate(hooks):
        if output.count(anchor) != 1:
            raise ValueError(f"hook anchor not unique: {anchor!r}")
        block = f"// BEGIN CAPTURE324 {index}\n" + body + f"// END CAPTURE324 {index}\n"
        # All insertions after a known boundary. pre-f0 must precede UB calculation,
        # and merged snapshot is before any master mutation (timing assignment only).
        output = output.replace(anchor, anchor + block)
        blocks.append(block)
    restored = output
    for block in blocks:
        if restored.count(block) != 1:
            raise ValueError("hook roundtrip ambiguous")
        restored = restored.replace(block, "")
    if restored != source:
        raise ValueError("mechanical source roundtrip failed")
    return output

def main():
    OUT.mkdir(parents=True, exist_ok=True)  # Individual outputs remain create-only.
    proof = []
    for name, hooks in HOOKS.items():
        original = ROOT / f"src/{name}.cpp"
        output = OUT / f"{name}.capture324.cpp"
        source = original.read_bytes().replace(b"\r\n", b"\n").decode("utf-8")
        generated = transform(source, hooks)
        with output.open("x", encoding="utf-8", newline="\n") as stream:
            stream.write(generated)
        proof.append({"source": str(original.relative_to(ROOT)), "source_sha256": digest(original),
            "generated": str(output.relative_to(ROOT)), "generated_sha256": digest(output),
            "insertions": len(hooks), "normalized_roundtrip_exact": True})
    (OUT / "source-roundtrip.json").write_text(json.dumps(proof, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(proof, indent=2))

if __name__ == "__main__": main()
