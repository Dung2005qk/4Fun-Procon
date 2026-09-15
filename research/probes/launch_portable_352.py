"""Verify and unpack the frozen diagnostic on the one authorized host."""
import hashlib
import os
from pathlib import Path
import subprocess
import tarfile

root=Path('/home/LMC/udon352-0908')
assert subprocess.check_output(['hostname'],text=True).strip()=='udon-f0-240-0829'
os.chdir(root)
assert Path.cwd().resolve()==root
archive=root/'package352.tar'
assert hashlib.sha256(archive.read_bytes()).hexdigest().upper()=='5522F1C3188A7534F42D3000BF568DC079268627372AEA7E063013AD7E4CFDEE'
assert not (root/'input352.json').exists(), 'already unpacked; no blind relaunch'
with tarfile.open(archive) as data:
    for member in data.getmembers():
        assert member.isfile() and not member.name.startswith('/') and '..' not in Path(member.name).parts
        assert (root/member.name).resolve().is_relative_to(root)
    data.extractall(root)
with (root/'runner352.stdout').open('x') as out,(root/'runner352.stderr').open('x') as err:
    p=subprocess.Popen(['bash','research/probes/build_portable_352.sh'],cwd=root,
        stdin=subprocess.DEVNULL,stdout=out,stderr=err,start_new_session=True)
with (root/'runner352.pid').open('x') as f:f.write(str(p.pid)+'\n')
print('352 launched build/contracts then frozen diagnostic; PID='+str(p.pid))
