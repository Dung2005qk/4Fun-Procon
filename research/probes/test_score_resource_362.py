"""Synthetic summary contract only; no fixture generation or solver execution."""
import copy
import unittest
import score_resource_362 as score


def fixture_rows():
    cases=[];rows=[];robust=[]
    for seed in range(6):
        c=dict(seed=seed,family=seed%2,fuel='low',side=8,window_ms=10000 if seed%2 else 15000,
               role='native',days=4,players=8,roadless=False)
        cases.append(c)
        for repeat in ('A','B'):
            rows.append(dict(c,repeat=repeat,delta=[0,0,1],comparison_B_vs_A='win',first_tier=2,
                A=[2,8,100],B=[2,8,101],causal=True,certificate={'frames':[{'day':1}]},first_takeover=1))
        robust.append(dict(c,outcome='win'))
    return cases,rows,robust


class Gates(unittest.TestCase):
    def test_positive_contract(self):
        self.assertTrue(score.classify(*fixture_rows(),'holdout')['passed'])
    def test_noncausal_cannot_qualify(self):
        c,r,b=fixture_rows()
        for x in r:x['causal']=False
        gate=score.classify(c,r,b,'holdout')
        self.assertFalse(gate['passed']);self.assertEqual(gate['robust_causal_wins'],0)
    def test_inactive_gain_cannot_supply_benefit(self):
        c,r,b=fixture_rows()
        for x in c+r+b:x['window_ms']=5000
        for x in r:x.update(causal=False,first_takeover=None,certificate={'frames':[]})
        gate=score.classify(c,r,b,'development')
        self.assertFalse(gate['checks']['A_active_benefit'])
        self.assertFalse(gate['checks']['causal_breadth'])
    def test_inactive_consumer_blocks(self):
        c,r,b=fixture_rows();r[0]['window_ms']=5000
        self.assertFalse(score.classify(c,r,b,'development')['checks']['causal_inactive_operation_equivalence'])
    def test_loss_never_disappears(self):
        c,r,b=fixture_rows();r[0].update(window_ms=5000,delta=[0,0,-3],first_takeover=None,certificate={'frames':[]})
        gate=score.classify(c,r,b,'development')
        self.assertFalse(gate['checks']['A_all_downside'])
        self.assertFalse(gate['checks']['A_all_strata'])
    def test_tier_loss_cannot_be_offset_by_servings(self):
        c,r,b=fixture_rows();r[0]['delta']=[-1,0,1000]
        self.assertFalse(score.classify(c,r,b,'development')['checks']['A_all_downside'])
    def test_all_robust_active_losses_count(self):
        c,r,b=fixture_rows();b[0]['outcome']='loss'
        gate=score.classify(c,r,b,'holdout')
        self.assertEqual(gate['all_robust_active_losses'],1)
        self.assertFalse(gate['checks']['robust_active_causal_sign'])

if __name__=='__main__':unittest.main()
