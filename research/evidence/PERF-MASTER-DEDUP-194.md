# PERF-MASTER-DEDUP-194

Parent: `f9c0019`.

Manifest SHA256:
`AE8F787E3CDDAC2859A881BC4247C77F1104EE16E9CED3886D1E1CADE712DAC7`.

## Mechanism and equivalence probe

The candidate replaced the canonical JSON string used by all three
`RouteMaster::solve` dedup sets with an injective length-prefixed fixed-endian
encoding. Survivor stable IDs remained canonical JSON. An exhaustive pairwise
property test over 134 structural/pathological plans found identical equality
classes. A complete no-deadline master comparison produced identical candidate
count, diagnostics, stable IDs, scores and plans after correcting an initial
candidate-only wiring bug where a membership key had accidentally been reused
as a stable ID. The full unit suite then passed.

## Development rejection

The generated development runner emitted nine completed parent/candidate
results before the registered early rejection gate stopped it. Log SHA256:
`2C733721C96B29B7F38E171979AC0D1A26383C3CADC10C71B4F92E1B28C5E08F`.

Fixed-role seeds `4770000` and `4770003` tied exactly in official score with
zero invalid/emergency. Native seed `4770001` also tied. Native seed `4770002`
violated the registered timed semantic gate: parent selected all patrols and
scored `6/30/163`, while candidate selected mask `1` and scored `6/30/150`.
Both were valid, so this was not a simulator or protocol failure.

Two immediate reversed-order reproductions showed the timed selector itself was
load-sensitive, but did not clear the candidate: candidate selected masks `4`
and `2` and scored `6/30/155` and `6/30/144`; parent selected mask `4` twice and
scored `6/30/150` twice. The all-engine optimization changes how much incomplete
evidence each role receives and therefore cannot satisfy byte/score equivalence.
Local elapsed time is not used as performance authority.

Verdict: rejected before holdout and BTC. The flat encoding is collision-safe,
but applying it during role selection changes the timed evidence allocation.
The only permitted successor is a freshly registered candidate that keeps role
selection and certification canonical and applies the encoding only to the
post-role day master.

Functionality preservation: the rejected candidate deleted or disabled no
designed functionality and deleted no files. No duplicate or shadow solver ran.
