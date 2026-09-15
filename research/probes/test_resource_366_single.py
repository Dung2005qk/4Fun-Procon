"""Synthetic adapter/classifier controls only; no sealed files or solver calls."""
from copy import deepcopy
import contextlib
import io
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest

import gate_resource_366_single as gate
import score_resource_366_single as runner


def fixture(seed=1):
    return dict(seed=seed,family=seed%2,fuel='default',side=32,window_ms=10000,
                days=4,players=9,roadless=False,role='native',setup={'daySeconds':[10]*4},
                run_order=['candidateB','parentB','parentA','candidateA'])


def empty_cert():
    return {'independently_checked':True,'frames':[],'certificates':[]}


def strict_cert():
    frame = dict(day=1,entered=True,failure=False,deadline=False,takeover=True,certified=1,
                 parentScore=[6,6,8],outputScore=[6,6,9],parentPlan=[[1]],outputPlan=[[2]])
    before = dict(ok=True,agrees=True,score=[6,6,8],road_footprint=[0],
                  agents=[dict(kind=0,pos=1,fuel=5)],ledger={'brands':[1,2]})
    after = deepcopy(before)
    after['score'] = [6,6,9]
    return {'independently_checked':True,'frames':[frame],
            'certificates':[dict(day=1,takeover=True,work=frame,before=before,after=after)]}


def full_rows():
    cases,rows = [],[]
    for i in range(18):
        c = fixture(i)
        c['window_ms'] = 5000 if i < 12 else 10000 if i%2 else 15000
        cases.append(c)
        gain = int(i >= 12)
        rows.append(dict(c,repeat='A',A=[6,24,100],B=[6,24,100+gain],delta=[0,0,gain],
            comparison_B_vs_A='win' if gain else 'tie',first_takeover=1 if gain else None,
            first_divergence=1 if gain else None,causal=bool(gain),safety=True,
            certificate={'frames':[{'day':1}] if gain else []}))
    return cases,rows


def change(row,delta):
    row['delta'] = delta
    row['B'] = [a+d for a,d in zip(row['A'],delta)]
    row['comparison_B_vs_A'] = 'win' if row['B'] > row['A'] else 'loss' if row['B'] < row['A'] else 'tie'


class ClassifierControls(unittest.TestCase):
    def test_pass_and_no_invented_replication(self):
        result = gate.classify(*full_rows())
        self.assertTrue(result['passed'])
        self.assertIsNone(result['same_fixture_aa'])
        self.assertIsNone(result['replicated_intervals'])

    def test_missing_duplicate_and_wrong_repeat(self):
        c,r = full_rows()
        for bad in (r[:-1],r[:-1]+[r[0]]):
            with self.assertRaises(AssertionError):gate.classify(c,bad)
        r[0]['repeat'] = 'B'
        with self.assertRaises(AssertionError):gate.classify(c,r)

    def test_score_and_stratum_provenance(self):
        c,r = full_rows();r=deepcopy(r);r[0]['delta'][2]=1
        with self.assertRaises(AssertionError):gate.classify(c,r)
        c,r = full_rows();r=deepcopy(r);r[0]['fuel']='low'
        with self.assertRaises(AssertionError):gate.classify(c,r)

    def test_safety_and_short_window_entry(self):
        c,r = full_rows();r[0]['safety']=False
        self.assertFalse(gate.classify(c,r)['passed'])
        c,r = full_rows();r[0]['certificate']['frames']=[{'day':1}]
        self.assertFalse(gate.classify(c,r)['passed'])

    def test_no_causal_or_late_takeover(self):
        for field,value in (('causal',False),('first_takeover',2),('first_divergence',None)):
            c,r = full_rows()
            for row in r[12:]:row[field]=value
            self.assertFalse(gate.classify(c,r)['passed'])

    def test_all_losses_counted(self):
        c,r = full_rows();change(r[-1],[0,0,-1]);r[-1]['causal']=False
        result=gate.classify(c,r)
        self.assertEqual(result['all_active_losses'],1)
        self.assertFalse(result['checks']['single_pass_causal_sign'])

    def test_no_upper_tier_loss_even_with_serving_gain(self):
        for k in (0,1):
            c,r=full_rows();d=[0,0,100];d[k]=-1;change(r[-1],d)
            self.assertFalse(gate.classify(c,r)['checks']['no_individual_upper_tier_loss'])

    def test_no_aa_tail_relief(self):
        for i in (0,-1):
            c,r=full_rows();change(r[i],[0,0,-3])
            self.assertFalse(gate.classify(c,r)['checks']['all_lane_serving_tail_no_aa_relief'])

    def test_small_systematic_harm_and_margin_ties(self):
        for loss in (-1,-2):
            c,r=full_rows()
            for row in r[:12]:change(row,[0,0,loss])
            result=gate.classify(c,r)
            self.assertFalse(result['passed'])
            if loss==-2:self.assertFalse(result['checks']['inactive_noninferiority'])

    def test_inactive_sample_and_unaffected_active(self):
        c,r=full_rows();self.assertFalse(gate.classify(c[1:],r[1:])['checks']['inactive_fixture_count'])
        c,r=full_rows()
        for row in r[12:]:change(row,[0,0,0])
        self.assertFalse(gate.classify(c,r)['checks']['active_benefit'])

    def test_negative_active_stratum_blocks(self):
        c,r=full_rows();c=deepcopy(c);r=deepcopy(r)
        c[-1]['fuel']=r[-1]['fuel']='low';change(r[-1],[0,0,-1])
        self.assertFalse(gate.classify(c,r)['checks']['active_strata'])


class RuntimeControls(unittest.TestCase):
    def test_order_filters_only_B(self):
        labels=['parentA','candidateA','candidateB','parentB']
        for i in range(4):
            c=fixture();c['run_order']=labels[i:]+labels[:i]
            self.assertEqual(runner.selected_order(c),[x for x in c['run_order'] if x.endswith('A')])
        c['run_order']=['parentA']*4
        with self.assertRaises(RuntimeError):runner.selected_order(c)
        with self.assertRaises(RuntimeError):runner.prefix(fixture(),'parentB',Path('.'))

    def test_previous_gate(self):
        s={'phase':'holdout','complete':True,'gate':{'passed':True}}
        c={'fixtures':54,'results':216,'actions':1368}
        runner.require_previous(s,c)
        s['gate']['passed']=False
        with self.assertRaises(RuntimeError):runner.require_previous(s,c)
        s['gate']['passed']=True;c['results']=215
        with self.assertRaises(RuntimeError):runner.require_previous(s,c)

    def test_certificate_positive_and_unsafe(self):
        runner.verify_certificate(empty_cert(),'parent',fixture())
        runner.verify_certificate(strict_cert(),'candidate',fixture())
        for field,value in (('deadline',True),('failure',True),('day',4),('entered',False)):
            cert=strict_cert();cert['frames'][0][field]=value
            with self.assertRaises(RuntimeError):runner.verify_certificate(cert,'candidate',fixture())
        with self.assertRaises(RuntimeError):runner.verify_certificate(strict_cert(),'parent',fixture())
        c=fixture();c['setup']['daySeconds']=[5]*4
        with self.assertRaises(RuntimeError):runner.verify_certificate(strict_cert(),'candidate',c)

    def test_independent_transition_ledger_and_certificate_coverage(self):
        for field,value in (('agrees',False),('road_footprint',[1]),('score',[6,6,7]),
                            ('agents',[dict(kind=0,pos=2,fuel=5)]),('ledger',{'brands':[]})):
            cert=strict_cert();cert['certificates'][0]['after'][field]=value
            with self.assertRaises(RuntimeError):runner.verify_certificate(cert,'candidate',fixture())
        cert=strict_cert();cert['certificates']=[]
        with self.assertRaises(RuntimeError):runner.verify_certificate(cert,'candidate',fixture())
        cert=strict_cert();cert['frames'].append(deepcopy(cert['frames'][0]))
        with self.assertRaises(RuntimeError):runner.verify_certificate(cert,'candidate',fixture())

    def test_no_takeover_identity(self):
        cert=strict_cert();cert['frames'][0]['takeover']=False
        with self.assertRaises(RuntimeError):runner.verify_certificate(cert,'candidate',fixture())

    def test_mechanical_execution_resume_and_ownership(self):
        with tempfile.TemporaryDirectory(prefix='udon366-p1-test-') as temporary:
            d=Path(temporary);case=fixture();calls=[];controls=[]
            for side in ('parent','candidate'):(d/'A'/side).mkdir(parents=True)
            def run_one(c,b,p,bridge,side):
                calls.append(side);runner.write(p.with_suffix('.result.json'),{'synthetic':True})
            engine=SimpleNamespace(configure_side=controls.append,rpc=SimpleNamespace(run_one=run_one),
                certificates=lambda *args:empty_cert())
            with contextlib.redirect_stdout(io.StringIO()):
                runner.audit([case],d,lambda *args:None)
                runner.execute([case],d,Path('fake'),None,engine)
                runner.audit([case],d,lambda *args:None)
                runner.execute([case],d,Path('fake'),None,engine)
            self.assertEqual(calls,['parent','candidate']);self.assertEqual(controls,calls)
            mark=d/(str(case['seed'])+'.fixture_complete.json')
            value=runner.load(mark);value['seed']=99;mark.write_text(__import__('json').dumps(value))
            with self.assertRaises(RuntimeError):runner.audit([case],d,lambda *args:None)

    def test_partial_missing_certificate_foreign_side(self):
        for kind in ('partial','missing_certificate','foreign','repeatB','duplicate_result'):
            with tempfile.TemporaryDirectory(prefix='udon366-p1-test-') as temporary:
                d=Path(temporary);c=fixture()
                for side in ('parent','candidate'):(d/'A'/side).mkdir(parents=True)
                p=runner.prefix(c,'parentA',d)
                if kind=='partial':p.with_suffix('.replay.jsonl').write_text('accepted action')
                elif kind=='missing_certificate':runner.write(p.with_suffix('.result.json'),{})
                elif kind=='foreign':runner.write(p.parent/'999.result.json',{})
                elif kind=='repeatB':(d/'B').mkdir()
                else:runner.write(p.with_suffix('.result.copy.json'),{})
                with self.assertRaises(RuntimeError):runner.audit([c],d,lambda *args:None)


if __name__=='__main__':
    unittest.main()
