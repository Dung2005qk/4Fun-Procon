# SCORE-TANKER-TERMINAL-ONLY-144

Date: 2026-08-13  
Production parent: `5dccb0f`  
Frozen manifest: `research/holdouts/SCORE-TANKER-TERMINAL-ONLY-144.csv`

Candidate 142 was rejected because a better immediate mobile-rendezvous plan can
change fuel and own traffic before a non-empty suffix, causing later tier-2
losses. Candidate 144 exposes the same fully wired capability only when
`state.dayNumber == config.day_count()`. This is a public horizon invariant, not
a fixture dispatcher. With a fixed role assignment, all earlier plans/states are
byte-equivalent to the parent and a terminal-day lexicographic improvement is
exactly a whole-match improvement because no suffix remains.

Fresh terminal fixtures must first beat the parent without loss. The role layer
is a separate gate because its approximate whole-horizon evaluator may change
assignments even when fixed-role dominance is proven.

## Results

- Fresh fixed-role development: `4/14/0`, invalid zero. The four gains were
  tier-3 `+1..+2` across rendezvous, stock-contention, rare-brand and
  delayed-claim under low fuel; default/high were controls and tied.
- Sealed fixed-role holdout, opened once: `12/42/0`, invalid zero. All three
  untouched seeds in each of the same four low-fuel families gained tier 3 by
  `+1..+3`; every other stratum tied.
- Role-layer gate 145: fresh deadline `0/6/0`, sealed deadline `1/11/0`,
  sealed exhaustive `0/12/0`; invalid/emergency zero. The BTC-scale local
  tier-3 result crossed when run order reversed and was classified as timed
  local noise because hashes diverged before terminal logic could run.
- BTC target-host `m-2159`: explicit hard/3-bot/10-day/32x32/100-step/
  5000-ms/8-agent/12-spot/6-brand/low-fuel configuration. Assignment and all
  ten day actions were HTTP-accepted and exact-valid; emergency zero; maximum
  solver total `3398 ms`, maximum server-reported response `3706 ms`, both
  below the 5000-ms hard cap. Replay-check independently validated all ten
  plans and reconstructed `6/60/285`. Replay SHA256:
  `AECD94B9EA033CB0642C483673CA8306BF9DA116427B97FD2C8FD477021DB790`.

Bot rank is not promotion evidence. The BTC match clears only protocol,
lifecycle, exact-validity and target-host hard-cap gates; paired strength comes
from the frozen candidate-vs-parent matrices and the terminal dominance proof.
