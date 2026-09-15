"""Create-only mechanical source copies. No canonical source is edited."""
import copy
import json
from run_http_baseline_314 import ROOT, digest
from build_pre_f0_capture_324 import HOOKS as BASE, transform

OUT=ROOT/"artifacts/research/325"
HOOKS=copy.deepcopy(BASE)
HOOKS["decision"][0]=('#include "udon/decision.hpp"\n','#include "pre_prune_capture_325.hpp"\n')
HOOKS["decision"] += [
    ('    ColumnGenerationOptions legacyGenerationOptions = generationOptions;\n', '    capture325::start("raw-legacy");\n'),
    ('    const RoutePortfolio legacyPortfolio = portfolio;\n', '    capture325::end();\n'),
    ('        const std::chrono::milliseconds beforeExpandedGeneration = elapsed();\n','        capture325::start("raw-expanded");\n'),
    ('        RoutePortfolio expandedPortfolio = generator_.generate(\n            state,\n            ledger,\n            expandedGenerationOptions,\n            &expandedDiagnostics);\n','        capture325::end();\n'),
]
HOOKS["planner"] = [
    ('#include "udon/planner.hpp"\n','#include "pre_prune_capture_325.hpp"\n'),
    ('void prune_columns(\n    std::vector<RouteColumn>& columns,\n    std::int32_t maximumColumns,\n    bool retainSpotDiversity) {\n',
     '    capture325::raw(columns);\n'),
]

def main():
    generated={name:transform((ROOT/f"src/{name}.cpp").read_text(),hooks) for name,hooks in HOOKS.items()}
    OUT.mkdir(parents=True,exist_ok=False)
    proof=[]
    for name,source in generated.items():
        path=OUT/f"{name}.capture325.cpp"
        with path.open("x",encoding="utf-8",newline="\n") as stream: stream.write(source)
        proof.append({"original":f"src/{name}.cpp","original_sha256":digest(ROOT/f"src/{name}.cpp"),
            "generated":str(path.relative_to(ROOT)).replace("\\","/"),"generated_sha256":digest(path),
            "normalized_roundtrip_exact":True,"insertions":len(HOOKS[name])})
    (OUT/"source-roundtrip.json").write_text(json.dumps(proof,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(proof,indent=2))

if __name__=="__main__": main()
