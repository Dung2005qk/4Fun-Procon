"""Sound-bound attribution, never a played-score or holdout experiment."""
import argparse
import terminal_quotient_work_339 as base
from run_http_baseline_314 import ROOT, digest

ID='ATTR-PAIR-STOCK-RELAXATION-BOUND-340'
base.ID=ID
base.M=ROOT/f'research/holdouts/{ID}.json'
base.E=ROOT/f'research/holdouts/{ID}-execution.json'
base.D=ROOT/f'research/evidence/{ID}'
base.S=ROOT/f'research/evidence/{ID}.summary.json'


def freeze_inputs():
    p=ROOT/'research/holdouts/ATTR-PAIR-TERMINAL-QUOTIENT-WORK-339.json'
    base.require(digest(p)=='5B288535C21B0907519730205EA7A03598E1B9DAD0DF806523FD54BA88DFAA90','339 input drift')
    m=base.load(p);base.verify(m)
    m['experiment']=ID;m['hashes'][str(p.relative_to(ROOT))]=digest(p)
    m['mechanism']='Independent residual-stock resource relaxation; componentwise optimistic lex bound; incumbent-safe strict branch cutting.'
    base.write_new(base.M,m);print('input_sha256='+digest(base.M))


def freeze_execution():
    m=base.load(base.M);base.verify(m)
    previous=ROOT/'research/holdouts/ATTR-PAIR-TERMINAL-QUOTIENT-WORK-339-execution.json'
    old=base.load(previous);base.verify(old)
    paths=set(old['hashes'])|set(m['hashes'])|{str(previous.relative_to(ROOT)),str(base.M.relative_to(ROOT)),
        'research/probes/pair_stock_bound_340.py','research/probes/test_pair_stock_bound_340.py',
        'research/probes/bounded_pair_probe_340.cpp','artifacts/research/340/horizon_pricing.cpp',
        'artifacts/research/340/probe.exe'}
    e={**old,'experiment':ID,'input':str(base.M.relative_to(ROOT)),
        'hashes':{p:digest(ROOT/p) for p in sorted(paths)},'candidate':'artifacts/research/340/probe.exe'}
    base.write_new(base.E,e);print('execution_sha256='+digest(base.E))


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze-inputs','freeze-execution','run','summarize'))
    {'freeze-inputs':freeze_inputs,'freeze-execution':freeze_execution,'run':base.run,'summarize':base.summarize}[p.parse_args().mode]()
