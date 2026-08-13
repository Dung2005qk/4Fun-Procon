# ATTR-LADDER-EARLIEST-LOSS-130

Date: 2026-08-13  
Parent: `5dccb0f`  
Fixture: consumed ladder-balanced/low seed `1800000`

## Earliest loss

Oracle and HEAD first diverge on day 1. The exact plan scores `6/6/6` versus
HEAD `6/6/7`, but ends the match `6/24/26` versus `6/23/25`.

The exact individual-route membership mask is only `001` in every ordinary
portfolio tested: legacy 12, expanded 16, merged, wide 32 and wide 64. Thus the
exact team plan and equivalent outcome are absent before master retention. The
merged master and a 256-candidate no-BnB master cannot recover absent routes.
F0 contains none of the exact plan because it never reaches the source pool.

Research-only `augment_with_candidate_routes` given the oracle plan restores
mask `111`; a wide augmented master then contains both exact plan and outcome.
This is capability attribution, not an implementable candidate or evidence for
oracle injection. The exact candidate upper is joint-best at `6/24/26`.

## Existing closed-loop limitation

Current HEAD already contains accepted SCORE-W1-CLOSED-LOOP-114. Its complete
exact enumeration exposes one minimum-fuel route ending at each served spot and
builds aligned same-terminal bundles. The new day-2 trace still misses the exact
class:

- exact agent terminals are 26 and 19, so the pair is cross-terminal;
- agent-0 route at terminal 26 is rank 2/3 by remaining-fuel objective and rank
  2/3 by current-score objective, but rank 1 by joint valid upper;
- both exact individual masks are absent from maximal/terminal projections;
- the complete fuel-exact W1 portfolio reaches only oracle mask `011`;
- the full exact day frontier contains 55 and 44 outcomes, composes 1820 unique
  joint states, and the oracle class is upper-rank 1 with 30 ties.

The exact day-1 profile reaches certified lower `6/23/26`, still one daily
distinct below the exact `6/24/26`; neither it nor the parent profile strictly
dominates the other. The gap is therefore a bounded cross-terminal resource
frontier plus joint-combination representation gap, not a master/F0 rank bug or
a reason to weaken the comparator.

## Verdict

Accepted attribution. Do not tune a single terminal rank, enable flags alone,
raise global caps or inject the oracle. A successor may only derive a bounded
fixture-independent resource Pareto representation, preserve all 114 baseline
routes/witnesses, and prove the existing master can turn that representation
into a stronger exact certificate before any score candidate.
