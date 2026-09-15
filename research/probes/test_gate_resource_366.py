"""Synthetic adversarial classifier controls; never reads experiment outcomes."""
from copy import deepcopy
from gate_resource_366 import inactive_gate,sign_p,classify

def rows(delta=0):
    return [{'seed':i,'repeat':r,'A':[6,60,100],'B':[6,60,100+delta],
             'delta':[0,0,delta]} for i in range(12) for r in ('A','B')]

def full():
    cases=[];rr=[];robust=[]
    for i in range(18):
        c=dict(seed=i,family=i%2,fuel='default',side=32,window_ms=5000 if i<12 else (10000 if i%2 else 15000),
               days=4,players=9,roadless=False,role='native')
        cases.append(c);gain=int(i>=12)
        for repeat in ('A','B'):
            rr.append(dict(c,repeat=repeat,A=[6,24,100],B=[6,24,100+gain],delta=[0,0,gain],
                           comparison_B_vs_A='win' if gain else 'tie',causal=bool(gain),
                           first_takeover=1 if gain else None,certificate={'frames':[{'day':1}] if gain else []}))
        robust.append(dict(c,outcome='win' if gain else 'exact-tie'))
    return cases,rr,robust

def main():
    assert sign_p(0,0)==1 and sign_p(12,0)==1/4096
    assert inactive_gate(rows())['passed']
    assert not inactive_gate(rows(-1))['passed'] # small but systematic harm blocks
    assert not inactive_gate(rows(-3))['passed'] # noninferiority and tail fail
    r=rows();r[0]['B'][2]=88;r[0]['delta'][2]=-12
    assert inactive_gate(r)['passed'] # same-fixture AA spread12; all raw data retained
    r=rows();r[0]['B'][2]=r[1]['B'][2]=88;r[0]['delta'][2]=r[1]['delta'][2]=-12
    assert not inactive_gate(r)['checks']['inactive_same_fixture_tail']
    r=rows();r[0]['delta'][0]=-1
    assert not inactive_gate(r)['passed']
    r=rows();r[0]['delta'][1]=-1
    assert not inactive_gate(r)['passed']
    assert not inactive_gate(rows()[:22])['passed']
    assert not inactive_gate(rows(-2))['checks']['inactive_noninferiority'] # all margin ties lack power
    r=rows();r[0]['B'][2]=99;r[0]['delta'][2]=-1
    assert inactive_gate(r)['passed']
    r=rows();r[0]['B'][2]=r[1]['B'][2]=97;r[0]['delta'][2]=r[1]['delta'][2]=-3
    r[2]['B'][2]=80;r[2]['delta'][2]=-20
    assert not inactive_gate(r)['checks']['inactive_same_fixture_tail'] # no borrowing other fixture noise
    try:inactive_gate(rows()[:-1]);raise RuntimeError('missing repetition accepted')
    except AssertionError:pass
    assert classify(*full(),'holdout')['passed']
    c,r,b=full()
    for x in r:x['causal']=False
    assert not classify(c,r,b,'holdout')['passed']
    c,r,b=full();r[0]['certificate']['frames']=[{'day':1}]
    assert not classify(c,r,b,'holdout')['passed']
    c,r,b=full();r[-1]['delta']=[-1,0,1000]
    assert not classify(c,r,b,'holdout')['passed']
    c,r,b=full();b[-1]['outcome']='loss'
    g=classify(c,r,b,'holdout');assert g['all_robust_active_losses']==1 and not g['passed']
    c,r,b=full()
    for x in c+r+b:
        if x['window_ms']==15000:x['window_ms']=10000
    assert not classify(c,r,b,'holdout')['passed']
    c,r,b=full()
    for x in r:
        if x['window_ms']>5000:x.update(delta=[0,0,0],comparison_B_vs_A='tie',causal=False)
    assert not classify(c,r,b,'holdout')['passed']
    c,r,b=full();r[-1].update(delta=[0,0,-3],B=[6,24,97],comparison_B_vs_A='loss')
    assert not classify(c,r,b,'development')['checks']['B_active_downside']
    print('366 classifier:20 synthetic controls passed')

if __name__=='__main__':main()
