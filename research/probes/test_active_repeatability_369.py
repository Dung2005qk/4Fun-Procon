"""Pure analyzer controls; no solver, VM, or evidence read."""
import unittest
from explain_active_repeatability_369 import score_compare, first_difference, semantic


class Controls(unittest.TestCase):
    def test_daily_loss_is_not_hidden_by_servings(self):
        result = score_compare([6,58,363],[6,56,365])
        self.assertEqual((result['outcome'],result['first_tier'],result['delta']),('loss',2,[0,-2,2]))

    def test_reverse_direction(self):
        self.assertEqual(score_compare([6,56,365],[6,58,363])['outcome'],'win')

    def test_tie(self):
        self.assertEqual(score_compare([6,58,363],[6,58,363])['outcome'],'tie')

    def test_length_mismatch_blocks(self):
        with self.assertRaises(AssertionError):
            first_difference([1],[1,2])

    def test_first_boundary(self):
        self.assertEqual(first_difference([1,1,1],[1,2,3]),2)
        self.assertIsNone(first_difference([1,2],[1,2]))

    def test_only_clock_removed(self):
        self.assertEqual(semantic({'endsAt':1,'fuel':0}),{'fuel':0})
        self.assertNotEqual(semantic({'endsAt':1,'fuel':0}),semantic({'endsAt':1,'fuel':1}))


if __name__=='__main__':
    unittest.main()
