# SCORE-FAIR-DIRECT-FRONTIER-193

Parent: `f9c0019`.

Frozen manifest: `research/holdouts/SCORE-FAIR-DIRECT-FRONTIER-193.csv`.
SHA256: `3A289DE666A43FDE0BC332E35C278D8BBBB38708E951847198DC36411BC1A9CA`.

## Gap and candidate

The opened `m-3897` production audit contained later-agent starvation in the
sequential column generator. Days 3, 5, 8 and 10 contained zero-query agents;
on day 8 agents 4--7 each recorded `0 ms / 0 queries` and retained three
columns. The candidate performed one direct-target phase across every agent,
using a remaining-time/remaining-agent slice and caching each result before the
unchanged deeper stages.

The candidate did not remove any designed stage, change the official score,
alter simulation or validation, add fixture routing, or exceed the `5000 ms`
hard cap. The holdout remained sealed.

## Consumed attribution

Both variants replayed `m-3897` with fixed role mask 2, the canonical closed
loop, harvest modes 7/7 and a `5000 ms` logic budget. These local runs are score
and causality evidence only; their elapsed times have no performance authority.

- Parent cumulative scores by day: `44,90,138,185,231,276,324,369,417,464`
  servings at lifetime/daily tiers `6/(6*day)`.
- Candidate cumulative scores by day:
  `44,90,138,185,231,276,324,370,415,461`.
- Final paired result: candidate loses tier 3 by 3 servings,
  `6/60/461 < 6/60/464`.
- Candidate direct coverage was 8/8 agents on every day. The contemporaneous
  parent rerun had no zero-query agent because the local timing path completed
  its direct work; therefore the candidate changed work ordering even when the
  observed starvation condition was absent.
- A separate fixed-public-state replay solve of original day 8 produced the
  same exact plan and the same immediate `6/48` under parent and candidate.
  Thus removing the recorded zero-query symptom did not create a score witness
  on the strongest consumed starvation day.

Decision evidence:

- Parent JSONL SHA256:
  `F960C5A549958B457185BD7DF785BF7F9090F13F7EF05C44C7AFF3FCB6A4E869`.
- Candidate JSONL SHA256:
  `D4FC861F337C9E19D9E05C817813996F0EC5F41364DC9B5E944A15F66744DC3F`.

## Fresh development falsification

Two preregistered fresh fixtures were run before opening the remaining matrix:

- 8x8, 4 days, 4 agents, 6 spots, low fuel, fixed role, seed 4750000:
  parent and candidate both `5/20/42`, zero invalid/emergency; both searches
  completed on all four days.
- 32x32, 10 days, 8 agents, 18 spots, low fuel, fixed role, seed 4754000:
  parent and candidate both `6/60/492`, zero invalid/emergency; both searches
  reached the bounded deadline on all ten days. The protected virtual score,
  terminal sparse result and exact-validation counts were identical.

The candidate therefore has no positive score witness on consumed or fresh
development evidence, while it has one closed-loop consumed regression. The
remaining development matrix and sealed holdout are not opened because the
registered causal/development gate already failed.

## Verdict

Rejected and reverted. Agent-order starvation is valid telemetry, but direct
frontier fairness is not a score-causal mechanism on the available evidence.
It spends bounded work reordering a complete frontier when the runtime happens
not to starve, can perturb the later closed loop, and does not improve the
strongest fixed-state starvation counterexample. No production source or
runtime option is retained.

Reopen only if a new fixed-state counterexample proves that a later agent has a
specific exact-valid direct column missing solely because of agent order and
that inserting that column strictly improves the official score. A successor
must preserve the non-starved route portfolio without a doubled solver.
