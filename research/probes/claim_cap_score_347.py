"""347 execution with unchanged344 gates and pre-frozen exact reporting normalization."""
import argparse
import copy
import json
from pathlib import Path
from unittest.mock import patch
import complete_pool_score_344 as core
from report_complete_pool_344 import normalize_transport

ROOT=core.ROOT
ID='SCORE-PATROL-CLAIM-CAP-COMPLETE-POOL-347'
M=ROOT/f'research/holdouts/{ID}-execution.json'
ORIGINAL_SUMMARY=core.summarize

def summarize(phase,check=False):
    m=core.load(M);core.verify(m)
    directory=ROOT/f'research/evidence/{ID}-{phase}'
    core.require((directory/'run_complete.json').exists(),'no partial summaries')
    cases=core.load(ROOT/m['splits'][phase]['path'])['cases']
    normalized={};reader=core.load;writer=core.write_new
    for c in cases:
        for label in core.LABELS:
            path=core.location(c,label,directory).with_suffix('.transport.json')
            t=normalize_transport(reader(path),c)
            if phase!='protected':
                days=core.decisions(path.with_name(path.name.replace('.transport.json','.replay.jsonl')))
                core.require(len(days)==len(t['actions']),'day count')
                for d,a in zip(days,t['actions'],strict=True):
                    core.require(d['candidate']['simulation']['roadFootprint']==a['validated']['road_footprint'],'footprint proof')
            normalized[path]=t
    def read(path):
        path=Path(path);return copy.deepcopy(normalized[path]) if path in normalized else reader(path)
    def write(path,value):
        value['report_contract']='Frozen347 exact roadless normalization;344 gates unchanged; no measurement correction.'
        value=json.loads(json.dumps(value))
        if check:core.require(reader(path)==value,'summary recomputation drift')
        else:writer(path,value)
    with patch.object(core,'load',side_effect=read),patch.object(core,'write_new',side_effect=write):ORIGINAL_SUMMARY(phase)

def configure():core.ID=ID;core.M=M;core.summarize=summarize

def main():
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('run','summarize'));p.add_argument('--phase',choices=('development','holdout','protected'),default='development')
    p.add_argument('--resume',action='store_true');p.add_argument('--check',action='store_true');a=p.parse_args();configure()
    if a.mode=='run':core.run(a.phase,a.resume)
    else:summarize(a.phase,a.check)

if __name__=='__main__':main()
