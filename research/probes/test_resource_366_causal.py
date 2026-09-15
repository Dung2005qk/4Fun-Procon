"""Synthetic-only controls for the user-directed decision amendment."""
import ast
from copy import deepcopy
import inspect
import unittest
import gate_resource_366_causal as gate
import score_resource_366_causal as runner
import score_resource_366_single as frozen_runner
import test_resource_366_single as base


class CausalClassifierControls(unittest.TestCase):
    def test_active_success_retains_diagnostics(self):
        out=gate.classify(*base.full_rows())
        self.assertTrue(out['passed'])
        self.assertTrue(out['original_p1_diagnostics']['passed'])
        self.assertIsNone(out['replicated_intervals'])

    def test_inactive_wins_and_losses_are_symmetric_diagnostics(self):
        for delta in ([0,-3,10],[0,0,-90],[-1,0,0],[0,3,10],[0,0,90]):
            c,r=base.full_rows();base.change(r[0],delta)
            out=gate.classify(c,r)
            self.assertTrue(out['passed'])
            self.assertEqual(len(out['causal_winner_seeds']),6)
            self.assertEqual(out['all_active_losses'],0)
        c,r=base.full_rows()
        for row in r[:12]:base.change(row,[0,0,-1])
        out=gate.classify(c,r)
        self.assertTrue(out['passed'])
        self.assertFalse(out['original_p1_diagnostics']['checks']['inactive_no_detected_negative_shift'])

    def test_inactive_success_cannot_rescue_active_failure(self):
        c,r=base.full_rows()
        for row in r[:12]:base.change(row,[1,10,1000])
        for row in r[12:]:base.change(row,[0,0,0])
        self.assertFalse(gate.classify(c,r)['passed'])

    def test_active_losses_count_without_takeover(self):
        c,r=base.full_rows();base.change(r[-1],[0,0,-1])
        r[-1]['causal']=False;r[-1]['first_takeover']=None;r[-1]['certificate']['frames']=[]
        out=gate.classify(c,r)
        self.assertEqual(out['all_active_losses'],1)
        self.assertFalse(out['passed'])

    def test_active_upper_tier_and_tail_unchanged(self):
        for delta,check in (([0,-1,100],'active_no_upper_tier_loss'),
                            ([-1,1,100],'active_no_upper_tier_loss'),
                            ([0,0,-3],'active_serving_tail')):
            c,r=base.full_rows();base.change(r[-1],delta)
            self.assertFalse(gate.classify(c,r)['checks'][check])

    def test_inactive_activation_and_safety_always_block(self):
        for field,value in (('first_takeover',1),('safety',False),
                            ('certificate',{'frames':[{'day':1}]})):
            c,r=base.full_rows();r[0][field]=value
            self.assertFalse(gate.classify(c,r)['passed'])

    def test_malformed_and_noncausal_active_success_block(self):
        c,r=base.full_rows()
        for row in r[12:]:row['causal']=False
        self.assertFalse(gate.classify(c,r)['passed'])
        c,r=base.full_rows();r[-1]['delta'][0]=1
        with self.assertRaises(AssertionError):gate.classify(c,r)
        c,r=base.full_rows()
        with self.assertRaises(AssertionError):gate.classify(c,r[:-1])

    def test_active_stratum_and_unknown_window_block(self):
        c,r=base.full_rows();c=deepcopy(c);r=deepcopy(r)
        c[-1]['fuel']=r[-1]['fuel']='low';base.change(r[-1],[0,0,-1])
        self.assertFalse(gate.classify(c,r)['checks']['active_strata'])
        c,r=base.full_rows();c=deepcopy(c);r=deepcopy(r)
        c[0]['window_ms']=r[0]['window_ms']=4000
        self.assertFalse(gate.classify(c,r)['checks']['registered_windows'])


class CausalRuntimeControls(base.RuntimeControls):
    def setUp(self):
        self.previous=base.runner;base.runner=runner

    def tearDown(self):
        base.runner=self.previous

    def test_previous_gate(self):
        s={'phase':'holdout','complete':True,'zero_safety_failure':True,
           'gate':{'checks':{'inactive_upper_tiers':False,'B_overall_components':False,
                            'causal_inactive_operation_equivalence':True,'A_active_downside':True}}}
        c={'fixtures':54,'results':216,'actions':1368}
        runner.require_previous(s,c)
        for check in ('causal_inactive_operation_equivalence','A_active_downside'):
            bad=deepcopy(s);bad['gate']['checks'][check]=False
            with self.assertRaises(RuntimeError):runner.require_previous(bad,c)
        bad=deepcopy(s);bad['zero_safety_failure']=False
        with self.assertRaises(RuntimeError):runner.require_previous(bad,c)
        with self.assertRaises(RuntimeError):runner.require_previous(s,dict(c,results=215))


class MechanicalEquivalenceControls(unittest.TestCase):
    def test_every_runtime_and_certificate_function_unchanged(self):
        changed={'require_previous','previous_gate','summarize','run'}
        names=[name for name,value in vars(frozen_runner).items()
               if inspect.isfunction(value) and value.__module__ == frozen_runner.__name__]
        for name in names:
            if name in changed:continue
            a=ast.dump(ast.parse(inspect.getsource(getattr(frozen_runner,name))))
            b=ast.dump(ast.parse(inspect.getsource(getattr(runner,name))))
            self.assertEqual(a,b,name)

    def test_run_changes_only_output_marker_text(self):
        a=inspect.getsource(frozen_runner.run)
        b=inspect.getsource(runner.run).replace('protected-causal-single','protected-single')
        self.assertEqual(ast.dump(ast.parse(a)),ast.dump(ast.parse(b)))

    def test_summarize_changes_only_classifier_identity_and_report_labels(self):
        a=inspect.getsource(frozen_runner.summarize)
        b=inspect.getsource(runner.summarize).replace('protected-causal-single','protected-single')
        b=b.replace('gate_resource_366_causal','gate_resource_366_single')
        b=b.replace('366-P2 user-directed causal decision amendment after original holdout; single-pass protected; not original gate pass or product promotion',
                    'User-amended single-pass protected; no A/A or replicated intervals; not automatic product promotion')
        self.assertEqual(ast.dump(ast.parse(a)),ast.dump(ast.parse(b)))


if __name__=='__main__':unittest.main()
