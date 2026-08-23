# ORACLE-TRAFFIC-HISTORY-QUOTIENT-178

Date: 2026-08-21  
Parent: `828ea78`  
Status: accepted exact-method experiment  

## Question

Can the complete public minimax oracle preserve identical semantics while
quotienting away which team produced the immediately previous day's traffic?

## Source proof before implementation

The current adversarial memo key stores `previousOwn`, `previousOpponent` and
the already-public `roadStatusKey`.  The two previous footprint vectors are not
read by public-state construction, day-action enumeration, scoring, causal
opponent selection, simulation or validation.  They are read only by
`adversarial_next_statuses`, where each road uses

`currentOwn + currentOpponent + previousOwn + previousOpponent`.

Therefore two states with identical day, positions, fuels, lifetime mask,
public road statuses and identical componentwise total previous traffic have
the same legal actions and the same next state for every own/opponent action
pair.  The ownership split is observationally irrelevant and defines a Markov
bisimulation quotient.

Each component may additionally saturate at
`players * jammedThreshold`.  Every future contribution is nonnegative; once
the previous total alone reaches the jam threshold, adding current traffic
cannot change the next status away from jammed.  Below the threshold the exact
integer total is retained.  This is exact status equivalence, not approximate
traffic dominance.

The tempting separate idea of deleting opponent outcomes with the same terminal
cell and footprint but lower fuel is not new: `opponent_day_outcomes` already
keeps only the maximum-fuel member of precisely that equivalence class.  It is
closed as duplicate and causes no edit.

## Frozen gates

Consumed-only manifest:
`research/holdouts/ORACLE-TRAFFIC-HISTORY-QUOTIENT-178.csv`  
SHA256: `AC97B64F4019476EA68EA69B05A09D997FA671324486924771955A18D47B5BF0`

1. Run original and quotient modes on complete 1-day and 2-day suffixes for all
   three consumed low-fuel families.
2. Require identical robust official score; reconstruct policies and require
   exact-valid witnesses under the independent validator.
3. Run full horizon in quotient mode and compare with the recorded original
   exact results for seeds `1720000`, `1721000` and `1722000`.
4. Benefit requires a material reduction in memo states or transitions.  Local
   elapsed is method-feasibility evidence only and has no BTC/performance or
   production-promotion authority.
5. Any score, action-set, public-state, simulator or validator mismatch rejects
   the quotient.  No sealed holdout is opened.

## Functionality-preservation answers

1. This removes no designed solver functionality: production and tests remain
   unchanged; the optional research mode explores the complete same game.
2. Nothing is deleted, so no canonical-equivalent deletion proof is required.

## Results

The original and quotient modes matched exactly on every registered suffix
gate:

- Complete final-day scores for balanced, threshold and terminal were
  `6/6/6`, `5/5/5` and `5/5/6` in both modes.
- Complete two-day scores were `6/9/9`, `5/8/8` and `5/8/9` in both modes.
  States changed respectively `1,430,951 -> 886,870`,
  `1,145,538 -> 695,260` and `964,892 -> 589,488`.
- Full-horizon robust scores stayed `6/12/12`, `5/11/11` and `5/11/12`.
  Under all three causal opponent policies per family, oracle score, HEAD score,
  plan hashes, exact validity and independent-validator result were identical to
  the recorded unquotiented runs; there were zero invalid results.

Full-horizon resource-structure diagnostics were:

| Seed | Family | States old -> quotient | State reduction | Transitions old -> quotient | Transition reduction |
|---|---|---:|---:|---:|---:|
| 1720000 | balanced | 8,016,487 -> 4,105,904 | 48.78% | 704,745,239 -> 506,432,422 | 28.14% |
| 1721000 | threshold | 6,203,765 -> 3,100,324 | 50.03% | 604,210,103 -> 427,643,887 | 29.22% |
| 1722000 | terminal | 5,227,857 -> 2,657,646 | 49.16% | 514,770,637 -> 372,943,472 | 27.55% |

Local elapsed was observed only to let each exact solve finish; it has no BTC or
competition-performance authority.

## Verdict

Accept the ownership-free previous-traffic representation as canonical exact-
oracle infrastructure.  It is a proved bisimulation quotient, composes with the
experiment-175 boundary dominance relation and materially reduces both memory-
driving memo states and full-horizon transitions across three distinct families.

This does **not** improve the production solver by itself and is not a score or
promotion candidate.  Its system value is stronger and cheaper counterexample/
ceiling research: an already-opened row that previously produced no result due
to OOM may be retried with unchanged semantics after the currently running 177
jobs terminate naturally.  The sealed holdout remains unopened.  Any broader
traffic-history merge without the exact public-state proof remains forbidden.
