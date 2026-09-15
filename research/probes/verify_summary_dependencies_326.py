"""Verify the reused323 comparator from its pre-existing frozen provenance."""
from http_prefix_option_loss_321 import ROOT,load,digest,require,write_new
from portfolio_representatives_326 import M,D,S,summarize

PRIOR=ROOT/"research/holdouts/ATTR-DAY2-POOL-EXACT-VALUE-323.json"
HELPER=ROOT/"research/probes/day2_pool_value_323.py"
RECORD=ROOT/"research/evidence/ATTR-CERTIFIED-PORTFOLIO-REPRESENTATIVES-326.summary-dependencies.json"
if __name__=="__main__":
    require(digest(PRIOR)=="A793940CA25D86FB25811C0DE5CD46120944C3E21FE8CE865DA5BD71EF17DB0B","prior323 frozen manifest")
    require(digest(HELPER)==load(PRIOR)["runner_sha256"],"unchanged frozen323 comparator")
    require(not S.exists(),"summary already exists")
    write_new(RECORD,{"reason":"326 reused the existing323 lexicographic comparator but did not directly duplicate its hash in local_hashes; verify its pre-existing frozen323 manifest before any326 score analysis.",
        "source_modified":False,"measurement_repeated":False,"metric_or_gate_changed":False,
        "hashes":{str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in (M,PRIOR,HELPER,ROOT/"research/probes/verify_summary_dependencies_326.py")}})
    summarize(M,D,S)
