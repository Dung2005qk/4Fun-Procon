"""Create-only provenance amendment authorized by the user; no score aggregation."""
import subprocess
import sys
from complete_pool_score_344 import ROOT, ID, M, load, digest, require, write_new, verify, audit


def rel(p):return str(p.relative_to(ROOT)).replace('\\','/')


def main():
    require(not M.exists(),'amended execution already exists')
    old_path=ROOT/f'research/holdouts/{ID}-execution.json'
    require(digest(old_path)=='925F79E5751CF057660D8460F782A91D82039C6B7E5E5514A98E4F5248EE507F',
            'original execution provenance drift')
    old=load(old_path)
    changed=('research/probes/complete_pool_score_344.py','research/probes/test_complete_pool_344.py')
    archives={changed[0]:f'research/evidence/{ID}-pre-ram-policy.runner.txt',
              changed[1]:f'research/evidence/{ID}-pre-ram-policy.tests.txt'}
    for p,h in old['hashes'].items():
        require(digest(ROOT/archives.get(p,p))==h,'prior dependency drift:'+p)
    before=(ROOT/archives[changed[0]]).read_text(encoding='utf-8')
    expected=before.replace('require, verify, memory_available,','require, verify,').replace(
        "M=ROOT/f'research/holdouts/{ID}-execution.json'",
        "M=ROOT/f'research/holdouts/{ID}-execution-ram-policy.json'").replace(
        "                require(memory_available()>=m['resource_floor_bytes'],'available RAM below frozen1024MiB floor')\n",'')
    require((ROOT/changed[0]).read_text(encoding='utf-8')==expected,'unapproved runner change')
    directory=ROOT/f'research/evidence/{ID}-development'
    cases=load(ROOT/old['splits']['development']['path'])['cases']
    audit(cases,directory,'development')
    require(not (directory/'run_complete.json').exists(),'phase already complete')
    require(len(list(directory.rglob('*.result.json')))==14,'unexpected resume boundary')
    require(len(list(directory.glob('*.fixture_complete.json')))==3,'unexpected marker boundary')
    command=[sys.executable,'-m','unittest','discover','-s','research/probes','-p','test_complete_pool_344.py']
    test=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
    print(test.stdout+test.stderr)
    require(test.returncode==0,'amendment contracts failed')
    evidence=ROOT/f'research/evidence/{ID}-ram-policy-amendment.json'
    note=ROOT/f'research/evidence/{ID}-ram-policy-amendment.md'
    write_new(evidence,{'experiment':ID,'authorization':'Explicit user removal of free-RAM launch gate2026-09-07',
        'prior_execution':rel(old_path),'prior_execution_sha256':digest(old_path),'amendment_note_sha256':digest(note),
        'original_archives':{p:{'path':a,'sha256':digest(ROOT/a)} for p,a in archives.items()},
        'changed_runtime_paths':list(changed),'removed_condition':'pre-side available physical RAM >=1073741824 bytes',
        'score_gate_changed':False,'binary_changed':False,'splits_changed':False,'partial_scores_inspected':False,
        'results_preserved':14,'fixture_markers_preserved':3,
        'existing_artifacts':{rel(p):digest(p) for p in sorted(directory.rglob('*')) if p.is_file()},
        'prior_runner_stderr_sha256':digest(ROOT/f'research/evidence/{ID}-development.runner.stderr'),
        'tests':{'command':command,'exit_code':test.returncode,'stdout':test.stdout,'stderr':test.stderr}})
    new=dict(old);new.pop('resource_floor_bytes')
    new['operational_amendment']={'path':rel(evidence),'sha256':digest(evidence),
        'prior_execution_sha256':digest(old_path),'available_ram_launch_floor':None}
    paths=set(old['hashes'])|set(archives.values())|{rel(old_path),rel(evidence),rel(note),rel(ROOT/'research/probes/amend_ram_policy_344.py')}
    new['hashes']={p:digest(ROOT/p) for p in sorted(paths)}
    verify(new)
    write_new(M,new)
    print({'execution_sha256':digest(M),'amendment_sha256':digest(evidence),'dependencies':len(paths),
           'existing_results':14,'fixture_markers':3,'free_ram_gate':'removed','partial_scores_inspected':False})


if __name__=='__main__':main()
