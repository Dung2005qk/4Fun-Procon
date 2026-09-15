import copy
import unittest
from replay_witness_suffix_317 import shared_start, first_tier


class SuffixTests(unittest.TestCase):
    def setUp(self):
        self.oracle = {"oracle_agents": [{"kind": 0, "pos": 17, "fuel": 9}],
            "oracle_ledger": {"brands": [1], "totalDailyDistinct": 1, "totalServings": 1},
            "oracle_score": [1, 1, 1]}
        self.output = {"current_agents": copy.deepcopy(self.oracle["oracle_agents"]),
            "current_ledger": copy.deepcopy(self.oracle["oracle_ledger"]), "current_score": [1, 1, 1]}

    def test_equal_cardinality_is_not_equal_ledger(self):
        self.output["current_ledger"]["brands"] = [2]
        with self.assertRaisesRegex(ValueError, "same exact"):
            shared_start(self.oracle, self.output)

    def test_equal_score_is_not_equal_state(self):
        self.output["current_agents"][0]["fuel"] = 8
        with self.assertRaisesRegex(ValueError, "same exact"):
            shared_start(self.oracle, self.output)

    def test_first_tier_and_valid_shared_state(self):
        shared_start(self.oracle, self.output)
        self.assertEqual(first_tier([4, 15, 100], [4, 16, 33]), 2)
        self.assertIsNone(first_tier([4, 16, 33], [4, 16, 33]))


if __name__ == "__main__":
    unittest.main()
