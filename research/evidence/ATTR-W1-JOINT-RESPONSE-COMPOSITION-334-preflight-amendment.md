# 334 premeasurement test correction

The initial negative-shape contract test expected RuntimeError, whereas the reused
frozen321 require helper raises ValueError. The malformed request was correctly
rejected; only the test's expected exception class was wrong. The following freeze
command ran despite that test error and produced manifest
CE7C277095CBB24EEA8C32941A870D9BF66C0C98D2AB2FDDFB12F4F7A4727FE1.
No334 measured composition, score or result was produced or inspected.

Preserve that original test, runner and manifest unchanged. The separate
test_joint_response_composition_334_contract.py inherits all original tests and
overrides ONLY the negative test to expect the helper's actual ValueError. It
must pass before any measurement. Candidate, inputs, evaluator, metric, gate,
scope and all hashes in the existing manifest are unchanged. This amendment has
no product/correctness/score verdict authority. No frozen evidence overwritten.
