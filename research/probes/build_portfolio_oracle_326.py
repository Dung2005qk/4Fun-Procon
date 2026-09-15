"""Mechanically reuse321 complete suffix DP; original source remains frozen."""
from pathlib import Path
from http_prefix_option_loss_321 import ROOT, require, digest, write_new

OUT=ROOT/"artifacts/research/326"
BASE=ROOT/"research/probes/http_prefix_oracle_321.cpp"
ADDITION=ROOT/"research/probes/portfolio_value_326.inc"
ANCHOR='J request(const J& q){'
CALL='return Solver(c).run(state,ledger);'

def transform():
    source=BASE.read_text(encoding="utf-8")
    require(source.count(ANCHOR)==1 and source.count(CALL)==1,"unique326 anchors")
    insertion='// BEGIN326\n'+ADDITION.read_text(encoding="utf-8")+'\n// END326\n'
    output=source.replace(ANCHOR,insertion+ANCHOR).replace(CALL,'return portfolio326(c,state,ledger,q);')
    require(output.replace(insertion,'').replace('return portfolio326(c,state,ledger,q);',CALL)==source,"326 roundtrip")
    return output

if __name__=="__main__":
    output=transform();OUT.mkdir(parents=True,exist_ok=False)
    path=OUT/"portfolio_oracle_326.cpp"
    with path.open("x",encoding="utf-8",newline="\n") as f:f.write(output)
    write_new(OUT/"source-roundtrip.json",{"base_sha256":digest(BASE),"addition_sha256":digest(ADDITION),
        "generated_sha256":digest(path),"normalized_roundtrip_exact":True,"suffix_solver_unchanged":True})
    print(digest(path))
