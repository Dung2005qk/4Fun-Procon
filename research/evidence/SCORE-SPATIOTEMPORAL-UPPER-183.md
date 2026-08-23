# SCORE-SPATIOTEMPORAL-UPPER-183

## Research question

Experiment 168 completed every registered long-horizon trajectory and improved
48 scenario lower witnesses, but the protected comparator never permitted a
resubmission because no challenger lower bound exceeded the incumbent valid
upper bound. Experiment 183 tested whether a sound candidate-independent
spatiotemporal serving upper could tighten that certificate enough to create a
strict takeover without changing `may_submit`.

## Bound construction and soundness

For each patrol and each remaining day, the probe computes the maximum number
of distinct spot claims reachable under smooth-road shortest-time lower bounds.
It relaxes fuel, opponent traffic, terminal coupling, cross-day position
coupling and shared stock, and permits free repositioning between future days.
The team claim capacity is the sum of these per-agent capacities. The serving
upper is the componentwise minimum of that capacity, exact remaining stock and
the existing `FastViabilityAnalyzer` upper. Lifetime distinct is unchanged.

The construction is an upper bound because every omitted constraint enlarges
the feasible set, never contracts it. Its per-agent, per-spot, per-day claim
count matches the simulator and validator rule that a patrol can claim a spot
at most once per day. Directed shortest paths use the smooth-road traversal
cost of the source cell. Instances with at most 16 spots use exact subset DP;
larger instances fall back to the existing upper rather than approximating
downward.

## Frozen evidence

Manifest: `research/holdouts/SCORE-SPATIOTEMPORAL-UPPER-183.csv`

SHA256: `4E046F2E81C64D05029A602CDE0AF2DE298850AB38EB5C41461552580F65BCE0`

Only already consumed development evidence was used. No sealed holdout was
opened.

### General development from experiment 168

Across 18 fixtures and 75 observed day states, the probe was never below the
realized final score. It tightened 12 of 75 states, all in the three high-stock
families, and left 63 unchanged.

- Seed `4410004`: root servings upper `96 -> 69`; terminal observed-state upper
  `66 -> 57`; realized final servings `56`.
- Seed `4410010`: root `84 -> 57`; terminal `56 -> 49`; realized final `46`.
- Seed `4410016`: root `104 -> 70`; terminal `66 -> 58`; realized final `57`.

The exact reconstruction of experiment 168 was then rerun with the new upper
and its protected two-stage submission path. All 18 final official scores tied
the parent, invalid and emergency counts were zero, all registered wide
trajectories still completed, and 48 scenario lower witnesses improved. The
unchanged `may_submit` gate accepted zero resubmissions in all 18 fixtures.
The three cases where the upper tightened remained inert:

- `4410004`: 4 refinements, 7 greedy and 7 guided completions, 3 strict lower
  improvements, 0 resubmissions.
- `4410010`: 4 refinements, 8 and 8 completions, 3 strict improvements,
  0 resubmissions.
- `4410016`: 4 refinements, 7 and 7 completions, 3 strict improvements,
  0 resubmissions.

### Opened BTC-large development row

Seed `4411000` tightened on all 10 day states. The root serving upper changed
from `840` to `621`; the terminal observed-state upper changed from `497` to
`473`; realized final servings were `467`. This is tightness evidence only, not
BTC target-host performance evidence.

### Completed exact experiment-178 anchors

The initial root upper remained safe but unchanged on all three anchors:

- `1720000`: existing/probe `6/16/16`; completed robust exact `6/12/12`.
- `1721000`: existing/probe `5/16/16`; completed robust exact `5/11/11`.
- `1722000`: existing/probe `5/16/16`; completed robust exact `5/11/12`.

No probe value undercut a completed exact continuation.

## UET configuration reference

The UET difficulty table supplied by the user is an external reference, not an
authoritative PTIT match contract. It confirms that future research matrices
should explicitly cover 5, 7, 8 and 10 days; 4, 6 and 8 agents; and 14, 20, 26
and 32 map sides. Its 45/60-second server windows do not change the current
internal 5000 ms competition cap without authoritative PTIT configuration.
Difficulty multipliers and rank points aggregate match placements; they do not
replace the official within-match lexicographic objective and do not justify a
weighted-sum promotion rule.

## Verdict

`rejected-tight-but-inert`. The proposed upper is sound on the consumed evidence
and materially tighter on high-stock states, but it creates no strict protected
takeover under the unchanged comparator. Therefore it does not improve playing
strength and is not a production candidate. No holdout or BTC performance gate
was opened. All experiment-only planner, comparator, harness and probe source
was reverted; production source remains the frozen competition artifact.

Reopen only with a stronger sound, state-coupled upper that first demonstrates
a strict unchanged-`may_submit` takeover on consumed evidence. Do not weaken the
comparator or dispatch by seed, family, deadline or fuel profile.

## Functionality-preservation check

1. No designed production functionality was removed, disabled, deferred or
   reduced; the failed research candidate was reverted.
2. No canonical production implementation was deleted. Only the experiment-only
   probe was removed after rejection.
