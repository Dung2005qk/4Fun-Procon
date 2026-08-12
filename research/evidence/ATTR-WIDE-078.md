# ATTR-WIDE-078 — wide full-trajectory probe

Date: 2026-08-12
Parent: frozen champion `7ef3694` plus the uncommitted 072+073 prerequisites.
Manifest SHA256:
`6329ADA27BCEF1EF6210D984C866309120AD2E9057D09D50F3A02BA300304C24`.

For each residual counterexample, the probe started from the exact oracle day-1
candidate and reconstructed two complete cap-16 future trajectories with
unchanged detailed/terminal master combination limits. One chose the current
greedy front; the other ranked retained candidates by the existing
candidate-state `FastViability` valid upper, then current exact score, terminal
slack and stable ID. Every selected day plan was replayed through both engines.

Results:

- seed 1300200: baseline W1 `3/7/8`; wide greedy `3/9/10` (exact oracle);
  wide guided `3/9/9`;
- seed 1310300: baseline W1 `5/12/12`; wide greedy `5/14/14`; wide guided
  `5/16/16` (exact oracle);
- seed 1310400: baseline W1 `4/12/13`; wide greedy `4/12/14`; wide guided
  `4/13/13`; oracle `4/15/16`.

Verdict: accepted attribution. Cap-16 capability has actionable value, and the
greedy and valid-upper views are complementary. This does not itself authorize
production replacement: an implementation must compute all old witnesses first
and accept only a strictly better complete exact trajectory.
