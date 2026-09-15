# HOTFIX-BTC-ROLE-FASTPATH-371

Date: 2026-09-10

Parent: canonical production `c76a8ea` / accepted258.

## Question

Can a time-aware pre-match role path prevent `E_STALE_DAY` assignment rejects
without changing the daily solver, canonical 5000 ms checkpoint, transport
reserve, score comparator, or resource semantics?

## Frozen scope before source change

- Keep the normal full role rollout when there is safe slack before `startsAt`.
- When setup arrives late, use the deterministic `blank_slate::Portfolio`
  nearest-central role mask immediately; this is an emergency lifecycle path,
  not a claim of global role optimality.
- Bound normal role selection by the remaining pre-start slack and transport
  reserve.
- Record the selected assignment mode and remaining time in the replay.
- Preserve the existing hard failure on a server-rejected assignment; no
  unverified server fallback is assumed.
- No production install or commit is authorized by this preregistration.

## Required checks

1. Build the isolated candidate with the recorded parent and toolchain.
2. Run the existing unit/CTest and role-regression fixtures, including the
   historical role counterexamples.
3. Run one BTC operational canary with exactly three bot opponents. Require
   accepted assignment, valid ACKs, reconciled transitions, zero safety and no
   deadline failure before considering competition use.
4. Any score promotion requires a new paired development/holdout experiment;
   this hotfix alone cannot waive that gate.

## Explicit non-goals

This does not prove that BTC defaults a missing/rejected assignment to Patrol,
does not remove role rollout globally, and does not alter the 5000 ms daily
checkpoint.
