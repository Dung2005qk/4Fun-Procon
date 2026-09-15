"""Recorded reporting-only null handling; preserves the original frozen script."""
import argparse
import json
from independent_retention_325 import ROOT, M, D, base, load, require, digest, write_new

SOURCE = ROOT / "research/probes/independent_retention_325.py"
AMENDMENT = ROOT / "research/evidence/ATTR-INDEPENDENT-COLUMN-RETENTION-325.reporting-amendment.json"
OLD = 'ref["option_loss"]["first_tier"]>0'
NEW = '(ref["option_loss"]["first_tier"] or 0)>0'
EXPECTED = "EE22625A669BAFB795EDFAF889784AE82C9E5118778C3F7E3599C754287BE527"

def corrected_source():
    require(digest(SOURCE) == EXPECTED, "original summarizer hash")
    source = SOURCE.read_text(encoding="utf-8")
    require(source.count(OLD) == 1, "unique null comparison")
    # All defined numeric values preserve the old predicate. A tie's null maps
    # to false, as specified by first_tier_loss=0 and components_lost=[0,0,0].
    for value in (None, 0, 1, 2, 3):
        ref = {"option_loss": {"first_tier": value}}
        require(eval(NEW, {}, {"ref": ref}) == (value is not None and value > 0), "null predicate test")
    return source.replace(OLD, NEW)

def freeze():
    corrected_source()
    base.verify(load(M))
    require(load(D / "run_complete.json") == {"cases": 12, "manifest_sha256": digest(M)}, "complete only")
    paths = [M, SOURCE, ROOT / "research/probes/summarize_retention_325_nullfix.py", *sorted(D.iterdir())]
    write_new(AMENDMENT, {
        "reason": "Original complete-only summarizer raised TypeError at line97 comparing first_tier=null to0 on a tie; no summary was emitted.",
        "error": "TypeError: '>' not supported between instances of 'NoneType' and 'int'",
        "old_expression": OLD, "new_expression": NEW,
        "predicate_cases_passed": 5, "measurement_repeated": False,
        "gate_or_metric_changed": False, "production_change": False,
        "hashes": {str(p.relative_to(ROOT)).replace('\\','/'): digest(p) for p in paths if p.is_file()}})
    print("reporting_amendment_sha256 " + digest(AMENDMENT))

def summarize():
    for path, expected in load(AMENDMENT)["hashes"].items():
        require(digest(ROOT / path) == expected, "amendment input drift: " + path)
    namespace = {"__name__": "retention325_reporting_amendment", "__file__": str(SOURCE)}
    exec(compile(corrected_source(), str(SOURCE), "exec"), namespace)
    namespace["summarize"]()

if __name__ == "__main__":
    p = argparse.ArgumentParser(); p.add_argument("mode", choices=("freeze", "summarize"))
    {"freeze": freeze, "summarize": summarize}[p.parse_args().mode]()
