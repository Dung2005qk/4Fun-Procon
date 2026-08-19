# ATTR-TANKER-STATE-DOMINANCE-164

- Parent: `cf7e4b4`.
- Scope: read-only attribution on consumed roadless low-fuel seed `3200100`.
- Source candidate: none.

The known exact day-1 witness scores `3/3/4`, while the parent day-1 plan scores
`3/3/3`. A monotonic insertion would be safe only if the strict score gain also
ended in the same complete authoritative state and road footprint, allowing the
unchanged parent suffix to continue.

That condition fails. Both plans have an empty road footprint, but the exact
witness ends with terminal agents `26:2|26:2|26:2`, whereas the parent ends
`36:2|34:1|36:2`. The gain therefore changes positions and fuel, not merely the
current score. Retaining both possible futures would require a dual or shadow
suffix evaluator under the same 5000-ms cap, violating the single-path and
non-overengineering rules.

Verdict: closed negative attribution. No source was changed and no holdout was
opened. Reopen only for a fresh general counterexample whose strict score gain
also preserves the full terminal state and road footprint.
