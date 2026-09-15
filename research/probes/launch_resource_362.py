"""One detached DEV run of frozen362. No restart or overwrite semantics."""
import json
from pathlib import Path
import subprocess
import sys

WORK=Path('/home/LMC/udon362-0909')
if __name__=='__main__':
    assert not (WORK/'runner362.json').exists()
    assert not (WORK/'runner362.stdout').exists() and not (WORK/'runner362.stderr').exists()
    subprocess.run([sys.executable,str(WORK/'test_score_resource_362.py')],check=True,cwd=WORK)
    subprocess.run([sys.executable,str(WORK/'score_resource_362.py'),'freeze'],check=True,cwd=WORK)
    with (WORK/'runner362.stdout').open('x')as out,(WORK/'runner362.stderr').open('x')as err:
        p=subprocess.Popen([sys.executable,str(WORK/'score_resource_362.py'),'run','--phase','development'],
            cwd=WORK,stdin=subprocess.DEVNULL,stdout=out,stderr=err,start_new_session=True)
    with (WORK/'runner362.json').open('x')as f:json.dump({'pid':p.pid,'phase':'development'},f);f.write('\n')
    print('detached362',p.pid)
