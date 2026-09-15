import unittest
import pricing_selection_351 as r

class Contract(unittest.TestCase):
    def test_equal_is_not_strict(self):
        self.assertFalse(r.dominates([(1,2,3)],[(1,2,3)],[1]))
    def test_strict_and_earlier_tier(self):
        self.assertTrue(r.dominates([(1,3,0)],[(1,2,1000)],[1]))
        self.assertFalse(r.dominates([(1,2,1000)],[(1,3,0)],[1]))
    def test_crossing_distributions(self):
        self.assertFalse(r.dominates([(1,2,2),(1,2,9)],[(1,2,3),(1,2,8)],[1,1]))
        self.assertTrue(r.dominates([(1,2,9),(1,2,3)],[(1,2,3),(1,2,8)],[2,1]))
    def test_zero_weight_cannot_make_strict(self):
        self.assertFalse(r.dominates([(9,9,9),(1,2,3)],[(0,0,0),(1,2,3)],[0,1]))
        with self.assertRaises(ValueError):r.dominates([(1,2,3)],[(1,2,3)],[0])
    def test_selected_identity_and_drift(self):
        s=dict(zip(r.KEYS,(1,2,3)));w={'certified':True,'lowerBoundOnly':False,'futurePlans':[[[-2],[-2]]],'score':s,'witnessScore':s}
        a={'afterWitness':r.witness_bytes(w),'afterScore':s}
        self.assertTrue(r.selected_identity(a,w))
        self.assertFalse(r.selected_identity({**a,'afterWitness':'different'},w))
        self.assertFalse(r.selected_identity(a,{**w,'lowerBoundOnly':True}))
    def test_current_floor_not_future_proof(self):
        self.assertTrue((1,2,3)<(1,2,4))
        self.assertFalse(r.dominates([(1,9,20)],[(1,9,30)],[1]))
        self.assertTrue(r.dominates([(1,9,31)],[(1,9,30)],[1]))

if __name__=='__main__':unittest.main()
