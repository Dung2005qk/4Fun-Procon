"""Freeze exact347 source, tests and execution after all preflight gates pass."""
import difflib
from pathlib import Path
import subprocess
import sys
from claim_cap_score_347 import ROOT,ID,M
from complete_pool_score_344 import load,digest,require,write_new,verify
from test_claim_cap_347 import reference_check,PROBE
from run_http_baseline_314 import Bridge
from http_prefix_option_loss_321 import close_bridge

def rel(p):return str(p.relative_to(ROOT)).replace('\\','/')

def main():
    require(not M.exists(),'execution already frozen')
    bp=ROOT/f'research/holdouts/{ID}.json';b=load(bp)
    require(digest(bp)=='FA04500368E82DFC575C1A3557CFFC4E15A884CABEEAE250C125B080B798216C','input provenance')
    verify(b)
    for s in b['splits'].values():require(digest(ROOT/s['path'])==s['sha256'],'split drift')
    require(subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()==b['parent_commit'],'HEAD drift')
    cmake=next(l.split('=',1)[1] for l in (ROOT/'build-release/CMakeCache.txt').read_text().splitlines() if l.startswith('CMAKE_COMMAND:INTERNAL='))
    commands=[[str(Path(cmake).with_name('ctest.exe')),'--test-dir','artifacts/research/347/build','--output-on-failure'],
        [sys.executable,'-m','unittest','discover','-s','research/probes','-p','test_claim_cap_347.py','-v']]
    records=[]
    for command in commands:
        r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
        records.append({'command':command,'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
        if r.returncode:
            p=ROOT/f'research/evidence/{ID}-preflight-failure-{len(list((ROOT/"research/evidence").glob(ID+"-preflight-failure-*.json")))}.json'
            write_new(p,{'passed':False,'records':records});print(r.stdout+r.stderr);raise RuntimeError('no execution freeze')
    bridge=Bridge(PROBE)
    try:references=reference_check(bridge)
    finally:close_bridge(bridge)
    tp=ROOT/f'research/evidence/{ID}-preflight.json'
    write_new(tp,{'passed':True,'records':records,'exact_reference_bound_checks':references,
        'authority':'Proof/contract only; consumed development is not promotion.'})
    src=ROOT/'artifacts/research/347/source'
    files=[src/'CMakeLists.txt']+[p for folder in ('src','include','strategies','tests','bench') for p in (src/folder).rglob('*') if p.suffix in ('.hpp','.cpp','.inc')]
    changes=[];diff=[];increment=[]
    for p in sorted(files):
        k=str(p.relative_to(src)).replace('\\','/');parent=ROOT/k
        before=parent.read_text(encoding='utf-8').splitlines(keepends=True) if parent.exists() else []
        after=p.read_text(encoding='utf-8').splitlines(keepends=True)
        prior=ROOT/'artifacts/research/344/source'/k
        if not prior.exists() or prior.read_bytes()!=p.read_bytes():increment.append(k)
        if before!=after:
            changes.append(k);diff+=list(difflib.unified_diff(before,after,fromfile='a/'+k if parent.exists() else '/dev/null',tofile='b/'+k))
    require(set(increment)=={'CMakeLists.txt','src/decision.cpp','tests/test_main.cpp','tests/claim_cap_347.inc'},'unexpected347increment:'+str(increment))
    expected={'CMakeLists.txt','src/decision.cpp','src/audit.cpp','src/orienteering.cpp','src/horizon_pricing.cpp',
        'include/udon/decision.hpp','include/udon/orienteering.hpp','include/udon/horizon_pricing.hpp',
        'tests/test_main.cpp','tests/complete_pool_344.inc','tests/claim_cap_347.inc'}
    require(set(changes)==expected,'unexpected product scope')
    pp=ROOT/f'research/evidence/{ID}-candidate.patch'
    with pp.open('x',encoding='utf-8',newline='\n') as f:f.write(''.join(diff))
    old=ROOT/'research/holdouts/SCORE-COMPLETE-W1-POOL-PAIR-PRICING-344-execution-ram-policy.json';prior=load(old);verify(prior)
    paths=set(b['hashes'])|set(prior['hashes'])|{rel(p) for p in files}|{rel(bp),rel(tp),rel(pp),rel(old)}
    paths|={s['path'] for s in b['splits'].values()}
    paths|={f'research/probes/{f}' for f in ('claim_cap_score_347.py','claim_cap_probe_347.cpp','test_claim_cap_347.py',
        'freeze_claim_cap_execution_347.py','freeze_claim_cap_347.py','build_claim_cap_347.cmd',
        'report_complete_pool_344.py','test_report_complete_pool_344.py','test_http_prefix_oracle_321.py',
        'conditional_ceiling_346.py','test_conditional_ceiling_346.py')}
    paths|={'research/holdouts/ATTR-CURRENT-FLOOR-CONDITIONAL-CEILING-346.json',
        'research/evidence/ATTR-CURRENT-FLOOR-CONDITIONAL-CEILING-346.summary.json'}
    paths|={f'artifacts/research/347/build/{f}' for f in ('udonshield_btc.exe','udonshield_tests.exe','udon_shield.lib',
        'udonshield_pricing_contract_344.exe','udonshield_claim_cap_347.exe','CMakeCache.txt','udonshield_strategy_bench.exe','udonshield_master_oracle.exe')}
    write_new(M,{'experiment':ID,'parent_commit':b['parent_commit'],'initial_manifest':rel(bp),
        'parent_binary':b['parent_btc_binary'],'candidate_binary':'artifacts/research/347/build/udonshield_btc.exe',
        'bridge_binary':b['bridge_binary'],'splits':b['splits'],'hashes':{p:digest(ROOT/p) for p in sorted(paths)},
        'source_changes':changes,'increment_from344':increment,'policy_sha256':digest(ROOT/b['policy']),
        'stage':'Development authorized; sealed/protected gated; production unchanged'})
    print({'execution_sha256':digest(M),'candidate_sha256':digest(ROOT/'artifacts/research/347/build/udonshield_btc.exe'),
        'patch_sha256':digest(pp),'preflight_sha256':digest(tp),'dependencies':len(paths)})

if __name__=='__main__':main()
