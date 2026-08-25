# DEADLINE-CHECKPOINT-CLOSED-LOOP-219

Status: registered before source change.

## Gap

Experiment 218 proved broad same-day candidate supply (`17/60`, `+28`) but its
minimal application failed the frozen causal gate. On fresh development seed
`5021002`, a strict protected `+1` continuation changed the later timed
trajectory and the full match fell from `6/30/174` to `6/30/162`. Interleaved
`off/on/off/on` reproduced `174/162/174/174`; the final on run tied only because
no continuation takeover occurred. Protecting the main solver's virtual parent
therefore does not preserve the complete current 5000-ms checkpoint.

## Candidate

Maintain three monotone state views only when a trusted public window exceeds
5000 ms:

1. the existing virtual main state used by the canonical solver;
2. an exact checkpoint state/ledger that executes the complete current 5000-ms
   main plus protected-refiner artifact;
3. the authoritative richer state/ledger after public-window continuation.

Each day computes the canonical main decision once. The unchanged current
protected pipeline runs once on the checkpoint state. Its exact checkpoint plan
is then replayed and independently validated on the richer state. On every
nonterminal day, equal positions and road footprint, no-less patrol fuel and
componentwise ledger dominance are mandatory. On the terminal day, where there
is no future state, exact validity and official lexicographic
non-regression/strict gain replace the transition certificate. Only after that
replay may the existing protected continuation use public time. The checkpoint
branch advances with the unchanged plan, while the richer branch advances with
the strict continuation. No second main solver, Long trajectory, resubmission,
shared retention or fixture dispatcher exists.

The public refiner starts each day from a snapshot of the checkpoint refiner's
already-earned route cache. This preserves useful same-day checkpoint work, but
all cache entries created by public continuation are discarded with the
snapshot and therefore cannot change the next day's checkpoint cutoff.

## Invariants and gates

- At public windows at or below 5000 ms, production action, score, state,
  validator result, telemetry and response lifecycle remain parent-identical.
  This is gated in-process by equality between the reported candidate and
  `checkpoint_closed_loop_parent`, with zero public probe days/takeovers/failures;
  two independent wall-clock-limited processes are not an identity oracle.
- The checkpoint plan is the incumbent every day, not merely the main solver's
  pre-refinement candidate. Failure, deadline, invalidity or loss of dominance
  submits that checkpoint unchanged.
- Before the terminal day, the checkpoint and richer branches must emit
  identical road footprints, so both observe the same future public traffic
  state. The terminal day is compared only by exact official score because no
  later traffic or state consumer exists.
- The solver is called exactly once per day; checkpoint protection may not
  duplicate, shadow or replace the solver logic.
- Promotion requires in-run paired zero tier-1/tier-2 loss, zero full-match
  score loss, zero invalid/emergency/refiner failure, broad tier-3 gain, 5000-ms
  equivalence, sealed holdout, historical dense/high-fuel champion and BTC
  target-host gates.
- The 45/60-second escalation remains sealed unless the accepted continuation
  is deadline-limited before fixed point at 15 seconds.

## Frozen evidence

Manifest: `research/holdouts/DEADLINE-CHECKPOINT-CLOSED-LOOP-219.csv`, SHA256
`48200C4B086EBE73DFC0E476A83ECDB72118B3BF1D24871318B70A29DC0D4794`.

Experiment 218 development and seed `5021002` are consumed attribution only.
They cannot promote, tune thresholds or route this successor.

## Frozen development execution

The first partial 5000-ms control run was stopped before any public-window
development case because public continuation and checkpoint work shared one
mutable route cache. Those logs are preserved as non-evidence under
`logs219-rejected-shared-refiner-cache` on the research VM.

The corrected implementation snapshots the cache after the unchanged daily
checkpoint prefix and discards public-side cache writes at day end. Frozen
Linux tournament binary SHA256 is
`164A8DE651F78BA182B77AFB2009EC37158F211C57CBE4E6E619BDBA4CCDB3CA`;
the corresponding harness source SHA256 is
`33B8F7E39DE6217B2DC96E5E177C3514943BD8C338B8EA56706A6947A27E8146`.
Development restarted from empty registered logs.

Development completed on that frozen binary. All `24/24` 5000-ms controls
matched their in-process checkpoint with zero public probe day, takeover or
failure. The `60` public-15000-ms cases produced `14/46/0` W/T/L and net
`+19` servings; all differences were tier 3. There were `16` protected
takeovers, zero public/refiner deadline day, and zero invalid, emergency,
terminal-sparse, mid-day or checkpoint-closed-loop failure. Gains covered all
three fuel profiles, both role modes, easy/hard/very-hard, and five of six
traffic families; medium and fuel-tight tied throughout. Summary SHA256 is
`56A92ED7D6382E0CC99EAA99C7002B3D6C0570593ED4CC21AB3184C8A4BB3805`.
The preregistered development gate therefore passes and authorizes the one-time
sealed holdout. No 45/60-second escalation is authorized because no 15-second
case reached the public continuation deadline.

The sealed `5060xxx` holdout was then opened exactly once on the same frozen
binary. It runs 108 public-15000-ms cases sequentially; no holdout result was
observed before this transition.

## Sealed holdout verdict

The one-time holdout completed all `108/108` cases on the unchanged frozen
binary and manifest. It produced `29/79/0` W/T/L and net `+88` servings; every
difference was tier 3. There were `50` protected continuation takeovers and no
full-match loss, public deadline day, invalid plan, emergency day,
terminal-sparse failure, mid-day failure or checkpoint-closed-loop failure.

Strict gains covered all four difficulty tiers (`easy +7`, `medium +1`,
`hard +11`, `very-hard +69`), all fuel profiles (`low +27`, `default +49`,
`high +12`), both role modes (`fixed +41`, `native +47`) and all six traffic
families. The result therefore passes the frozen global-benefit,
zero-full-match-loss and zero-safety-failure gate. No case reached the
15-second continuation deadline, so the preregistered `45000/60000 ms`
escalation remains closed.

Experiment 219 is accepted as a research mechanism. Its operation-equivalent
three-branch runtime/replay integration, Linux source-rank portability repair,
fresh production protected matrix and BTC target-host gates subsequently passed.
Production evidence is recorded in
`research/evidence/DEADLINE-CHECKPOINT-CLOSED-LOOP-219-PRODUCTION.md`. The
consumed development and holdout sets may not be used to tune that integration
or any successor. Exact holdout log, summary and empty-runner hashes are recorded
in `research/evidence/DEADLINE-CHECKPOINT-CLOSED-LOOP-219-holdout-sha256.txt`.
