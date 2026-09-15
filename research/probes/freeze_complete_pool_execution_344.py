"""Freeze the candidate only after full unit/resource/runner contracts pass."""
import difflib
import json
from pathlib import Path
import subprocess
import sys
from complete_pool_score_344 import ROOT, ID, M, load, digest, require, write_new, verify


def rel(p):return str(p.relative_to(ROOT)).replace('\\','/')


def main():
    require(not M.exists(),'execution already frozen')
    bp=ROOT/f'research/holdouts/{ID}.json';b=load(bp)
    require(digest(bp)=='17E2995FAD42F71B2A3C64927BC7C9CF6DF9D564C0ED090EED5199FEB5D58B24','input provenance')
    verify(b)
    for s in b['splits'].values():require(digest(ROOT/s['path'])==s['sha256'],'split drift')
    require(subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()==b['parent_commit'],'HEAD drift')
    cmake=next(l.split('=',1)[1] for l in (ROOT/'build-release/CMakeCache.txt').read_text().splitlines()
               if l.startswith('CMAKE_COMMAND:INTERNAL='))
    commands=[[str(Path(cmake).with_name('ctest.exe')),'--test-dir','artifacts/research/344/build','--output-on-failure'],
        [sys.executable,'-m','unittest','discover','-s','research/probes','-p','test_complete_pool_344.py']]
    records=[]
    for command in commands:
        r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
        records.append({'command':command,'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
        if r.returncode:
            fail=ROOT/f'research/evidence/{ID}-preflight-failure-{len(list((ROOT/"research/evidence").glob(ID+"-preflight-failure-*.json")))}.json'
            write_new(fail,{'passed':False,'records':records});print(r.stdout+r.stderr);raise RuntimeError('preflight failed; no execution freeze')
    tp=ROOT/f'research/evidence/{ID}-preflight.json';write_new(tp,{'passed':True,'records':records})
    src=ROOT/'artifacts/research/344/source'
    files=[src/'CMakeLists.txt']+[p for folder in ('src','include','strategies','tests','bench')
        for p in (src/folder).rglob('*') if p.suffix in ('.hpp','.cpp','.inc')]
    changes=[];diff=[]
    for p in sorted(files):
        k=rel(p.relative_to(src)) if False else str(p.relative_to(src)).replace('\\','/')
        parent=ROOT/k;before=parent.read_text(encoding='utf-8').splitlines(keepends=True) if parent.exists() else []
        after=p.read_text(encoding='utf-8').splitlines(keepends=True)
        if before!=after:
            changes.append(k);diff+=list(difflib.unified_diff(before,after,fromfile='a/'+k if parent.exists() else '/dev/null',tofile='b/'+k))
    expected={'CMakeLists.txt','src/decision.cpp','src/audit.cpp','src/orienteering.cpp','src/horizon_pricing.cpp',
              'include/udon/decision.hpp','include/udon/orienteering.hpp','include/udon/horizon_pricing.hpp',
              'tests/test_main.cpp','tests/complete_pool_344.inc'}
    require(set(changes)==expected,'unreviewed source scope:'+str(set(changes)^expected))
    pp=ROOT/f'research/evidence/{ID}-candidate.patch'
    with pp.open('x',encoding='utf-8',newline='\n') as f:f.write(''.join(diff))
    old=ROOT/'research/holdouts/SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341-protected-execution.json'
    prior=load(old);verify(prior)
    paths=set(b['hashes'])|set(prior['hashes'])|{rel(p) for p in files}|{rel(bp),rel(tp),rel(pp),rel(old)}
    paths|={s['path'] for s in b['splits'].values()}
    paths|={f'research/probes/{f}' for f in ('complete_pool_score_344.py','test_complete_pool_344.py',
        'freeze_complete_pool_execution_344.py','inactive_runtime_aa_342.py','freeze_complete_pool_344.py')}
    paths|={f'artifacts/research/344/build/{f}' for f in ('udonshield_btc.exe','udonshield_tests.exe','udon_shield.lib',
        'udonshield_pricing_contract_344.exe','CMakeCache.txt','udonshield_strategy_bench.exe','udonshield_master_oracle.exe')}
    write_new(M,{'experiment':ID,'parent_commit':b['parent_commit'],'initial_manifest':rel(bp),
        'parent_binary':b['parent_btc_binary'],'candidate_binary':'artifacts/research/344/build/udonshield_btc.exe',
        'bridge_binary':b['bridge_binary'],'splits':b['splits'],'resource_floor_bytes':b['resource_floor_bytes'],
        'hashes':{p:digest(ROOT/p) for p in sorted(paths)},'source_changes':changes,
        'policy_sha256':digest(ROOT/b['policy']),'stage':'Development authorized; sealed/protected gated; production unchanged'})
    print({'execution_sha256':digest(M),'candidate_sha256':digest(ROOT/'artifacts/research/344/build/udonshield_btc.exe'),
           'patch_sha256':digest(pp),'preflight_sha256':digest(tp),'dependencies':len(paths)})


if __name__=='__main__':main()
