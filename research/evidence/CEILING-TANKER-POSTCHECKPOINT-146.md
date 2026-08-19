# CEILING-TANKER-POSTCHECKPOINT-146

Date: 2026-08-13
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/CEILING-TANKER-TERMINAL-138.csv`
SHA256: `7D966C853B0B13B0E2E9223310E540B9BC6E869251711ABEA1EB6ECC519FD2A1`

Checkpoint `cf7e4b4` passed the fresh and sealed paired candidate-vs-parent
matrices for terminal mobile rendezvous, the independent role gate and the BTC
5000-ms lifecycle/hard-cap gate. The exact 54-case tanker terminal holdout from
experiment 138 remains unopened. This read-only sweep opens it exactly once to
measure residual exact score headroom against the new checkpoint.

The complete joint step DP, official lexicographic score, exact simulator and
independent validator are unchanged. No production or oracle source changes.
Local elapsed time has no performance authority and cannot affect the verdict.

## Result

Infrastructure-inconclusive. The one-time aggregate holdout run emitted no
incremental stdout and exceeded its 900-second wrapper. The child process
remained alive without an observable result and was terminated only after its
PID and executable name were verified. No case score, invalid flag or exact
completion result was recovered. The 138 holdout is treated as consumed and
will not be rerun. This timeout is neither an oracle win nor a HEAD tie.
