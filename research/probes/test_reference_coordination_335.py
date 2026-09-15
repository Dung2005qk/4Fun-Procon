import unittest
from reference_coordination_335 import minimum_changes, gate


class Contract(unittest.TestCase):
    def test_minimum_joint_coordinates(self):
        rows = [{"mask": m, "score": [4,16,36 if m != 3 else 38]} for m in range(8)]
        self.assertEqual(minimum_changes(rows,[4,16,36]),2)
        self.assertEqual(minimum_changes(rows,[4,16,36],False),0)
        self.assertEqual(minimum_changes(rows,[4,16,38],False),2)
        self.assertIsNone(minimum_changes(rows,[4,16,38]))

    def test_official_lexicographic(self):
        rows = [{"mask": 1,"score": [4,15,999]}, {"mask": 3,"score": [4,17,1]}]
        self.assertEqual(minimum_changes(rows,[4,16,36]),2)

    def test_target_and_breadth(self):
        r = {"seed":1,"family":"a","better_action":True,"min_changes_above_single":2}
        self.assertFalse(gate([r,{**r,"seed":2}]))
        self.assertFalse(gate([r,{**r,"seed":2,"family":"b","better_action":False}]))
        self.assertTrue(gate([r,{**r,"seed":2,"family":"b"}]))


if __name__ == "__main__": unittest.main()
