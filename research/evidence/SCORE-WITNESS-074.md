# SCORE-WITNESS-074 rejection evidence

Date: 2026-08-12
Parent research state: frozen champion `7ef3694` plus the uncommitted,
inconclusive `SCORE-HORIZON-072` and `SCORE-SCENARIO-073` prerequisites.
Frozen development/holdout manifest SHA256:
`6329ADA27BCEF1EF6210D984C866309120AD2E9057D09D50F3A02BA300304C24`.

The bounded two-branch W1 repair retained the lexicographically best exact
branch and a distinct best-valid-upper branch, but divided the pre-existing
per-day master combination cap by the number of live branches. The division
changed the route portfolio seen by the alleged lower-bound-preserving branch;
therefore it did not actually contain the old greedy witness.

On the 18 already-open development matches, the complete oracle's W/T/L against
the candidate was `6/12/0`, versus `3/15/0` against the combined 072+073
prerequisite. All six differences were tier 2, invalid remained zero, maximum
gap was `+6`, and the deterministic result hash was `4c781fad088b38f5`.
None of the three target residual gaps closed. Terminal-position/default widened
from `+3` to `+4`; three gaps that 072+073 had closed reopened.

Verdict: rejected and beam source fully reverted. The sealed 54-match holdout
was not opened. Reopening requires strict dominance by construction: reproduce
the complete original greedy witness with its original cap and portfolio first,
retain it as an exact fallback, and only then spend a separately bounded budget
on recourse. Retuning beam width or cap division on these opened fixtures is
forbidden.
