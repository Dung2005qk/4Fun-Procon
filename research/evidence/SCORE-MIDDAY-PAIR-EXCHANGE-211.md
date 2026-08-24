# SCORE-MIDDAY-PAIR-EXCHANGE-211 — mid-day joint pair exchange (REJECTED-INERT)

Registered 2026-08-25, parent `d2674c6`. Frozen manifest
`research/holdouts/SCORE-MIDDAY-PAIR-EXCHANGE-211.csv` SHA256
`1F3A993411ADA1EF8C9E2B8D43F69D254B02AF615AD386F59838E42FD2EA5AFB`.
Question: after the accepted 210 one-agent mid-day ascent reaches its fixed
point, do joint two-patrol replacements under the same
`strict_protected_improvement` certificate (both terminals pinned, equal
joint road footprint, fuel ≥, strict lexicographic day gain) add anything —
the mid-day analog of accepted 207's terminal pair phase (041/042
precedent)?

## First artifact DISCARDED (external review NO-GO)

The first dev run paired the pools as maximal×maximal only, while the
one-agent ascent consumes maximal+supplemental; under the tight
same-terminal pre-filter this risks a false-inert verdict. The run
(60/60 cases, pair_acc=0) is archived on the VM as
`logs211-discarded-maximal-only` and carries no verdict authority. The
same review fixed the analyzer (acceptance-conditional now
`--cond=pair` — both sides run 210, so chain acceptances cannot attribute
the pair phase) and the witness-fuel telemetry ordering, and added direct
unit tests (flag-off byte-inertness, expired-deadline parent preservation,
certificate-on-acceptance, fixed-point re-entry).

## Amended dev gate (widened pools, quiet VM, sequential off→on)

Both sides: terminal-pair 1, midday-chain 1; sides differ only in
`--midday-pair 0|1`. paired=60 **W/T/L=1/57/2, servingsDelta=-1** —
57 ties, exactly what an inert phase between two identical accepted
configurations should produce, confirming clean measurement. Chain
acceptances fired symmetrically (on 91 / off 92). **Midday pair
acceptances: 0 on BOTH sides across all 60 cases.** Zero
invalid/emergency/lane-failure; runtime parity mean 2471/2468 ms,
max 3015/3023 ms. The 3 non-ties are wall-clock timed-search noise
(no differential acceptances anywhere).
Logs: off `2E8E66526A296331A8A3E5BF104D84D70B4A1F3FBBFBFCC9839395A9F31DA33D`,
on `8EB34069730BDD5F6BFA7CC0A215CEA854746AC63F26D1376AF46CC11554897F`.

## Verdict

Closed **rejected-inert** per pre-registered kill condition (a): after the
one-agent mid-day fixed point, no joint two-patrol improvement passes the
both-terminals-pinned equal-footprint certificate anywhere in the dev
split. The 041 result reproduces at mid-days: with terminals pinned for
BOTH agents, the joint neighborhood collapses into the one-agent
neighborhood's closure. NO production change:
`enableMiddayPairExchange` stays default-off (research-only, byte-inert,
unit-tested). The sealed holdout was never opened. Reopen only with a
sound certificate that frees at least one terminal (which today would
require the closed multi-day evaluation theory — 164/167/168).

## Venue

GCP VM udon-stream-185-0822, quiet, sequential sides (208 rule).
