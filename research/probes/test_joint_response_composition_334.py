import copy
import unittest
from joint_response_composition_334 import compose, gate


class Contract(unittest.TestCase):
    def test_all_subsets_and_whole_trajectory(self):
        original = [[[-2],[-3],[-4]], [[-5],[-6],[-7]]]
        responses = [{"agent": a, "days": [{"plan": [[a+1],[a+2],[a+3]]}, {"plan": [[-a-8],[-a-9],[-a-10]]}]} for a in range(3)]
        frozen = copy.deepcopy((original, responses))
        for mask in range(8):
            r = compose(original, responses, mask)
            for d in range(2):
                for a in range(3):
                    self.assertEqual(r[d][a], responses[a]["days"][d]["plan"][a] if mask & (1<<a) else original[d][a])
            r[0][0].append(99)
            self.assertEqual((original, responses), frozen)

    def test_shape_failure(self):
        with self.assertRaises(RuntimeError): compose([], [], 0)
        with self.assertRaises(RuntimeError): compose([], [], 8)

    def test_gain_over_single_not_just_original(self):
        row = {"seed": 1, "family": "a", "best": [4,16,38], "best_single": [4,16,37], "better_action": True}
        self.assertFalse(gate([row, {**row, "seed": 2}]))
        self.assertFalse(gate([row, {**row, "seed": 2, "family": "b", "best_single": [4,16,38]}]))
        self.assertTrue(gate([row, {**row, "seed": 2, "family": "b"}]))


if __name__ == "__main__": unittest.main()
