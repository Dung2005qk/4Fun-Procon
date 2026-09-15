"""Close tests/provenance once, before the first protected gameplay measurement."""
import json
import subprocess
import sys
from pathlib import Path
import run_pair_holdout_341 as held
import run_pair_protected_341 as run


def rel(p):return p.relative_to(run.ROOT).as_posix()


def main():
    root=run.ROOT;require=run.require;digest=run.digest;load=run.load
    require(not run.M.exists() and not run.D.exists(),'protected already frozen or begun')
    prior=load(held.M);held.verify(prior)
    require(digest(held.S)=='14A3699BE45B7BA7A6D57FF753A398452124EA2FCA5534F87C53A6340E7AD976','holdout summary drift')
    summary=load(held.S);require(summary['gate_passed'],'holdout did not qualify')
    # Recompute every side/marker and the frozen summary without overwriting it.
    original=held.write_new
    def equal_only(path,value):require(path==held.S and value==summary,'holdout summary recomputation differs')
    held.write_new=equal_only
    try:held.summarize()
    finally:held.write_new=original
    p=root/f'research/holdouts/{run.ID}-protected.json'
    require(digest(p)=='A2AC983D560603AF8835D8173F52305D6BAD82733439E2BE06A02DD212A50203','pre-source protected input drift')
    run.coverage(load(p)['cases'])
    smoke=root/f'research/evidence/{run.ID}-protected-http-preflight-v2'
    done=load(smoke/'run_complete.json');inputs=load(smoke/'inputs.json')
    require(done['complete'] and done['actions']==8 and done['transitions']==6,'HTTP preflight incomplete')
    require(done['input_sha256']==digest(smoke/'inputs.json'),'preflight input identity')
    for name,h in inputs['hashes'].items():require(digest(root/name)==h,'preflight dependency drift:'+name)
    for e,finished in zip(inputs['cases'],done['results'],strict=True):
        run.validate_side(e['case'],e['side'],smoke)
        require(finished['result_sha256']==digest((smoke/e['side']/str(e['case']['seed'])).with_suffix('.result.json')),'preflight result drift')
    tests=subprocess.run([sys.executable,'-m','unittest','test_protected_http_341','test_pair_protected_341'],
        cwd=root/'research/probes',capture_output=True,text=True)
    require(tests.returncode==0,'protected contract tests failed:'+tests.stdout+tests.stderr)
    require(subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()=='c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c','HEAD drift')
    require(subprocess.run(['git','diff','--quiet','--','src','include','CMakeLists.txt'],cwd=root).returncode==0,'production source changed')
    tp=root/f'research/evidence/{run.ID}-protected-preflight.json'
    run.write_new(tp,{'tests':tests.stdout+tests.stderr,'exit_code':tests.returncode,'http_contract_sha256':digest(smoke/'run_complete.json'),
        'holdout_summary_recomputed_exactly':True,'production_source_unchanged':True,
        'authority':'Contracts only; no protected score measurement yet.'})
    paths=set(prior['hashes'])|{rel(held.M),rel(held.S),rel(tp),rel(p)}
    paths|={rel(f) for f in held.D.rglob('*') if f.is_file()}
    for folder in (smoke,root/f'research/evidence/{run.ID}-protected-http-preflight'):
        paths|={rel(f) for f in folder.rglob('*') if f.is_file()}
    paths|={f'research/probes/{n}' for n in ('protected_http_transport_341.py','protected_http_bridge_341.cpp',
        'run_pair_protected_341.py','freeze_pair_protected_341.py','test_pair_protected_341.py',
        'test_protected_http_341.py','preflight_protected_http_341.py','test_http_baseline_314.py')}
    paths|={'artifacts/research/341/protected-bridge.exe','build-release/udon_shield.lib',
            f'research/evidence/{run.ID}-holdout-closure.md',f'research/evidence/{run.ID}-protected-contract-closure.md'}
    run.write_new(run.M,{'experiment':run.ID,'phase':'protected','protected':rel(p),
        'parent_binary':prior['parent_binary'],'candidate_binary':prior['candidate_binary'],
        'bridge_binary':'artifacts/research/341/protected-bridge.exe',
        'hashes':{n:digest(root/n) for n in sorted(paths)},'resource_floor_bytes':1073741824,
        'counts':run.coverage(load(p)['cases']),'public_day_seconds_total':13680,
        'gate_contract':f'research/evidence/{run.ID}-protected-contract-closure.md',
        'holdout_gate_passed':True,'promotion_authorized':False,
        'authority':'Parent-paired synthetic Windows HTTP protected screen. Not BTC, not Linux sandbox, not real opponent policy or target-host performance.'})
    print(json.dumps({'execution_sha256':digest(run.M),'runner_sha256':digest(root/'research/probes/run_pair_protected_341.py'),
        'preflight_sha256':digest(tp),'dependencies':len(paths)},indent=2))


if __name__=='__main__':main()
