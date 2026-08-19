# SCORE-TANKER-JOINT-CARAVAN-153

Date: 2026-08-14
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/SCORE-TANKER-JOINT-CARAVAN-153.csv`
SHA256: `6355744188DE9B8F2FEB5B605FAA9AC1D9F47A010B0398240416255E027C8AAE`

Exact 150 requires patrols starting apart to join the tanker trajectory at
different action boundaries and then remain lockstep. Source attribution shows
the canonical master already accepts a group with one tanker and multiple
patrols, but generation creates multi-patrol lockstep only when every patrol is
co-located with the tanker at the start of the day. Rejected 152 proves adding
more rendezvous windows without a shared patrol route cannot close the gap.

Candidate 153 adds a bounded atomic whole-plan primitive inside the independent
EventConflict lane. It beams at most four public start/spot waypoints for one
tanker. Each patrol takes a fuel-feasible Pareto prefix to its earliest reachable
tanker action boundary, then follows the exact remaining tanker actions. Plans
are evaluated only by the existing exact simulator and independent validator.
The byte-equivalent parent runs first; nonterminal admission retains the 151/152
roadless, all-brand/all-stock, full-patrol-fuel and common-terminal guard.
Terminal behavior is unchanged.

Anchor `3200100` must close before any fresh row opens. No waypoint, beam, seed,
family, bot or match-specific dispatcher is allowed.

## Development result

Parent baseline was captured before candidate source for the consumed anchor and
all 12 fresh development rows. The candidate compiled and the full unit suite
passed. On anchor `3200100`, candidate HEAD remained `3/12/15`; day 1 remained
`3/3/3`, while exact remains `3/12/16` with day 1 `3/3/4`. The selected HEAD plan
changed without an official gain, so this is not promotion evidence. Production
source was restored immediately and no fresh candidate score was run.

Verdict: rejected. Earliest-join plus a common tanker suffix is too restrictive;
the exact witness needs patrol-specific detach/continuation after convoy service.

## Holdout result

Sealed and unopened.
