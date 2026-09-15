"""Unit/consumed contract data only; no protected gameplay invocation."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import run_pair_protected_341 as r
from test_protected_http_341 import contract_case


class ProtectedRunnerTests(unittest.TestCase):
    def row(self,delta=(0,0,0),active=False):
        result,tier,diff=r.compare([10+delta[0],20+delta[1],30+delta[2]],[10,20,30])
        return {'seed':1,'family':'a','fuel':'low','delta':list(delta),'comparison':result,
            'first_tier':tier,'activated_before_divergence':active,
            'trajectory_equal':not active,'first_certificate_gain_day':1 if active else None,
            'safety':{s:{'safety_pass':True} for s in r.SIDES}}
    def test_preregistered_input_coverage_without_solver(self):
        c=r.load(r.ROOT/f'research/holdouts/{r.ID}-protected.json')['cases']
        self.assertEqual(r.coverage(c),{'pairs':108,'results':216,'actions':1368,'transitions':1152})
        bad=copy.deepcopy(c);bad[0]['days']=7
        with self.assertRaises(ValueError):r.coverage(bad)
    def test_lexicographic_priority_not_weighted_sum(self):
        self.assertEqual(r.compare([1,100,100000],[2,0,0]),('loss',1,-1))
        self.assertEqual(r.compare([2,0,100000],[2,1,0]),('loss',2,-1))
    def test_neutral_protected_lanes_need_no_new_win_quota(self):
        q=r.protected_checks([self.row()]);self.assertTrue(q['passed']);self.assertFalse(q['promotion_authorized'])
    def test_bounded_loss_not_mechanically_rejected(self):
        rows=[self.row((0,0,2),True),self.row((0,0,-1),True)]
        self.assertTrue(r.protected_checks(rows)['passed'])
        self.assertEqual(r.aggregate(rows)['delta'],[0,0,1])
        self.assertEqual(len(r.aggregate(rows)['loss_tail']),1)
    def test_large_and_higher_tier_losses_block(self):
        for loss in ((0,0,-2),(0,-1,100),(-1,100,1000)):
            self.assertFalse(r.protected_checks([self.row((0,0,5000),True),self.row(loss,True)])['passed'])
    def test_negative_family_or_fuel_net_blocks(self):
        for key in ('family','fuel'):
            rows=[self.row((0,0,3),True),self.row((0,0,-1),True)];rows[1][key]='b'
            self.assertFalse(r.protected_checks(rows)['passed'])
    def test_inactive_cutoff_cannot_claim_gain_or_equivalence(self):
        self.assertFalse(r.protected_checks([self.row((0,0,1))])['passed'])
        row=self.row();row['trajectory_equal']=False
        self.assertFalse(r.protected_checks([row])['passed'])
    def test_any_safety_failure_blocks(self):
        row=self.row();row['safety']['candidate']['safety_pass']=False
        self.assertFalse(r.protected_checks([row])['passed'])
    def setup_empty(self,directory):
        for s in r.SIDES:(directory/s).mkdir()
    def test_clean_boundary_can_resume(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);self.setup_empty(root);r.audit_resume([contract_case()],root)
    def test_any_partial_or_foreign_prefix_blocks(self):
        for filename in ('3419001.replay.jsonl','3419001.stderr','wrong.result.json'):
            with tempfile.TemporaryDirectory() as d:
                root=Path(d);self.setup_empty(root);(root/'parent'/filename).touch()
                with self.assertRaises(ValueError):r.audit_resume([contract_case()],root)
    def test_complete_side_skipped_but_forged_pair_never_trusted(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);self.setup_empty(root);case=contract_case();case['order']=list(r.SIDES)
            for s in r.SIDES:r.write_new(root/s/'3419001.result.json',{'side':s})
            with patch.object(r,'validate_side') as check:
                r.audit_resume([case],root);self.assertEqual(check.call_count,2)
                r.write_new(root/'3419001.pair_complete.json',r.pair_row(case,root))
                r.audit_resume([case],root)
                with patch.object(r,'pair_row',return_value={'wrong':True}):
                    with self.assertRaisesRegex(ValueError,'pair marker'):r.audit_resume([case],root)
    def test_live_unit_preflight_artifacts_validate(self):
        directory=r.ROOT/f'research/evidence/{r.ID}-protected-http-preflight-v2'
        inputs=r.load(directory/'inputs.json')
        for e in inputs['cases']:r.validate_side(e['case'],e['side'],directory)
    def test_early_summary_cannot_open_partial_scores(self):
        with tempfile.TemporaryDirectory() as d:
            cases=r.load(r.ROOT/f'research/holdouts/{r.ID}-protected.json')['cases']
            with self.assertRaises(FileNotFoundError):r.complete_audit({},cases,Path(d))


if __name__=='__main__':unittest.main()
