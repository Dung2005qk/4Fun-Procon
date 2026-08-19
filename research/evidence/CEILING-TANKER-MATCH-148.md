# CEILING-TANKER-MATCH-148

Date: 2026-08-13
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/CEILING-TANKER-MATCH-148.csv`
SHA256: `31AAECC56CB45DCDD06DB0E29B4E6A4C3F53FDCF0E417F962DBC5F5CFB9AA9F6`

The first loader preflight rejected two rare-return/low rows with one extra CSV
column before any fixture or score ran. Removing only that extra column kept
every registered field unchanged; the hash above is authoritative.

Terminal experiment 147 closed same-day tanker/refuel planning with 72/72 exact
ties. Candidate 142 separately proved that enabling mobile rendezvous before
the terminal day can regress because the resulting fuel/state changes feed a
non-empty suffix. This sweep tests that missing domain directly rather than
reopening the rejected heuristic.

The research oracle is a complete joint step DP over a three-day roadless 2x2
component with two patrols, one tanker and three spots. It carries physical
state, fuel, terminal refuel, lifetime brands and official accumulated score
across days. Daily stock and per-patrol visits reset exactly at day boundaries.
Every reconstructed whole-match witness must agree between the exact simulator
and independent validator. No state cap, approximate value, opponent future,
weighted sum or local performance claim is allowed.

## Development result

Rejected at preflight with no score result. After correcting the two malformed
CSV rows, official config validation rejected the proposed 2x2 fixture because
three distinct start cells plus three spot cells cannot fit without a spot at
an initial agent position. No HEAD or oracle fixture ran. Overlapping starts
would reduce the intended domain, so 148 closes and a separately frozen 2x3
successor is required.

## Holdout result

Unopened.
