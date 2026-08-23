# DEADLINE-ANYTIME-167

Parent: `828ea78` plus the source-minimal 166 deadline-policy prerequisite.

Frozen manifest: `research/holdouts/DEADLINE-ANYTIME-167.csv`

SHA256: `E748A798B530DDB2945BE076F94E5DAF72A42B44FAE83BD01868A7BDF52E55E5`

## Mechanism

1. The existing stateful `MatchSession` produces the unchanged 5000-ms decision.
2. That decision becomes the acknowledged same-day incumbent.
3. With trusted time remaining, the same session and engine re-enter the same
   authoritative state with the unchanged pre-day ledger.
4. Existing `may_submit` permits a resend only on strict certified dominance.
5. Final match state, score and road footprint advance exactly once from the last
   accepted plan; response time for a resend replaces the earlier same-day value.

## Invariants

- No parallel, shadow or second engine.
- First decision remains parent-equivalent at 5000 ms.
- No map, seed, family, fuel, role, bot, opponent or match dispatcher.
- Same canonical generator/master/simulator/validator/comparator.
- Non-dominant or failed refinement preserves the first accepted plan.
- Unknown budget remains 5000 ms; server deadline can only tighten.
- No designed functionality is removed, disabled, deferred or reduced; no deletion.
- Local elapsed is diagnostic only; BTC target-host is final for runtime.

## Results

- Consumed general: all 25 first submissions retained parent action hashes and
  protected both known direct-Long losses; `resubmits=0`.
- Consumed BTC-default: ten refinement attempts, `resubmits=0`.
- Fresh general18: 75 refinement attempts, `resubmits=0`, invalid/emergency zero.
- On consumed seed4300000, two of four refinement plans differed from the first
  plan, but all four tied current-day official score and none had strict certified
  suffix dominance.
- Fresh BTC development and all holdouts remained sealed.

Verdict: safe protection attribution, rejected as inert. The research harness
implementation is reverted. Reopen only after a fresh long-budget W1/suffix proof
can certify a different terminal state without weakening `may_submit`.
