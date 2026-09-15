"""Launch the one frozen366 DEV job, detached; never overwrite or blindly resume."""
import json
from pathlib import Path
import subprocess
import sys

WORK=Path('/home/LMC/udon366-0909')
if __name__=='__main__':
    assert not (WORK/'runner366.json').exists()
    assert not (WORK/'runner366.stdout').exists() and not (WORK/'runner366.stderr').exists()
    subprocess.run([sys.executable,str(WORK/'test_gate_resource_366.py')],check=True,cwd=WORK)
    subprocess.run([sys.executable,str(WORK/'score_resource_366.py'),'freeze'],check=True,cwd=WORK)
    with (WORK/'runner366.stdout').open('x') as out,(WORK/'runner366.stderr').open('x') as err:
        p=subprocess.Popen([sys.executable,str(WORK/'score_resource_366.py'),'run','--phase','development'],
            cwd=WORK,stdin=subprocess.DEVNULL,stdout=out,stderr=err,start_new_session=True)
    with (WORK/'runner366.json').open('x') as f:json.dump({'pid':p.pid,'phase':'development'},f);f.write('\n')
    print('detached366',p.pid)
