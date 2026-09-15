"""Correct only the negative test's expected exception class; original retained."""
import unittest
from joint_response_composition_334 import compose


class CorrectedContract(__import__("test_joint_response_composition_334").Contract):
    def test_shape_failure(self):
        # The frozen321 require helper deliberately raises ValueError, not RuntimeError.
        with self.assertRaises(ValueError): compose([], [], 0)
        with self.assertRaises(ValueError): compose([], [], 8)


if __name__ == "__main__": unittest.main()
