"""Create-only execution closure after exact preflight; no fresh score run."""
import difflib
import json
import subprocess
import sys
from pathlib import Path
from run_pair_score_341 import ROOT,ID,M,load,digest,write_new,require


def rel(p):return str(p.relative_to(ROOT)).replace('\\','/')


def main():
    require(not M.exists(),'already frozen')
    bp=ROOT/f'research/holdouts/{ID}.json';base=load(bp)
    require(digest(bp)=='C2F325F1F5A4C93C0FA15A276D0B92BE6BA5EE08725017E02501C01B73B70D0E','initial identity')
    for p,h in base['parent_hashes'].items():require(digest(ROOT/p)==h,'parent drift:'+p)
    for r in base['splits'].values():require(digest(ROOT/r['path'])==r['sha256'],'split drift')
    require(subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()==base['parent_commit'],'HEAD drift')
    tests=subprocess.run([sys.executable,'-m','unittest','test_pair_stock_bound_341','test_pair_score_runner_341','test_http_baseline_314'],cwd=ROOT/'research/probes',capture_output=True,text=True)
    cache=(ROOT/'build-release/CMakeCache.txt').read_text().splitlines()
    cmake=next(s.split('=',1)[1] for s in cache if s.startswith('CMAKE_COMMAND:INTERNAL='))
    unit=subprocess.run([str(Path(cmake).with_name('ctest.exe')),'--test-dir','artifacts/research/341/build','--output-on-failure'],cwd=ROOT,capture_output=True,text=True)
    require(tests.returncode==0 and unit.returncode==0,'preflight gate failed')
    vp=ROOT/f'research/evidence/{ID}-vm-result.json';v=load(vp)
    require(v['complete'] and v['cases']==17,'VM contract incomplete')
    require(v['manifest_sha256']==digest(ROOT/f'research/evidence/{ID}-vm-contract.json'),'VM inputs drift')
    require(v['source_probe_sha256']==digest(ROOT/'research/probes/bounded_pair_probe_340.cpp'),'VM probe drift')
    require(v['runner_sha256']==digest(ROOT/'research/probes/pair_vm_contract_341.py'),'VM runner drift')
    vl = ROOT/f'research/evidence/{ID}-vm-build.log'
    linux_build = vl.read_text(encoding='utf-8')
    require('100% tests passed, 0 tests failed out of 1' in linux_build, 'Linux unit gate missing')
    require('cross_compiler_complete cases=17' in linux_build, 'Linux conformance gate missing')
    require(digest(ROOT/'artifacts/research/341/source-linux.tar').lower() in linux_build, 'Linux source archive identity missing')
    tp=ROOT/f'research/evidence/{ID}-preflight.json'
    write_new(tp,{'tests':tests.stdout+tests.stderr,'unit':unit.stdout+unit.stderr,'vm_result_sha256':digest(vp),
        'mechanism_identity':'Integrated pricing source is exactly the frozen340 source; parent is canonical HEAD, not rejected338.',
        'authority':'Correctness/rollback/parity only. No new score split executed; no performance claim.'})
    src=ROOT/'artifacts/research/341/source'
    require(digest(src/'src/horizon_pricing.cpp') == base['parent_hashes']['artifacts/research/340/horizon_pricing.cpp'], 'frozen340 mechanism drift')
    files=[src/'CMakeLists.txt']+[p for folder in ('src','include','strategies/blank_slate','tests') for p in (src/folder).rglob('*') if p.suffix in ('.cpp','.hpp')]
    changes=[];diff=[]
    for p in sorted(files):
        key=str(p.relative_to(src)).replace('\\','/');parent=ROOT/key
        before=parent.read_text(encoding='utf-8').splitlines(keepends=True) if parent.exists() else []
        after=p.read_text(encoding='utf-8').splitlines(keepends=True)
        if before!=after:
            changes.append(key);diff+=list(difflib.unified_diff(before,after,fromfile='a/'+key if parent.exists() else '/dev/null',tofile='b/'+key))
    require(set(changes)=={'CMakeLists.txt','src/orienteering.cpp','src/horizon_pricing.cpp','src/decision.cpp','src/audit.cpp',
        'include/udon/orienteering.hpp','include/udon/horizon_pricing.hpp','include/udon/decision.hpp'},'unexpected source scope')
    pp=ROOT/f'research/evidence/{ID}-candidate.patch'
    with pp.open('x',encoding='utf-8',newline='\n') as f:f.write(''.join(diff))
    paths=set(base['parent_hashes'])|{rel(p) for p in files}|{rel(bp),rel(tp),rel(pp),rel(vp),rel(vl)}
    paths.update(load(ROOT/'research/holdouts/ATTR-W1-PAIR-PRICING-PREVALENCE-337.json')['local_hashes'])
    paths.update(load(ROOT/'research/holdouts/ATTR-W1-PAIR-RESOURCE-RESPONSE-336.json')['local_hashes'])
    paths|={r['path'] for r in base['splits'].values()}
    paths|={f'artifacts/research/341/{p}' for p in ('probe.exe','build/udonshield_btc.exe','build/udon_shield.lib','build/udonshield_tests.exe','build/CMakeCache.txt')}
    paths|={f'research/probes/{p}' for p in ('run_pair_score_341.py','test_pair_score_runner_341.py','test_pair_stock_bound_341.py',
        'bounded_pair_probe_340.cpp','freeze_pair_execution_341.py','pair_vm_contract_341.py','test_bounded_horizon_332.py','test_pair_resource_response_336.py')}
    paths.add(f'research/evidence/{ID}-vm-contract.json')
    paths |= {'research/probes/build_pair_vm_341.sh', 'artifacts/research/341/source-linux.tar'}
    for mp in ('ATTR-PAIR-STOCK-RELAXATION-BOUND-340-execution', 'ATTR-PAIR-STOCK-RELAXATION-BOUND-340-crosscheck'):
        inherited = ROOT/f'research/holdouts/{mp}.json'
        paths.add(rel(inherited))
        prior = load(inherited)
        for key in ('hashes', 'local_hashes'):
            for p,h in prior.get(key, {}).items():
                require(digest(ROOT/p) == h, 'inherited dependency drift:'+p)
            paths.update(prior.get(key, {}))
    paths |= {'research/probes/test_pair_stock_bound_340.py', 'research/probes/test_terminal_quotient_339.py',
              'research/probes/test_bounded_pair_338.py', 'artifacts/research/338/probe.exe',
              'artifacts/research/339/probe.exe', 'artifacts/research/340/probe.exe'}
    write_new(M,{'experiment':ID,'initial_manifest':rel(bp),'development':base['splits']['development']['path'],
        'holdout':base['splits']['holdout'],'protected':base['splits']['protected']['path'],
        'parent_binary':base['parent_btc_binary'],'candidate_binary':'artifacts/research/341/build/udonshield_btc.exe',
        'bridge_binary':base['bridge_binary'],'hashes':{p:digest(ROOT/p) for p in sorted(paths)},'changes':changes,
        'resource_floor_bytes':1073741824,'automatic_holdout_open':False,
        'stage':'Candidate frozen; development authorized; holdout sealed; production untouched',
        'VM_scope':'Same frozen C++ source built on existing Linux VM;17 cross-compiler conformance checks and C++ unit pass. HTTP341 remains native Windows WinHTTP, never substituted with Linux sandbox.'})
    print(json.dumps({'execution_sha256':digest(M),'candidate_sha256':digest(ROOT/'artifacts/research/341/build/udonshield_btc.exe'),
        'patch_sha256':digest(pp),'preflight_sha256':digest(tp),'dependencies':len(paths)},indent=2))


if __name__=='__main__':main()
