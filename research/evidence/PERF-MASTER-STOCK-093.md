# PERF-MASTER-STOCK-093 rejected development

Parent is `f77c101`. Candidate executable SHA256 was
`51924C6907611C2DCB1E69E282FF3B200442E4C1E520B58FE09F0E03CE7B1E22`.
Manifest SHA256 is
`DAB0096E18863B8AC753AD06EEB808FB11DB9ED5CE48558486533BD7BF715F1B`.

The source identity is exact: start from the maintained generic stock upper and
subtract capped contribution differences only where bundle suffix claims differ.
The full unit suite passed. On `m-2077` day 1, diagnostic reference/difference
visits were `119936/45`, proving a large arithmetic-work opportunity without
using local elapsed time.

The required frozen 60-state deadline screen nevertheless failed. Candidate vs
parent official-score W/T/L was `6/47/7`, action bytes matched `36/60`, gain/loss
serving sums were `32/52`, and all differences were tier 3. Per replay:

- m-2077: `2/5/3`, actions `4/10`;
- m-2078: `0/8/2`, actions `7/10`;
- m-2081: `2/7/1`, actions `5/10`;
- m-2082: `2/8/0`, actions `6/10`;
- m-2084: `0/9/1`, actions `6/10`;
- m-2085: `0/10/0`, actions `8/10`.

Every loss was rerun A/B/B/A. Six did not follow the candidate: they became ties,
crossed by order, or also appeared in the second parent run. The remaining
`m-2077` day 4 loss reproduced by binary: parent scored `6/24/155` and
`6/24/164`, while candidate scored `6/24/142` and `6/24/141`. This is a frozen
semantic-output failure at the public 5000-ms deadline, not a latency judgment.

Verdict: rejected before BTC. The algebraic bound is exact, but changing work per
deadline poll changes the incomplete-search output and does not satisfy the
performance-only equivalence gate. All 093 source and telemetry fields are
restored; the holdout target matrix remains unopened. Reopen only if an
operation-bounded master can make completion/output independent of wall-clock
sampling while retaining the same hard cap.
