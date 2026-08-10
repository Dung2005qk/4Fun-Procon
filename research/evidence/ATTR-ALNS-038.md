# ATTR-ALNS-038 substage diagnostics

Date: 2026-08-09
Parent: `afcd2da`
Verdict: inconclusive for exact route subsource; proof starvation confirmed

The research-only wrapper linked the unchanged frozen parent library,
reconstructed the frozen `m-1285` day-10 state and ledger, and ran the canonical
5000 ms solver once. The selected score was `6/60/321` and its canonical plan
bytes exactly matched the frozen witness. Local elapsed time was ignored.

Existing `AlnsDiagnostics` reported:

- exact seed/local servings: `34/34`;
- regular ALNS iterations/accepted/improvements: `35/25/1`;
- synthesized routes/accepted: `66/11`;
- proof-guided iterations/routes/accepted/improvements: `0/0/0/0`;
- recombination improvements: `0`;
- attempted by operator in enum order: `8,6,3,2,5,3,3,5`;
- accepted by operator in enum order: `8,5,2,0,5,0,2,3`.

`StockMultiVisit` is operator index 4, so regular ALNS attempted and accepted it
five times. The aggregate diagnostics do not say whether the sole global
improvement came from an existing portfolio alternative or one of the eleven
accepted synthesized routes; that exact subsource remains unobservable without
changing library telemetry and is not inferred.

The independent actionable fact is that a valid tier-3 guidance gap remained
(`6/60/322` versus `6/60/321`) while the already implemented proof-guided phase
executed zero iterations because it is scheduled after the generic ALNS loop.
Thus a separately gated candidate may test priority ordering of the existing
proof-guided capability. It may not increase search caps, add a fixture route or
claim that the frozen `0x9D8` path was synthesized.
