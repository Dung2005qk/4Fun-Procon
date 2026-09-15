"""Create-only DEV upload bundle; never exports sealed setup files."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'artifacts/research/362/stage'
ID='SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()

if __name__=='__main__':
    assert not WORK.exists(),'never overwrite a stage'
    manifest=ROOT/f'research/holdouts/{ID}.json'
    assert sha(manifest)=='09FF94371D108006FAB7E3E774A25EC3167E500516372A5D31FDB95118CCD853'
    m=json.loads(manifest.read_text())
    for p,h in m['hashes'].items():assert sha(ROOT/p)==h,p
    sources={
        'input362.json':manifest,
        'score_resource_362.py':ROOT/'research/probes/score_resource_362.py',
        'launch_resource_362.py':ROOT/'research/probes/launch_resource_362.py',
        'verify_late_control_360_copy.py':ROOT/'research/probes/verify_late_control_360_copy.py',
        'test_score_resource_362.py':ROOT/'research/probes/test_score_resource_362.py',
        'preregistration.md':ROOT/f'research/evidence/{ID}-preregistration.md',
        m['splits']['development']['path']:ROOT/m['splits']['development']['path']}
    for relative,source in sources.items():
        target=WORK/relative;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,target)
        assert sha(target)==sha(source)
    with (WORK/'stage362.json').open('x')as f:
        json.dump({'hashes':{p:sha(WORK/p)for p in sources},'no_sealed_setup_exported':True},f,indent=2,sort_keys=True)
        f.write('\n')
    print('stage362',sha(WORK/'stage362.json'))
