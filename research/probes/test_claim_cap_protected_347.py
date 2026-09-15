"""No protected gameplay measurement; operational binding must preserve all gates."""
import copy
import unittest
from unittest.mock import patch
import claim_cap_protected_347 as runner


class BindingContract(unittest.TestCase):
    def setUp(self):
        self.base = {'bridge_binary': 'old', 'hashes': {'candidate': 'sha'},
                     'stage': 'old', 'candidate_binary': 'candidate',
                     'splits': {'protected': {'sha256': 'sealed'}}, 'policy_sha256': 'gate'}
        self.amended = copy.deepcopy(self.base)
        self.amended['bridge_binary'] = runner.BRIDGE
        self.amended['stage'] = 'repaired binding'
        self.amended['operational_amendment'] = {'original_execution_sha256': runner.BASE_SHA}

    def test_only_bridge_binding_changes(self):
        runner.validate_binding(self.base, self.amended)

    def test_candidate_input_and_metric_changes_rejected(self):
        for key in ('candidate_binary', 'splits', 'policy_sha256'):
            changed = copy.deepcopy(self.amended)
            changed[key] = 'changed'
            with self.assertRaisesRegex(ValueError, 'non-operational'):
                runner.validate_binding(self.base, changed)

    def test_dependency_removal_and_hash_change_rejected(self):
        for hashes in ({}, {'candidate': 'changed'}):
            changed = copy.deepcopy(self.amended)
            changed['hashes'] = hashes
            with self.assertRaisesRegex(ValueError, 'dependency'):
                runner.validate_binding(self.base, changed)

    def test_other_bridge_and_new_policy_key_rejected(self):
        changed = copy.deepcopy(self.amended)
        changed['bridge_binary'] = 'third bridge'
        with self.assertRaisesRegex(ValueError, 'wrong protected bridge'):
            runner.validate_binding(self.base, changed)
        self.amended['new_cap'] = 6000
        with self.assertRaisesRegex(ValueError, 'unexpected manifest keys'):
            runner.validate_binding(self.base, self.amended)

    def test_original_provenance_required(self):
        self.amended['operational_amendment']['original_execution_sha256'] = 'changed'
        with self.assertRaisesRegex(ValueError, 'provenance'):
            runner.validate_binding(self.base, self.amended)

    def test_prior_qualification_not_bypassed(self):
        with patch.object(runner, 'base_qualification', side_effect=ValueError('prior gate failed')):
            with self.assertRaisesRegex(ValueError, 'prior gate failed'):
                runner.authorize(self.amended, 'protected')
        with self.assertRaisesRegex(ValueError, 'protected-only'):
            runner.authorize(self.amended, 'holdout')


if __name__ == '__main__':
    unittest.main()
