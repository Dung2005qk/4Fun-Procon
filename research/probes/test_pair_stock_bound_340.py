"""Independent exhaustive values exercise resource-relaxed incumbent cuts."""
import itertools
from run_http_baseline_314 import ROOT, Bridge
import test_terminal_quotient_339 as quotient
import test_pair_resource_response_336 as independent


class Contract(quotient.Contract):
    def setUp(self):
        super().setUp()
        self.p.close();self.p.process.stdout.close();self.p.process.stderr.close()
        self.p=Bridge(ROOT/'artifacts/research/340/probe.exe')

    def test_independent_exhaustive_joint_optimum_with_cuts(self):
        cuts=0
        for stock,fuel,reverse in itertools.product((1,2,3),(0,1,2),(False,True)):
            q=independent.Contract.q(self);q.update(op='price',expired=False,corrupt=False)
            for s in q['setup']['spots']:s['stocks']=stock
            for a in q['state']['agents'][:2]:a['fuel']=fuel
            if reverse:
                q['state']['agents'].reverse()
                for p in q['plans']:p.reverse()
            out=self.p.request(q);before=self.control.request(q)
            self.assertTrue(out['ok'],out);self.assertEqual(out['completed'],1,out)
            self.assertEqual(out['failures'],0);self.assertTrue(out['originalUnchanged'])
            optimum=max(independent.Contract.brute(self,q,p) for p in itertools.combinations(range(3),2))
            self.assertEqual(out['score'],optimum)
            self.assertEqual(out['score'],before['score']);self.assertEqual(out['plans'],before['plans'])
            cuts+=out['boundCuts']
        self.assertGreater(cuts,0,'Tests did not exercise the new pruning boundary')


if __name__=='__main__':
    import unittest
    unittest.main()
