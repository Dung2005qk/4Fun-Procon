"""Synthetic analyzer checks only; no solver or experiment execution."""
import unittest
from explain_active_divergence_368 import proof_semantics, checkpoint_comparison, served_claims, telescoping_delta
from audit_active_divergence_368 import first_difference


class ActiveAuditTests(unittest.TestCase):
    def test_first_boundary(self):
        self.assertEqual(first_difference([True, True, False, True]), 3)
        self.assertIsNone(first_difference([True, True]))

    def test_drop_only_known_work_counters(self):
        self.assertEqual(proof_semantics({'complete': False, 'combinationsVisited': 2,
                                         'branchesPruned': 1, 'futureField': 8}),
                         {'complete': False, 'futureField': 8})

    def test_proof_completion_stays_semantic(self):
        a = {'acceptedDay': 1, 'cachedContingencies': [], 'strongProofs': [{'complete': False}]}
        b = dict(a, strongProofs=[{'complete': True}])
        self.assertFalse(checkpoint_comparison([a], [b])['last_proof_semantics_equal'])

    def test_cache_change_not_hidden_by_work(self):
        a = {'acceptedDay': 1, 'cachedContingencies': [1], 'strongProofs': []}
        b = dict(a, cachedContingencies=[2])
        self.assertFalse(checkpoint_comparison([a], [b])['last_cached_contingencies_equal'])

    def test_rejected_claim_not_a_serving(self):
        setup = {'spots': [{'brand': 0, 'pos': 1}, {'brand': 5, 'pos': 2}]}
        simulation = {'claims': [{'spot': 0, 'served': True}, {'spot': 1, 'served': False}],
                      'score': {'servings': 1, 'dailyDistinct': 1, 'brandsMask': '1'}}
        self.assertEqual(served_claims(setup, simulation)['brands'], [0])

    def test_mask_mismatch_fails_closed(self):
        with self.assertRaises(AssertionError):
            served_claims({'spots': []}, {'claims': [], 'score': {
                'servings': 0, 'dailyDistinct': 0, 'brandsMask': '1'}})

    def test_daily_delta_telescopes_without_weighting(self):
        days = [{'daily_delta': [0, 0, 5]}, {'daily_delta': [0, -2, -3], 'sides': {
            'parent': {'score': [6, 58, 363]}, 'candidate': {'score': [6, 56, 365]}}}]
        self.assertEqual(telescoping_delta(days), [0, -2, 2])

    def test_incomplete_delta_fails_closed(self):
        with self.assertRaises(AssertionError):
            telescoping_delta([{'daily_delta': [0, 0, 0], 'sides': {
                'parent': {'score': [6, 58, 363]}, 'candidate': {'score': [6, 56, 365]}}}])


if __name__ == '__main__':
    unittest.main()
