import unittest
from witness_realization_345 import classify,gate,cache_has
from test_ack_suffix_consumption_333 import Contract as InheritedContract


class CompletePoolContract(InheritedContract):
    def test_repetitions_do_not_manufacture_independent_breadth(self):
        self.assertFalse(gate([self.row(repeat='A'),self.row(repeat='B',family='b')]))

    def test_current_score_is_not_future_score(self):
        self.assertEqual(classify(self.row(current_floor_eligible=False)), 'current-day-floor-conflict')
        self.assertFalse(gate([self.row(current_floor_eligible=False),self.row(seed=2,family='b',current_floor_eligible=False)]))


if __name__=='__main__':unittest.main()
