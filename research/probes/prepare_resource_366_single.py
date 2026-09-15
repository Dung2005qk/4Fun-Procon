"""Create-only adapter package, synthetic tests; never reads/stages protected setups."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'artifacts/research/366/protected-single-driver'
ARCHIVE=OUT.parent/'protected-single-driver.tar.gz'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def main():
    assert not OUT.exists() and not ARCHIVE.exists(),'package already frozen'
    probes=ROOT/'research/probes'
    amendment=ROOT/'research/evidence/SCORE-CAUSAL-RESOURCE-QUALIFICATION-366-protected-single-pass-amendment.md'
    assert sha(amendment)=='5797E4E2551D3B65D5315FC10F9E41588AA38FFD04389BEC8AFF862EB6C7E5FD'
    assert sha(probes/'gate_resource_366.py')=='4ABDD8E700D1256F98EDD4FFED83513AC920C123D76AD452C663F08F536B42F9'
    assert sha(probes/'score_resource_366.py')=='6FB21016FD4A6C65D70BF402253BDE35781F7663E95C54AF912D99A7F0459215'
    check=subprocess.run([sys.executable,'-B',str(probes/'test_resource_366_single.py'),'-v'],
                         capture_output=True,text=True,check=True)
    files={name+'.py':probes/(name+'.py') for name in ('score_resource_366_single',
        'gate_resource_366_single','test_resource_366_single','gate_resource_366','prepare_resource_366_single')}
    files['amendment.md']=amendment
    OUT.mkdir()
    for name,source in files.items():
        shutil.copy2(source,OUT/name)
        assert sha(OUT/name)==sha(source)
    (OUT/'synthetic-tests.txt').write_text(check.stdout+check.stderr,encoding='utf-8')
    manifest={'protocol':'366-P1 single-pass protected','original_execution_sha256':
        '94CF0B0FC04DE3277BD0584C2E54898D510F0B076923E8EECC9FBB7F02D648AC',
        'protected_sha256':'02E4C55AD2C8C506239E0B947BB5E2DA955D329C8BADC2B5979135F93A52DEB7',
        'candidate_sha256':'D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678',
        'hashes':{p.name:sha(p) for p in sorted(OUT.iterdir())},
        'selected_labels':['parentA','candidateA'],'fixtures':108,'results':216,
        'actions':1368,'transitions':1152,'protected_setup_read':False,'measurements_run':False}
    with (OUT/'execution366-single.json').open('x',encoding='utf-8',newline='\n') as out:
        json.dump(manifest,out,indent=2,sort_keys=True);out.write('\n')
    with tarfile.open(ARCHIVE,'x:gz') as archive:
        for p in sorted(OUT.iterdir()):archive.add(p,arcname=p.name)
    print(json.dumps({'execution_sha256':sha(OUT/'execution366-single.json'),
        'archive_sha256':sha(ARCHIVE),'files':manifest['hashes'],'sealed_read':False,'vm_touched':False},indent=2))


if __name__=='__main__':main()
