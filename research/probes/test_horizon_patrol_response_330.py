import copy
import itertools
import unittest
from http_prefix_option_loss_321 import ROOT, close_bridge
from run_http_baseline_314 import Bridge
from test_http_prefix_oracle_321 import fixture

class Contract(unittest.TestCase):
    def setUp(self):
        self.probe=Bridge(ROOT/'artifacts/research/330/probe.exe')
        self.judge=Bridge(ROOT/'artifacts/research/314/bridge.exe')
    def tearDown(self):close_bridge(self.probe);close_bridge(self.judge)
    def q(self):
        q=fixture();q['state']['day']=3;q['setup']['daySteps']=[4]*4
        q['state']['agents']=[{'kind':0,'pos':p,'fuel':2} for p in (17,18,7)]
        q['ledger']={'brands':[10],'totalDailyDistinct':3,'totalServings':5}
        q['plans']=[[[-4],[-4],[-4]],[[-4],[-4],[-4]]];return q
    def exhaustive(self,q,agent):
        def visit(state,ledger):
            if state['day']==5:return [len(ledger['brands']),ledger['totalDailyDistinct'],ledger['totalServings']]
            best=None
            # Complete small corridor walks, including a one-tick initial pickup.
            for count in range(3):
                for moves in itertools.product((2,5),repeat=count):
                    for initial_wait in (0,1):
                        used=initial_wait+2*count
                        if used>4:continue
                        actions=([-1] if initial_wait else [])+list(moves)+([- (4-used)] if used<4 else [])
                        plan=[[-4],[-4],[-4]];plan[agent]=actions
                        r=self.judge.request({'op':'step','setup':q['setup'],'state':state,'ledger':ledger,'plan':plan})
                        if not r.get('ok'):continue
                        self.assertTrue(r['agrees'])
                        value=visit({**state,'day':state['day']+1,'agents':r['agents']},r['ledger'])
                        if best is None or value>best:best=value
            return best
        return visit(q['state'],q['ledger'])
    def test_complete_vs_independent_small_walk_enumeration(self):
        for stock in (1,2,3):
            q=self.q()
            for s in q['setup']['spots']:s['stocks']=stock
            r=self.probe.request(q);self.assertTrue(r['ok'],r)
            for a,response in enumerate(r['responses']):
                self.assertEqual(response['score'],self.exhaustive(q,a))
    def test_zero_fuel_not_reset(self):
        q=self.q()
        for a in q['state']['agents']:a['fuel']=0
        r=self.probe.request(q);self.assertTrue(r['ok'],r)
        self.assertEqual(r['baseline'],[2,7,9])
        for response in r['responses']:self.assertEqual(response['score'],r['baseline'])
    def test_scope_and_incomplete_witness(self):
        q=self.q();q['plans'].pop();self.assertFalse(self.probe.request(q)['ok'])
        q=self.q();q['state']['agents'][0]['kind']=1;self.assertFalse(self.probe.request(q)['ok'])
        q=self.q();q['setup']['map']['cells'][2][1]=1;self.assertFalse(self.probe.request(q)['ok'])
        q=self.q();q['plans'][0][0]=[2]*4;self.assertFalse(self.probe.request(q)['ok'])

if __name__=='__main__':unittest.main()
