# DEADLINE-PUBLIC-WINDOW-218

Status: rejected at the registered causal kill condition; holdout unopened.

## Gap

The official rules make response time a per-match parameter. Archived BTC
setups include both 5-second and 15-second days, while external competition
configuration reports 45-second days. The production adapter nevertheless
clamps every requested budget to 5000 ms. ATTR-NONTERMINAL-SPARSE-EXCHANGE-216
found protected gains on 11/45 replay-days, including all nine nonterminal days
of m-4108, but the accepted 215 target-terminal suffix received zero work after
the global prefix exhausted the 5-second protected window. Experiment 217 tried
to avoid the second traversal by sharing retention and failed the long-window
stratum; it does not prove the already accepted standalone suffix is useless
when the public window supplies enough time.

## Candidate

Preserve and freeze the complete current 5000-ms production result. If and only
if setup `daySeconds`, the locally received state time, and authoritative
`endsAt` expose a trusted longer window, continue from that frozen plan using
the existing protected wait/midday/target-terminal or terminal-sparse refiner.
The main solver is never rerun with the larger budget. Every takeover must pass
the existing exact simulator, independent validator, official lexicographic
comparator, and transition/ledger dominance certificate. The host submits once
after the continuation stops, reaches a fixed point, or reaches the public
deadline minus transport safety.

## Invariants and gates

- At 5000 ms the candidate must be action-, score-, state-, validator- and
  telemetry-equivalent to parent 177b588.
- Longer windows retain the exact 5000-ms incumbent. Deadline, failure,
  invalidity, no strict gain, or a failed dominance check returns it unchanged.
- Activation uses only the public window and remaining time. No map, seed,
  fuel, family, bot, opponent, or match routing is allowed.
- Development first measures 15-second value. The 45/60-second escalation opens
  only if the 15-second refiner remains deadline-limited or marginal gains have
  not reached a fixed point; this rule is frozen before measurement.
- Promotion requires zero tier-1/tier-2 loss, zero invalid/emergency/refiner
  failure, positive broad paired value with bounded tier-3 downside, a separate
  historical dense/high-fuel champion gate, and BTC target-host lifecycle,
  latency and transport-safety evidence.

## Forbidden equivalences to rejected deadline work

Experiment 218 is not permission to retry an earlier deadline mechanism under a
new name. The following substitutions are explicit kill conditions:

- `DEADLINE-LONG-166`: the public window must not enlarge or rerun the main
  solver and must not select the last long-search trajectory.
- `DEADLINE-ANYTIME-167`: the candidate must not depend on resubmission or on
  comparing a realised score against the incumbent's loose theoretical upper.
- `DEADLINE-STRATIFIED-ANYTIME-184`: actual-vs-actual same-day comparison is
  insufficient unless the complete 5000-ms checkpoint and its future-domain
  invariants remain protected across the closed loop.
- `DEADLINE-VIRTUAL-PARENT-186`: ordinary Long pools are not a continuation
  source; only candidates emitted by the already accepted protected refiner are
  admissible.
- `SCORE-MIDDAY-SHARED-TARGET-217`: no shared-retention allocation, pool-order
  change or second-path dispatcher may be reintroduced. The 5000-ms prefix is
  frozen before any continuation begins.

If the existing protected continuation is inert or too sparse on the frozen
development split, 218 closes. It must not be rescued by weakening these
constraints, changing the split or adding an unregistered generator.

## Frozen evidence

Manifest: `research/holdouts/DEADLINE-PUBLIC-WINDOW-218.csv`.

Consumed replays m-3908 and m-4108 are attribution/champion gates only and may
not tune the mechanism or promote it.

## Stage-A non-propagating value probe

The frozen 15-second development probe completed all 60 cases sequentially on
the quiet Spot VM. It kept the complete current checkpoint and every later-day
state unchanged, so these numbers establish candidate supply only and cannot
promote a closed-loop implementation.

- Gainful cases: `17/60`; protected takeovers: `23`; serving gain: `+28`.
- Tier-1/tier-2 changes: `0/0`; deadline/failure events: `0/0`.
- Difficulty: easy `2/8 +3`, medium `1/8 +2`, hard `4/20 +4`, very-hard
  `10/24 +19`.
- Fuel: low `10/25 +18`, default `6/19 +9`, high `1/16 +1`.
- Role: fixed `11/39 +15`, native `6/21 +13`.
- Every registered family produced a gain: balanced `+6`, fuel-tight `+5`,
  high-stock `+8`, overnight `+2`, rare-brand `+6`, threshold-corridor `+1`.

The probe emitted 11 accepted sparse-refiner steps; the remaining protected
takeovers came from another exact WAIT-detour round. Fresh seeds therefore
satisfy the explicit reopen condition of rejected attribution 204: an
independent day now contains a compatible strict protected detour after the
current first round. Because no 15-second probe reached its deadline, the
preregistered 45/60-second escalation remains closed.

Frozen log SHA256 values are
`6AD1131B43D4893F78E0218D3D3E3D16CB369133359010AAFB541E9DDD2586A1`
easy, `3EFFAD6655FC150F5EDCD92C52FAEC40B2EC329DBC357A7D2B014E6B32DB1F94`
medium, `A10F733774D7ACB4429E733D1F57DE3FDBD74482AC2DD3798F8E962C5E4A7097`
hard and
`DEA0B5A7687DAE1733197E31A0E48DEE0DFFFF28226879FF41CE4179DC1AAB73`
very-hard; runner stderr is the empty-file
`E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`.
The next gate must apply the continuation across days and compare the full match
directly with the parent on the same development fixtures.

## Causal application verdict

The minimal single-pass application used frozen Linux binary SHA256
`85684C978CFF302DC87A31CF779DD474D2C188EB3BD15B31C37186F3DB7F0305`.
The run was stopped after 16 complete pairs when seed `5021002` fell from
`6/30/174` to `6/30/162`, a tier-3 loss of 12. Its candidate had one strict
same-day serving takeover and no invalid, emergency, deadline or refiner
failure.

Interleaved `off/on/off/on` attribution on the same frozen binary produced
`174/162/174/174`. The first on run accepted the extra `+1` continuation and
regressed; the second on run found no extra takeover and tied. This isolates a
causal closed-loop failure rather than ordinary cross-binary noise. The simple
application protects only the virtual main candidate, so later deadline-bounded
work can fall below the complete current 5000-ms protected checkpoint.

The frozen attribution log SHA256 is
`EE6BF53ABAB07187FA7BA453EC5790A9B355BB88A7492C525E70FE530DC4FCC1`.
Easy off/on log SHA256 values are
`CDC23E277AAEF2E75355649780BFC785DEEF130BAB65F3C24CF6B92352E00443`
and `4752E389FDF5E68ACDAA8753029721A03CD0AE06A2F4E12C2C79EE4DDBBD25CD`;
medium off/on values are
`484CB58BF0B9DABA779EFA6779839A5ED915ECE1A7CF5B904F0FD49DCB463EE4`
and `E78ACEA490510EDFDBBB59962E4B75D34257A985324B0AB2491399B6D0941DC5`.
The sealed 218 holdout was never opened. Successor 219 must protect the complete
checkpoint as a first-class state branch without a second main solve.
