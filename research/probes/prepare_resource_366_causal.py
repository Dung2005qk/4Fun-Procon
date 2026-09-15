"""Freeze P2 measurement package only; no sealed setups or optimizer execution."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'artifacts/research/366/protected-causal-driver'
ARCHIVE=OUT.parent/'protected-causal-driver.tar.gz'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def main():
    assert not OUT.exists() and not ARCHIVE.exists(),'already frozen'
    probes=ROOT/'research/probes'
    amendment=ROOT/'research/evidence/SCORE-CAUSAL-RESOURCE-QUALIFICATION-366-causal-decision-amendment.md'
    assert sha(amendment)=='8E12975B4FFD9FA59118261726629391B8179CE832A0A76CFE773A318C734394'
    frozen={'score_resource_366_single':'04B54BEC3DCB028CC7D091CCF3B3DD9914DA968A4999E7151C7ACB70A65F4952',
            'gate_resource_366_single':'05A7E1B595AF1AF6C6009A7A40E67A982BCB053A167923624471968BD1C26A21',
            'test_resource_366_single':'CD3452EA6F2163413E9814428FF52FC8D452F5E94011BBDFE1FBD164CFBF190A',
            'gate_resource_366':'4ABDD8E700D1256F98EDD4FFED83513AC920C123D76AD452C663F08F536B42F9'}
    for name,h in frozen.items():assert sha(probes/(name+'.py'))==h,name
    outputs=[]
    for test in ('test_resource_366_single','test_resource_366_causal'):
        result=subprocess.run([sys.executable,'-B',str(probes/(test+'.py')),'-v'],
                              capture_output=True,text=True,check=True)
        outputs.append(result.stdout+result.stderr)
    names=tuple(frozen)+('score_resource_366_causal','gate_resource_366_causal',
                        'test_resource_366_causal','prepare_resource_366_causal')
    OUT.mkdir()
    for name in names:
        shutil.copy2(probes/(name+'.py'),OUT/(name+'.py'))
        assert sha(probes/(name+'.py'))==sha(OUT/(name+'.py'))
    shutil.copy2(amendment,OUT/'amendment.md')
    (OUT/'synthetic-tests.txt').write_text('\n'.join(outputs),encoding='utf-8')
    manifest={'protocol':'366-P2 user-directed causal decision; one-pass protected',
        'original_execution_sha256':'94CF0B0FC04DE3277BD0584C2E54898D510F0B076923E8EECC9FBB7F02D648AC',
        'original_holdout_summary_sha256':'88E7FF81955C7AFAC838D430508809B8B21C2AC00C9906E7BA6E3994D9EF00B3',
        'original_holdout_gate_passed':False,'decision_amended_after_holdout':True,
        'protected_sha256':'02E4C55AD2C8C506239E0B947BB5E2DA955D329C8BADC2B5979135F93A52DEB7',
        'candidate_sha256':'D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678',
        'hashes':{p.name:sha(p) for p in sorted(OUT.iterdir())},
        'selected_labels':['parentA','candidateA'],'fixtures':108,'results':216,
        'actions':1368,'transitions':1152,'protected_setup_read':False,'measurements_run':False}
    with (OUT/'execution366-causal.json').open('x',encoding='utf-8',newline='\n') as out:
        json.dump(manifest,out,indent=2,sort_keys=True);out.write('\n')
    with tarfile.open(ARCHIVE,'x:gz') as archive:
        for p in sorted(OUT.iterdir()):archive.add(p,arcname=p.name)
    print(json.dumps({'execution_sha256':sha(OUT/'execution366-causal.json'),
        'archive_sha256':sha(ARCHIVE),'files':manifest['hashes'],'sealed_read':False},indent=2))


if __name__=='__main__':main()
