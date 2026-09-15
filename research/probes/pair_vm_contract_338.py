"""Portable, fixed tiny contract only. Never load gameplay score splits."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()
def save(path,obj):
    with Path(path).open('x',encoding='utf-8') as f:json.dump(obj,f,indent=2,sort_keys=True);f.write('\n')


def freeze():
    from run_http_baseline_314 import ROOT,Bridge
    from http_prefix_option_loss_321 import close_bridge
    import test_bounded_pair_338 as t
    obj=t.Contract();q=obj.q();queries=[]
    import copy
    for stock in (1,2,3):
        for reverse in (False,True):
            a=copy.deepcopy(q)
            for s in a['setup']['spots']:s['stocks']=stock
            if reverse:a['state']['agents'].reverse()
            queries.append(a)
    for field in ('settled','created','actions','memo','transitions','queries'):
        queries.append({**q,'limit':field,'value':0})
    queries+=[{**q,'expired':True},{**q,'corrupt':True}]
    a=copy.deepcopy(q);a['state']['agents'][0]['kind']=1;queries.append(a)
    a=copy.deepcopy(q);a['setup']['map']['cells'][7][7]=1;a['state']['traffics']=[{'pos':63,'status':0}];queries.append(a)
    a=copy.deepcopy(q)
    for s in a['state']['agents']:s['fuel']=0
    queries.append(a)
    binary=ROOT/'artifacts/research/338/probe.exe';p=Bridge(binary)
    try:expected=[p.request(q) for q in queries]
    finally:close_bridge(p)
    if not all(e.get('ok') for e in expected):raise ValueError('local conformance request failed')
    path=ROOT/'research/evidence/SCORE-W1-BOUNDED-PAIR-PRICING-338-vm-contract.json'
    save(path,{'experiment':'338','authority':'17 tiny deterministic cross-compiler equivalence/rollback checks; not paired score or latency.',
        'windows_probe_sha256':sha(binary),'queries':queries,'expected':expected})
    print('contract_sha256='+sha(path))


def run(a):
    m=json.loads(a.manifest.read_text());p=subprocess.Popen([str(a.binary.resolve())],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    rows=[]
    try:
        for i,(q,e) in enumerate(zip(m['queries'],m['expected'],strict=True)):
            p.stdin.write(json.dumps(q)+'\n');p.stdin.flush();r=json.loads(p.stdout.readline())
            if r!=e:raise ValueError('cross-compiler contract mismatch at '+str(i))
            rows.append({'index':i,'equal':True})
    finally:
        p.stdin.close();p.wait(timeout=10);error=p.stderr.read();p.stdout.close();p.stderr.close()
        if p.returncode or error:raise ValueError('probe failed:'+error)
    save(a.output,{'complete':True,'cases':len(rows),'rows':rows,'manifest_sha256':sha(a.manifest),
        'binary_sha256':sha(a.binary),'runner_sha256':sha(__file__),'source_probe_sha256':sha(a.source),
        'library_sha256':sha(a.library),'authority':m['authority']})
    print('cross_compiler_complete cases='+str(len(rows))+' summary_sha256='+sha(a.output))


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','run'));p.add_argument('--manifest',type=Path)
    p.add_argument('--binary',type=Path);p.add_argument('--output',type=Path);p.add_argument('--source',type=Path);p.add_argument('--library',type=Path)
    a=p.parse_args();freeze() if a.mode=='freeze' else run(a)
