"""Freeze production-path score execution only after the budget-retention gate."""
import difflib
from pathlib import Path
import subprocess
import sys
from anytime_score_350 import ROOT,ID,M,core
from anytime_preflight_350 import E as PRE,S as SUMMARY

load,digest,require,write_new,verify=core.load,core.digest,core.require,core.write_new,core.verify
def rel(p):return p.relative_to(ROOT).as_posix()

def main():
    require(not M.exists(),'execution already frozen')
    bp=ROOT/f'research/holdouts/{ID}.json';b=load(bp);verify(b)
    require(digest(bp)=='3ABB3D35C5FCFDD49B66680BD563FBFCD70D228BE6F0821948EBD8D5F403E754','input provenance')
    pe=load(PRE);verify(pe);s=load(SUMMARY)
    require(digest(SUMMARY)=='7DA90C951A83FD1B5E0E7E3C06CD32FF74345592B8849A03DBC808BEBE4B71B0' and s['complete'] and s['gate_passed'],'budget gate failed')
    require(s['execution_sha256']==digest(PRE),'budget execution mismatch')
    for x in b['splits'].values():require(digest(ROOT/x['path'])==x['sha256'],'split drift')
    require(subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()==b['parent_commit'],'HEAD drift')
    commands=[[sys.executable,'research/probes/test_anytime_score_350.py'],
        [sys.executable,'research/probes/test_protected_http_341.py'],
        [sys.executable,'research/probes/test_report_complete_pool_344.py']]
    records=[]
    for c in commands:
        r=subprocess.run(c,cwd=ROOT,capture_output=True,text=True)
        records.append({'command':c,'code':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
        require(r.returncode==0,r.stdout+r.stderr)
    testpath=ROOT/f'research/evidence/{ID}-runner-preflight.json';write_new(testpath,{'passed':True,'records':records})
    src=ROOT/'artifacts/research/350/source'
    files=[src/'CMakeLists.txt']+[p for f in ('src','include','strategies','tests','bench') for p in (src/f).rglob('*') if p.suffix in ('.cpp','.hpp','.inc')]
    changes=[];increment=[];diff=[]
    for p in sorted(files):
        k=p.relative_to(src).as_posix();parent=ROOT/k;prior=ROOT/'artifacts/research/347/source'/k
        before=parent.read_text(encoding='utf-8').splitlines(keepends=True) if parent.exists() else []
        after=p.read_text(encoding='utf-8').splitlines(keepends=True)
        if not prior.exists() or prior.read_bytes()!=p.read_bytes():increment.append(k)
        if before!=after:
            changes.append(k);diff+=list(difflib.unified_diff(before,after,fromfile='a/'+k if parent.exists() else '/dev/null',tofile='b/'+k))
    require(set(increment)=={'src/horizon_pricing.cpp','src/audit.cpp','include/udon/decision.hpp','include/udon/horizon_pricing.hpp'},'unexpected350source increment')
    patch=ROOT/f'research/evidence/{ID}-candidate.patch'
    with patch.open('x',encoding='utf-8',newline='\n') as f:f.write(''.join(diff))
    old=ROOT/'research/holdouts/SCORE-PATROL-CLAIM-CAP-COMPLETE-POOL-347-protected-execution.json'
    prior=load(old);verify(prior)
    paths=set(b['hashes'])|set(pe['hashes'])|set(prior['hashes'])|{rel(p) for p in files}|{rel(bp),rel(PRE),rel(SUMMARY),rel(testpath),rel(patch),rel(old)}
    paths|={x['path'] for x in b['splits'].values()}
    paths|={f'research/probes/{f}' for f in ('anytime_score_core_350.py','anytime_score_350.py','anytime_diagnostics_350.py','test_anytime_score_350.py','freeze_anytime_execution_350.py','test_complete_pool_344.py')}
    paths|={f'artifacts/research/350/build/{f}' for f in ('udonshield_btc.exe','udonshield_tests.exe','udon_shield.lib','udonshield_pricing_contract_344.exe','udonshield_claim_cap_347.exe','CMakeCache.txt','udonshield_strategy_bench.exe','udonshield_master_oracle.exe')}
    m={'experiment':ID,'parent_commit':b['parent_commit'],'initial_manifest':rel(bp),
        'parent_binary':b['parent_btc_binary'],'candidate_binary':'artifacts/research/350/build/udonshield_btc.exe',
        'bridge_binary':b['bridge_binary'],'splits':b['splits'],'hashes':{p:digest(ROOT/p) for p in sorted(paths)},
        'source_changes':changes,'increment_from347':increment,'policy_sha256':digest(ROOT/b['policy']),
        'stage':'Development then broad development; sealed gated; production unchanged'}
    require(m['hashes'][m['bridge_binary']]=='B8C26E696885FB3C9CC8F99D2F6194C0CFD62FF0229C8C9CA0D9DF862EA47EC6','wrong HTTP bridge')
    write_new(M,m)
    print({'execution_sha256':digest(M),'candidate_sha256':digest(ROOT/m['candidate_binary']),
        'patch_sha256':digest(patch),'runner_preflight_sha256':digest(testpath),'dependencies':len(paths)},flush=True)

if __name__=='__main__':main()
