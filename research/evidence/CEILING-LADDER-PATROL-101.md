# CEILING-LADDER-PATROL-101

Parent: `f77c101`

Frozen manifest: `research/holdouts/CEILING-LADDER-PATROL-101.csv`

SHA256: `B9D1D990CDBE224AA848C92175B3ACDDA94E83D2B7F73F3EDA2EE1A3F72DD4A3`

The research-only adapter added a new 2x4 plain ladder with three linked cycles,
six spots, two active patrols and one isolated control. The manifest contains 18
development and 54 sealed holdout fixtures across six new families and
low/default/high fuel. Production source and behavior were unchanged.

The unchanged complete joint full-match DP did not complete the first
development fixture within the 180-second probe limit and produced no score.
The timeout wrapper left one child process alive; it was identified by the exact
probe executable path and stopped. No topology, horizon, fuel, state, dominance,
simulator or validator semantics were reduced to force completion. Holdout was
not opened.

101 is infrastructure-inconclusive. It is not evidence that HEAD has no exact
gap, and it does not establish a practical ceiling. Together with 097 and 100,
it shows that unrestricted complete joint enumeration on cyclic components is
no longer an economical general sweep at this state size. Reopen only with a
mathematically exact quotient or bounded external proof host; do not shrink this
frozen suite after observing its computational cost.

## Reopen resolution (2026-08-13)

`PERF-ORACLE-RESOURCE-DOMINANCE-129` supplied the required mathematically exact
quotient and preserved all 54 previously completed 085/089/095 oracle scores.
Without shrinking topology, horizon, fuel, action space or transitions, the
first frozen ladder development fixture seed `1800000` completed dual-valid at
oracle `6/24/26` versus HEAD `6/23/25`, tier-2 gain `+1`, result hash
`94088825f11f84c5`. CEILING-LADDER-PATROL-101 is now an accepted exact gap via
129. Remaining development and holdout fixtures stay sealed for a separately
justified candidate.
