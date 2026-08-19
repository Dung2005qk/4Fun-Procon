# CORRECT-HARD-CAP-WIRING-163

- Parent: `cf7e4b4` plus the locally proven core 161 cap prerequisite.
- Frozen manifest:
  `research/holdouts/CORRECT-HARD-CAP-WIRING-163.csv`, SHA256
  `8F8FA420C5129AE28D9FBC5B3C8CF81A8B2A2F9E9A8CF515EE76BC81378B0EC1`.

The rejected 162 deadline policy was fully removed. Git diff proves the body of
the old `UdonShieldEngine::select_roles` exhaustive implementation is unchanged;
only its public name became `select_roles_exhaustive_oracle`. Its only callers
are the benchmark and historical harness, both research-only. Generic CLI roles now calls
`select_roles_until(kCompetitionComputeHardCap)`; HTTP, sandbox, replay roles
and `MatchSession` already use the same bounded selector. There is no wrapper
back to the ambiguous no-budget name.

Unit/simulator/validator tests pass. An output-equivalence attempt on consumed
seeds cannot have causal authority: the frozen parent binary changed its own
mask and score between repeats on identical inputs (rare-brand `100003` changed
from mask 4, `6/24/43`, to mask 0, `6/24/40`; other masks also crossed). Source
identity, not those unstable local outcomes, protects the research oracle.

BTC target-host gate `m-3573` used hard, three bots, 10 days, 32x32, 100
steps/day, 5000 ms/day, eight agents, 12 spots, six brands and low fuel 1x.
All 10 submissions were HTTP 200 and valid; all 9 authoritative transitions
reconciled and replay-check agreed with the independent validator. Maximum
server-reported response was 2556 ms and maximum solver decision time was 2447
ms, both below the 5000-ms hard cap. Replay SHA256 is
`7878AB5DC19B401F59D97DE077AA1C997A6E213C455ACF3092628427E5535ACA`.

The live score was `6/59/296`: day 10 served only five brands. This is a valid
development counterexample, not evidence against the hard-cap repair and not a
bot-rank quality claim. `ATTR-BTC-SELECTION-165` corrected an initial diagnostic
run that omitted the production current-score floor. With that floor enabled,
current ended `6/60/266` and frozen parent `6/60/267`; both preserved `6/60`
through day 9 and the one-serving tail difference is local timed-search noise.
More importantly, the replay-preferred day-1 plan was already certified in the
live portfolio, but tied the selected plan on lower and upper bound while its
q50 was one serving lower. It did not dominate under the information available
at selection time, so changing policy to win the realized suffix would overfit.

The protected fixed-role 5000-ms screen tied frozen parent `0/6/0`, with zero
invalid/emergency. The final all-target build and unit/simulator/validator gate
passed after migrating the remaining research benchmark caller to the explicit
oracle name. The combined 161/163 correctness/wiring candidate therefore passes
its registered semantic, score, build and BTC validity/lifecycle/hard-cap gates.
The BTC rank itself is not promotion evidence.
