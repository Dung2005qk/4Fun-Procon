import copy
import unittest
from unittest.mock import patch
import anytime_score_350 as run
import anytime_diagnostics_350 as diag
import complete_pool_score_344 as old
import test_complete_pool_344 as inherited
from run_http_baseline_314 import ROOT


class Contract(unittest.TestCase):
    def test_completion_is_not_feasibility(self):
        d={k:0 for k in diag.FIELDS}
        d.update(calls=1,supported=1,exhausted=1,anytimeReturns=1,improvements=1,certifiedIncumbents=1)
        diag.check(d)
        for field,value in (('exhausted',0),('certifiedIncumbents',0),('completed',1),('anytimeReturns',2),('failures',1),('transitions',262145)):
            bad={**d,field:value}
            with self.assertRaises(ValueError):diag.check(bad)
        with self.assertRaises(ValueError):diag.check({k:v for k,v in d.items() if k!='anytimeReturns'})

    def test_measured_day_keeps_new_counters(self):
        d={k:0 for k in diag.FIELDS};d.update(calls=1,supported=1,exhausted=1,anytimeReturns=1,improvements=1,certifiedIncumbents=2)
        day={'audit':{'candidates':[{'horizonPricing':d}]},'profile':{'horizonPricing':d,'outcomes':[], 'certifiedLowerBound':{}},'dayNumber':1,'candidate':{'plan':[]}}
        r=run.core.measured_day(day,None)
        self.assertEqual(r['all_candidates']['anytimeReturns'],1)
        self.assertEqual(r['all_candidates']['certifiedIncumbents'],2)

    def test_frozen_gate_no_relaxation(self):
        for phase in ('development','holdout','protected'):
            p=ROOT/f'research/evidence/SCORE-PATROL-CLAIM-CAP-COMPLETE-POOL-347-{phase}.summary.json'
            s=run.core.load(p)
            actual=run.core.protected_gate(s['rows'],s['robust_intervals']) if phase=='protected' else run.core.narrow_gate(s['rows'],phase)
            self.assertEqual(actual,s['gate'])

    def test_no_holdout_before_both_development_gates(self):
        m={}; good={'execution_sha256':'abc','gate':{'passed':True}}
        def read(p):return good if 'broad-development' not in str(p) else {**good,'gate':{'passed':False}}
        with patch.object(run.core,'digest',return_value='abc'),patch.object(run.core,'load',side_effect=read):
            with self.assertRaises(ValueError):run.core.authorize(m,'holdout')
            with self.assertRaises(ValueError):run.core.authorize(m,'protected')
            run.core.authorize(m,'broad-development')

    def test_new_input_coverage_without_opening_sealed(self):
        for phase,n in (('development',24),('broad-development',36)):
            cases=run.core.load(ROOT/f'research/holdouts/{run.ID}-{phase}.json')['cases']
            c=run.core.coverage(cases,phase);self.assertEqual(c['fixtures'],n);self.assertEqual(c['results'],n*4)
            if phase=='broad-development':
                for k,values in {'side':{8,32},'players':{8,9,10},'days':{4,5,10},'window_ms':{5000,10000,15000},'fuel':{'low','default','high'},'role':{'native','fixed-all-Patrol'}}.items():
                    self.assertEqual({r[k] for r in cases},values)
                self.assertEqual(len({r['family'] for r in cases}),6)


class InheritedRunnerContract(inherited.RunnerContract):
    def setUp(self):
        self.saved=inherited.runner
        inherited.runner=run.core
    def tearDown(self):inherited.runner=self.saved


if __name__=='__main__':unittest.main()
