"""Create-only staging of the frozen357 package on the existing authorized VM."""
import hashlib
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tarfile

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
root=Path('/home/LMC/udon357-0909')
archive=Path('/home/LMC/udon352-0908/resource357-preflight2.tar.gz')
bridge=Path('/home/LMC/udon352-0908/bridge352')
assert socket.gethostname()=='udon-f0-240-0829'
assert not root.exists(), 'existing357 work; inspect instead of restarting'
assert sha(archive)=='9068AD7CBB8070FB8FFDFA89F7005F76C16FB58112B6ADA6749757D1F9190D0B'
assert sha(bridge)=='41723ACDCC1F090D146A28E4DBFE67C697818DC4B1D00E659E8930602B66D730'
with tarfile.open(archive)as t:
    for m in t.getmembers():
        assert m.isfile() and not Path(m.name).is_absolute() and '..'not in Path(m.name).parts
    root.mkdir()
    t.extractall(root,filter='data')
shutil.copy2(bridge,root/'bridge352')
env=os.environ.copy();env.pop('HEXUDON_TOKEN',None)
with (root/'development.runner.stdout').open('xb')as out,(root/'development.runner.stderr').open('xb')as err:
    p=subprocess.Popen(['bash','research/probes/build_resource_357.sh'],cwd=root,env=env,
        stdin=subprocess.DEVNULL,stdout=out,stderr=err,start_new_session=True)
with (root/'development.runner.pid').open('x')as f:f.write(str(p.pid)+'\n')
print('357 staged and detached build/contracts/development runner PID',p.pid)
