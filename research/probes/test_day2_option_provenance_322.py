import unittest
from day2_option_provenance_322 import permutation, state_key, audit_classification

class Contract(unittest.TestCase):
    def test_physical_identity_and_duplicates(self):
        a=[{"kind":0,"pos":17,"fuel":2},{"kind":0,"pos":18,"fuel":1},{"kind":0,"pos":17,"fuel":2}]
        self.assertEqual(permutation(a,[a[1],a[0],a[2]]),[1,0,2])
        with self.assertRaises(ValueError):permutation(a,a[:2]+[{"kind":0,"pos":17,"fuel":3}])
    def test_brand_identity_not_only_cardinality(self):
        a=[{"kind":0,"pos":17,"fuel":2}]
        l={"brands":[10],"totalDailyDistinct":1,"totalServings":1}
        self.assertNotEqual(state_key(a,l,True), state_key(a,dict(l,brands=[20]),True))
    def test_unshortlisted_not_generation_absence(self):
        rows=[{"selected":False,"certified":False,"disposition":"not-shortlisted"}]
        self.assertEqual(audit_classification(rows),"present-but-not-shortlisted")
        self.assertEqual(audit_classification([]),"absent-from-recorded-audit")

if __name__ == "__main__":unittest.main()
