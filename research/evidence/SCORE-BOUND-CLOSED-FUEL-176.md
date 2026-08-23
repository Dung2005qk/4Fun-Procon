# SCORE-BOUND-CLOSED-FUEL-176

Date: 2026-08-20  
Parent: `828ea78`  
Status: rejected on fresh development; production source reverted  

## Confirmed counterexample

Experiment 175 completed an exact public-information four-day minimax on the
valid one-active-patrol subdomain. On consumed seed `1720000`, oracle score was
`6/12/12`; unchanged HEAD was `6/11/11` under all three causal opponent
policies, with zero invalidity and zero deadline-limited HEAD days.

The oracle preserves fuel rather than front-loading daily brands. At day 1 its
current score is `2/2/2`, terminal fuel `10`; HEAD takes `6/6/6` and ends with
fuel `2`. After the oracle prefix through day 2, HEAD again front-loads day 3,
ending cumulative `6/10/10` and final `6/11/11`. Forcing the oracle prefix
through day 3 then returning day 4 to unchanged HEAD yields exact `6/12/12`, so
the terminal-day solver is sufficient and the causal gap is pre-terminal
coverage plus selection.

The existing complete fuel-constrained exact generator contains the oracle
route and joint master outcome on both required days: day 1 route mask `111`,
251 settled states; day 3 route mask `111`, 50 settled states. The production
portfolio does not contain either complete sequence. Prior experiments
082/104/105/116/132 rejected blind flag or width changes; this independent exact
score counterexample satisfies their reopen condition but does not invalidate
their negative fixtures.

At day 3, forced profile evaluation gives challenger lower/upper
`6/12/12 .. 6/13/13` and parent lower/upper
`6/11/11 .. 6/12/12`. At day 1 they are respectively
`6/11/11 .. 6/15/15` and `6/11/11 .. 6/11/11`. Current
`certified_dominates` rejects both because challenger survival does not strictly
exceed incumbent possible survival at the shared boundary. The protected
current-floor rule from experiment 057 must remain intact outside an exact
bound-closed takeover.

## Candidate mechanism

1. Under the existing harvest mode, node/column caps and shared production
   deadline, permit the canonical complete fuel-constrained exact enumerator on
   nonterminal days when a patrol is fuel-constrained. No second solver, new
   route representation, width increase or fixture dispatcher is allowed.
2. Add a separate bound-closed takeover relation. Challenger certified outcome
   survival must be no worse than incumbent valid-upper survival at every
   threshold with identical scenario weights. Challenger valid-upper survival
   must be strictly better at at least one threshold. The final implementation
   preserved the unchanged current-floor choice and allowed a challenger to
   replace that protected choice directly; it did not delete the incumbent or
   use the relation as a transitive dominance edge.

The relation is safe because every realized incumbent result is bounded above
by its valid upper, while every challenger scenario has a certified witness at
least as good. Strict challenger upper is only an anti-churn/upside condition;
it is never used to establish the no-regression half of the proof.

## Invariants and gates

- No designed functionality is removed, disabled, deferred or reduced; no
  deletion is proposed.
- Official lexicographic score only; no weighted promotion metric.
- Same generator, master, simulator, independent validator, scenarios, fuel
  semantics, traffic model and 5000-ms compute cap.
- Existing current-floor behavior remains byte/score stable unless the complete
  bound-closed proof passes against every higher-current certified candidate.
- Candidate must close consumed `1720000` causally, then beat parent on fresh
  development with zero invalid/emergency and bounded downside before holdout.
- Holdout stays sealed until the proof unit tests, consumed closure, fixed-role
  development and native-role development all pass.
- Local elapsed has no performance authority. Any deadline/performance-related
  promotion requires BTC target-host telemetry at the same 5000-ms config.

Frozen manifest:
`research/holdouts/SCORE-BOUND-CLOSED-FUEL-176.csv`  
SHA256: `EE9EE677FD189EC61D37936C2B5B84FAD8157F8BFC4F17089D4C157165B16113`

Reject if bound closure changes any case without a complete scenario-wise
certificate, if exact enumeration displaces the protected baseline before
certification, if the consumed counterexample does not close, or if fresh
development shows systematic family/fuel loss. Reopen neither raw width nor
unprotected current-floor relaxation from this experiment.

## Executed result

The proof unit suite passed. Runtime telemetry then exposed and closed two
missing capability links without increasing the route width: complete exact
bundles had to be imported from the expanded portfolio and their provenance had
to survive the F0 profile gate. Exact candidates were ranked by their already
valid upper bounds inside the existing diversity quota; the bound was cached
and not recomputed. The final takeover was applied only against the protected
baseline selected by the unchanged current-floor logic.

On consumed seed `1720000`, the resulting source candidate selected exact bundle
5 on day 1 and the bound-closed continuation on day 3. It reproduced the exact
oracle sequence and score `6/12/12`, versus parent `6/11/11`, with a valid plan
and zero deadline-limited days. This established causal closure but had no
promotion authority.

Fresh fixed-role general development at 5000 ms rejected the mechanism before
native, BTC-like or holdout execution. Paired candidate-versus-parent result was
`1/5/6` W/T/L across seeds `1760000..1760011`, invalid `0`, emergency `0`:

- win: overnight `1760003`, tier 3 `+3` servings;
- tier-2 losses: threshold-corridor `1760000` (`-1` daily), high-stock
  `1760002` (`-2` daily), fuel-tight `1760007` (`-1` daily), overnight
  `1760009` (`-1` daily);
- tier-3 losses: high-stock `1760008` (`-3` servings), balanced `1760010`
  (`-2` servings).

The losses span four families and include four first-open-tier-2 regressions;
they are not an acceptable small trade-off. Exact work also expanded from 4 to
12 supported agent-days in these fixtures, but local elapsed and throughput are
explicitly excluded from the verdict. The sealed holdout was never opened and
BTC was not run. All production and unit-test source changes were reverted to
`828ea78` after recording this result.

Reopen only if an independent fresh counterexample yields a public structural
condition that predicts when the complete exact continuation is globally
protective on multiple development families. Do not reopen broad first-day or
penultimate-day fuel-exact activation, F0 exact reservation, raw width, or
bound-closed takeover by itself.
