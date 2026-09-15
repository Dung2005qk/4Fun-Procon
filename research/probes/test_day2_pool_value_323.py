import unittest
from day2_pool_value_323 import comparison, verify_value

def score(a):return dict(zip(("lifetimeDistinct","totalDailyDistinct","totalServings"),a))

class Contract(unittest.TestCase):
    def test_lexicographic_not_weighted(self):
        r=comparison([4,16,100],[5,12,10]);self.assertEqual((r["result"],r["tier"]),("loss",1))
    def test_selected_and_ceiling(self):
        c={"ceiling":[5,20,38],"selected_value":[5,20,36],"candidate":{"selected":True,"certified":True,
           "validUpperBound":score([5,20,38]),"finalCertifiedLowerBound":score([5,20,36]),"provisionalLowerBound":score([5,20,35])}}
        self.assertFalse(any(verify_value(c,[5,20,36]).values()))
        with self.assertRaises(ValueError):verify_value(c,[5,20,37])
        c["candidate"]["selected"]=False
        with self.assertRaises(ValueError):verify_value(c,[5,20,39])
        c["candidate"]["validUpperBound"]=score([5,20,35])
        self.assertTrue(verify_value(c,[5,20,36])["upper_unsound"])

if __name__=="__main__":unittest.main()
