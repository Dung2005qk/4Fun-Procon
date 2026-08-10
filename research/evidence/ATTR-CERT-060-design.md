# ATTR-CERT-060 design

Date: 2026-08-09

## Question

After cooperative exact-state initialization, which operation first carries
future-witness certification past its global compute deadline on BTC?

`m-1836` is the frozen development counterexample. Pre-certification remained
inside the `total - 1600 ms` boundary on every day, but certification crossed it
on days 3, 6, 8 and 9. Current-day exact orienteering separately overran its own
deadline by 42 ms on day 4. These are attribution targets, not map-specific
optimization targets.

## Diagnostic mechanism

Add audit-only counters around the existing operations in
`FutureWitnessRepairer::repair_profile`:

- monotone WAIT-floor construction;
- scenario setup and traffic prediction;
- terminal viability analysis;
- route column generation, including its existing exact-orienteering diagnostics;
- route master;
- exact simulation;
- independent validation;
- state/traffic transition;
- certified WAIT fallback;
- final profile bookkeeping.

For every coarse phase, record call count and elapsed microseconds. Record the
first phase ending after the per-candidate slice deadline and the first phase
ending after the shared global certification deadline, together with overrun.
The engine already computes both deadlines; the diagnostic may observe them but
must not change them.

## Invariants

- No branch condition, loop bound, candidate order, deadline, route cap, search
  space, comparator, simulator, validator, fallback or submitted action changes.
- No map/seed/fuel/opponent dispatcher and no threshold tuning from `m-1836`.
- Diagnostic timestamps are not performance claims; only BTC target-host phase
  attribution has authority.
- The diagnostic binary must be replay-check byte-equivalent to frozen
  `PERF-CERT-059` on all 300 frozen states before BTC use.
- Diagnostic code is not a production candidate and is never committed as a
  strength improvement.

## Decision rule

If fresh BTC crossings consistently identify a phase, trace that phase to its
first unpolled operation and register a separately frozen semantics-equivalent
successor. If crossings do not occur or point to incompatible phases without a
source-level common operation, close inconclusive. Do not lower the 1600 ms
network reserve or 800 ms submission floor.
