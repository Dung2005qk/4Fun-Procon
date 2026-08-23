# SCORE-PROTECTED-EXACT-179

Date: 2026-08-21  
Parent: `828ea78`  
Status: rejected on consumed attribution; production source reverted  

## Reopen evidence

Experiment 177 confirms the exact low-fuel horizon gap on three independent
development constructions. Experiment 176 nevertheless regressed six of twelve
fresh general fixtures. Source-level attribution now proves that 176 did not
actually preserve the completed parent search path: enabling nonterminal exact
orienteering made `stagedDeepHarvestSearch` skip `solve_legacy_until`, and moved
the expanded-generation boundary from 85% to 95%. The protected incumbent in
the final comparator was therefore not the complete legacy candidate set that
the parent would have produced.

Unchanged parent day traces on consumed 176 regressions confirm that exact is
supported only on the terminal day (four supported agent-days) while 176
expanded it to twelve. Losses span threshold-corridor, high-stock, fuel-tight,
overnight and balanced families, so a fuel/family dispatcher is forbidden.

## Candidate

Reconstruct 176 exactly, then change only the staged control-flow boundary:
when the new nonterminal fuel-exact allocation boundary is active, run the
unchanged legacy master on the unchanged legacy portfolio through its original
60% checkpoint and merge those candidates before exact candidates enter master,
ALNS, F0 certification and final selection. Keep the original parent incumbent,
legacy candidates and exact candidates concurrently. Bound-closed takeover may
still replace only the protected final current-floor choice with a complete
scenario-wise certificate.

## Invariants and gates

- No designed functionality is removed, disabled, deferred or reduced; no
  duplicate production solver or fallback path is introduced.
- Parent legacy generation, master semantics, candidate objects, official
  comparator, exact simulator, independent validator and 5000-ms hard cap stay
  unchanged. Exact work is additive inside the existing portfolio window.
- Consumed seeds `1760000..1760011` only test the blast-radius explanation;
  they have no promotion authority. Consumed `1720000` only checks causal gap
  closure.
- Fresh development must show broad paired benefit with bounded downside before
  any holdout opens. Any tier-1 loss, systematic tier-2 loss, invalid/emergency,
  or failure to retain the anchor closes the candidate.
- Performance and deadline promotion require BTC target-host telemetry; local
  elapsed is excluded.

Frozen manifest: `research/holdouts/SCORE-PROTECTED-EXACT-179.csv`  
SHA256: `7602CFFBC108B39268B6A6D0201DE28814C0546C41793DB04D1945133F3A0B6F`

Reopen after rejection only with evidence that the parent checkpoint itself was
not preserved or with an equivalent-semantics speedup creating additional exact
headroom. Do not tune the 60/85/95 boundaries or route by seed/family.

## Result

Compile and unit gates passed. On all twelve consumed 176 fixtures, successor
179 reproduced candidate 176 exactly at the official score and at the recorded
search telemetry: the same exact-supported days, exact settled-state counts and
master combination counts were observed. The added legacy checkpoint therefore
did not change any selected plan or recover any of 176's paired regressions.

This falsifies the proposed blast-radius explanation as an exploitable fix. The
static control-flow omission was real, but its legacy candidates were already
noncompetitive or duplicated by the surviving pipeline. The remaining losses
come from shared post-merge bounded search/selection displacement; protecting
that path would require running the complete parent pipeline and exact pipeline
as separate searches inside the same 5000-ms cap, which is duplicate work and
not justified without an equivalent-semantics speedup.

No fresh 179 development or sealed holdout row was opened. The exact anchor was
not rerun because the candidate was already falsified before that gate. All
production and test changes were reverted; experiment 178 research-oracle work
remains independent.
